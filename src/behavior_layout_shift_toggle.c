#define DT_DRV_COMPAT zmk_behavior_layout_shift_toggle

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <string.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

enum toggle_mode {
    TOGGLE_MODE_ON,
    TOGGLE_MODE_OFF,
    TOGGLE_MODE_FLIP
};

struct behavior_layout_shift_toggle_config {
    enum toggle_mode toggle_mode;
};

struct behavior_layout_shift_toggle_data {};

// Global layout shift state
static bool layout_shift_active = false;

#if IS_ENABLED(CONFIG_LAYOUT_SHIFT_PERSISTENT_STATE)
static void layout_shift_save_work_handler(struct k_work *work) {
    settings_save_one("layout_shift/state", &layout_shift_active, sizeof(layout_shift_active));
    LOG_DBG("Saved layout shift state: %d", layout_shift_active);
}

static struct k_work_delayable layout_shift_save_work;

static int layout_shift_settings_load_cb(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    const char *next;
    if (settings_name_steq(name, "state", &next) && !next) {
        if (len != sizeof(layout_shift_active)) {
            return -EINVAL;
        }

        int rc = read_cb(cb_arg, &layout_shift_active, sizeof(layout_shift_active));
        if (rc >= 0) {
            LOG_INF("Loaded layout shift state: %d", layout_shift_active);
        }
        return MIN(rc, 0);
    }
    return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(layout_shift, "layout_shift", NULL, layout_shift_settings_load_cb, NULL, NULL);
#endif // IS_ENABLED(CONFIG_LAYOUT_SHIFT_PERSISTENT_STATE)

// Function to get current layout shift state
bool zmk_layout_shift_is_active(void) {
    return layout_shift_active;
}

// Function to set layout shift state
void zmk_layout_shift_set_active(bool active) {
    if (layout_shift_active != active) {
        layout_shift_active = active;
        LOG_INF("Layout shift %s", active ? "activated" : "deactivated");
        
#if IS_ENABLED(CONFIG_LAYOUT_SHIFT_PERSISTENT_STATE)
        // Debounce saving to flash to reduce wear
        k_work_reschedule(&layout_shift_save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
#endif
    }
}

// Function to toggle layout shift state
void zmk_layout_shift_toggle(void) {
    zmk_layout_shift_set_active(!layout_shift_active);
}

static int on_layout_shift_toggle_binding_pressed(struct zmk_behavior_binding *binding,
                                                 struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_layout_shift_toggle_config *config = dev->config;

    switch (config->toggle_mode) {
        case TOGGLE_MODE_ON:
            zmk_layout_shift_set_active(true);
            break;
        case TOGGLE_MODE_OFF:
            zmk_layout_shift_set_active(false);
            break;
        case TOGGLE_MODE_FLIP:
            zmk_layout_shift_toggle();
            break;
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
    layout_shift_active = false;
    
#if IS_ENABLED(CONFIG_LAYOUT_SHIFT_PERSISTENT_STATE)
    k_work_init_delayable(&layout_shift_save_work, layout_shift_save_work_handler);
#endif
    
    LOG_INF("Layout Shift Toggle Behavior Initialized!");
    return 0;
}

#define TOGGLE_MODE_FROM_STR(str) \
    (strcmp(str, "on") == 0 ? TOGGLE_MODE_ON : \
     strcmp(str, "off") == 0 ? TOGGLE_MODE_OFF : \
     TOGGLE_MODE_FLIP)

// Only define behavior instances if devicetree nodes exist
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define LAYOUT_SHIFT_TOGGLE_INST(n) \
    static struct behavior_layout_shift_toggle_data behavior_layout_shift_toggle_data_##n = {}; \
    static const struct behavior_layout_shift_toggle_config behavior_layout_shift_toggle_config_##n = { \
        .toggle_mode = TOGGLE_MODE_FROM_STR(DT_INST_PROP_OR(n, toggle_mode, "flip")), \
    }; \
    BEHAVIOR_DT_INST_DEFINE(n, layout_shift_toggle_init, NULL, \
                            &behavior_layout_shift_toggle_data_##n, \
                            &behavior_layout_shift_toggle_config_##n, \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
                            &behavior_layout_shift_toggle_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LAYOUT_SHIFT_TOGGLE_INST)
#endif