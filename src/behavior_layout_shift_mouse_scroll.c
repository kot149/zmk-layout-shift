#define DT_DRV_COMPAT zmk_behavior_mscls

#include "layouts/layout_common.h"
#include <drivers/behavior.h>
#include <dt-bindings/zmk/pointing.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

extern bool zmk_layout_shift_is_active(void);

#define DEFINE_MSC_MAPPING
#include "layouts/index.h"
#undef DEFINE_MSC_MAPPING

#ifdef MSC_MAP_DEFINED
#define MSC_MAP_SIZE (sizeof(msc_map) / sizeof(msc_map[0]))
#else
#define MSC_MAP_SIZE 0
static const struct msc_mapping msc_map[0];
#endif

struct behavior_mscls_config {
    const struct device *msc_dev;
};

static uint32_t lookup_mapped_msc(uint32_t code) {
    if (!zmk_layout_shift_is_active()) {
        return code;
    }
    for (size_t i = 0; i < MSC_MAP_SIZE; i++) {
        if (msc_map[i].base == code) {
            return msc_map[i].shifted;
        }
    }
    return code;
}

extern int behavior_input_two_axis_adjust_speed(const struct device *dev, int16_t dx, int16_t dy);

static int mscls_binding_pressed(struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_mscls_config *cfg = dev->config;

    uint32_t mapped = lookup_mapped_msc(binding->param1);
    int16_t x = MOVE_X_DECODE(mapped);
    int16_t y = MOVE_Y_DECODE(mapped);
    behavior_input_two_axis_adjust_speed(cfg->msc_dev, x, y);
    return 0;
}

static int mscls_binding_released(struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_mscls_config *cfg = dev->config;

    uint32_t mapped = lookup_mapped_msc(binding->param1);
    int16_t x = MOVE_X_DECODE(mapped);
    int16_t y = MOVE_Y_DECODE(mapped);
    behavior_input_two_axis_adjust_speed(cfg->msc_dev, -x, -y);
    return 0;
}

static const struct behavior_driver_api behavior_mscls_driver_api = {
    .binding_pressed = mscls_binding_pressed,
    .binding_released = mscls_binding_released,
};

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define MSCLS_INST(n) \
    static const struct behavior_mscls_config behavior_mscls_config_##n = { \
        .msc_dev = DEVICE_DT_GET(DT_INST_PHANDLE(n, msc)), \
    }; \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, &behavior_mscls_config_##n, \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
                            &behavior_mscls_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MSCLS_INST)
#endif
