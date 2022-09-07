# CommonSourceCodeProjectGodot

v0.1.0 (2022/9/7)

## Introduction

CommonSourceCodeProjectGodotは、TAKEDA, toshiyaさんのCommon Source Code Projectの一部をGodot Engine( https://godotengine.org/ )で動作するよう移植したものです。

⇒ [Common Source Code Project](http://takeda-toshiya.my.coocan.jp/)

現状、X1とMSXのみで、ある程度の動作確認が出来ています。その他のものは未確認です。

汎用的なエミュレータとしての利用を想定しておらず、独自開発の旧パソコン用ゲーム(のディスクイメージ等)をこのエミュレーターに組み込んだ上で、Godot Engineにてビルドし、単独アプリ化した上で、Steamやitch.io、自サイトなどにて配布/販売出来れば……という意図で開発しておりますが、諸々権利的な問題があるため、現状では実験的な実装となっています。

※組み込まれているCommonSourceCodeProjectは (8/30/2022)版 です。

## ビルド方法

まずはGDNativeのビルドを行います。

リポジトリ取得後にサブモジュールを更新
```
cd godot-cpp
submodule update --init
cd ..
```

Windowsの場合(Visual StudioのDeveloper Commnand Promptにて動作確認済)
```
scons platform=windows
```

macOSの場合
```
scons platform=osx macos_arch=universal
```

X1の場合は emu=x1 、MSXの場合は emu=msx をコマンドラインに付与してください(省略時はX1になります)。

Linuxでは動作確認を出来ていません。

ビルドが終了すると下記の表示が出ますので、Godot Engineを起動して、プロジェクトを読み込んでください。
```
scons: done building targets.
```

## 起動

Godot Engineのプロジェクトファイルはリポジトリのフォルダ「godotemu」の中にあるので、そちらを起動してください。
Godot Engineのバージョンは v3.5 stable 推奨です。

また、本リポジトリには起動に必要なファイルがいくつか含まれていませんので、適切に、プロジェクトフォルダ(godotemu内)にそれぞれコピーしておいてください。

### X1の場合

必要なものは下記です。

* IPLROM.X1 (IPL ROM。互換IPL可 )
* FNT0808.X1 (フォントファイル。互換フォント可)
* 任意のディスクイメージ

起動後、シーン内の「Main」を選択し、インスペクタの「Drivea Path」「Driveb Path」に、起動時に読み込ませたいフロッピーディスクのパスを記載します(例: プロジェクトフォルダ直下に置いた場合は、 `res://diskimg.d88` 等)。

### MSXの場合
必要なものは下記です。

* MSXJ.ROM または MSX.ROM (CBIOSをリネームしたもので確認しています)

起動後、シーン内の「Main」を選択し、インスペクタの「ROM Path」に、読み込ませたいROMを指定してください。


## 操作方法

現在、必要最低限しか実装していません。

* ALT
  * ローマ字カナ入力モードの切り替え
* ALT + Enter
  * フルスクリーン切り替え
* F12
  * アプリ終了ダイアログの表示

2ボタンジョイスティックに対応しています(ポート1のみ)。

## アプリビルド

普通にビルド出来ますが、デフォルト設定では一部必要なファイルがアプリに含まれませんので、エクスポートの「リソース」タブの「リソース以外のファイル/フォルダーをエクスポートするためのフィルター」のところに、下記のように記載してからビルドしてください。

```
*.d88,*.X1,*.rom,*.ROM,*.BIN
```
※この他、Godotが認識しないファイルをプロジェクトに含めたい場合は都度追加してください。

## 制限

* デバッグ機能は無効です
* プリンタ機能は無効です
* 全体的に実装が雑です
* ディスクイメージは読み書きが必要な関係上、ユーザーフォルダにコピーしてから使われます
* ユーザーフォルダはGodotのユーザーデータのフォルダになります(参考⇒ https://docs.godotengine.org/ja/stable/tutorials/io/data_paths.html )
* ディスクイメージは暗号化して保存されます(独自ゲームのカジュアルコピー防止用。ただし実行ファイルのコピー防止機能などは無いため、その部分が必要な場合は独自で検討、実装してください)
* MSXの(ゲーム)ROMイメージは現状暗号化されません
* 速度調整や音まわりの調整が雑です
* MSXのMapper設定が出来ません(自動判別のみ)

## History
* v0.1.0 (2022/9/7)
  * 初版

## License
GNU General Public License v2.0

※Godot対応を含むエミュレーター自体のライセンスは上記のとおりですが、組み込んだゲームのディスクイメージ内にも適用されるかは未確認です
