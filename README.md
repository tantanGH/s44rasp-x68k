# S44RASP.X

S44/A44 player with Raspberry Pi for Human68k/X680x0

---

## About This

X680x0 の RS232C(UART) インターフェイスと Raspberry Pi を活用した、まーきゅりーゆにっと用PCMデータのプレーヤーです。以下のような特長があります。

- まーきゅりーゆにっとが無くてもオリジナルの音質でステレオPCM再生が可能
- KMD歌詞表示・アルバムアートワーク表示に対応
- インダイレクトファイルによるプレイリスト演奏に対応

基本的には [S44EXP.X](https://github.com/tantanGH/s44exp) のPCM再生部分をラズパイに外出しした形となっています。

---

## Setup

本ソフトを動作させるには、以下のものが必要です。

* X680x0 (X68000Zも可)
* RS232C(UART) - USBクロスケーブル
* Raspberry Pi 3A+/3B+/4B

---

## Install (Raspberry Pi)

Raspberry Pi Linux 用 S44/A44プレーヤーである [s44rasp](https://github.com/tantanGH/s44rasp) を予め導入しておきます。

s44raspd を pip で導入します。

s44raspd を起動します。

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
