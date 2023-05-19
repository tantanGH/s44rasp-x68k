# S44P_RAS.X

A simple S44/WAV/ADPCM player for ras68k-ext on Human68k/X680x0

---

## About This

X680x0 のパラレルインターフェイスと Raspberry Pi + Pico を活用したレトロ音源システムである ras68k-ext 専用の S44/WAV/ADPCM プレーヤーです。以下のような特長があります。

- まーきゅりーゆにっとが無くても高品質のステレオPCM再生が可能
- ras68-ext の機能を生かして、リバーブ効果をかけることが可能
- ディスクからの逐次読み込みにより長い曲が演奏可能

ras68k-ext については、開発者である opmregister氏のサイトを参照してください。

技術資料
* [http://opmregisters.web.fc2.com/ras68k/](http://opmregisters.web.fc2.com/ras68k/)

BOOTH
* [https://booth.pm/ja/items/1178236](https://booth.pm/ja/items/1178236)


注意：本プログラムは 2023年5月より頒布の始まった、Raspberry Pi Picoを搭載した新バージョンのras68k-extシステムのみを想定しています。(前バージョンについてはハードウェアを所有しておらず検証不可能なため)

---

## Install

S44PRxxx.ZIP をダウンロードして、S44P_RAS.X をパスの通ったディレクトリにコピーします。

---

## Usage

注意：本プログラムの動作には ras68k-ext サポートライブラリ兼ドライバである PIlib.X の導入と常駐が必要になります。PIlib.X が常駐していない場合はエラーとなり起動できません。


引数をつけずに実行するか、`-h` オプションをつけて実行するとヘルプメッセージが表示されます。

    usage: s44p_ras [options] <input-file[.pcm|.sXX|.mXX|.wav]>
    options:
         -v[n] ... volume (1-15, default:7)
         -r[n] ... reverb type (0-7, default:1)
         -q[n] ... quality (0:high, 1:normal, default:1)
         -b[n] ... buffer size (1-32, default:4)
         -h    ... show help message

サポートしているファイル形式は以下で、拡張子により判断します。

- PCM ... X68k ADPCM 15.6kHz mono
- S16/S22/S24/S32/S44/S48 ... 16bit signed raw PCM stereo (big endian)
- M16/M22/M24/M32/M44/M48 ... 16bit signed raw PCM mono (big endian)
- WAV ... 16bit signed PCM stereo/mono (little endian)

リバーブタイプ(`-r`)は以下の8種類から選択できます。デフォルトは1のroomです。

* -r0 ... no reverb
* -r1 ... room (default)
* -r2 ... studio small
* -r3 ... studio medium
* -r4 ... studio large
* -r5 ... hall
* -r6 ... space echo
* -r7 ... half echo

再生品質(`-q`)は以下の2通りです。デフォルトは1です。

* -q0 ... オリジナルの周波数、ビット数のまま再生を試みます。転送が間に合わない場合は音が途切れます。
* -q1 ... ハーフレート、ハーフビットでの再生になります。ADPCMについてはオリジナルのままです。

---

## Special Thanks

* ras68k-ext ハードウェア, PIlib.X および技術資料 / opmregistersさん
* xdev68k thanks to ファミべのよっしんさん
* HAS060.X on run68mac thanks to YuNKさん / M.Kamadaさん / GOROmanさん
* HLK301.X on run68mac thanks to SALTさん / GOROmanさん

---

## History

* 0.1.0 (2023/05/19) ... 初版

---
