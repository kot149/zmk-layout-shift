#define DT_DRV_COMPAT zmk_layout_shift_map

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <string.h>
#include <dt-bindings/zmk/modifiers.h>
#include "layout_shift_map.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static zmk_mod_flags_t mod_keycode_to_flag(uint32_t keycode) {
    switch (STRIP_MODS(keycode)) {
        case LEFT_CONTROL:  return MOD_LCTL;
        case LEFT_SHIFT:    return MOD_LSFT;
        case LEFT_ALT:      return MOD_LALT;
        case LEFT_GUI:      return MOD_LGUI;
        case RIGHT_CONTROL: return MOD_RCTL;
        case RIGHT_SHIFT:   return MOD_RSFT;
        case RIGHT_ALT:     return MOD_RALT;
        case RIGHT_GUI:     return MOD_RGUI;
        default:            return 0;
    }
}

static void sort_indices(const struct layout_shift_map_config *config) {
    for (size_t i = 0; i < config->entry_count; i++) {
        config->sorted_indices[i] = i;
    }

    for (size_t i = 1; i < config->entry_count; i++) {
        uint16_t tmp = config->sorted_indices[i];
        uint32_t tmp_base = STRIP_MODS(config->mappings_raw[tmp * 3]);
        int j = (int)i - 1;
        while (j >= 0 &&
               STRIP_MODS(config->mappings_raw[config->sorted_indices[j] * 3]) > tmp_base) {
            config->sorted_indices[j + 1] = config->sorted_indices[j];
            j--;
        }
        config->sorted_indices[j + 1] = tmp;
    }
}

#if DT_HAS_COMPAT_STATUS_OKAY(zmk_layout_shift_map)
const struct device *layout_shift_map_devs[] = {
    DT_FOREACH_STATUS_OKAY(zmk_layout_shift_map, _LAYOUT_SHIFT_MAP_DEV_REF)
};
const size_t layout_shift_map_dev_count = ARRAY_SIZE(layout_shift_map_devs);
#endif

static void sort_devs_by_priority(void) {
    for (size_t i = 1; i < layout_shift_map_dev_count; i++) {
        const struct device *tmp = layout_shift_map_devs[i];
        const struct layout_shift_map_config *tmp_cfg = tmp->config;
        int j = (int)i - 1;
        while (j >= 0) {
            const struct layout_shift_map_config *j_cfg = layout_shift_map_devs[j]->config;
            if (j_cfg->priority <= tmp_cfg->priority) {
                break;
            }
            layout_shift_map_devs[j + 1] = layout_shift_map_devs[j];
            j--;
        }
        layout_shift_map_devs[j + 1] = tmp;
    }
}

/* Runs after all device init (POST_KERNEL) completes, so declaration_index
 * is populated before any consumer iterates layout_shift_map_devs[]. */
static int layout_shift_map_post_init(void) {
    sort_devs_by_priority();
    for (size_t i = 0; i < layout_shift_map_dev_count; i++) {
        const struct layout_shift_map_config *cfg = layout_shift_map_devs[i]->config;
        LOG_DBG("Layout shift map order [%zu]: %s (priority=%d)",
                i, layout_shift_map_devs[i]->name, cfg->priority);
    }
    return 0;
}

SYS_INIT(layout_shift_map_post_init, APPLICATION, 0);

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
    layout_shift_map_set_active(dev, !layout_shift_map_is_active(dev));
}

zmk_mod_flags_t layout_shift_map_translate_mods(const struct device *dev, zmk_mod_flags_t mods,
                                                bool *changed) {
    const struct layout_shift_map_data *data = dev->data;
    zmk_mod_flags_t result = 0;

    while (mods != 0) {
        zmk_mod_flags_t bit = mods & -mods;
        zmk_mod_flags_t mapped = data->modifier_map[__builtin_ctz(bit)];
        result |= mapped != 0 ? mapped : bit;
        if (mapped != 0 && mapped != bit && changed != NULL) {
            *changed = true;
        }
        mods &= ~bit;
    }

    return result;
}

#if IS_ENABLED(CONFIG_LAYOUT_SHIFT_PERSISTENT_STATE)

static void layout_shift_save_work_handler(struct k_work *work) {
    for (size_t i = 0; i < layout_shift_map_dev_count; i++) {
        char key[64];
        snprintk(key, sizeof(key), "layout_shift/m/%s", layout_shift_map_devs[i]->name);
        struct layout_shift_map_data *data = layout_shift_map_devs[i]->data;
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

    for (size_t i = 0; i < layout_shift_map_dev_count; i++) {
        if (strcmp(next, layout_shift_map_devs[i]->name) == 0) {
            if (len != sizeof(bool)) {
                return -EINVAL;
            }
            struct layout_shift_map_data *data = layout_shift_map_devs[i]->data;
            int rc = read_cb(cb_arg, &data->active, sizeof(bool));
            if (rc >= 0) {
                LOG_INF("Loaded layout shift state %s: %d", layout_shift_map_devs[i]->name, data->active);
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
    struct layout_shift_map_data *data = dev->data;
    const struct layout_shift_map_config *config = dev->config;

    data->active = false;

    sort_indices(config);
    for (size_t i = 0; i < config->entry_count; i++) {
        struct layout_shift_map_entry entry = layout_shift_map_entry(dev, i);
        zmk_mod_flags_t from_mod = mod_keycode_to_flag(entry.from_keycode);
        zmk_mod_flags_t to_mod = mod_keycode_to_flag(entry.to_keycode);
        if (from_mod != 0 && to_mod != 0 && data->modifier_map[__builtin_ctz(from_mod)] == 0) {
            data->modifier_map[__builtin_ctz(from_mod)] = to_mod;
        }
    }

#if IS_ENABLED(CONFIG_LAYOUT_SHIFT_PERSISTENT_STATE)
    static bool work_initialized = false;
    if (!work_initialized) {
        k_work_init_delayable(&layout_shift_save_work, layout_shift_save_work_handler);
        work_initialized = true;
    }
#endif

    LOG_INF("Layout shift map %s initialized (%zu entries)", dev->name, config->entry_count);
    return 0;
}

#define _RAW_ENTRY(node, prop, idx) DT_PROP_BY_IDX(node, prop, idx),

#define LAYOUT_SHIFT_MAP_INST(n)                                                                  \
    static const uint32_t layout_map_raw_##n[] = {                                                 \
        DT_FOREACH_PROP_ELEM(DT_DRV_INST(n), mappings, _RAW_ENTRY)                                \
    };                                                                                            \
    BUILD_ASSERT(ARRAY_SIZE(layout_map_raw_##n) / 3 <= UINT16_MAX,                                \
                 "layout shift map supports at most UINT16_MAX entries");                        \
    static uint16_t sorted_indices_##n[ARRAY_SIZE(layout_map_raw_##n) / 3];                        \
    static struct layout_shift_map_data layout_shift_map_data_##n = {                              \
        .declaration_index = n,                                                                    \
    };                                                                                            \
    static const struct layout_shift_map_config layout_shift_map_config_##n = {                   \
        .mappings_raw = layout_map_raw_##n,                                                       \
        .sorted_indices = sorted_indices_##n,                                                     \
        .entry_count = ARRAY_SIZE(layout_map_raw_##n) / 3,                                        \
        .priority = DT_INST_PROP_OR(n, priority, 0),                                              \
    };                                                                                            \
    DEVICE_DT_INST_DEFINE(n, layout_shift_map_init, NULL,                                         \
                          &layout_shift_map_data_##n, &layout_shift_map_config_##n,                \
                          POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(LAYOUT_SHIFT_MAP_INST)
