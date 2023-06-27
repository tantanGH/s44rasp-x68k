# S44RASP.X

Mercury-Unit S44/A44 data player for Human68k/X680x0 with Raspberry Pi

---

## About This

まーきゅりーゆにっと用PCMデータ(S44/A44)を、まーきゅりーゆにっとの無いX680x0で16bitステレオPCMの品質のまま再生させることを目指したプレーヤーです。

- まーきゅりーゆにっとが無くてもオリジナルの音質でステレオPCM再生が可能
- KMD歌詞表示・アルバムアートワーク表示に対応
- インダイレクトファイルによるプレイリスト演奏に対応

具体的には Raspberry Pi と X680x0 を RS232C(UART) でクロス接続し、PCMデータ保持および再生は Raspberry Pi 側が担当します。X68k側からPCM再生制御を行うとともに、歌詞・アルバムアートワーク表示を行います。

<img src='images/s44rasp-x68k.png'/>

---

## Setup

本ソフトを動作させるには、以下のものが必要です。

* X680x0 (X68000Zも可)
* RS232C(UART) - USBクロスケーブル
* Raspberry Pi 3A+/3B+/4B

---

## Install (Raspberry Pi)

Raspberry Pi Linux 用 S44/A44プレーヤーである [s44rasp](https://github.com/tantanGH/s44rasp) をあらかじめコンパイルしてパスの通った場所に置いておきます。

s44raspd を pip で導入します。

    pip install git+https://github.com/tantanGH/s44rasp-x68k.git

s44raspd を起動します。

    s44raspd [options] <pcm-data-path>
      pcm-data-path ... PCMデータが置かれたディクレトリのルート ここ以下にあるデータを相対パスで公開することになります

    options:
      -a <alsa-device-name>   ... alsa pcm device name (hw:1,0 など。s44rasp にそのまま渡されます)
      -o                      ... OLED表示を行う (s44rasp にそのまま渡されます)
      -d <serial-device-name> ... serial device name (UART直結なら /dev/serial0, USB serialなら /dev/ttyUSB0 など)
      -s <serial-baud-rate>   ... serial baud rate (default:38400)

バックグラウンドで起動する例：

    nohup s44raspd -a hw:3,0 -o -d /dev/ttyUSB0 -s 38400 > s44raspd.log &


---

## Install (X680x0)

S44RAxxx.ZIP をダウンロードして、S44RASP.X をパスの通ったディレクトリにコピーします。


---

## Usage

注意：本プログラムの動作には純正RSDRV.SYSまたはTMSIO.Xが必要です。

注意：本プログラムはTimer-D割り込みを使うので、他のTimer-D割り込みを使う常駐プログラムとは同時に動作できません。また、CONFIG.SYS内のPROCESS=の行をコメントアウトする必要があります。

引数をつけずに実行するか、`-h` オプションをつけて実行するとヘルプメッセージが表示されます。

        S44RASP.X - S44/A44 PCM player over UART version " PROGRAM_VERSION " by tantan
        usage: s44rasp [options] <remote-pcm-path> [<remote-pcm-path> ...]
               s44rasp [options] -k <kmd-file> [<kmd-file> ...]
               s44rasp [options] -i <indirect-file>
        options:
               -l<n> ... loop count (default:1)
               -s    ... shuffle mode
               -t<n> ... album artwork brightness (default:75)
               -b<n> ... baud rate (default:38400)
               -h    ... show help message

サポートしているファイル形式は以下で、拡張子により判断します。

- S16/S22/S24/S32/S44/S48 ... 16bit signed raw PCM stereo (big endian)
- M16/M22/M24/M32/M44/M48 ... 16bit signed raw PCM mono (big endian)


---

## 動作確認環境

以下の環境でのみ動作確認しています。


---

## History

* 0.4.0 (2023/06/xx) ... 初版

---
