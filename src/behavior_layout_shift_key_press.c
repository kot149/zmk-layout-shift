#define DT_DRV_COMPAT zmk_behavior_layout_shift_key_press

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>
#include <zmk/keys.h>
#include <dt-bindings/zmk/keys.h>
#include <dt-bindings/zmk/modifiers.h>
#include <dt-bindings/zmk/hid_usage.h>
#include <dt-bindings/zmk/hid_usage_pages.h>
#include "layout_shift_map.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// Collect all layout shift map devices
#if DT_HAS_COMPAT_STATUS_OKAY(zmk_layout_shift_map)

static const struct device *layout_map_devs[] = {
    DT_FOREACH_STATUS_OKAY(zmk_layout_shift_map, _LAYOUT_SHIFT_MAP_DEV_REF)
};
#define LAYOUT_MAP_DEV_COUNT ARRAY_SIZE(layout_map_devs)

#else

static const struct device **layout_map_devs = NULL;
#define LAYOUT_MAP_DEV_COUNT 0

#endif

static bool any_layout_shift_active(void) {
    for (size_t d = 0; d < LAYOUT_MAP_DEV_COUNT; d++) {
        if (layout_shift_map_is_active(layout_map_devs[d])) {
            return true;
        }
    }
    return false;
}

// Convert a modifier keycode (e.g. LEFT_CONTROL) to its corresponding mod flag bit.
// Returns 0 if the keycode is not a modifier keycode.
static zmk_mod_flags_t mod_keycode_to_flag(uint32_t keycode) {
    switch (STRIP_MODS(keycode)) {
        case LEFT_CONTROL:  return MOD_LCTL;
        case LEFT_SHIFT:    return MOD_LSFT;
        case LEFT_ALT:      return MOD_LALT;
        case LEFT_GUI:      return MOD_LGUI;
        case RIGHT_CONTROL: return MOD_RCTL;
        case RIGHT_SHIFT:   return MOD_RSFT;
        case RIGHT_ALT:     return MOD_RALT;
        case RIGHT_GUI:     return MOD_RGUI;
        default:            return 0;
    }
}

// Translate modifier bits embedded in a keycode (e.g. LCTL(C)) using modifier-to-modifier
// entries. Maps are applied sequentially so that chaining works (e.g. map1: Ctrl->Cmd,
// map2: Cmd->Alt results in Ctrl->Alt).
static zmk_mod_flags_t translate_embedded_mods(zmk_mod_flags_t mods, bool *changed) {
    zmk_mod_flags_t result = mods;

    for (size_t d = 0; d < LAYOUT_MAP_DEV_COUNT; d++) {
        if (!layout_shift_map_is_active(layout_map_devs[d])) {
            continue;
        }
        zmk_mod_flags_t remaining = result;
        zmk_mod_flags_t translated = 0;

        size_t count = layout_shift_map_entry_count(layout_map_devs[d]);
        for (size_t i = 0; i < count; i++) {
            struct layout_shift_map_entry entry = layout_shift_map_get_entry(layout_map_devs[d], i);
            zmk_mod_flags_t from_mod = mod_keycode_to_flag(entry.from_keycode);
            zmk_mod_flags_t to_mod = mod_keycode_to_flag(entry.to_keycode);
            if (from_mod == 0 || to_mod == 0) {
                continue;
            }
            if (remaining & from_mod) {
                remaining &= ~from_mod;
                translated |= to_mod;
                if (changed != NULL) {
                    *changed = true;
                }
            }
        }
        result = remaining | translated;
    }

    return result;
}

struct lookup_result {
    uint32_t keycode;
    zmk_mod_flags_t matched_opt_mods;
    bool matched;
};

// Lookup mapped keycode by applying active layout maps sequentially (chained).
// The output of one map becomes the input to the next, so map1: A->B, map2: B->C
// produces A->C.
static struct lookup_result lookup_mapped_keycode(uint32_t input_keycode) {
    struct lookup_result result = {
        .keycode = input_keycode,
        .matched_opt_mods = 0,
        .matched = false,
    };

