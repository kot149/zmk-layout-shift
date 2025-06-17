#ifdef CONFIG_LAYOUT_SHIFT_TARGET_DVORAK
#define LAYOUT_DEFINED
// Dvorak keyboard layout mappings
// Maps US QWERTY keycodes to their Dvorak equivalents
static const struct keycode_mapping layout_map[] = {
    /* Letter and symbol position changes (QWERTY -> Dvorak) */
    /* For Dvorak, most modifiers are optional since it's primarily about key position changes */
    {MINUS, LEFT_BRACKET, OPTIONAL_ALL},    // - -> [ (all modifiers optional)
    {EQUAL, RIGHT_BRACKET, OPTIONAL_ALL},   // = -> ] (all modifiers optional)
    {Q, SINGLE_QUOTE, OPTIONAL_ALL},        // Q -> ' (all modifiers optional)
    {W, COMMA, OPTIONAL_ALL},               // W -> , (all modifiers optional)
    {E, DOT, OPTIONAL_ALL},                 // E -> . (all modifiers optional)
    {R, P, OPTIONAL_ALL},                   // R -> P (all modifiers optional)
    {T, Y, OPTIONAL_ALL},                   // T -> Y (all modifiers optional)
    {Y, F, OPTIONAL_ALL},                   // Y -> F (all modifiers optional)
    {U, G, OPTIONAL_ALL},                   // U -> G (all modifiers optional)
    {I, C, OPTIONAL_ALL},                   // I -> C (all modifiers optional)
    {O, R, OPTIONAL_ALL},                   // O -> R (all modifiers optional)
    {P, L, OPTIONAL_ALL},                   // P -> L (all modifiers optional)
    {LEFT_BRACKET, SLASH, OPTIONAL_ALL},    // [ -> / (all modifiers optional)
    {RIGHT_BRACKET, EQUAL, OPTIONAL_ALL},   // ] -> = (all modifiers optional)
    {S, O, OPTIONAL_ALL},                   // S -> O (all modifiers optional)
    {D, E, OPTIONAL_ALL},                   // D -> E (all modifiers optional)
    {F, U, OPTIONAL_ALL},                   // F -> U (all modifiers optional)
    {G, I, OPTIONAL_ALL},                   // G -> I (all modifiers optional)
    {H, D, OPTIONAL_ALL},                   // H -> D (all modifiers optional)
    {J, H, OPTIONAL_ALL},                   // J -> H (all modifiers optional)
    {K, T, OPTIONAL_ALL},                   // K -> T (all modifiers optional)
    {L, N, OPTIONAL_ALL},                   // L -> N (all modifiers optional)
    {SEMICOLON, S, OPTIONAL_ALL},           // ; -> S (all modifiers optional)
    {SINGLE_QUOTE, MINUS, OPTIONAL_ALL},    // ' -> - (all modifiers optional)
    {Z, SEMICOLON, OPTIONAL_ALL},           // Z -> ; (all modifiers optional)
    {X, Q, OPTIONAL_ALL},                   // X -> Q (all modifiers optional)
    {C, J, OPTIONAL_ALL},                   // C -> J (all modifiers optional)
    {V, K, OPTIONAL_ALL},                   // V -> K (all modifiers optional)
    {B, X, OPTIONAL_ALL},                   // B -> X (all modifiers optional)
    {N, B, OPTIONAL_ALL},                   // N -> B (all modifiers optional)
    {COMMA, W, OPTIONAL_ALL},               // , -> W (all modifiers optional)
    {DOT, V, OPTIONAL_ALL},                 // . -> V (all modifiers optional)
    {SLASH, Z, OPTIONAL_ALL},               // / -> Z (all modifiers optional)
};
#endif