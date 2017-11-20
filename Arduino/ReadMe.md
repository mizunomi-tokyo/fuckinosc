<!-- ## Description -->

## Requirement
ハード：

ESP-WROOM-02開発ボード(http://akizukidenshi.com/catalog/g/gK-12236/)

温湿度・気圧センサBME280

加速度・ジャイロ・地磁気センサMPU9250


ソフト：

Arduino IDEでESP8266ボードを使えるようにしてください。

/libraries内にあるArduinoライブラリをインストールしてください。

-BME280

-MPU9250

-quaternionFilters



## Usage
1.書き込み

開発ボード上のRSTボタンとPGMボタンを同時に押し、RST・PGMの順に離すと書き込みモードになります。

Arduino IDEから書き込んでください。


開発ボードとI2Cセンサとの接続は以下の通りです。


  3V - VCC

  GND - GND

  4 - SDA

  5 - SCL


2.Wifi設定

開発ボード上のRSTボタンを押した後、すぐPGMボタンを押し、少し待つとサーバモードで起動します。

シリアルモニタに表示されるSSID/PASSでネットワークに接続してください。


http://192.168.4.1/　にアクセスするとWiFi設定画面が開きます。


<img width="340" alt="2017-11-20 13 48 29" src="https://user-images.githubusercontent.com/30066904/33002784-ac7fd048-cdf9-11e7-8437-1ad911337ad2.png">


接続したいSSIDとそのPASSを入力して"送信"を押せば設定完了です。


3.OSCぶん投げ

開発ボード上のRSTボタンを押すと、OSCにセンサデータをブロードキャストします。

BMEは7000ポート、

MPU9250は7001ポートに設定しています。


## Licence

[MIT](https://github.com/mizunomi/fuckinosc/blob/master/LICENSE)

## Author

[mizunomi hyakushow](https://github.com/mizunomi)