    if (!any_layout_shift_active()) {
        return result;
    }

    zmk_mod_flags_t current_mods = zmk_hid_get_explicit_mods();
    uint32_t current_keycode = input_keycode;

    for (size_t d = 0; d < LAYOUT_MAP_DEV_COUNT; d++) {
        if (!layout_shift_map_is_active(layout_map_devs[d])) {
            continue;
        }

        zmk_mod_flags_t keycode_mods = SELECT_MODS(current_keycode);
        zmk_mod_flags_t total_input_mods = current_mods | keycode_mods;
        uint32_t base_input = STRIP_MODS(current_keycode);

        size_t count = layout_shift_map_entry_count(layout_map_devs[d]);
        for (size_t i = 0; i < count; i++) {
            struct layout_shift_map_entry entry = layout_shift_map_get_entry(layout_map_devs[d], i);

            uint32_t base_us = STRIP_MODS(entry.from_keycode);
            zmk_mod_flags_t us_mods = SELECT_MODS(entry.from_keycode);

            if (base_input != base_us) {
                continue;
            }

            zmk_mod_flags_t required_mods = us_mods & ~entry.optional_mods;
            zmk_mod_flags_t input_required_mods = total_input_mods & ~entry.optional_mods;

            if (required_mods != input_required_mods) {
                continue;
            }

            uint32_t target_base = STRIP_MODS(entry.to_keycode);
            zmk_mod_flags_t target_mods = SELECT_MODS(entry.to_keycode);
            zmk_mod_flags_t final_mods = target_mods | (total_input_mods & entry.optional_mods);

            current_keycode = (final_mods != 0) ? APPLY_MODS(final_mods, target_base) : target_base;
            result.matched_opt_mods |= entry.optional_mods;
            result.matched = true;

            LOG_DBG("LAYOUT_SHIFT: Mapping %08X -> %08X (dev=%s, input_mods: %02X, target_mods: %02X, final: %02X)",
                    input_keycode, current_keycode, layout_map_devs[d]->name,
                    total_input_mods, target_mods, final_mods);
            break;
        }
    }

    if (result.matched) {
        result.keycode = current_keycode;
        return result;
    }

    // No base-keycode mapping found. Try translating modifiers embedded in the keycode
    // (e.g. LCTL(C) -> LGUI(C) when swapping Ctrl/Cmd) using modifier-to-modifier entries.
    zmk_mod_flags_t keycode_mods = SELECT_MODS(input_keycode);
    if (keycode_mods != 0) {
        bool changed = false;
        zmk_mod_flags_t new_keycode_mods = translate_embedded_mods(keycode_mods, &changed);
        if (changed) {
            result.keycode = APPLY_MODS(new_keycode_mods, STRIP_MODS(input_keycode));
            LOG_DBG("LAYOUT_SHIFT: Mapping embedded mods %08X -> %08X (mods: %02X -> %02X)",
                    input_keycode, result.keycode, keycode_mods, new_keycode_mods);
            return result;
        }
    }

    LOG_DBG("LAYOUT_SHIFT: No mapping found for %08X", input_keycode);
    return result;
}

// Storage for tracking key press/release mappings
#define MAX_PRESSED_KEYS 16

struct key_mapping_entry {
    uint32_t original_keycode;
    uint32_t mapped_keycode;
    bool active;
};

struct behavior_layout_shift_key_press_data {
    struct key_mapping_entry pressed_keys[MAX_PRESSED_KEYS];
    zmk_mod_flags_t currently_masked_mods;
};

struct behavior_layout_shift_key_press_config {};

