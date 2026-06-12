# ZMK Layout Shift Module

[ English / [日本語](README_ja.md) ]

This module provides a mechanism to dynamically shift keyboard layouts at runtime, primarily intended to solve discrepancies when an OS is configured for a non-US layout (e.g., JIS).

Specifically, this module provides a behavior `&kpls` that maps keycodes according to the current layout shift state. You can override the `&kp` behavior with `&kpls`, which lets it work without modifying your existing keymap, while preserving [Keymap Editor](https://nickcoutsos.github.io/keymap-editor/) compatibility. If you prefer not to override `&kp`, you can also use `&kpls` in your keymap instead.

Multiple layouts can be enabled simultaneously, each with independent on/off state.

## Behaviors

This module defines the following behaviors:

- `&kpls`: A layout-aware version of `&kp`; maps keycodes according to active layout shift maps. For example, `&kpls EQUAL` normally outputs `=`, but outputs `_` (which is `=` in JIS layout) when JIS layout is enabled.
- `&tog_ls`: Toggles layout shift maps on/off (configure target layout(s) via `layout-maps` property)
- `&tog_ls_on`: Turns on layout shift maps
- `&tog_ls_off`: Turns off layout shift maps

These toggle behaviors require the user to configure which layout(s) to control via the `layout-maps` property (see [Usage](#usage)). Multiple layout maps can be specified.

## List of Supported Layouts

- **JIS**: Japanese keyboard layout
- **Dvorak**: Dvorak keyboard layout
- **Swap Ctrl and Cmd**: Swap Ctrl / Cmd for Windows / Mac

## Usage

### 1. Add the Module to your `west.yml`

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

### 2. Configure Toggle Behaviors with Target Layout(s)

Configure the toggle behaviors to point to the desired layout map(s). Layout maps are automatically enabled when referenced. If `layout-maps` is omitted, the toggle controls all available layout maps at once:

```dts
&tog_ls { layout-maps = <&layout_shift_map_jis>; };
&tog_ls_on { layout-maps = <&layout_shift_map_jis>; };
&tog_ls_off { layout-maps = <&layout_shift_map_jis>; };
```

Available layout map nodes:

| Node Label | Layout |
|---|---|
| `layout_shift_map_jis` | Japanese (JIS) |
| `layout_shift_map_dvorak` | Dvorak |
| `layout_shift_map_swap_ctrl_cmd` | Swap Ctrl / Cmd |

You can also define your own custom layout maps. See [Adding New Layouts](#adding-new-layouts) for details.

You can control multiple layouts with a single toggle by specifying multiple phandles:

```dts
&tog_ls { layout-maps = <&layout_shift_map_jis &layout_shift_map_swap_ctrl_cmd>; };
&tog_ls_on { layout-maps = <&layout_shift_map_jis &layout_shift_map_swap_ctrl_cmd>; };
&tog_ls_off { layout-maps = <&layout_shift_map_jis &layout_shift_map_swap_ctrl_cmd>; };
```

> **Note:** When multiple layout maps are active simultaneously, maps are applied sequentially. By default, they are applied in devicetree declaration order (`layouts.dtsi`). The output of one map becomes the input to the next, so if map 1 maps `A → B` and map 2 maps `B → C`, the final output is `C`.
>
> To control the application order explicitly, set the `priority` property on each layout map node — smaller values are applied first. Ties fall back to devicetree declaration order.
>
> ```dts
> &layout_shift_map_jis            { priority = <10>; };
> &layout_shift_map_swap_ctrl_cmd  { priority = <20>; };  // applied after JIS
> ```

### 3. Include `layout_shift_kp_override.dtsi` to Override `&kp` Behavior

`#include` [`layout_shift_kp_override.dtsi`](dts/layout_shift_kp_override.dtsi) to override `&kp` with custom implementation which is layout-aware. This allows to use layout shift without modifying your existing keymap, while preserving [Keymap Editor](https://nickcoutsos.github.io/keymap-editor/) compatibility.

```c
#include <layout_shift_kp_override.dtsi>
```

> [!important]
> You need to add this include \*\***below**\*\* the `#include <behaviors.dtsi>` or other includes to make it work.
> However, [Keymap Editor](https://nickcoutsos.github.io/keymap-editor/) automatically reorders the includes. To avoid this, you can copy-paste the definition of `&kp` from [`layout_shift_kp_override.dtsi`](dts/layout_shift_kp_override.dtsi) directly to your keymap file.

### 4. Use Toggle Behaviors to Control Layout Shift State

Use `&tog_ls`, `&tog_ls_on`, `&tog_ls_off` in your keymap to control layout shift state. Then `&kp` will output the keycode according to the active layout shift maps.

```dts
#include <layout_shift_kp_override.dtsi>

&tog_ls { layout-maps = <&layout_shift_map_jis>; };

/ {
    keymap {
        compatible = "zmk,keymap";

        default_layer {
            bindings = <
                &kp EQUAL      // Will output = normally, but _ (which is = on JIS layout) when JIS layout is active
                &tog_ls         // Toggle JIS layout shift on/off
            >;
        };
    };
};
```

> [!note]
> If you prefer not to override `&kp`, `#include` [`layout_shift.dtsi`](dts/layout_shift.dtsi) instead of [`layout_shift_kp_override.dtsi`](dts/layout_shift_kp_override.dtsi) and use `&kpls` in your keymap instead:
>
> ```dts
> #include <layout_shift.dtsi>
>
> &tog_ls { layout-maps = <&layout_shift_map_jis>; };
>
> / {
>     keymap {
>         compatible = "zmk,keymap";
>
>         default_layer {
>             bindings = <
>                 &kpls EQUAL    // Will output = normally, but _ (which is = on JIS layout) when JIS layout is active
>                 &tog_ls         // Toggle JIS layout shift on/off
>             >;
>         };
>     };
> };
> ```

## Per-Layout Toggle Behaviors (Optional)

If you need separate toggle keys for different layouts (e.g., one key for JIS, another for Swap Ctrl/Cmd), you can define per-layout toggle behaviors in your keymap:

```dts
#include <layout_shift_kp_override.dtsi>

/ {
    behaviors {
        tog_ls_jis: toggle_layout_shift_jis {
            compatible = "zmk,behavior-layout-shift-toggle";
            #binding-cells = <0>;
            toggle-mode = "flip";
            layout-maps = <&layout_shift_map_jis>;
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
                &tog_ls_jis     // Toggle JIS layout
                &tog_ls_swap    // Toggle Swap Ctrl/Cmd layout
            >;
        };
    };
};
```

You can also define `on` / `off` variants by setting `toggle-mode` to `"on"` or `"off"`.

## Adding New Layouts

You can define custom layout maps in your own keymap or `..dtsi` files.

### Define a Layout Map Node

Add a layout map node with `compatible = "zmk,layout-shift-map"`:

```dts
/ {
    layout_shift_map_colemak: layout_shift_map_colemak {
        compatible = "zmk,layout-shift-map";
        mappings = <
            E  F  OPTIONAL_ALL
            R  P  OPTIONAL_ALL
            T  G  OPTIONAL_ALL
            // ... add more mappings as needed
        >;
    };
};
```

Then reference it from a toggle behavior:

```dts
&tog_ls { layout-maps = <&layout_shift_map_colemak>; };
```

Or define a dedicated toggle behavior for it (see [Per-Layout Toggle Behaviors](#per-layout-toggle-behaviors-optional)).

Each mapping consists of three values: `from_keycode`, `to_keycode`, `optional_modifiers`.

**Optional Modifier Control Options:**
- `OPTIONAL_NONE` (0): All modifiers required (exact match)
- `OPTIONAL_SHIFT`: Shift keys are optional during matching
- `OPTIONAL_CTRL`: Ctrl keys are optional during matching
- `OPTIONAL_ALT`: Alt keys are optional during matching
- `OPTIONAL_GUI`: GUI (Windows/Cmd) keys are optional during matching
- `OPTIONAL_ALL` (0xFF): All modifiers optional during matching
- Custom combinations: `OPTIONAL_CTRL | OPTIONAL_ALT` (Ctrl/Alt optional, Shift/GUI required)

References:
- [`zmk/keys.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/keys.h)
- [`zmk/modifiers.h`](https://github.com/zmkfirmware/zmk/blob/main/app/include/dt-bindings/zmk/modifiers.h)
