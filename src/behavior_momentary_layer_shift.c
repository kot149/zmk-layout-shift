#define DT_DRV_COMPAT zmk_behavior_mols

#include "layouts/layout_common.h"
#include <drivers/behavior.h>
#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

extern bool zmk_layout_shift_is_active(void);

struct layer_mapping {
    uint8_t base_layer;
    uint8_t shift_layer;
};

#define DEFINE_LAYER_MAPPING
#include "layouts/index.h"
#undef DEFINE_LAYER_MAPPING

#ifdef LAYER_MAP_DEFINED
#define LAYER_MAP_SIZE (sizeof(layer_map) / sizeof(layer_map[0]))
#else
#define LAYER_MAP_SIZE 0
static const struct layer_mapping layer_map[0];
#endif

struct behavior_momentary_layer_shift_config {};

struct behavior_momentary_layer_shift_data {
    uint8_t active_layer;
};

static int mols_binding_pressed(struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_momentary_layer_shift_data *data = dev->data;

    uint8_t base_layer = binding->param1;
    uint8_t target_layer = base_layer;

    if (zmk_layout_shift_is_active()) {
        for (size_t i = 0; i < LAYER_MAP_SIZE; i++) {
            if (layer_map[i].base_layer == base_layer) {
                target_layer = layer_map[i].shift_layer;
                break;
            }
        }
    }

    data->active_layer = target_layer;
    return zmk_keymap_layer_activate(target_layer);
}

static int mols_binding_released(struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_momentary_layer_shift_data *data = dev->data;

    uint8_t layer = data->active_layer;
    data->active_layer = 0;
    return zmk_keymap_layer_deactivate(layer);
}

#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
static const struct behavior_parameter_value_metadata param_values[] = {
    {
        .display_name = "Layer",
        .type = BEHAVIOR_PARAMETER_VALUE_TYPE_LAYER_ID,
    },
};

static const struct behavior_parameter_metadata_set param_metadata_set[] = {{
    .param1_values = &param_values[0],
    .param1_values_len = 1,
}};

static const struct behavior_parameter_metadata mols_parameter_metadata = {
    .sets_len = ARRAY_SIZE(param_metadata_set),
    .sets = param_metadata_set,
};
#endif

static const struct behavior_driver_api behavior_momentary_layer_shift_driver_api = {
    .binding_pressed = mols_binding_pressed,
    .binding_released = mols_binding_released,
    .locality = BEHAVIOR_LOCALITY_CENTRAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = &mols_parameter_metadata,
#endif
};

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define MOLS_INST(n) \
    static struct behavior_momentary_layer_shift_data behavior_momentary_layer_shift_data_##n = {}; \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, \
                            &behavior_momentary_layer_shift_data_##n, NULL, \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
                            &behavior_momentary_layer_shift_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MOLS_INST)
#endif
