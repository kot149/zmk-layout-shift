# ZMK Layout Shiftモジュール

[[English](README.md)/日本語]

このモジュールは、主にOSがJIS配列などの非USレイアウトに設定されている場合のキーコードの相違を解消することを目的として、実行時にキーボードレイアウトを動的に切り替える機能を提供します。

具体的には、現在のレイアウトシフト状態に応じてキーコードをマッピングする`&kpls` behaviorを提供します。`&kp` behaviorを`&kpls`でオーバーライドすることで、既存のキーマップを変更せずに利用でき、[Keymap Editor](https://nickcoutsos.github.io/keymap-editor/)との互換性が保てます。`&kp`をオーバーライドしたくない場合は、代わりに`&kpls`を使うこともできます。

複数のレイアウトを同時に有効化でき、それぞれ独立したオン/オフ状態を持ちます。

## Behavior一覧

このモジュールは以下のbehaviorを定義します。

- `&kpls`: 現在のレイアウトシフト状態に応じてキーコードをマッピングする、レイアウト対応版の`&kp`。例えば、`&kpls EQUAL`は通常は`=`を出力するが、US -> JISレイアウトが有効な場合は`_`(JISレイアウトでの`=`に対応)を出力する
- `&tog_ls`: レイアウトシフトマップのオンオフを切り替える
- `&tog_ls_on`: レイアウトシフトマップをオンにする
- `&tog_ls_off`: レイアウトシフトマップをオフにする

これらのトグルbehaviorは、`layout-maps`プロパティで制御対象のレイアウトを指定する必要があります（[使用方法](#使用方法)を参照）。複数のレイアウトマップを指定できます。

## 利用可能なレイアウト一覧

- **US -> JIS**: US配列前提のキーコードをJIS配列設定のOS向けに変換
- **Dvorak**: Dvorakレイアウト
- **Swap Ctrl and Cmd**: Windows/Mac間のCtrl/Cmd入れ替え

自分で新しいレイアウトマップを定義することもできます。詳細は[新しいレイアウトの追加手順](#新しいレイアウトの追加手順)を参照してください。

## 使用方法

### 1. `west.yml`にモジュールを追加する

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
      revision: v2
  self:
    path: config
```

### 2. トグルbehaviorに対象レイアウトを設定する

トグルbehaviorが制御するレイアウトマップを設定します。レイアウトマップは参照されると自動的に有効化されます。`layout-maps`を省略すると、利用可能なすべてのレイアウトマップを一括制御します:

```dts
&tog_ls { layout-maps = <&layout_shift_map_us_to_jis>; };
&tog_ls_on { layout-maps = <&layout_shift_map_us_to_jis>; };
&tog_ls_off { layout-maps = <&layout_shift_map_us_to_jis>; };
```

利用可能なレイアウトマップノード:

| ノードラベル | レイアウト |
|---|---|
| `layout_shift_map_us_to_jis` | US -> JIS |
| `layout_shift_map_dvorak` | Dvorak |
| `layout_shift_map_swap_ctrl_cmd` | Ctrl/Cmd入れ替え |

自分で新しいレイアウトマップを定義することもできます。詳細は[新しいレイアウトの追加手順](#新しいレイアウトの追加手順)を参照してください。

1つのトグルで複数のレイアウトを制御するには、複数のphandleを指定します:

```dts
&tog_ls { layout-maps = <&layout_shift_map_us_to_jis &layout_shift_map_swap_ctrl_cmd>; };
&tog_ls_on { layout-maps = <&layout_shift_map_us_to_jis &layout_shift_map_swap_ctrl_cmd>; };
&tog_ls_off { layout-maps = <&layout_shift_map_us_to_jis &layout_shift_map_swap_ctrl_cmd>; };
```

> **Note:** 複数のレイアウトマップが同時に有効な場合、マップは逐次適用されます。デフォルトではdevicetreeの宣言順（[`layout_shift_maps.dtsi`](dts/layout_shift_maps.dtsi)）に適用され、あるマップの出力が次のマップの入力になります。マップ1が`A -> B`、マップ2が`B -> C`にマッピングする場合、最終出力は`C`になります。
>
> 適用順を明示的に制御したい場合は、各レイアウトマップノードに`priority`プロパティを設定してください。値が小さいものから先に適用されます。
>
> ```dts
> &layout_shift_map_us_to_jis { priority = <1>; };
> &layout_shift_map_swap_ctrl_cmd { priority = <2>; }; // JISの後に適用
> ```

### 3. `layout_shift_kp_override.dtsi`をincludeして、`&kp` behaviorをオーバーライドする

[`layout_shift_kp_override.dtsi`](dts/layout_shift_kp_override.dtsi)を`#include`することで、レイアウト対応の実装で`&kp`を置き換えます。これにより、既存のキーマップを変更せずに利用でき、[Keymap Editor](https://nickcoutsos.github.io/keymap-editor/)との互換性が保てます。

```c
#include <layout_shift_kp_override.dtsi>
```

> [!important]
> このincludeは`#include <behaviors.dtsi>`やその他のinclude \*\***よりも下**\*\*に追加する必要があります。
> しかし、[Keymap Editor](https://nickcoutsos.github.io/keymap-editor/)を使用すると、勝手にincludeの順序が並べ替わります。これを避けるには、[`layout_shift_kp_override.dtsi`](dts/layout_shift_kp_override.dtsi)から`&kp`の定義を直接キーマップファイルにコピペしてください。

### 4. トグルbehaviorでレイアウトシフト状態を切り替える

キーマップに`&tog_ls` / `&tog_ls_on` / `&tog_ls_off`を追加し、レイアウトシフト状態を切り替えます。`&kp`は有効なレイアウトシフトマップに応じたキーコードを出力します。

```dts
#include <layout_shift_kp_override.dtsi>

&tog_ls { layout-maps = <&layout_shift_map_us_to_jis>; };
&tog_ls_on { layout-maps = <&layout_shift_map_us_to_jis>; };
&tog_ls_off { layout-maps = <&layout_shift_map_us_to_jis>; };

/ {
    keymap {
        compatible = "zmk,keymap";

        default_layer {
            bindings = <
                &kp EQUAL    // 通常は=を出力するが、US -> JISレイアウトシフトが有効なときは_(JISレイアウトでの=に対応)を出力する
                &tog_ls      // US ->JISレイアウトシフトのオン/オフを切り替え
                &tog_ls_on   // US -> JISレイアウトシフトをオンにする
                &tog_ls_off  // US -> JISレイアウトシフトをオフにする
            >;
        };
    };
};
```

> [!note]
> `&kp`をオーバーライドしたくない場合は、[`layout_shift_kp_override.dtsi`](dts/layout_shift_kp_override.dtsi)の代わりに[`layout_shift.dtsi`](dts/layout_shift.dtsi)を`#include`し、`&kpls`を代わりに使います:
>
> ```dts
> #include <layout_shift.dtsi>
>
> &tog_ls { layout-maps = <&layout_shift_map_us_to_jis>; };
> &tog_ls_on { layout-maps = <&layout_shift_map_us_to_jis>; };
> &tog_ls_off { layout-maps = <&layout_shift_map_us_to_jis>; };
>
> / {
>     keymap {
>         compatible = "zmk,keymap";
>
>         default_layer {
>             bindings = <
>                 &kpls EQUAL  // 通常は=を出力するが、US -> JISレイアウトシフトが有効なときは_(JISレイアウトでの=に対応)を出力する
>                 &tog_ls      // US -> JISレイアウトシフトのオン/オフを切り替え
>                 &tog_ls_on   // US -> JISレイアウトシフトをオンにする
>                 &tog_ls_off  // US -> JISレイアウトシフトをオフにする
>             >;
>         };
>     };
> };
> ```

## レイアウト別トグルbehavior（オプション）

レイアウトごとに個別のトグルキーが必要な場合（例: JIS用のキーとCtrl/Cmd入れ替え用のキーを分けたい場合）、キーマップ内でレイアウト別のトグルbehaviorを定義できます:

```dts
#include <layout_shift_kp_override.dtsi>

/ {
    behaviors {
        tog_ls_jis: toggle_layout_shift_jis {
            compatible = "zmk,behavior-layout-shift-toggle";
            #binding-cells = <0>;
            toggle-mode = "flip";
            layout-maps = <&layout_shift_map_us_to_jis>;
        };

        tog_ls_swap: toggle_layout_shift_swap {
            compatible = "zmk,behavior-layout-shift-toggle";
            #binding-cells = <0>;
            toggle-mode = "flip";
            layout-maps = <&layout_shift_map_swap_ctrl_cmd>;
        };
    };

    keymap {
        compatible = "zmk,keymap";

        default_layer {
            bindings = <
                &tog_ls_us_to_jis  // US -> JISレイアウトシフトの切り替え
                &tog_ls_swap       // Ctrl/Cmd入れ替えレイアウトを切り替え
            >;
        };
    };
};
```

`toggle-mode`を`"on"`または`"off"`に設定することで、オン/オフ専用のbehaviorも定義できます。

## 新しいレイアウトの追加手順

カスタムレイアウトマップは、キーマップや`.dtsi`ファイル内で定義できます。

### レイアウトマップノードの定義

`compatible = "zmk,layout-shift-map"`を持つレイアウトマップノードを追加します:

```dts
/ {
    layout_shift_map_colemak: layout_shift_map_colemak {
        compatible = "zmk,layout-shift-map";
        mappings = <
            E  F  LAYOUT_SHIFT_OPTIONAL_ALL
            R  P  LAYOUT_SHIFT_OPTIONAL_ALL
            T  G  LAYOUT_SHIFT_OPTIONAL_ALL
            // ... 必要に応じてマッピングを追加
        >;
    };
};
```

トグルbehaviorから参照します:

```dts
&tog_ls { layout-maps = <&layout_shift_map_colemak>; };
```

または、専用のトグルbehaviorを定義します（[レイアウト別トグルbehavior](#レイアウト別トグルbehaviorオプション)を参照）。

各マッピングは3つの値で構成されます: `変換元キーコード`、`変換先キーコード`、`オプション修飾キー`。

**修飾キー制御オプション:**
- `LAYOUT_SHIFT_OPTIONAL_NONE`(0): すべての修飾キーが必須(完全一致)
- `LAYOUT_SHIFT_OPTIONAL_SHIFT`: Shiftキーは任意
- `LAYOUT_SHIFT_OPTIONAL_CTRL`: Ctrlキーは任意
- `LAYOUT_SHIFT_OPTIONAL_ALT`: Altキーは任意
- `LAYOUT_SHIFT_OPTIONAL_GUI`: GUI(Windows/Cmd)キーは任意
- `LAYOUT_SHIFT_OPTIONAL_ALL`(0xFF): すべての修飾キーが任意
- カスタムの組み合わせ: `LAYOUT_SHIFT_OPTIONAL_CTRL | LAYOUT_SHIFT_OPTIONAL_ALT`(Ctrl/Altは任意、Shift/GUIは必須)

参考:
- [`zmk/keys.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/keys.h)
- [`zmk/modifiers.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/modifiers.h)
