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
    uint16_t *sorted_indices;
    size_t entry_count;
    int priority;
};

struct layout_shift_map_data {
    zmk_mod_flags_t modifier_map[8];
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

static inline struct layout_shift_map_entry layout_shift_map_entry(const struct device *dev,
                                                                   size_t index) {
    const struct layout_shift_map_config *cfg = dev->config;
    size_t raw_index = cfg->sorted_indices[index] * 3;

    return (struct layout_shift_map_entry){
        .from_keycode = cfg->mappings_raw[raw_index],
        .to_keycode = cfg->mappings_raw[raw_index + 1],
        .optional_mods = (zmk_mod_flags_t)cfg->mappings_raw[raw_index + 2],
    };
}

static inline int layout_shift_map_find_base(const struct device *dev, uint32_t base_keycode) {
    const struct layout_shift_map_config *cfg = dev->config;
    int lo = 0, hi = (int)cfg->entry_count - 1;
    int result = -1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        uint32_t mid_base = STRIP_MODS(layout_shift_map_entry(dev, mid).from_keycode);
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
zmk_mod_flags_t layout_shift_map_translate_mods(const struct device *dev, zmk_mod_flags_t mods,
                                                bool *changed);

#if DT_HAS_COMPAT_STATUS_OKAY(zmk_layout_shift_map)
extern const struct device *layout_shift_map_devs[];
extern const size_t layout_shift_map_dev_count;
#else
#define layout_shift_map_devs ((const struct device **)NULL)
#define layout_shift_map_dev_count 0
#endif

#endif
