# プロジェクトについて
RX65N ターゲットボードにおいてFreeRTOSのデモプロジェクトが動作するように移植したもの。
[Target Board for RX family](https://www.renesas.com/jp/ja/products/software-tools/boards-and-kits/cpu-mpu-boards/rx-family-target-board.html)

動作の詳細についてはFreeRTOSのデモプロジェクトの説明を見てほしい。
ターゲットボードの使い方はRENESASのユーザーズマニュアルを見てほしい。

# 特徴
- Blinky_DemoだけではなくFull_Demoも移植した
- FreeRTOSのサンプルにおいて、RXシリーズには無かった機能のデモも移植した
- 基本的な初期化コードはRENESASのスマートコンフィグレータを使った

# 使い方
RENESAS提供のe2studio v7.0.0 / CCRX 2.7.0上で開発したので、同様の環境であれば動くはず。
一応12時間程度の連続動作でエラーが発生したいことは確認したが、より長期の動作確認はしていない。

# ライセンス
- MITライセンス
