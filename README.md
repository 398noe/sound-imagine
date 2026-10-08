# sound-imagine

ミキシング・マスタリング用のステレオ解析プラグイン。周波数帯域ごとの **レベル・Sideエネルギー・左右相関・左右バランス** を表示します。音声はビット単位でそのまま通過し、追加レイテンシーは0です。

旧版のFFT・描画・ビルド構成を置き換えた2.0です。Windows x64 / VST3と、確認用Standaloneをビルドします。旧版と同じプラグインID（Nyao / Imag）を維持しています。旧版には保存パラメータがなかったため、新しい表示設定は既定値で開始します。

## 使い方

マスターやバスの、確認したい処理の後ろに挿入してください。

- **3D**：横方向が対数周波数、奥行きがSideエネルギー割合、高さが帯域RMS（dBFS）。表示角度は固定で、軸を見失わず比較できます。
- **Map**：周波数とSideエネルギーの平面表示。点の大きさが帯域レベルです。
- **色**：緑は相関+1、砂色は0、コーラルは-1。片チャンネルしかないときは相関を定義できないのでグレーです。
- 点にマウスを合わせると、下部に帯域範囲、RMS、Side%、相関、左右バランス、モノラル和の相対レベルを表示します。
- **Freeze**：表示を保持します。解析と音声通過は続きます。再度押すとライブ表示に戻ります。
- **Floor**：-48 / -72 / -90 dBFS。これより小さい点は隠れます。解析値自体は変わりません。
- **?**：指標の読み方を表示します。

3D / MapとFloorはDAWのプロジェクトに保存します。Freezeとヘルプは一時的な表示操作です。入力更新が500 ms以上止まると `NO RECENT AUDIO` と表示し、最後の測定値を保持します。デジタル無音が処理されている間はLIVEですが、点は徐々に消えます。

## 「ステレオの位相」をどう読むか

ステレオかモノラルかは単一の位相値で判定できません。**Sideの多さ、左右の相関、左右の偏りを合わせて確認**します。

| 入力 | Sideエネルギー | 左右相関 | バランス |
|---|---:|---:|---|
| L = R | 0% | +1 | 中央 |
| L = -R | 100% | -1 | 中央 |
| 等パワーの無相関L/R | 約50% | 約0 | 約中央 |
| 左のみ | 50% | 定義不可 `--` | 100% L |
| R = 0.5 L | 10% | +1 | 60% L |

例えば、Side 50%というだけでは、広がった音と左だけの音を区別できません。逆相の音はSideが100%になりますが、モノラル和では打ち消されます。

`Mono sum`は `M=(L+R)/2` のパワーをステレオの平均チャンネルパワーと比較した値です。片チャンネルだけの場合も-3.01 dBになります。モノラル出力のラウドネス、聴感上の良し悪し、ホスト固有のパン則を評価する値ではありません。相関0には無相関ノイズだけでなく90度の位相差のトーンも含まれます。詳しい式と限界は [解析仕様](docs/analysis.md) に記載しています。

## ビルド

GitとMSVC Build Tools（C++デスクトップ開発、Windows SDK、CMake/Ninja）を使用します。通常のPowerShellから実行できます。noenoeと同じ環境検出スクリプトを使います。

```powershell
git submodule update --init --recursive
.\scripts\build.ps1 -Preset debug
.\scripts\build.ps1 -Preset release
```

JUCEは **8.0.15** (`91ad83ae34a81e0833b1a2b0866f54846370ae53`) に固定。C++20、CMake 3.24以上。スクリプトはvswhereでx64ツールを検出し、configure・build・CTestを実行します。キャッシュを再作成するときは `-Fresh` を付けてください。

VS Codeでは推奨拡張のC/C++とCMake Toolsを使い、debug / releaseプリセットを選択します。手動ビルドの場合はx64開発シェルから `cmake --preset release`、`cmake --build --preset release`、`ctest --preset release` を実行します。

| 生成物 | Releaseの場所 |
|---|---|
| VST3バンドル | `build/release/SoundImagine_artefacts/Release/VST3/SoundImagine.vst3` |
| Standalone | `build/release/SoundImagine_artefacts/Release/Standalone/SoundImagine.exe` |
| 検証画像 | `build/release/verification/` |

Debugは上のパスのrelease / Releaseをdebug / Debugに置き換えます。VST3は外側の **SoundImagine.vst3フォルダ全体** を配置してください。ビルド時の自動インストールは無効です。

```powershell
.\scripts\install-vst3.ps1
# 別の検索フォルダを使う場合
.\scripts\install-vst3.ps1 -InstallPath 'D:\Audio\VST3'
```

既定の配置先は `C:\Program Files\Common Files\VST3`。そこへの書き込み権限が必要です。DAWを閉じて旧版のバンドルを置き換え、プラグインを再スキャンしてください。Standaloneは音声デバイスを設定して入力を解析できます。JUCEのStandaloneはフィードバック防止の入力ミュートを初期状態で有効にします。

## 構成・検証

- `src/Analyzer.*`：8192点Hann FFT、32対数帯域、パワーとクロススペクトルの平滑化。
- `src/PluginProcessor.*`：音声通過、固定容量SPSCキュー、解析スレッド、表示設定の保存。
- `src/PluginEditor.*`：ソフトウェア描画による3D投影とMap。OpenGLは不要です。
- `tests/Tests.cpp`：32〜192 kHzで同相・逆相・90度位相差・片側の信号、RMS正規化、無相関ノイズ、無音、非有限値、再初期化、音声通過、状態保存、画面レンダリングを検証。

生成したVST3バンドル自体もテストホストでスキャン・ロードし、音声通過、状態保存、ネイティブウィンドウへのエディタ接続・再表示を確認します。Freeze中に新しい音声を入力しても描画が保持されることと、FIFO過負荷から復帰することも検証します。検証結果は [検証記録](docs/verification.md) を参照してください。

解析が追いつかない場合は音声を待たせず解析用サンプルを破棄し、キューと解析窓をリセットします。画面下にスキップ数を表示します。オフライン高速レンダリング中などの表示は連続測定を保証しません。

CTestはDAW内の実機テストを代替しません。実際のホストで読み込み、再生・停止・シーク、サンプルレート変更、ウィンドウの再表示、プロジェクト保存・再読込を確認してください。Windows x64で検証します。その他のOS用ビルドは未検証です。

## ライセンス

このリポジトリは [AGPL-3.0](LICENSE)。JUCEの利用条件は [固定バージョンのライセンス](https://github.com/juce-framework/JUCE/blob/8.0.15/LICENSE.md) を参照してください。
