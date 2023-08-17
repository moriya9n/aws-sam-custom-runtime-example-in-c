#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>

#define MAX_URL_LEN 256
#define REQUEST_ID_LEN 256
#define NUM_TRY 1

static CURLcode get(char *url, FILE *out)
{
	CURLcode res = -1;
	CURL *curl = curl_easy_init();
	if (curl) {
		curl_easy_reset(curl);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)out);
		curl_easy_setopt(curl, CURLOPT_HEADER, 1);
		res = curl_easy_perform(curl);
	}
	curl_easy_cleanup(curl);
	return res;
}

static CURLcode post(char *url, char *content_type, char *post_data)
{
	CURLcode res = -1;
	CURL *curl = curl_easy_init();
	struct curl_slist *list = NULL;
	char buf[BUFSIZ];

	if (curl) {
		curl_easy_reset(curl);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);

		snprintf(buf, BUFSIZ, "Content-Type: %s", content_type);
		list = curl_slist_append(list, buf);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
		res = curl_easy_perform(curl);
		curl_slist_free_all(list);
	}
	curl_easy_cleanup(curl);
	return res;
}

static void strip_end(char *str)
{
	if (str) {
		for (char *p = rindex(str, '\0') - 1; p >= str; p--) {
			if (!isspace(*p)) {
				break;
			}
			*p = '\0';
		}
	}
}

static void strip_start(char *str)
{
	if (str) {
		char *p = strdup(str);
		char *freep = p;
		for (; *p != '\0'; p++) {
			if (!isspace(*p)) {
				break;
			}
		}
		strcpy(str, p);
		free(freep);
		fprintf(stderr, "str: %s\n", str);
	}
}

static void strip(char *str)
{
	strip_end(str);
	strip_start(str);
}

static void print_all(FILE *fp)
{
	char buf[BUFSIZ];

	rewind(fp);
	while (fgets(buf, BUFSIZ, fp) != NULL) {
		strip(buf);
		fprintf(stderr, "buf: %s\n", buf);
	}
}

static int parse_next(FILE *fp, char *request_id, json_t **json_ret, json_error_t *json_error_ret)
{
	char header[BUFSIZ];
	int found = 0;

	rewind(fp);
	// parse header
	while (fgets(header, BUFSIZ, fp) != NULL) {
		strip(header);
		if (strlen(header) == 0) {
			break;
		}
		fprintf(stderr, "header: %s\n", header);

		char *inp = header;
		char *key;
		char *value;
		key = strsep(&inp, ":");
		value = inp;
		strip(key);
		if (key) {
			fprintf(stderr, "key: %s\n", key);
		}
		strip(value);
		if (value) {
			fprintf(stderr, "value: %s\n", value);
		}

		if (strcasecmp(key, "Lambda-Runtime-Aws-Request-Id") == 0) {
			strcpy(request_id, value);
			fprintf(stderr, "requestId: %s\n", request_id);
			found = 1;
		}
	}
	if (!found) {
		return 1;
	}

	// parse body
	json_t *json = json_loadf(fp, JSON_ENCODE_ANY, json_error_ret);
	if (!json) {
		return 2;
	}

	*json_ret = json;
	return 0;
}

int main(void)
{
	CURL *curl;
	CURLcode res;

	char url[MAX_URL_LEN];

	fprintf(stderr, "start function\n");

	char *api = getenv("AWS_LAMBDA_RUNTIME_API");
	fprintf(stderr, "AWS_LAMBDA_RUNTIME_API: %s\n", api);

	curl_global_init(CURL_GLOBAL_ALL);

	FILE *out = tmpfile();
	if (!out) {
		perror("tmpfile");
		return 1;
	}

	// lambda へのリクエストを api から get する
	snprintf(url, MAX_URL_LEN, "http://%s/2018-06-01/runtime/invocation/next", api);
	fprintf(stderr, "get url: %s\n", url);
	res = get(url, out);
	fprintf(stderr, "get response: %d\n", res);
	if (res != CURLE_OK) {
		return 2;
	}
	print_all(out);

	char request_id[REQUEST_ID_LEN];
	json_t *json = NULL;
	json_error_t json_error;
	if (parse_next(out, request_id, &json, &json_error) != 0) {
		fprintf(stderr, "parse_next error");
		return 3;
	}

	fprintf(stderr, "request id: %s\n", request_id);

	// ハンドラーを呼ぶ

	// lambda からの応答を api に post する
	// 通常は OK / NG で post 先を変える。
	// ここでは、固定メッセージを返す
	snprintf(url, MAX_URL_LEN, "http://%s/2018-06-01/runtime/invocation/%s/response", api, request_id);
	fprintf(stderr, "post url: %s\n", url);
	res = post(url, "application/json", "{\"hell\": \"world\"}");
	//res = post(url, "text/plain", "hell world"); // これは駄目だった
	fprintf(stderr, "post response: %d\n", res);
	if (res != CURLE_OK) {
		return 4;
	}
	
	curl_global_cleanup();
	return 0;
}

/*
int main(void)
{
	char buf[BUFSIZ];

	strcpy(buf, "   AAA: BBB   \r\n");
	strip_end(buf);
	printf("strip_end: '%s'\n", buf);
	strip_start(buf);
	printf("strip_start: '%s'\n", buf);

	strcpy(buf, "   AAA: BBB   \r\n");
	strip(buf);
	printf("strip: '%s'\n", buf);
}
*/
