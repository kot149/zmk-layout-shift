#define DT_DRV_COMPAT zmk_behavior_layout_shift_toggle

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include "layout_shift_map.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

enum toggle_mode {
    TOGGLE_MODE_ON,
    TOGGLE_MODE_OFF,
    TOGGLE_MODE_FLIP
};

struct behavior_layout_shift_toggle_config {
    enum toggle_mode toggle_mode;
    const struct device **layout_maps;
    size_t layout_map_count;
};

struct behavior_layout_shift_toggle_data {};

static void apply_toggle(const struct device *map_dev, enum toggle_mode mode) {
    switch (mode) {
        case TOGGLE_MODE_ON:
            layout_shift_map_set_active(map_dev, true);
            break;
        case TOGGLE_MODE_OFF:
            layout_shift_map_set_active(map_dev, false);
            break;
        case TOGGLE_MODE_FLIP:
            layout_shift_map_toggle(map_dev);
            break;
    }
}

static int on_layout_shift_toggle_binding_pressed(struct zmk_behavior_binding *binding,
                                                 struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_layout_shift_toggle_config *config = dev->config;

    for (size_t i = 0; i < config->layout_map_count; i++) {
        apply_toggle(config->layout_maps[i], config->toggle_mode);
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_layout_shift_toggle_binding_released(struct zmk_behavior_binding *binding,
                                                  struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_layout_shift_toggle_driver_api = {
    .binding_pressed = on_layout_shift_toggle_binding_pressed,
    .binding_released = on_layout_shift_toggle_binding_released,
    .locality = BEHAVIOR_LOCALITY_CENTRAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

static int layout_shift_toggle_init(const struct device *dev) {
    LOG_INF("Layout Shift Toggle Behavior Initialized!");
    return 0;
}

#define TOGGLE_MODE_FROM_STR(str) \
    (strcmp(str, "on") == 0 ? TOGGLE_MODE_ON : \
     strcmp(str, "off") == 0 ? TOGGLE_MODE_OFF : \
     TOGGLE_MODE_FLIP)

#define _TOGGLE_MAP_DEV(node, prop, idx) DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node, prop, idx)),

// Only define behavior instances if devicetree nodes exist
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define LAYOUT_SHIFT_TOGGLE_INST(n) \
    static const struct device *toggle_layout_maps_##n[] = { \
        DT_INST_FOREACH_PROP_ELEM(n, layout_maps, _TOGGLE_MAP_DEV) \
    }; \
    static struct behavior_layout_shift_toggle_data behavior_layout_shift_toggle_data_##n = {}; \
    static const struct behavior_layout_shift_toggle_config behavior_layout_shift_toggle_config_##n = { \
        .toggle_mode = TOGGLE_MODE_FROM_STR(DT_INST_PROP_OR(n, toggle_mode, "flip")), \
        .layout_maps = toggle_layout_maps_##n, \
        .layout_map_count = DT_INST_PROP_LEN(n, layout_maps), \
    }; \
    BEHAVIOR_DT_INST_DEFINE(n, layout_shift_toggle_init, NULL, \
                            &behavior_layout_shift_toggle_data_##n, \
                            &behavior_layout_shift_toggle_config_##n, \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
                            &behavior_layout_shift_toggle_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LAYOUT_SHIFT_TOGGLE_INST)
#endif
