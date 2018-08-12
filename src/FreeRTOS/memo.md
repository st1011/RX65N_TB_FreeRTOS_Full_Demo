# メモ
## 階層構成

FreeRTOS
│  FreeRTOSConfig.h
│
├─Demo
│  │  main.c
│  │  ParTest.c
│  │  RegTest.src
│  │  ApplicationHook.c
│  │
│  ├─Blinky_Demo
│  │      main_blinky.c
│  │
│  ├─Common
│  │  │  ReadMe.txt
│  │  │
│  │  ├─drivers
│  │  ├─Full
│  │  ├─include
│  │  └─Minimal
│  │
│  └─Full_Demo
│          IntQueueTimer.c
│          IntQueueTimer.h
│          main_full.c
│
└─Source
    ├─include
    └─portable
        ├─MemMang
        └─Renesas
            └─RX600v2
                    port.c
                    portmacro.h
                    port_asm.src

### 構成の説明

FreeRTOS
FreeRTOSConfig.hはOSの設定をいろいろいじる
動作クロックが変わると変更が必要だったりするのでよく見ること

├─Demo
デモ（というよりはFreeRTOS機能のデバッグ）用ソース

main.cはデモ関係の呼び出しルート

ParTest.cはデモで使うパーツっぽいもの
主にLED駆動用

RegTest.srcはそのマイコンで使われているレジスタのテストタスク用
レジスタに既知の値を書き込んで、読み込みで値が変わっていないかを延々と繰り返す

ParTest.c / RegTest.srcはportableにいく方がいいと思うんだけど
サンプルがportableに入れていないので、こっちに置いておく

│  ├─Blinky_Demo
はLEDを点滅させるタスク
内部ではqueueを使っている
ソフトｔｉｍｅｒを使うバージョンもあるようだけど、Fullでやっているので無しにしておく

│  └─Full_Demo
はいろいろなOS機能をチェックする
参考にするDemoプロジェクトによっては内容が古かったりするので、
FreeRTOSで最近修正されたプロジェクトのものを参考にすると良いかも
MSVCのものは大抵最新っぽいけれど、パソコン用なので少し特殊

│  ├─Common
は各デモで共通に使われるものが入っている
ただ、実際に使うのはincludeとMinimalだけ
Fullはfull_demoで使いそうだけど、実際には使っていないっぽい
（readme.txtを見てほしい）
使わないファイルはビルドから除外している

└─Source
FreeRTOSのソース
portableは使用するマイコンに応じてポーティング（移植）するファイル群

        ├─MemMang
はメモリ管理用のファイルが入っていて、実際に使用する一つだけをビルドする格好になる
いまはheap_4.cを使っている

## 移植に際して
変更の必要があるのは
- Demo直下のCommon以外
- portableの中

また、本来は変更不要のはずだけど
C:\WorkSpace\e2s\rx65n_target_board\src\FreeRTOS\Demo\Common\Minimal\StreamBufferDemo.c
のL781の待ち時間を少し変更している
これは、デフォルト値だと時々アサートで止まってしまうため……

