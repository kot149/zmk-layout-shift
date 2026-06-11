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

bool layout_shift_map_is_active(const struct device *dev);
void layout_shift_map_set_active(const struct device *dev, bool active);
void layout_shift_map_toggle(const struct device *dev);

size_t layout_shift_map_entry_count(const struct device *dev);
struct layout_shift_map_entry layout_shift_map_get_entry(const struct device *dev, size_t index);

#endif
