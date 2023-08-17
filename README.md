# aws-sam-custom-runtime-example-in-c

AWS SAM では、GO や RUST もサポートされている。

であれば C でも作れるのか？と思い試してみたコード。

解説については、[AWS SAM のカスタムランタイム](https://www.runserver.jp/posts/aws-sam-custom-runtime-in-c/) に投稿。

## 準備

1. Amazon Linux 2 インスタンスを立ち上げる
1. SAM をインストール - [SAM インストール手順](https://docs.aws.amazon.com/ja_jp/serverless-application-model/latest/developerguide/install-sam-cli.html)
1. yum install clang autoconf automake libtool
1. git clone https://github.com/akheron/jansson.git
1. cd jansson 
1. ./configure --prefix=/opt/jansson
1. make
1. make install

## ビルドとデプロイ

1. git clone https://github.com/moriya9n/aws-sam-custom-runtime-example-in-c.git
1. cd aws-sam-custom-runtime-example-in-c/
1. sam build && sam deploy
1. 不要になったら sam delete で削除

URL が表示されるので、表示された URL にアクセスして確認。


