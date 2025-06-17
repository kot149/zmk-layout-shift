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

// Storage for tracking key press/release mappings
#define MAX_PRESSED_KEYS 16

struct key_mapping_entry {
    uint32_t original_keycode;
    uint32_t mapped_keycode;
    bool active;
};

struct behavior_layout_shift_key_press_data {
    struct key_mapping_entry pressed_keys[MAX_PRESSED_KEYS];
};

struct behavior_layout_shift_key_press_config {};

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
    uint32_t mapped_keycode = lookup_mapped_keycode(original_keycode);

    LOG_DBG("LAYOUT_SHIFT: Input keycode 0x%08X -> Mapped keycode 0x%08X", original_keycode, mapped_keycode);

    // Store the mapping for use during release
    int ret = store_key_mapping(data, original_keycode, mapped_keycode);
    if (ret < 0) {
        LOG_ERR("LAYOUT_SHIFT: Failed to store key mapping: %d", ret);
        // Fall back to direct mapping if storage fails
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
        mapped_keycode = lookup_mapped_keycode(original_keycode);
        LOG_DBG("LAYOUT_SHIFT: No stored mapping found, recalculating 0x%08X -> 0x%08X", 
                original_keycode, mapped_keycode);
    }

    LOG_DBG("LAYOUT_SHIFT: Released input keycode 0x%08X -> Mapped keycode 0x%08X", original_keycode, mapped_keycode);

    // Release the mapped keycode
    return raise_zmk_keycode_state_changed_from_encoded(
        mapped_keycode,
        false, // released
        event.timestamp
    );
}

static const struct behavior_driver_api behavior_layout_shift_key_press_driver_api = {
    .binding_pressed = on_layout_shift_key_press_binding_pressed,
    .binding_released = on_layout_shift_key_press_binding_released,
    .locality = BEHAVIOR_LOCALITY_CENTRAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = {
        .count = 1,
        .types = {
            ZMK_BEHAVIOR_PARAM_TYPE_KEYCODE,
        },
    },
#endif
};

static int layout_shift_key_press_init(const struct device *dev) {
    struct behavior_layout_shift_key_press_data *data = 
        (struct behavior_layout_shift_key_press_data *)dev->data;
    
    // Initialize the pressed keys storage
    for (int i = 0; i < MAX_PRESSED_KEYS; i++) {
        data->pressed_keys[i].active = false;
    }
    
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