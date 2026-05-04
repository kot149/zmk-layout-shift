# ZMK Layout Shift モジュール

[ [English](README.md) / 日本語 ]

このモジュールは、主にOSがJIS配列などの非USレイアウトに設定されている場合のキーコードの相違を解消することを目的として、実行時にキーボードレイアウトを動的に切り替える機能を提供します。

具体的には、現在のレイアウトシフト状態に応じてキーコードをマッピングする `&kpls` behaviorを提供します。また、`&kpls` で `&kp` をオーバーライドすることで、既存のキーマップを変更することなく、[Keymap Editor](https://nickcoutsos.github.io/keymap-editor/)との互換性を維持しながら使用することができます。

## Behavior 一覧

このモジュールは以下のbehaviorを定義します。

- `&kpls`: 現在のレイアウトシフト状態に応じてキーコードをマッピングする、レイアウト対応版の `&kp`。例えば、`&kpls EQUAL` は通常は `=` を出力するが、JISレイアウトが有効な場合は `_` (JISレイアウトでの `=` に対応)を出力する
- `&tog_ls`: レイアウトシフト状態のオンオフを切り替える
- `&tog_ls_on`: レイアウトシフト状態をオンにする
- `&tog_ls_off`: レイアウトシフト状態をオフにする

オプションとして、[`layout_shift_kp_override.dtsi`](dts/layout_shift_kp_override.dtsi) を `#include` することで、`&kpls` で `&kp` をオーバーライドできます。これにより、既存のキーマップを変更することなく、[Keymap Editor](https://nickcoutsos.github.io/keymap-editor/)との互換性を維持しながら使用することができます。

## 利用可能なレイアウト一覧

- **JIS**: JISレイアウト
- **Dvorak**: Dvorakレイアウト
- **Swap Ctrl and Cmd**: Windows / Mac向けのCtrl / Cmd入れ替え(注: 現状、純粋な修飾キー押下(`&kp LEFT_CONTROL` など)やmod-tap(`&mt LEFT_CONTROL A` など)でのみ動作します。非修飾キーに適用されたもの(`&kp LCTL(C)` など)に対しては動作しません)

## 使用方法

### 1. `west.yml` にモジュールを追加する

```yml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: kot149
      url-base: https://github.com/kot149
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: zmk-layout-shift
      remote: kot149
      revision: v1
  self:
    path: config
```

### 2. キーマップを編集する

1. キーマップの先頭で [`layout_shift.dtsi`](dts/layout_shift.dtsi) を `#include` する
   ```c
   #include <layout_shift.dtsi>
   ```

2. [`LAYOUT_SHIFT_TARGET_LAYOUT` choice](Kconfig) からターゲットレイアウトを1つ選択し、.confファイルに追記する。例:
   ```kconfig
   CONFIG_LAYOUT_SHIFT_TARGET_JIS=y # 日本語(JIS)レイアウト
   ```
   または
   ```kconfig
   CONFIG_LAYOUT_SHIFT_TARGET_DVORAK=y # Dvorakレイアウト
   ```

3. キーマップに `&tog_ls` / `&tog_ls_on` / `&tog_ls_off` を追加し、`&kp`の代わりに`&kpls`を使用することでレイアウトを切り替え可能にする
   ```dts
   #include <layout_shift.dtsi>

   / {
       keymap {
           compatible = "zmk,keymap";

           default_layer {
               bindings = <
                   &kpls EQUAL    // 通常は = を出力するが、JISレイアウトでは _ (JISレイアウトでの = に対応)を出力する
                   &tog_ls        // レイアウトシフトのオン/オフを切り替える
                   &tog_ls_on     // レイアウトシフトをオンにする
                   &tog_ls_off    // レイアウトシフトをオフにする
               >;
           };
       };
   };
   ```

### 3. `&kp`のオーバーライド(オプション)

[`layout_shift_kp_override.dtsi`](dts/layout_shift_kp_override.dtsi) を `#include` することで、`&kpls` で `&kp` をオーバーライドできます。これにより、既存のキーマップを変更することなく、[Keymap Editor](https://nickcoutsos.github.io/keymap-editor/)との互換性を維持しながら使用することができます。

```c
#include <layout_shift_kp_override.dtsi>
```
Note: `layout_shift_kp_override.dtsi` には `layout_shift.dtsi` も含まれているため、`layout_shift.dtsi` の include は省略できます。

> [!important]
> この include は `#include <behaviors.dtsi>` やその他の include \*\***よりも下**\*\*に追加する必要があります。
> しかし、[Keymap Editor](https://nickcoutsos.github.io/keymap-editor/)を使用すると、勝手に include の順序が並べ替わります。これを避けるには、[`layout_shift_kp_override.dtsi`](dts/layout_shift_kp_override.dtsi) から `&kp` の定義を直接キーマップファイルにコピペしてください。

これで通常通り `&kp` を使用可能になります。

```dts
#include <layout_shift_kp_override.dtsi>

/ {
    keymap {
        compatible = "zmk,keymap";

        default_layer {
            bindings = <
                &kp EQUAL      // 通常は = を出力するが、JISレイアウトでは _ (JISレイアウトでの = に対応)を出力する
                &tog_ls        // レイアウトシフトのオン/オフを切り替え
                &tog_ls_on     // レイアウトシフトをオンにする
                &tog_ls_off    // レイアウトシフトをオフにする
            >;
        };
    };
};
```

## 新しいレイアウトの追加手順

### Step 1: Kconfigオプションの追加

[`Kconfig`](Kconfig) の `choice` ブロックに新しいオプションを追加する。

```kconfig
choice LAYOUT_SHIFT_TARGET_LAYOUT
    prompt "Target keyboard layout"
    default LAYOUT_SHIFT_TARGET_JIS

config LAYOUT_SHIFT_TARGET_JIS
    bool "Japanese (JIS)"

config LAYOUT_SHIFT_TARGET_DVORAK
    bool "Dvorak"

...

config LAYOUT_SHIFT_TARGET_COLEMAK    # この行を追加
    bool "Colemak"                    # この行を追加

endchoice
```

### Step 2: レイアウト定義ファイルの作成

[`src/layouts/`](src/layouts/) に新しいレイアウトファイル(例: `layout_colemak.h`)を作成する。

```c
#ifdef CONFIG_LAYOUT_SHIFT_TARGET_COLEMAK
#define LAYOUT_DEFINED
// Colemak keyboard layout mappings
// Maps US QWERTY keycodes to their Colemak equivalents
static const struct keycode_mapping layout_map[] = {
    /* from -> to, optional_modifiers */
    {E, F, OPTIONAL_ALL},                   // E -> F (all modifiers optional for letters)
    {R, P, OPTIONAL_ALL},                   // R -> P (all modifiers optional for letters)
    {T, G, OPTIONAL_ALL},                   // T -> G (all modifiers optional for letters)
    // ... add more mappings as needed
    // For symbols, you might want to require certain modifiers:
    // {COMMA, W, OPTIONAL_CTRL | OPTIONAL_ALT},  // , -> W (Shift required, Ctrl/Alt optional)
};
#endif
```

**修飾キー制御オプション:**
- `OPTIONAL_NONE` (0): すべての修飾キーが必須(完全一致)
- `OPTIONAL_SHIFT`: Shiftキーは任意
- `OPTIONAL_CTRL`: Ctrlキーは任意
- `OPTIONAL_ALT`: Altキーは任意
- `OPTIONAL_GUI`: GUI(Windows/Cmd)キーは任意
- `OPTIONAL_ALL` (0xFF): すべての修飾キーが任意
- カスタムの組み合わせ: `OPTIONAL_CTRL | OPTIONAL_ALT`(Ctrl/Altは任意、Shift/GUIは必須)

参考:
- [`zmk/keys.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/keys.h)
- [`zmk/modifiers.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/modifiers.h)

### Step 3: Indexファイルへの追加

[`src/layouts/index.h`](src/layouts/index.h) に include 文を追加する。

```c
// Layout index - includes all available layout definitions
// Each layout file contains its own conditional compilation directives

#include "layout_jis.h"
#include "layout_dvorak.h"
...
#include "layout_colemak.h"    // この行を追加

// Ensure at least one layout is defined
#ifndef LAYOUT_DEFINED
#error "No target layout selected. Please select a layout in Kconfig."
#endif
```

### Step 4: ドキュメントの更新

このREADME.mdの「利用可能なレイアウト一覧」セクションに新しいレイアウトを追加する。
