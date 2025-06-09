# [WIP] ZMK Layout Shift Module

This module provides a mechanism to dynamically shift keyboard layouts at runtime, primarily intended to solve discrepancies when an OS is configured for a non-US layout (e.g., JIS).
By overriding the `&kp` behavior, it works without modifying your keymap, while preserving [Keymap Editor](https://nickcoutsos.github.io/keymap-editor/) compatibility.

> [!Warning]
> 🚧 **This module is work in progress.** 🚧
>
> Use it at your own risk; it may not work as expected, or the behavior may change in the future.

This module defines the following behaviors:

- `&kpls`: A layout-aware version of `&kp`; maps keycodes according to the current layout shift state
- `&tog_ls`: Toggles the layout shift state
- `&tog_ls_on`: Turns on the layout shift state
- `&tog_ls_off`: Turns off the layout shift state

Optionally, you can `#include` [`layout_shift_overlay.dtsi`](dts/layout_shift_overlay.dtsi) to override the `&kp` behavior with `&kpls`, so that you can use layout shift without modifying your keymap, while preserving [Keymap Editor](https://nickcoutsos.github.io/keymap-editor/) compatibility.

## Current Implementation Status

- ✅ `&kpls` behavior core functionality
- ✅ `&kpls` behavior alias
- ✅ `&kp` overlay
- ✅ JIS keyboard layout translation map
- ✅ `&tog_ls`, `&tog_ls_on`, `&tog_ls_off` behavior
- ✅ Save layout shift state between reboots
- 🚧 Support for other layouts

## List of Supported Layouts

- JIS: Japanese keyboard layout

Currently, only JIS is supported and no configuration is available.

## Usage

### 1. Add the Module to your `west.yml`

Include this module in your `west.yml` manifest file (and run `west update` if building locally).

Example:
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

### 2. Update your Keymap

`#include` [`layout_shift.dtsi`](dts/layout_shift.dtsi) to your keymap, then use the following behaviors in your keymap to make your keyboard layout-aware:

- `&kpls`: A layout-aware version of `&kp`; maps keycodes according to the current layout shift state
- `&tog_ls`: Toggles the layout shift state
- `&tog_ls_on`: Turns on the layout shift state
- `&tog_ls_off`: Turns off the layout shift state

Example:
```dts
#include <layout_shift.dtsi>

/ {
    keymap {
        compatible = "zmk,keymap";

        default_layer {
            bindings = <
                &kpls EQUAL    // Will output = normally, but _ when layout shift is active
                &tog_ls        // Toggle layout shift on/off
                &tog_ls_on     // Turn layout shift on
                &tog_ls_off    // Turn layout shift off
            >;
        };
    };
};
```

### 3. Include the Overlay (Optional)

You can also `#include` [`layout_shift_overlay.dtsi`](dts/layout_shift_overlay.dtsi) to override the `&kp` behavior with `&kpls`, so that you can use layout shift without modifying your keymap, while preserving [Keymap Editor](https://nickcoutsos.github.io/keymap-editor/) compatibility.

Example:
```dts
#include <layout_shift_overlay.dtsi>

/ {
    keymap {
        compatible = "zmk,keymap";

        default_layer {
            bindings = <
                &kp EQUAL    // Will output = normally, but _ when layout shift is active
                &tog_ls        // Toggle layout shift on/off
                &tog_ls_on     // Turn layout shift on
                &tog_ls_off    // Turn layout shift off
            >;
        };
    };
};
```

## Configuration

### Persistent State

The layout shift state can be configured to persist between reboots using the following Kconfig option:

- `CONFIG_LAYOUT_SHIFT_PERSISTENT_STATE` (default: `y`): Enable persistent storage of layout shift state across reboots

When enabled, the layout shift state is automatically saved to flash memory whenever it changes and restored on boot. To disable persistent state and always start with layout shift off, add the following to your configuration:

```
CONFIG_LAYOUT_SHIFT_PERSISTENT_STATE=n
```
