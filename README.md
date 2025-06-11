# ZMK Layout Shift Module

This module provides a mechanism to dynamically shift keyboard layouts at runtime, primarily intended to solve discrepancies when an OS is configured for a non-US layout (e.g., JIS).

By overriding the `&kp` behavior, it works without modifying your keymap, while preserving [Keymap Editor](https://nickcoutsos.github.io/keymap-editor/) compatibility.

## Behaviors

This module defines the following behaviors:

- `&kpls`: A layout-aware version of `&kp`; maps keycodes according to the current layout shift state
- `&tog_ls`: Toggles the layout shift state
- `&tog_ls_on`: Turns on the layout shift state
- `&tog_ls_off`: Turns off the layout shift state

Optionally, you can `#include` [`layout_shift_kp_override.dtsi`](dts/layout_shift_kp_override.dtsi) to override the `&kp` behavior with `&kpls`, so that you can use layout shift without modifying your keymap, while preserving [Keymap Editor](https://nickcoutsos.github.io/keymap-editor/) compatibility.

## List of Supported Layouts

- **JIS**: Japanese keyboard layout (default)
- **Dvorak**: Dvorak keyboard layout

## Usage

### 1. Add the Module to your `west.yml`

Include this module in your `west.yml` manifest file:

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

1. `#include` [`layout_shift.dtsi`](dts/layout_shift.dtsi) at the top of your keymap:
   ```c
   #include <layout_shift.dtsi>
   ```

2. Select the target layout by selecting one from [`LAYOUT_SHIFT_TARGET_LAYOUT` choice](Kconfig) in your configuration file (e.g., `your_keyboard.conf`):
   ```kconfig
   # Japanese (JIS) layout
   CONFIG_LAYOUT_SHIFT_TARGET_JIS=y

   # or

   # Dvorak layout
   CONFIG_LAYOUT_SHIFT_TARGET_DVORAK=y
   ```

3. Use `&kpls` / `&tog_ls` / `&tog_ls_on` / `&tog_ls_off` in your keymap to make your keyboard layout-aware:
   ```dts
   #include <layout_shift.dtsi>

   / {
       keymap {
           compatible = "zmk,keymap";

           default_layer {
               bindings = <
                   &kpls EQUAL    // Will output = normally, but _ when layout shift is active for JIS layout
                   &tog_ls        // Toggle layout shift on/off
                   &tog_ls_on     // Turn layout shift on
                   &tog_ls_off    // Turn layout shift off
               >;
           };
       };
   };
   ```

### 3. Override `&kp` Behavior (Optional)

You can `#include` [`layout_shift_kp_override.dtsi`](dts/layout_shift_kp_override.dtsi) to override the `&kp` behavior with `&kpls`, so that you can use layout shift without modifying your keymap, while preserving [Keymap Editor](https://nickcoutsos.github.io/keymap-editor/) compatibility.
```dts
#include <layout_shift_kp_override.dtsi>

/ {
    keymap {
        compatible = "zmk,keymap";

        default_layer {
            bindings = <
                &kp EQUAL      // Will output = normally, but _ when layout shift is active for JIS layout
                &tog_ls        // Toggle layout shift on/off
                &tog_ls_on     // Turn layout shift on
                &tog_ls_off    // Turn layout shift off
            >;
        };
    };
};
```

In this case, you can omit `layout_shift.dtsi` as it's also included in `layout_shift_kp_override.dtsi`.

## Adding New Layouts

### Step 1: Add Kconfig Option

Add a new option to the `choice` block in `Kconfig`:

```kconfig
choice LAYOUT_SHIFT_TARGET_LAYOUT
    prompt "Target keyboard layout"
    default LAYOUT_SHIFT_TARGET_JIS

config LAYOUT_SHIFT_TARGET_JIS
    bool "Japanese (JIS)"

config LAYOUT_SHIFT_TARGET_DVORAK
    bool "Dvorak"

config LAYOUT_SHIFT_TARGET_COLEMAK    # Add this line
    bool "Colemak"                    # Add this line

endchoice
```

### Step 2: Create Layout Definition File

Create a new layout file in `src/layouts/` (e.g., `layout_colemak.h`):

```c
#ifdef CONFIG_LAYOUT_SHIFT_TARGET_COLEMAK
#define LAYOUT_DEFINED
// Colemak keyboard layout mappings
// Maps US QWERTY keycodes to their Colemak equivalents
static const struct keycode_mapping layout_map[] = {
    /* Letter position changes (QWERTY -> Colemak) */
    {E, F},                   // E -> F
    {R, P},                   // R -> P
    {T, G},                   // T -> G
    // ... add more mappings as needed
};
#endif
```

### Step 3: Include in Index File

Add the include statement to `src/layouts/index.h`:

```c
// Layout index - includes all available layout definitions
// Each layout file contains its own conditional compilation directives

#include "layout_jis.h"
#include "layout_dvorak.h"
#include "layout_colemak.h"    // Add this line

// Ensure at least one layout is defined
#ifndef LAYOUT_DEFINED
#error "No target layout selected. Please select a layout in Kconfig."
#endif
```

### Step 4: Update Documentation

Update this README.md to list the new layout in the "List of Supported Layouts" section.
