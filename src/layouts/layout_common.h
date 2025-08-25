/*
 * MIT License
 *
 * Original code by kot149 (https://github.com/kot149/zmk-layout-shift)
 * See https://opensource.org/licenses/MIT for license details.
 * Forked and modified for Surround1x0-AKDK.
 */

#pragma once

#include <dt-bindings/zmk/modifiers.h>
#include <zmk/hid.h>
#include <zmk/keys.h>

struct keycode_mapping {
    uint32_t us_keycode;
    uint32_t target_keycode;
    zmk_mod_flags_t optional_modifiers;
};

struct msc_mapping {
    uint32_t base;
    uint32_t shifted;
};

#define OPTIONAL_SHIFT (MOD_LSFT | MOD_RSFT)
#define OPTIONAL_CTRL  (MOD_LCTL | MOD_RCTL)
#define OPTIONAL_ALT   (MOD_LALT | MOD_RALT)
#define OPTIONAL_GUI   (MOD_LGUI | MOD_RGUI)
#define OPTIONAL_ALL   (0xFF)
#define OPTIONAL_NONE  (0)