// Helper function to mask unwanted modifiers
static void mask_unwanted_modifiers(struct behavior_layout_shift_key_press_data *data,
                                   zmk_mod_flags_t optional_mods, zmk_mod_flags_t mapped_keycode,
                                   zmk_mod_flags_t current_mods) {
    // Get modifiers that are defined in the mapped keycode
    zmk_mod_flags_t mapped_mods = SELECT_MODS(mapped_keycode);

    // Calculate unwanted modifiers:
    // - NOT in optional_modifiers (required modifiers)
    // - NOT in mapped_keycode modifiers (not needed for output)
    // - BUT in current explicit modifiers (currently active)
    zmk_mod_flags_t required_mods = ~optional_mods;  // Modifiers that are NOT optional
    zmk_mod_flags_t not_needed_mods = ~mapped_mods;  // Modifiers NOT in mapped keycode
    zmk_mod_flags_t unwanted_mods = required_mods & not_needed_mods & current_mods;

    if (unwanted_mods != 0 && unwanted_mods != data->currently_masked_mods) {
        // Clear any previously masked modifiers
        if (data->currently_masked_mods != 0) {
            zmk_hid_masked_modifiers_clear();
        }

        // Apply new modifier mask
        zmk_hid_masked_modifiers_set(unwanted_mods);
        data->currently_masked_mods = unwanted_mods;

        LOG_DBG("LAYOUT_SHIFT: Masking unwanted modifiers: %02X (optional: %02X, mapped: %02X, current: %02X)",
                unwanted_mods, optional_mods, mapped_mods, current_mods);
    }
}

// Helper function to clear modifier mask
static void clear_modifier_mask(struct behavior_layout_shift_key_press_data *data) {
    if (data->currently_masked_mods != 0) {
        zmk_hid_masked_modifiers_clear();
        LOG_DBG("LAYOUT_SHIFT: Cleared modifier mask: %02X", data->currently_masked_mods);
        data->currently_masked_mods = 0;
    }
}

// Helper functions for key mapping storage
static int store_key_mapping(struct behavior_layout_shift_key_press_data *data,
                            uint32_t original_keycode, uint32_t mapped_keycode) {
    for (int i = 0; i < MAX_PRESSED_KEYS; i++) {
        if (!data->pressed_keys[i].active) {
            data->pressed_keys[i].original_keycode = original_keycode;
            data->pressed_keys[i].mapped_keycode = mapped_keycode;
            data->pressed_keys[i].active = true;
            return 0;
        }
    }
    LOG_WRN("LAYOUT_SHIFT: No free slots to store key mapping");
    return -ENOMEM;
}

static uint32_t get_stored_mapping(struct behavior_layout_shift_key_press_data *data,
                                  uint32_t original_keycode) {
    for (int i = 0; i < MAX_PRESSED_KEYS; i++) {
        if (data->pressed_keys[i].active &&
            data->pressed_keys[i].original_keycode == original_keycode) {
            return data->pressed_keys[i].mapped_keycode;
        }
    }
    return 0; // Not found
}

static int remove_key_mapping(struct behavior_layout_shift_key_press_data *data,
                             uint32_t original_keycode) {
    for (int i = 0; i < MAX_PRESSED_KEYS; i++) {
        if (data->pressed_keys[i].active &&
            data->pressed_keys[i].original_keycode == original_keycode) {
            data->pressed_keys[i].active = false;
            return 0;
        }
    }
    return -ENOENT;
}

static int on_layout_shift_key_press_binding_pressed(struct zmk_behavior_binding *binding,
                                                    struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_layout_shift_key_press_data *data = dev->data;

    uint32_t original_keycode = binding->param1;
    struct lookup_result lr = lookup_mapped_keycode(original_keycode);

    LOG_DBG("LAYOUT_SHIFT: Input keycode 0x%08X -> Mapped keycode 0x%08X", original_keycode, lr.keycode);

    // If layout shift is active and mapping occurred, handle unwanted modifier masking
    if (lr.matched) {
        zmk_mod_flags_t current_mods = zmk_hid_get_explicit_mods();
        zmk_mod_flags_t keycode_mods = SELECT_MODS(original_keycode);
        zmk_mod_flags_t total_mods = current_mods | keycode_mods;

        // Use the already found layout entry
        mask_unwanted_modifiers(data, lr.matched_opt_mods, lr.keycode, total_mods);
    }

    // Store the mapping for use during release
    int ret = store_key_mapping(data, original_keycode, lr.keycode);
    if (ret < 0) {
        LOG_ERR("LAYOUT_SHIFT: Failed to store key mapping: %d", ret);
    }

    // Raise the mapped keycode event
    return raise_zmk_keycode_state_changed_from_encoded(
        lr.keycode,
        true, // pressed
        event.timestamp
    );
}

