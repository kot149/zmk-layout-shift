#ifdef CONFIG_LAYOUT_SHIFT_TARGET_DVORAK
#define LAYOUT_DEFINED
// Dvorak keyboard layout mappings
// Maps US QWERTY keycodes to their Dvorak equivalents
static const struct keycode_mapping layout_map[] = {
    /* Letter and symbol position changes (QWERTY -> Dvorak) */
    /* For Dvorak, most modifiers are optional since it's primarily about key position changes */
    {MINUS, LEFT_BRACKET, OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},     // - -> [ (Shift important for symbols)
    {EQUAL, RIGHT_BRACKET, OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},    // = -> ] (Shift important for symbols)
    {Q, SINGLE_QUOTE, OPTIONAL_ALL},        // Q -> ' (all modifiers optional for letters)
    {W, COMMA, OPTIONAL_ALL},               // W -> , (all modifiers optional for letters)
    {E, DOT, OPTIONAL_ALL},                 // E -> . (all modifiers optional for letters)
    {R, P, OPTIONAL_ALL},                   // R -> P (all modifiers optional for letters)
    {T, Y, OPTIONAL_ALL},                   // T -> Y (all modifiers optional for letters)
    {Y, F, OPTIONAL_ALL},                   // Y -> F (all modifiers optional for letters)
    {U, G, OPTIONAL_ALL},                   // U -> G (all modifiers optional for letters)
    {I, C, OPTIONAL_ALL},                   // I -> C (all modifiers optional for letters)
    {O, R, OPTIONAL_ALL},                   // O -> R (all modifiers optional for letters)
    {P, L, OPTIONAL_ALL},                   // P -> L (all modifiers optional for letters)
    {LEFT_BRACKET, SLASH, OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},    // [ -> / (Shift important for symbols)
    {RIGHT_BRACKET, EQUAL, OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},   // ] -> = (Shift important for symbols)
    {A, A, OPTIONAL_ALL},                   // A -> A (no change, but enable modifier optional)
    {S, O, OPTIONAL_ALL},                   // S -> O (all modifiers optional for letters)
    {D, E, OPTIONAL_ALL},                   // D -> E (all modifiers optional for letters)
    {F, U, OPTIONAL_ALL},                   // F -> U (all modifiers optional for letters)
    {G, I, OPTIONAL_ALL},                   // G -> I (all modifiers optional for letters)
    {H, D, OPTIONAL_ALL},                   // H -> D (all modifiers optional for letters)
    {J, H, OPTIONAL_ALL},                   // J -> H (all modifiers optional for letters)
    {K, T, OPTIONAL_ALL},                   // K -> T (all modifiers optional for letters)
    {L, N, OPTIONAL_ALL},                   // L -> N (all modifiers optional for letters)
    {SEMICOLON, S, OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},           // ; -> S (Shift important for symbols)
    {SINGLE_QUOTE, MINUS, OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},    // ' -> - (Shift important for symbols)
    {Z, SEMICOLON, OPTIONAL_ALL},           // Z -> ; (all modifiers optional for letters)
    {X, Q, OPTIONAL_ALL},                   // X -> Q (all modifiers optional for letters)
    {C, J, OPTIONAL_ALL},                   // C -> J (all modifiers optional for letters)
    {V, K, OPTIONAL_ALL},                   // V -> K (all modifiers optional for letters)
    {B, X, OPTIONAL_ALL},                   // B -> X (all modifiers optional for letters)
    {N, B, OPTIONAL_ALL},                   // N -> B (all modifiers optional for letters)
    {M, M, OPTIONAL_ALL},                   // M -> M (no change, but enable modifier optional)
    {COMMA, W, OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},               // , -> W (Shift important for symbols)
    {DOT, V, OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},                 // . -> V (Shift important for symbols)
    {SLASH, Z, OPTIONAL_CTRL | OPTIONAL_ALT | OPTIONAL_GUI},               // / -> Z (Shift important for symbols)
};
#endif