#define DT_DRV_COMPAT zmk_behavior_layout_shift_to

#include <drivers/behavior.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// External function to set layout shift state
extern void zmk_layout_shift_set_active(bool active);

struct behavior_layout_shift_to_config {};
struct behavior_layout_shift_to_data {};

static int on_layout_shift_to_binding_pressed(struct zmk_behavior_binding *binding,
                                              struct zmk_behavior_binding_event event) {
    bool state = binding->param1 != 0;
    zmk_layout_shift_set_active(state);
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_layout_shift_to_binding_released(struct zmk_behavior_binding *binding,
                                               struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_layout_shift_to_driver_api = {
    .binding_pressed = on_layout_shift_to_binding_pressed,
    .binding_released = on_layout_shift_to_binding_released,
    .locality = BEHAVIOR_LOCALITY_CENTRAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

static int layout_shift_to_init(const struct device *dev) { return 0; }

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define LAYOUT_SHIFT_TO_INST(n) \
    static struct behavior_layout_shift_to_data behavior_layout_shift_to_data_##n = {}; \
    BEHAVIOR_DT_INST_DEFINE(n, layout_shift_to_init, NULL, \
                            &behavior_layout_shift_to_data_##n, NULL, \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
                            &behavior_layout_shift_to_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LAYOUT_SHIFT_TO_INST)
#endif
