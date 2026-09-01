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

#if IS_ENABLED(CONFIG_LAYOUT_SHIFT_DEBUG)
#define LAYOUT_SHIFT_LOG_DBG(...) LOG_DBG(__VA_ARGS__)
#else
#define LAYOUT_SHIFT_LOG_DBG(...)
#endif

static zmk_mod_flags_t translate_embedded_mods(zmk_mod_flags_t mods, bool *changed) {
    zmk_mod_flags_t result = mods;

    for (size_t d = 0; d < layout_shift_map_dev_count; d++) {
        if (layout_shift_map_is_active(layout_shift_map_devs[d])) {
            result = layout_shift_map_translate_mods(layout_shift_map_devs[d], result, changed);
        }
    }

    return result;
}

struct lookup_result {
    uint32_t keycode;
    zmk_mod_flags_t matched_opt_mods;
    bool matched;
};

static struct lookup_result lookup_mapped_keycode(uint32_t input_keycode) {
    struct lookup_result result = {
        .keycode = input_keycode,
    };
    zmk_mod_flags_t current_mods = 0;
    uint32_t current_keycode = input_keycode;
    bool active_map_found = false;

    for (size_t d = 0; d < layout_shift_map_dev_count; d++) {
        const struct device *map_dev = layout_shift_map_devs[d];
        if (!layout_shift_map_is_active(map_dev)) {
            continue;
        }
        if (!active_map_found) {
            current_mods = zmk_hid_get_explicit_mods();
            active_map_found = true;
        }

        zmk_mod_flags_t keycode_mods = SELECT_MODS(current_keycode);
        zmk_mod_flags_t total_input_mods = current_mods | keycode_mods;
        uint32_t base_input = STRIP_MODS(current_keycode);

        int idx = layout_shift_map_find_base(map_dev, base_input);
        if (idx < 0) {
            continue;
        }

        size_t count = layout_shift_map_entry_count(map_dev);
        for (size_t i = idx; i < count; i++) {
            struct layout_shift_map_entry entry = layout_shift_map_entry(map_dev, i);
            if (STRIP_MODS(entry.from_keycode) != base_input) {
                break;
            }

            zmk_mod_flags_t from_mods = SELECT_MODS(entry.from_keycode);
            zmk_mod_flags_t required_mods = from_mods & ~entry.optional_mods;
            zmk_mod_flags_t input_required_mods = total_input_mods & ~entry.optional_mods;
            if (required_mods != input_required_mods) {
                continue;
            }

            uint32_t target_base = STRIP_MODS(entry.to_keycode);
            zmk_mod_flags_t target_mods = SELECT_MODS(entry.to_keycode);
            zmk_mod_flags_t final_mods = target_mods | (total_input_mods & entry.optional_mods);

            current_keycode = final_mods != 0 ? APPLY_MODS(final_mods, target_base) : target_base;
            result.matched_opt_mods |= entry.optional_mods;
            result.matched = true;

            LAYOUT_SHIFT_LOG_DBG(
                "LAYOUT_SHIFT: Mapping %08X -> %08X (dev=%s, input_mods: %02X, "
                "target_mods: %02X, final: %02X)",
                input_keycode, current_keycode, map_dev->name, total_input_mods, target_mods,
                final_mods);
            break;
        }
    }

    if (!active_map_found) {
        return result;
    }
    if (result.matched) {
        result.keycode = current_keycode;
        return result;
    }

    zmk_mod_flags_t keycode_mods = SELECT_MODS(input_keycode);
    if (keycode_mods != 0) {
        bool changed = false;
        zmk_mod_flags_t new_keycode_mods = translate_embedded_mods(keycode_mods, &changed);
        if (changed) {
            result.keycode = APPLY_MODS(new_keycode_mods, STRIP_MODS(input_keycode));
            LAYOUT_SHIFT_LOG_DBG(
                "LAYOUT_SHIFT: Mapping embedded mods %08X -> %08X (mods: %02X -> %02X)",
                input_keycode, result.keycode, keycode_mods, new_keycode_mods);
            return result;
        }
    }

    LAYOUT_SHIFT_LOG_DBG("LAYOUT_SHIFT: No mapping found for %08X", input_keycode);
    return result;
}

struct key_mapping_entry {
    uint32_t position;
    uint32_t mapped_keycode;
};

struct behavior_layout_shift_key_press_data {
    struct key_mapping_entry pressed_keys[CONFIG_LAYOUT_SHIFT_MAX_PRESSED_KEYS];
    zmk_mod_flags_t masked_mods[CONFIG_LAYOUT_SHIFT_MAX_PRESSED_KEYS];
    zmk_mod_flags_t currently_masked_mods;
    uint8_t pressed_key_count;
};

