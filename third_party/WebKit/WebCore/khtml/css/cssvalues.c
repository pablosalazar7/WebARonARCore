/* ANSI-C code produced by gperf version 2.7.2 */
/* Command-line: gperf -L ANSI-C -E -C -n -o -t -k '*' -NfindValue -Hhash_val -Wwordlist_value -D cssvalues.gperf  */
/* This file is automatically generated from cssvalues.in by makevalues, do not edit */
/* Copyright 1999 W. Bastian */
#include "cssvalues.h"
struct css_value {
    const char *name;
    int id;
};
/* maximum key range = 1382, duplicates = 1 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash_val (register const char *str, register unsigned int len)
{
  static const unsigned short asso_values[] =
    {
      1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382,
      1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382,
      1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382,
      1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382,
      1382, 1382, 1382, 1382, 1382,   40, 1382, 1382,    0,   25,
        35,   40,   45,   50,    5,   10,   20,    0, 1382, 1382,
      1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382,
      1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382,
      1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382,
      1382, 1382, 1382, 1382, 1382, 1382, 1382,    0,  163,    4,
        64,   15,    0,  235,  160,   15,    5,  145,    0,  175,
        70,    0,    9,  120,  120,   40,    0,  255,  179,  115,
        15,   40,  195, 1382, 1382, 1382, 1382, 1382, 1382, 1382,
      1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382,
      1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382,
      1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382,
      1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382,
      1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382,
      1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382,
      1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382,
      1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382,
      1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382,
      1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382,
      1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382,
      1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382, 1382,
      1382, 1382, 1382, 1382, 1382, 1382
    };
  register int hval = 0;

  switch (len)
    {
      default:
      case 21:
        hval += asso_values[(unsigned char)str[20]];
      case 20:
        hval += asso_values[(unsigned char)str[19]];
      case 19:
        hval += asso_values[(unsigned char)str[18]];
      case 18:
        hval += asso_values[(unsigned char)str[17]];
      case 17:
        hval += asso_values[(unsigned char)str[16]];
      case 16:
        hval += asso_values[(unsigned char)str[15]];
      case 15:
        hval += asso_values[(unsigned char)str[14]];
      case 14:
        hval += asso_values[(unsigned char)str[13]];
      case 13:
        hval += asso_values[(unsigned char)str[12]];
      case 12:
        hval += asso_values[(unsigned char)str[11]];
      case 11:
        hval += asso_values[(unsigned char)str[10]];
      case 10:
        hval += asso_values[(unsigned char)str[9]];
      case 9:
        hval += asso_values[(unsigned char)str[8]];
      case 8:
        hval += asso_values[(unsigned char)str[7]];
      case 7:
        hval += asso_values[(unsigned char)str[6]];
      case 6:
        hval += asso_values[(unsigned char)str[5]];
      case 5:
        hval += asso_values[(unsigned char)str[4]];
      case 4:
        hval += asso_values[(unsigned char)str[3]];
      case 3:
        hval += asso_values[(unsigned char)str[2]];
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

#ifdef __GNUC__
__inline
#endif
const struct css_value *
findValue (register const char *str, register unsigned int len)
{
  enum
    {
      TOTAL_KEYWORDS = 181,
      MIN_WORD_LENGTH = 3,
      MAX_WORD_LENGTH = 21,
      MIN_HASH_VALUE = 0,
      MAX_HASH_VALUE = 1381
    };

  static const struct css_value wordlist_value[] =
    {
      {"900", CSS_VAL_900},
      {"600", CSS_VAL_600},
      {"top", CSS_VAL_TOP},
      {"700", CSS_VAL_700},
      {"left", CSS_VAL_LEFT},
      {"800", CSS_VAL_800},
      {"100", CSS_VAL_100},
      {"text", CSS_VAL_TEXT},
      {"italic", CSS_VAL_ITALIC},
      {"200", CSS_VAL_200},
      {"300", CSS_VAL_300},
      {"400", CSS_VAL_400},
      {"500", CSS_VAL_500},
      {"static", CSS_VAL_STATIC},
      {"collapse", CSS_VAL_COLLAPSE},
      {"text-top", CSS_VAL_TEXT_TOP},
      {"icon", CSS_VAL_ICON},
      {"caption", CSS_VAL_CAPTION},
      {"fixed", CSS_VAL_FIXED},
      {"solid", CSS_VAL_SOLID},
      {"ltr", CSS_VAL_LTR},
      {"rtl", CSS_VAL_RTL},
      {"disc", CSS_VAL_DISC},
      {"wait", CSS_VAL_WAIT},
      {"crop", CSS_VAL_CROP},
      {"inset", CSS_VAL_INSET},
      {"dotted", CSS_VAL_DOTTED},
      {"pre", CSS_VAL_PRE},
      {"fantasy", CSS_VAL_FANTASY},
      {"none", CSS_VAL_NONE},
      {"circle", CSS_VAL_CIRCLE},
      {"repeat", CSS_VAL_REPEAT},
      {"scroll", CSS_VAL_SCROLL},
      {"table", CSS_VAL_TABLE},
      {"help", CSS_VAL_HELP},
      {"inline", CSS_VAL_INLINE},
      {"serif", CSS_VAL_SERIF},
      {"compact", CSS_VAL_COMPACT},
      {"always", CSS_VAL_ALWAYS},
      {"separate", CSS_VAL_SEPARATE},
      {"landscape", CSS_VAL_LANDSCAPE},
      {"cross", CSS_VAL_CROSS},
      {"mix", CSS_VAL_MIX},
      {"level", CSS_VAL_LEVEL},
      {"repeat-x", CSS_VAL_REPEAT_X},
      {"small", CSS_VAL_SMALL},
      {"inside", CSS_VAL_INSIDE},
      {"center", CSS_VAL_CENTER},
      {"bold", CSS_VAL_BOLD},
      {"pointer", CSS_VAL_POINTER},
      {"table-cell", CSS_VAL_TABLE_CELL},
      {"repeat-y", CSS_VAL_REPEAT_Y},
      {"thin", CSS_VAL_THIN},
      {"static-position", CSS_VAL_STATIC_POSITION},
      {"lower", CSS_VAL_LOWER},
      {"expanded", CSS_VAL_EXPANDED},
      {"capitalize", CSS_VAL_CAPITALIZE},
      {"hide", CSS_VAL_HIDE},
      {"auto", CSS_VAL_AUTO},
      {"avoid", CSS_VAL_AVOID},
      {"portrait", CSS_VAL_PORTRAIT},
      {"no-repeat", CSS_VAL_NO_REPEAT},
      {"x-small", CSS_VAL_X_SMALL},
      {"decimal", CSS_VAL_DECIMAL},
      {"xx-small", CSS_VAL_XX_SMALL},
      {"below", CSS_VAL_BELOW},
      {"hand", CSS_VAL_HAND},
      {"list-item", CSS_VAL_LIST_ITEM},
      {"small-caps", CSS_VAL_SMALL_CAPS},
      {"lowercase", CSS_VAL_LOWERCASE},
      {"outset", CSS_VAL_OUTSET},
      {"block", CSS_VAL_BLOCK},
      {"monospace", CSS_VAL_MONOSPACE},
      {"nowrap", CSS_VAL_NOWRAP},
      {"show", CSS_VAL_SHOW},
      {"table-caption", CSS_VAL_TABLE_CAPTION},
      {"baseline", CSS_VAL_BASELINE},
      {"loud", CSS_VAL_LOUD},
      {"both", CSS_VAL_BOTH},
      {"thick", CSS_VAL_THICK},
      {"wider", CSS_VAL_WIDER},
      {"middle", CSS_VAL_MIDDLE},
      {"default", CSS_VAL_DEFAULT},
      {"bottom", CSS_VAL_BOTTOM},
      {"condensed", CSS_VAL_CONDENSED},
      {"dashed", CSS_VAL_DASHED},
      {"relative", CSS_VAL_RELATIVE},
      {"smaller", CSS_VAL_SMALLER},
      {"small-caption", CSS_VAL_SMALL_CAPTION},
      {"justify", CSS_VAL_JUSTIFY},
      {"above", CSS_VAL_ABOVE},
      {"katakana", CSS_VAL_KATAKANA},
      {"bolder", CSS_VAL_BOLDER},
      {"normal", CSS_VAL_NORMAL},
      {"move", CSS_VAL_MOVE},
      {"large", CSS_VAL_LARGE},
      {"lower-latin", CSS_VAL_LOWER_LATIN},
      {"sans-serif", CSS_VAL_SANS_SERIF},
      {"hidden", CSS_VAL_HIDDEN},
      {"outside", CSS_VAL_OUTSIDE},
      {"blink", CSS_VAL_BLINK},
      {"inherit", CSS_VAL_INHERIT},
      {"invert", CSS_VAL_INVERT},
      {"inline-table", CSS_VAL_INLINE_TABLE},
      {"text-bottom", CSS_VAL_TEXT_BOTTOM},
      {"overline", CSS_VAL_OVERLINE},
      {"x-large", CSS_VAL_X_LARGE},
      {"visible", CSS_VAL_VISIBLE},
      {"embed", CSS_VAL_EMBED},
      {"super", CSS_VAL_SUPER},
      {"xx-large", CSS_VAL_XX_LARGE},
      {"extra-expanded", CSS_VAL_EXTRA_EXPANDED},
      {"transparent", CSS_VAL_TRANSPARENT},
      {"ridge", CSS_VAL_RIDGE},
      {"table-row", CSS_VAL_TABLE_ROW},
      {"e-resize", CSS_VAL_E_RESIZE},
      {"sub", CSS_VAL_SUB},
      {"lower-alpha", CSS_VAL_LOWER_ALPHA},
      {"armenian", CSS_VAL_ARMENIAN},
      {"uppercase", CSS_VAL_UPPERCASE},
      {"absolute", CSS_VAL_ABSOLUTE},
      {"s-resize", CSS_VAL_S_RESIZE},
      {"close-quote", CSS_VAL_CLOSE_QUOTE},
      {"larger", CSS_VAL_LARGER},
      {"se-resize", CSS_VAL_SE_RESIZE},
      {"double", CSS_VAL_DOUBLE},
      {"crosshair", CSS_VAL_CROSSHAIR},
      {"n-resize", CSS_VAL_N_RESIZE},
      {"menu", CSS_VAL_MENU},
      {"open-quote", CSS_VAL_OPEN_QUOTE},
      {"ne-resize", CSS_VAL_NE_RESIZE},
      {"right", CSS_VAL_RIGHT},
      {"extra-condensed", CSS_VAL_EXTRA_CONDENSED},
      {"upper-latin", CSS_VAL_UPPER_LATIN},
      {"semi-expanded", CSS_VAL_SEMI_EXPANDED},
      {"lighter", CSS_VAL_LIGHTER},
      {"groove", CSS_VAL_GROOVE},
      {"square", CSS_VAL_SQUARE},
      {"w-resize", CSS_VAL_W_RESIZE},
      {"narrower", CSS_VAL_NARROWER},
      {"oblique", CSS_VAL_OBLIQUE},
      {"run-in", CSS_VAL_RUN_IN},
      {"marker", CSS_VAL_MARKER},
      {"hebrew", CSS_VAL_HEBREW},
      {"sw-resize", CSS_VAL_SW_RESIZE},
      {"no-close-quote", CSS_VAL_NO_CLOSE_QUOTE},
      {"hiragana", CSS_VAL_HIRAGANA},
      {"upper-alpha", CSS_VAL_UPPER_ALPHA},
      {"underline", CSS_VAL_UNDERLINE},
      {"nw-resize", CSS_VAL_NW_RESIZE},
      {"semi-condensed", CSS_VAL_SEMI_CONDENSED},
      {"cursive", CSS_VAL_CURSIVE},
      {"no-open-quote", CSS_VAL_NO_OPEN_QUOTE},
      {"-konq-center", CSS_VAL__KONQ_CENTER},
      {"lower-roman", CSS_VAL_LOWER_ROMAN},
      {"status-bar", CSS_VAL_STATUS_BAR},
      {"ultra-expanded", CSS_VAL_ULTRA_EXPANDED},
      {"-konq-auto", CSS_VAL__KONQ_AUTO},
      {"georgian", CSS_VAL_GEORGIAN},
      {"katakana-iroha", CSS_VAL_KATAKANA_IROHA},
      {"medium", CSS_VAL_MEDIUM},
      {"higher", CSS_VAL_HIGHER},
      {"table-column", CSS_VAL_TABLE_COLUMN},
      {"-konq-nowrap", CSS_VAL__KONQ_NOWRAP},
      {"message-box", CSS_VAL_MESSAGE_BOX},
      {"ultra-condensed", CSS_VAL_ULTRA_CONDENSED},
      {"-konq-normal", CSS_VAL__KONQ_NORMAL},
      {"upper-roman", CSS_VAL_UPPER_ROMAN},
      {"lower-greek", CSS_VAL_LOWER_GREEK},
      {"bidi-override", CSS_VAL_BIDI_OVERRIDE},
      {"cjk-ideographic", CSS_VAL_CJK_IDEOGRAPHIC},
      {"-konq-xxx-large", CSS_VAL__KONQ_XXX_LARGE},
      {"hiragana-iroha", CSS_VAL_HIRAGANA_IROHA},
      {"-konq-around-floats", CSS_VAL__KONQ_AROUND_FLOATS},
      {"table-footer-group", CSS_VAL_TABLE_FOOTER_GROUP},
      {"line-through", CSS_VAL_LINE_THROUGH},
      {"decimal-leading-zero", CSS_VAL_DECIMAL_LEADING_ZERO},
      {"-konq-baseline-middle", CSS_VAL__KONQ_BASELINE_MIDDLE},
      {"table-row-group", CSS_VAL_TABLE_ROW_GROUP},
      {"table-header-group", CSS_VAL_TABLE_HEADER_GROUP},
      {"table-column-group", CSS_VAL_TABLE_COLUMN_GROUP}
    };

  static const short lookup[] =
    {
         0,   -1,   -1,   -1,   -1,    1,   -1,   -1,
        -1,    2,    3,   -1,   -1,   -1,   -1,    4,
        -1,   -1,   -1,   -1,    5,   -1,   -1,   -1,
        -1,    6,   -1,   -1,   -1,   -1,    7,   -1,
        -1,   -1,    8,    9,   -1,   -1,   -1,   -1,
        10,   -1,   -1,   -1,   -1,   11,   -1,   -1,
        -1,   -1,   12,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   13,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   14,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   15,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   16,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   17,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   18,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   19,
      -303, -161,   -2,   22,   -1,   -1,   -1,   -1,
        -1,   -1,   23,   -1,   -1,   24,   -1,   -1,
        -1,   -1,   -1,   -1,   25,   -1,   -1,   26,
        27,   -1,   -1,   -1,   -1,   -1,   28,   -1,
        -1,   -1,   -1,   29,   -1,   -1,   30,   31,
        -1,   -1,   -1,   -1,   32,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   33,   -1,   -1,   -1,   -1,   -1,
        34,   35,   -1,   -1,   -1,   -1,   36,   -1,
        37,   -1,   -1,   38,   -1,   -1,   -1,   39,
        -1,   -1,   40,   -1,   41,   42,   -1,   -1,
        -1,   43,   -1,   -1,   -1,   -1,   44,   45,
        -1,   -1,   -1,   46,   -1,   -1,   -1,   -1,
        47,   -1,   -1,   48,   -1,   49,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   50,   -1,   51,
        -1,   -1,   -1,   -1,   -1,   52,   -1,   -1,
        53,   -1,   54,   -1,   55,   56,   57,   58,
        -1,   -1,   59,   -1,   -1,   -1,   -1,   -1,
        60,   -1,   -1,   -1,   -1,   61,   62,   -1,
        -1,   63,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   64,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   65,   66,   -1,
        -1,   -1,   -1,   -1,   67,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   68,   69,   70,   -1,
        71,   72,   73,   74,   75,   -1,   76,   77,
        -1,   -1,   -1,   78,   79,   -1,   -1,   -1,
        -1,   80,   -1,   -1,   -1,   81,   82,   -1,
        -1,   -1,   83,   -1,   -1,   -1,   84,   85,
        86,   -1,   -1,   -1,   -1,   -1,   87,   -1,
        -1,   88,   -1,   89,   -1,   90,   -1,   -1,
        91,   -1,   92,   -1,   -1,   93,   -1,   -1,
        -1,   94,   95,   -1,   -1,   -1,   -1,   96,
        -1,   -1,   -1,   -1,   97,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   98,   99,   -1,   -1,
        -1,  100,   -1,  101,   -1,   -1,   -1,  102,
        -1,   -1,   -1,  103,   -1,   -1,   -1,   -1,
       104,   -1,   -1,   -1,   -1,   -1,  105,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,  106,   -1,  107,   -1,   -1,   -1,   -1,
       108,   -1,   -1,   -1,   -1,   -1,   -1,  109,
       110,   -1,  111,   -1,  112,   -1,   -1,   -1,
        -1,  113,   -1,   -1,   -1,  114,   -1,  115,
        -1,   -1,  116,  117,   -1,   -1,   -1,   -1,
        -1,  118,   -1,  119,   -1,   -1,   -1,   -1,
        -1,  120,   -1,   -1,   -1,   -1,   -1,   -1,
       121,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,  122,  123,   -1,   -1,   -1,   -1,  124,
        -1,  125,   -1,  126,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,  127,   -1,
        -1,   -1,   -1,  128,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,  129,  130,   -1,   -1,
        -1,   -1,  131,   -1,  132,  133,   -1,   -1,
        -1,  134,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,  135,   -1,   -1,   -1,  136,  137,   -1,
        -1,   -1,   -1,  138,   -1,   -1,   -1,   -1,
       139,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       140,   -1,  141,   -1,   -1,   -1,   -1,  142,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,  143,   -1,   -1,   -1,
        -1,   -1,   -1,  144,   -1,   -1,   -1,  145,
       146,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,  147,   -1,   -1,   -1,   -1,   -1,   -1,
       148,  149,   -1,  150,  151,   -1,   -1,   -1,
        -1,   -1,  152,   -1,   -1,   -1,   -1,  153,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,  154,
        -1,   -1,  155,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,  156,   -1,   -1,  157,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,  158,   -1,   -1,   -1,   -1,  159,
        -1,   -1,   -1,  160,   -1,   -1,   -1,   -1,
        -1,  161,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,  162,   -1,   -1,   -1,   -1,   -1,
        -1,  163,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,  164,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,  165,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,  166,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,  167,   -1,   -1,
        -1,   -1,   -1,   -1,  168,   -1,   -1,   -1,
        -1,  169,   -1,   -1,   -1,   -1,   -1,  170,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,  171,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,  172,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,  173,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,  174,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,  175,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,  176,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,  177,   -1,   -1,   -1,   -1,   -1,
       178,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,  179,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,  180
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash_val (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int index = lookup[key];

          if (index >= 0)
            {
              register const char *s = wordlist_value[index].name;

              if (*str == *s && !strcmp (str + 1, s + 1))
                return &wordlist_value[index];
            }
          else if (index < -TOTAL_KEYWORDS)
            {
              register int offset = - 1 - TOTAL_KEYWORDS - index;
              register const struct css_value *wordptr = &wordlist_value[TOTAL_KEYWORDS + lookup[offset]];
              register const struct css_value *wordendptr = wordptr + -lookup[offset + 1];

              while (wordptr < wordendptr)
                {
                  register const char *s = wordptr->name;

                  if (*str == *s && !strcmp (str + 1, s + 1))
                    return wordptr;
                  wordptr++;
                }
            }
        }
    }
  return 0;
}
static const char * const valueList[] = {
"",
"inherit", 
"none", 
"hidden", 
"dotted", 
"dashed", 
"double", 
"solid", 
"outset", 
"inset", 
"groove", 
"ridge", 
"caption", 
"icon", 
"menu", 
"message-box", 
"small-caption", 
"status-bar", 
"italic", 
"oblique", 
"small-caps", 
"normal", 
"bold", 
"bolder", 
"lighter", 
"100", 
"200", 
"300", 
"400", 
"500", 
"600", 
"700", 
"800", 
"900", 
"xx-small", 
"x-small", 
"small", 
"medium", 
"large", 
"x-large", 
"xx-large", 
"-konq-xxx-large", 
"smaller", 
"larger", 
"wider", 
"narrower", 
"ultra-condensed", 
"extra-condensed", 
"condensed", 
"semi-condensed", 
"semi-expanded", 
"expanded", 
"extra-expanded", 
"ultra-expanded", 
"serif", 
"sans-serif", 
"cursive", 
"fantasy", 
"monospace", 
"transparent", 
"repeat", 
"repeat-x", 
"repeat-y", 
"no-repeat", 
"baseline", 
"middle", 
"sub", 
"super", 
"text-top", 
"text-bottom", 
"top", 
"bottom", 
"-konq-baseline-middle", 
"-konq-auto", 
"left", 
"right", 
"center", 
"justify", 
"-konq-center", 
"outside", 
"inside", 
"disc", 
"circle", 
"square", 
"decimal", 
"decimal-leading-zero", 
"lower-roman", 
"upper-roman", 
"lower-greek", 
"lower-alpha", 
"lower-latin", 
"upper-alpha", 
"upper-latin", 
"hebrew", 
"armenian", 
"georgian", 
"cjk-ideographic", 
"hiragana", 
"katakana", 
"hiragana-iroha", 
"katakana-iroha", 
"inline", 
"block", 
"list-item", 
"run-in", 
"compact", 
"marker", 
"table", 
"inline-table", 
"table-row-group", 
"table-header-group", 
"table-footer-group", 
"table-row", 
"table-column-group", 
"table-column", 
"table-cell", 
"table-caption", 
"auto", 
"crosshair", 
"default", 
"pointer", 
"move", 
"e-resize", 
"ne-resize", 
"nw-resize", 
"n-resize", 
"se-resize", 
"sw-resize", 
"s-resize", 
"w-resize", 
"text", 
"wait", 
"help", 
"ltr", 
"rtl", 
"capitalize", 
"uppercase", 
"lowercase", 
"visible", 
"collapse", 
"above", 
"absolute", 
"always", 
"avoid", 
"below", 
"bidi-override", 
"blink", 
"both", 
"close-quote", 
"crop", 
"cross", 
"embed", 
"fixed", 
"hand", 
"hide", 
"higher", 
"invert", 
"landscape", 
"level", 
"line-through", 
"loud", 
"lower", 
"mix", 
"no-close-quote", 
"no-open-quote", 
"nowrap", 
"open-quote", 
"overline", 
"portrait", 
"pre", 
"relative", 
"scroll", 
"separate", 
"show", 
"static", 
"static-position", 
"thick", 
"thin", 
"underline", 
"-konq-nowrap", 
"-konq-normal", 
"-konq-around-floats", 
    0
};
DOMString getValueName(unsigned short id)
{
    if(id >= CSS_VAL_TOTAL || id == 0)
      return DOMString();
    else
      return DOMString(valueList[id]);
};

