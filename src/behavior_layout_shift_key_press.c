#define DT_DRV_COMPAT zmk_behavior_layout_shift_key_press

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>
#include <zmk/keys.h>
#include <dt-bindings/zmk/keys.h>
#include <dt-bindings/zmk/hid_usage.h>
#include <dt-bindings/zmk/hid_usage_pages.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// External function to check layout shift state
extern bool zmk_layout_shift_is_active(void);

// US to JIS keycode mapping table
// Maps US layout keycodes to their JIS equivalents
struct keycode_mapping {
    uint32_t us_keycode;
    uint32_t jis_keycode;
};

// Custom Japanese keyboard layout mapping table
static const struct keycode_mapping us_to_jis_map[] = {
    // User-specified mappings (to <- from):
    {UNDERSCORE, EQUAL},           // _ <- =
    {EQUAL, CARET},                // = <- ^
    {PLUS, TILDE},                 // + <- ~
    {LEFT_BRACKET, AT_SIGN},       // [ <- @
    {LEFT_BRACE, GRAVE},           // { <- `
    {RIGHT_BRACKET, LEFT_BRACKET}, // ] <- [
    {BACKSLASH, RIGHT_BRACKET},    // \ <- ]
    {RIGHT_BRACE, LEFT_BRACE},     // } <- {
    {PIPE, RIGHT_BRACE},           // | <- }
    {COLON, PLUS},                 // : <- +
    {SINGLE_QUOTE, COLON},         // ' <- :
    {DOUBLE_QUOTES, ASTERISK},     // " <- *
    {AT_SIGN, DOUBLE_QUOTES},      // @ <- "
    {CARET, AMPERSAND},            // ^ <- &
    {AMPERSAND, SINGLE_QUOTE},     // & <- '
    {ASTERISK, LEFT_PARENTHESIS},  // * <- (
    {LEFT_PARENTHESIS, RIGHT_PARENTHESIS}, // ( <- )
    {0x89, BACKSLASH },            // backslash
    {LS(0x89), PIPE},              // |

};

#define US_TO_JIS_MAP_SIZE (sizeof(us_to_jis_map) / sizeof(us_to_jis_map[0]))

// Function to lookup mapped keycode from input keycode
static uint32_t lookup_mapped_keycode(uint32_t input_keycode) {
    // Only apply mapping if layout shift is active
    if (!zmk_layout_shift_is_active()) {
        return input_keycode;
    }

    // Letters A-Z remain the same
    if (input_keycode >= A && input_keycode <= Z) {
        return input_keycode;
    }

    // Look up in mapping table (jis_keycode is the input, us_keycode is the output)
    for (size_t i = 0; i < US_TO_JIS_MAP_SIZE; i++) {
        if (us_to_jis_map[i].jis_keycode == input_keycode) {
            return us_to_jis_map[i].us_keycode;
        }
    }

    // If no mapping found, return original keycode
    return input_keycode;
}

struct behavior_layout_shift_key_press_data {};

struct behavior_layout_shift_key_press_config {};

static int on_layout_shift_key_press_binding_pressed(struct zmk_behavior_binding *binding,
                                                    struct zmk_behavior_binding_event event) {
    uint32_t original_keycode = binding->param1;
    uint32_t mapped_keycode = lookup_mapped_keycode(original_keycode);

    LOG_DBG("LAYOUT_SHIFT: Input keycode 0x%08X -> Mapped keycode 0x%08X", original_keycode, mapped_keycode);

    // Raise the mapped keycode event
    return raise_zmk_keycode_state_changed_from_encoded(
        mapped_keycode,
        true,  // pressed
        event.timestamp
    );
}

static int on_layout_shift_key_press_binding_released(struct zmk_behavior_binding *binding,
                                                     struct zmk_behavior_binding_event event) {
    uint32_t original_keycode = binding->param1;
    uint32_t mapped_keycode = lookup_mapped_keycode(original_keycode);

    LOG_DBG("LAYOUT_SHIFT: Released input keycode 0x%08X -> Mapped keycode 0x%08X", original_keycode, mapped_keycode);

    // Release the mapped keycode
    return raise_zmk_keycode_state_changed_from_encoded(
        mapped_keycode,
        false, // released
        event.timestamp
    );
}

static const struct behavior_driver_api behavior_layout_shift_key_press_driver_api = {
    .binding_pressed = on_layout_shift_key_press_binding_pressed,
    .binding_released = on_layout_shift_key_press_binding_released,
    .locality = BEHAVIOR_LOCALITY_CENTRAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .parameter_metadata = {
        .count = 1,
        .types = {
            ZMK_BEHAVIOR_PARAM_TYPE_KEYCODE,
        },
    },
#endif
};

static int layout_shift_key_press_init(const struct device *dev) {
    LOG_INF("Custom Layout Shift Behavior Initialized - Japanese keyboard mapping active!");
    return 0;
}

// Define behavior instance
static struct behavior_layout_shift_key_press_data behavior_layout_shift_key_press_data_0 = {};
static const struct behavior_layout_shift_key_press_config behavior_layout_shift_key_press_config_0 = {};

BEHAVIOR_DT_INST_DEFINE(0, layout_shift_key_press_init, NULL,
                        &behavior_layout_shift_key_press_data_0, &behavior_layout_shift_key_press_config_0,
                        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_layout_shift_key_press_driver_api);