static zmk_mod_flags_t unwanted_modifiers(zmk_mod_flags_t optional_mods,
                                          uint32_t mapped_keycode,
                                          zmk_mod_flags_t current_mods) {
    zmk_mod_flags_t mapped_mods = SELECT_MODS(mapped_keycode);
    return ~optional_mods & ~mapped_mods & current_mods;
}

static void update_modifier_mask(struct behavior_layout_shift_key_press_data *data) {
    zmk_mod_flags_t masked_mods = 0;
    for (uint8_t i = 0; i < data->pressed_key_count; i++) {
        masked_mods |= data->masked_mods[i];
    }
    if (masked_mods == data->currently_masked_mods) {
        return;
    }

    if (masked_mods == 0) {
        zmk_hid_masked_modifiers_clear();
    } else {
        zmk_hid_masked_modifiers_set(masked_mods);
    }
    data->currently_masked_mods = masked_mods;
    LAYOUT_SHIFT_LOG_DBG("LAYOUT_SHIFT: Modifier mask set to %02X", masked_mods);
}

static int store_key_mapping(struct behavior_layout_shift_key_press_data *data, uint32_t position,
                             uint32_t mapped_keycode, zmk_mod_flags_t masked_mods) {
    if (data->pressed_key_count >= CONFIG_LAYOUT_SHIFT_MAX_PRESSED_KEYS) {
        LOG_WRN("LAYOUT_SHIFT: No free slots to store key mapping");
        return -ENOMEM;
    }

    uint8_t index = data->pressed_key_count++;
    data->pressed_keys[index] = (struct key_mapping_entry){
        .position = position,
        .mapped_keycode = mapped_keycode,
    };
    data->masked_mods[index] = masked_mods;
    update_modifier_mask(data);
    return 0;
}

static bool take_key_mapping(struct behavior_layout_shift_key_press_data *data, uint32_t position,
                             uint32_t *mapped_keycode) {
    for (uint8_t i = 0; i < data->pressed_key_count; i++) {
        if (data->pressed_keys[i].position != position) {
            continue;
        }

        *mapped_keycode = data->pressed_keys[i].mapped_keycode;
        uint8_t last = --data->pressed_key_count;
        data->pressed_keys[i] = data->pressed_keys[last];
        data->masked_mods[i] = data->masked_mods[last];
        update_modifier_mask(data);
        return true;
    }
    return false;
}

static int on_layout_shift_key_press_binding_pressed(struct zmk_behavior_binding *binding,
                                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_layout_shift_key_press_data *data = dev->data;
    uint32_t original_keycode = binding->param1;
    struct lookup_result result = lookup_mapped_keycode(original_keycode);
    zmk_mod_flags_t masked_mods = 0;

    if (result.matched) {
        zmk_mod_flags_t current_mods = zmk_hid_get_explicit_mods();
        zmk_mod_flags_t total_mods = current_mods | SELECT_MODS(original_keycode);
        masked_mods = unwanted_modifiers(result.matched_opt_mods, result.keycode, total_mods);
    }

    LAYOUT_SHIFT_LOG_DBG("LAYOUT_SHIFT: Input keycode 0x%08X -> Mapped keycode 0x%08X",
                         original_keycode, result.keycode);

    int ret = store_key_mapping(data, event.position, result.keycode, masked_mods);
    if (ret < 0) {
        LOG_ERR("LAYOUT_SHIFT: Failed to store key mapping: %d", ret);
    }

    return raise_zmk_keycode_state_changed_from_encoded(result.keycode, true, event.timestamp);
}

static int on_layout_shift_key_press_binding_released(struct zmk_behavior_binding *binding,
                                                      struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_layout_shift_key_press_data *data = dev->data;
    uint32_t original_keycode = binding->param1;
    uint32_t mapped_keycode;

    if (take_key_mapping(data, event.position, &mapped_keycode)) {
        LAYOUT_SHIFT_LOG_DBG(
            "LAYOUT_SHIFT: Using stored mapping for release 0x%08X -> 0x%08X",
            original_keycode, mapped_keycode);
    } else {
        mapped_keycode = lookup_mapped_keycode(original_keycode).keycode;
        LAYOUT_SHIFT_LOG_DBG(
            "LAYOUT_SHIFT: No stored mapping found, recalculating 0x%08X -> 0x%08X",
            original_keycode, mapped_keycode);
    }

    LAYOUT_SHIFT_LOG_DBG("LAYOUT_SHIFT: Released input keycode 0x%08X -> Mapped keycode 0x%08X",
                         original_keycode, mapped_keycode);
    return raise_zmk_keycode_state_changed_from_encoded(mapped_keycode, false, event.timestamp);
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

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
static struct behavior_layout_shift_key_press_data behavior_layout_shift_key_press_data_0;

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL,
                        &behavior_layout_shift_key_press_data_0, NULL,
                        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_layout_shift_key_press_driver_api);
#endif
