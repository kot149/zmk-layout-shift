#ifdef CONFIG_LAYOUT_SHIFT_TARGET_JIS
#define LAYOUT_DEFINED
// Japanese (JIS) keyboard layout mappings
// Maps US layout keycodes to their JIS equivalents
static const struct keycode_mapping layout_map[] = {
    /* from -> to, optional_modifiers */
    {EQUAL,             UNDERSCORE,        OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},  /* = -> _ (Ctrl/Alt/GUI optional) */
    {CARET,             EQUAL,             OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},  /* ^ -> = (Ctrl/Alt/GUI optional) */
    {TILDE,             PLUS,              OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},  /* ~ -> + (Ctrl/Alt/GUI optional) */
    {AT_SIGN,           LEFT_BRACKET,      OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},  /* @ -> [ (Ctrl/Alt/GUI optional) */
    {GRAVE,             LEFT_BRACE,        OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},  /* ` -> { (Ctrl/Alt/GUI optional) */
    {LEFT_BRACKET,      RIGHT_BRACKET,     OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},  /* [ -> ] (Ctrl/Alt/GUI optional) */
    {RIGHT_BRACKET,     BACKSLASH,         OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},  /* ] -> \ (Ctrl/Alt/GUI optional) */
    {LEFT_BRACE,        RIGHT_BRACE,       OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},  /* { -> } (Ctrl/Alt/GUI optional) */
    {RIGHT_BRACE,       PIPE,              OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},  /* } -> | (Ctrl/Alt/GUI optional) */
    {PLUS,              COLON,             OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},  /* + -> : (Ctrl/Alt/GUI optional) */
    {COLON,             SINGLE_QUOTE,      OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},  /* : -> ' (Ctrl/Alt/GUI optional) */
    {ASTERISK,          DOUBLE_QUOTES,     OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},  /* * -> " (Ctrl/Alt/GUI optional) */
    {DOUBLE_QUOTES,     AT_SIGN,           OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},  /* " -> @ (Ctrl/Alt/GUI optional) */
    {AMPERSAND,         CARET,             OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},  /* & -> ^ (Ctrl/Alt/GUI optional) */
    {SINGLE_QUOTE,      AMPERSAND,         OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},  /* ' -> & (Ctrl/Alt/GUI optional) */
    {LEFT_PARENTHESIS,  ASTERISK,          OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},  /* ( -> * (Ctrl/Alt/GUI optional) */
    {RIGHT_PARENTHESIS, LEFT_PARENTHESIS,  OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},  /* ) -> ( (Ctrl/Alt/GUI optional) */
    {UNDERSCORE,        LS(0x87),          OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},  /* _      (Ctrl/Alt/GUI optional) */
    {BACKSLASH,         0x89,              OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},  /* \      (Ctrl/Alt/GUI optional) */
    {PIPE,              LS(0x89),          OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},  /* |      (Ctrl/Alt/GUI optional) */
};
#endif