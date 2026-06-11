#ifndef LAYOUT_SHIFT_MAP_H
#define LAYOUT_SHIFT_MAP_H

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zmk/hid.h>

#define _LAYOUT_SHIFT_MAP_DEV_REF(node) DEVICE_DT_GET(node),

struct layout_shift_map_entry {
    uint32_t from_keycode;
    uint32_t to_keycode;
    zmk_mod_flags_t optional_mods;
};

struct layout_shift_map_config {
    const uint32_t *mappings_raw;
    size_t entry_count;
};

struct layout_shift_map_data {
    bool active;
};

static inline bool layout_shift_map_is_active(const struct device *dev) {
    const struct layout_shift_map_data *data = dev->data;
    return data->active;
}

static inline size_t layout_shift_map_entry_count(const struct device *dev) {
    const struct layout_shift_map_config *cfg = dev->config;
    return cfg->entry_count;
}

static inline struct layout_shift_map_entry layout_shift_map_get_entry(const struct device *dev,
                                                                       size_t index) {
    const struct layout_shift_map_config *cfg = dev->config;
    return (struct layout_shift_map_entry){
        .from_keycode = cfg->mappings_raw[index * 3],
        .to_keycode = cfg->mappings_raw[index * 3 + 1],
        .optional_mods = (zmk_mod_flags_t)cfg->mappings_raw[index * 3 + 2],
    };
}

void layout_shift_map_set_active(const struct device *dev, bool active);
void layout_shift_map_toggle(const struct device *dev);

#if DT_HAS_COMPAT_STATUS_OKAY(zmk_layout_shift_map)
extern const struct device *const layout_shift_map_devs[];
extern const size_t layout_shift_map_dev_count;
#else
#define layout_shift_map_devs NULL
#define layout_shift_map_dev_count 0
#endif

#endif
