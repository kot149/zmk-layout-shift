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

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// External function to check layout shift state
extern bool zmk_layout_shift_is_active(void);

// Keycode mapping structure
struct keycode_mapping {
    uint32_t us_keycode;
    uint32_t target_keycode;
    zmk_mod_flags_t optional_modifiers;  // Bitmask of modifiers that are optional during matching
};

// Convenient modifier mask definitions (defined before including layouts)
#define OPTIONAL_SHIFT  (MOD_LSFT | MOD_RSFT)
#define OPTIONAL_CTRL   (MOD_LCTL | MOD_RCTL)
#define OPTIONAL_ALT    (MOD_LALT | MOD_RALT)
#define OPTIONAL_GUI    (MOD_LGUI | MOD_RGUI)
#define OPTIONAL_ALL    (0xFF)
#define OPTIONAL_NONE   (0)

// Include all layout definitions (with conditional compilation)
#include "layouts/index.h"

#define LAYOUT_MAP_SIZE (sizeof(layout_map) / sizeof(layout_map[0]))


// Function to lookup mapped keycode from input keycode with optional modifier support
// Returns the mapped keycode, and optionally stores the matched layout entry index
static uint32_t lookup_mapped_keycode(uint32_t input_keycode, int *matched_index) {
    // Only apply mapping if layout shift is active
    if (!zmk_layout_shift_is_active()) {
        return input_keycode;
    }

    // Get current explicit modifier state and any modifiers embedded in the keycode
    zmk_mod_flags_t current_mods = zmk_hid_get_explicit_mods();
    zmk_mod_flags_t keycode_mods = SELECT_MODS(input_keycode);

    // Combine explicit modifiers with modifiers from the keycode
    zmk_mod_flags_t total_input_mods = current_mods | keycode_mods;

    // Get the base keycode without modifiers for comparison
    uint32_t base_input = STRIP_MODS(input_keycode);

    // Look up in mapping table with optional modifier support
    for (size_t i = 0; i < LAYOUT_MAP_SIZE; i++) {
        uint32_t base_us = STRIP_MODS(layout_map[i].us_keycode);
        zmk_mod_flags_t us_mods = SELECT_MODS(layout_map[i].us_keycode);

        // Check if base keycodes match
        if (base_input == base_us) {
            // Check if non-optional modifiers match
            zmk_mod_flags_t required_mods = us_mods & ~layout_map[i].optional_modifiers;
            zmk_mod_flags_t input_required_mods = total_input_mods & ~layout_map[i].optional_modifiers;

            if (required_mods == input_required_mods) {
                // Match found! Apply target keycode with layout-defined modifiers
                uint32_t target_base = STRIP_MODS(layout_map[i].target_keycode);
                zmk_mod_flags_t target_mods = SELECT_MODS(layout_map[i].target_keycode);

                // Combine target modifiers with non-optional input modifiers
                zmk_mod_flags_t final_mods = target_mods | (total_input_mods & layout_map[i].optional_modifiers);

                uint32_t result = (final_mods != 0) ? APPLY_MODS(final_mods, target_base) : target_base;

                // Store the matched index if caller wants it
                if (matched_index != NULL) {
                    *matched_index = i;
                }

                LOG_DBG("LAYOUT_SHIFT: Mapping %08X -> %08X (input_mods: %02X, required: %02X, target_mods: %02X, final: %02X)",
                        input_keycode, result, total_input_mods, required_mods, target_mods, final_mods);
                return result;
            }
        }
    }

    // If no mapping found, return original keycode
    if (matched_index != NULL) {
        *matched_index = -1;  // Indicate no match found
    }
    LOG_DBG("LAYOUT_SHIFT: No mapping found for %08X", input_keycode);
    return input_keycode;
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
    int matched_layout_index = -1;
    uint32_t mapped_keycode = lookup_mapped_keycode(original_keycode, &matched_layout_index);

    LOG_DBG("LAYOUT_SHIFT: Input keycode 0x%08X -> Mapped keycode 0x%08X", original_keycode, mapped_keycode);

    // If layout shift is active and mapping occurred, handle unwanted modifier masking
    if (zmk_layout_shift_is_active() && mapped_keycode != original_keycode && matched_layout_index >= 0) {
        zmk_mod_flags_t current_mods = zmk_hid_get_explicit_mods();
        zmk_mod_flags_t keycode_mods = SELECT_MODS(original_keycode);
        zmk_mod_flags_t total_mods = current_mods | keycode_mods;

        // Use the already found layout entry
        mask_unwanted_modifiers(data, layout_map[matched_layout_index].optional_modifiers, mapped_keycode, total_mods);
    }

    // Store the mapping for use during release
    int ret = store_key_mapping(data, original_keycode, mapped_keycode);
    if (ret < 0) {
        LOG_ERR("LAYOUT_SHIFT: Failed to store key mapping: %d", ret);
    }

    // Raise the mapped keycode event
    return raise_zmk_keycode_state_changed_from_encoded(
        mapped_keycode,
        true,  // pressed
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
        mapped_keycode = lookup_mapped_keycode(original_keycode, NULL);
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
        .display_name = "Keycode",
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