#define DT_DRV_COMPAT zmk_layout_shift_map

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <string.h>
#include "layout_shift_map.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

bool layout_shift_map_is_active(const struct device *dev) {
    const struct layout_shift_map_data *data = dev->data;
    return data->active;
}

size_t layout_shift_map_entry_count(const struct device *dev) {
    const struct layout_shift_map_config *cfg = dev->config;
    return cfg->entry_count;
}

struct layout_shift_map_entry layout_shift_map_get_entry(const struct device *dev, size_t index) {
    const struct layout_shift_map_config *cfg = dev->config;
    if (index >= cfg->entry_count) {
        return (struct layout_shift_map_entry){0};
    }
    return (struct layout_shift_map_entry){
        .from_keycode = cfg->mappings_raw[index * 3],
        .to_keycode = cfg->mappings_raw[index * 3 + 1],
        .optional_mods = (zmk_mod_flags_t)cfg->mappings_raw[index * 3 + 2],
    };
}

#if IS_ENABLED(CONFIG_LAYOUT_SHIFT_PERSISTENT_STATE)
static struct k_work_delayable layout_shift_save_work;
#endif

void layout_shift_map_set_active(const struct device *dev, bool active) {
    struct layout_shift_map_data *data = dev->data;
    if (data->active != active) {
        data->active = active;
        LOG_INF("Layout shift map %s %s", dev->name, active ? "activated" : "deactivated");

#if IS_ENABLED(CONFIG_LAYOUT_SHIFT_PERSISTENT_STATE)
        k_work_reschedule(&layout_shift_save_work, K_MSEC(CONFIG_ZMK_SETTINGS_SAVE_DEBOUNCE));
#endif
    }
}

void layout_shift_map_toggle(const struct device *dev) {
    const struct layout_shift_map_data *data = dev->data;
    layout_shift_map_set_active(dev, !data->active);
}

#if IS_ENABLED(CONFIG_LAYOUT_SHIFT_PERSISTENT_STATE)

static const struct device *all_map_devs[] = {
    DT_FOREACH_STATUS_OKAY(zmk_layout_shift_map, _LAYOUT_SHIFT_MAP_DEV_REF)
};
#define INST_COUNT ARRAY_SIZE(all_map_devs)

static void layout_shift_save_work_handler(struct k_work *work) {
    for (size_t i = 0; i < INST_COUNT; i++) {
        char key[64];
        snprintf(key, sizeof(key), "layout_shift/m/%s", all_map_devs[i]->name);
        struct layout_shift_map_data *data = all_map_devs[i]->data;
        settings_save_one(key, &data->active, sizeof(data->active));
    }
    LOG_DBG("Saved layout shift states");
}

static int layout_shift_settings_load_cb(const char *name, size_t len,
                                         settings_read_cb read_cb, void *cb_arg) {
    const char *next;
    if (!settings_name_steq(name, "m", &next) || !next) {
        return -ENOENT;
    }

    for (size_t i = 0; i < INST_COUNT; i++) {
        if (strcmp(next, all_map_devs[i]->name) == 0) {
            if (len != sizeof(bool)) {
                return -EINVAL;
            }
            struct layout_shift_map_data *data = all_map_devs[i]->data;
            int rc = read_cb(cb_arg, &data->active, sizeof(bool));
            if (rc >= 0) {
                LOG_INF("Loaded layout shift state %s: %d", all_map_devs[i]->name, data->active);
            }
            return MIN(rc, 0);
        }
    }
    return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(layout_shift, "layout_shift", NULL,
                               layout_shift_settings_load_cb, NULL, NULL);

#endif

static int layout_shift_map_init(const struct device *dev) {
    struct layout_shift_map_data *data = (struct layout_shift_map_data *)dev->data;
    data->active = false;

#if IS_ENABLED(CONFIG_LAYOUT_SHIFT_PERSISTENT_STATE)
    static bool work_initialized = false;
    if (!work_initialized) {
        k_work_init_delayable(&layout_shift_save_work, layout_shift_save_work_handler);
        work_initialized = true;
    }
#endif

    const struct layout_shift_map_config *config = dev->config;
    LOG_INF("Layout shift map %s initialized (%zu entries)", dev->name, config->entry_count);
    return 0;
}

#define _RAW_ENTRY(node, prop, idx) DT_PROP_BY_IDX(node, prop, idx),

#define LAYOUT_SHIFT_MAP_INST(n)                                                                   \
    static const uint32_t layout_map_raw_##n[] = {                                                 \
        DT_FOREACH_PROP_ELEM(DT_DRV_INST(n), mappings, _RAW_ENTRY)                                \
    };                                                                                             \
    static struct layout_shift_map_data layout_shift_map_data_##n = {};                            \
    static const struct layout_shift_map_config layout_shift_map_config_##n = {                    \
        .mappings_raw = layout_map_raw_##n,                                                        \
        .entry_count = ARRAY_SIZE(layout_map_raw_##n) / 3,                                         \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, layout_shift_map_init, NULL,                                          \
                          &layout_shift_map_data_##n, &layout_shift_map_config_##n,                \
                          POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(LAYOUT_SHIFT_MAP_INST)
