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
#include <dt-bindings/zmk/hid_usage.h>
#include <dt-bindings/zmk/hid_usage_pages.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// External function to check layout shift state
extern bool zmk_layout_shift_is_active(void);

// Keycode mapping structure
struct keycode_mapping {
    uint32_t us_keycode;
    uint32_t target_keycode;
};

// Include all layout definitions (with conditional compilation)
#include "layouts/index.h"

#define LAYOUT_MAP_SIZE (sizeof(layout_map) / sizeof(layout_map[0]))

// Function to lookup mapped keycode from input keycode considering shift state
static uint32_t lookup_mapped_keycode(uint32_t input_keycode) {
    // Only apply mapping if layout shift is active
    if (!zmk_layout_shift_is_active()) {
        return input_keycode;
    }

    // Get current explicit modifier state
    zmk_mod_flags_t explicit_mods = zmk_hid_get_explicit_mods();
    bool shift_pressed = (explicit_mods & (MOD_LSFT | MOD_RSFT)) != 0;

    // If shift is pressed, apply LS() macro to get shifted keycode
    uint32_t effective_keycode = input_keycode;
    if (shift_pressed) {
        effective_keycode = LS(input_keycode);
        LOG_DBG("LAYOUT_SHIFT: Shift+%08X detected, converted to %08X",
                input_keycode, effective_keycode);
    }

    // Look up in mapping table (us_keycode is the input, target_keycode is the output)
    for (size_t i = 0; i < LAYOUT_MAP_SIZE; i++) {
        if (layout_map[i].us_keycode == effective_keycode) {
            LOG_DBG("LAYOUT_SHIFT: Mapping %08X -> %08X",
                    effective_keycode, layout_map[i].target_keycode);
            return layout_map[i].target_keycode;
        }
    }

    // If no mapping found, return effective keycode (which might be the shifted version)
    return effective_keycode;
}

struct behavior_layout_shift_key_press_data {};

struct behavior_layout_shift_key_press_config {};

static int on_layout_shift_key_press_binding_pressed(struct zmk_behavior_binding *binding,
                                                    struct zmk_behavior_binding_event event) {
    uint32_t original_keycode = binding->param1;
    uint32_t mapped_keycode = lookup_mapped_keycode(original_keycode);

    LOG_DBG("LAYOUT_SHIFT: Input keycode 0x%08X -> Mapped keycode 0x%08X", original_keycode, mapped_keycode);

    // Raise the mapped keycode event
    return raise_zmk_keycode_state_changed_from_encoded(
        mapped_keycode,
        true,  // pressed
        event.timestamp
    );
}

static int on_layout_shift_key_press_binding_released(struct zmk_behavior_binding *binding,
                                                     struct zmk_behavior_binding_event event) {
    uint32_t original_keycode = binding->param1;
    uint32_t mapped_keycode = lookup_mapped_keycode(original_keycode);

    LOG_DBG("LAYOUT_SHIFT: Released input keycode 0x%08X -> Mapped keycode 0x%08X", original_keycode, mapped_keycode);

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