static int on_layout_shift_key_press_binding_released(struct zmk_behavior_binding *binding,
                                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_layout_shift_key_press_data *data = dev->data;

    uint32_t original_keycode = binding->param1;
    uint32_t mapped_keycode;

    // Try to get the stored mapping first
    uint32_t stored_mapping = get_stored_mapping(data, original_keycode);
    if (stored_mapping != 0) {
        mapped_keycode = stored_mapping;
        LOG_DBG("LAYOUT_SHIFT: Using stored mapping for release 0x%08X -> 0x%08X",
                original_keycode, mapped_keycode);

        // Remove the mapping from storage
        remove_key_mapping(data, original_keycode);
    } else {
        // Fall back to recalculating if no stored mapping found
        struct lookup_result lr = lookup_mapped_keycode(original_keycode);
        mapped_keycode = lr.keycode;
        LOG_DBG("LAYOUT_SHIFT: No stored mapping found, recalculating 0x%08X -> 0x%08X",
                original_keycode, mapped_keycode);
    }

    LOG_DBG("LAYOUT_SHIFT: Released input keycode 0x%08X -> Mapped keycode 0x%08X", original_keycode, mapped_keycode);

    // Clear modifier mask when key is released
    // This ensures that modifier masking is only active while layout-shifted keys are pressed
    clear_modifier_mask(data);

    // Release the mapped keycode
    return raise_zmk_keycode_state_changed_from_encoded(
        mapped_keycode,
        false, // released
        event.timestamp
    );
}

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
static const struct behavior_parameter_value_metadata keycode_value_metadata[] = {
    {
        .display_name = "Key",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_HID_USAGE,
    },
};

static const struct behavior_parameter_metadata_set keycode_parameter_set = {
    .param1_values = keycode_value_metadata,
    .param1_values_len = ARRAY_SIZE(keycode_value_metadata),
};

static const struct behavior_parameter_metadata_set metadata_sets[] = {keycode_parameter_set};

static const struct behavior_parameter_metadata layout_shift_key_press_parameter_metadata = {
    .sets_len = ARRAY_SIZE(metadata_sets),
    .sets = metadata_sets,
};
#endif

static const struct behavior_driver_api behavior_layout_shift_key_press_driver_api = {
    .binding_pressed = on_layout_shift_key_press_binding_pressed,
    .binding_released = on_layout_shift_key_press_binding_released,
    .locality = BEHAVIOR_LOCALITY_CENTRAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &layout_shift_key_press_parameter_metadata,
#endif
};

static int layout_shift_key_press_init(const struct device *dev) {
    struct behavior_layout_shift_key_press_data *data =
        (struct behavior_layout_shift_key_press_data *)dev->data;

    // Initialize the pressed keys storage
    for (int i = 0; i < MAX_PRESSED_KEYS; i++) {
        data->pressed_keys[i].active = false;
    }

    // Initialize modifier masking state
    data->currently_masked_mods = 0;

    LOG_INF("Layout Shift Behavior Initialized");
    return 0;
}

// Define behavior instance only if devicetree node exists
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
static struct behavior_layout_shift_key_press_data behavior_layout_shift_key_press_data_0 = {};
static const struct behavior_layout_shift_key_press_config behavior_layout_shift_key_press_config_0 = {};

BEHAVIOR_DT_INST_DEFINE(0, layout_shift_key_press_init, NULL,
                        &behavior_layout_shift_key_press_data_0, &behavior_layout_shift_key_press_config_0,
                        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_layout_shift_key_press_driver_api);
#endif
