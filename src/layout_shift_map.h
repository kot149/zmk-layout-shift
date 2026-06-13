#ifndef LAYOUT_SHIFT_MAP_H
#define LAYOUT_SHIFT_MAP_H

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zmk/hid.h>
#include <dt-bindings/zmk/modifiers.h>

#define _LAYOUT_SHIFT_MAP_DEV_REF(node) DEVICE_DT_GET(node),

struct layout_shift_map_entry {
    uint32_t from_keycode;
    uint32_t to_keycode;
    zmk_mod_flags_t optional_mods;
};

struct layout_shift_map_config {
    const uint32_t *mappings_raw;
    size_t entry_count;
    struct layout_shift_map_entry *sorted_entries;
    int priority;
};

struct layout_shift_map_data {
    bool active;
    size_t declaration_index;
};

static inline bool layout_shift_map_is_active(const struct device *dev) {
    const struct layout_shift_map_data *data = dev->data;
    return data->active;
}

static inline size_t layout_shift_map_entry_count(const struct device *dev) {
    const struct layout_shift_map_config *cfg = dev->config;
    return cfg->entry_count;
}

static inline const struct layout_shift_map_entry *
layout_shift_map_entries(const struct device *dev) {
    const struct layout_shift_map_config *cfg = dev->config;
    return cfg->sorted_entries;
}

static inline int layout_shift_map_find_base(const struct device *dev, uint32_t base_keycode) {
    const struct layout_shift_map_config *cfg = dev->config;
    const struct layout_shift_map_entry *entries = cfg->sorted_entries;
    int lo = 0, hi = (int)cfg->entry_count - 1;
    int result = -1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        uint32_t mid_base = STRIP_MODS(entries[mid].from_keycode);
        if (mid_base < base_keycode) {
            lo = mid + 1;
        } else if (mid_base > base_keycode) {
            hi = mid - 1;
        } else {
            result = mid;
            hi = mid - 1;
        }
    }
    return result;
}

void layout_shift_map_set_active(const struct device *dev, bool active);
void layout_shift_map_toggle(const struct device *dev);

#if DT_HAS_COMPAT_STATUS_OKAY(zmk_layout_shift_map)
extern const struct device *layout_shift_map_devs[];
extern const size_t layout_shift_map_dev_count;
#else
#define layout_shift_map_devs ((const struct device **)NULL)
#define layout_shift_map_dev_count 0
#endif

#endif
