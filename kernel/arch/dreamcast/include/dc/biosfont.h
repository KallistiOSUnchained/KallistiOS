/* KallistiOS ##version##

   dc/biosfont.h
   Copyright (C) 2000-2001 Megan Potter
   Japanese Functions Copyright (C) 2002 Kazuaki Matsumoto
   Copyright (C) 2017 Donald Haase
   Copyright (C) 2024 Falco Girgis
   Copyright (C) 2024 Andress Barajas

*/

/** \file    dc/biosfont.h
    \brief   BIOS font drawing functions.
    \ingroup bfont

    This file provides support for utilizing the font built into the Dreamcast's
    BIOS. These functions allow access to both the western character set and
    Japanese characters.

    \author Megan Potter
    \author Kazuaki Matsumoto
    \author Donald Haase
    \author Falco Girgis

    \todo
        - More user-friendly way to fetch/print DC-specific icons.
*/

#ifndef __DC_BIOSFONT_H
#define __DC_BIOSFONT_H

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include <arch/types.h>

/** \defgroup bfont  BIOS
    \brief    API for the Dreamcast's built-in BIOS font.
    \ingroup  video_fonts
    @{
*/

/** \defgroup bfont_size  Dimensions
    \brief    Font size definitions for the BIOS fonts.
    @{
*/
#define BFONT_THIN_WIDTH                        12  /**< \brief Width of Thin Font (ISO8859_1, half-JP) */
#define BFONT_WIDE_WIDTH    (BFONT_THIN_WIDTH * 2)  /**< \brief Width of Wide Font (full-JP) */
#define BFONT_HEIGHT                            24  /**< \brief Height of All Fonts */
#define BFONT_ICON_DIMEN                        32  /**< \brief Dimension of vmu icons */
/** @} */

/** \defgroup bfont_byte_size  Byte Sizes
    \brief    Byte size definitions for the BIOS fonts.
    @{
*/
/** \brief Number of bytes to represent a single thin character within the BIOS font. */
#define BFONT_BYTES_PER_CHAR        (BFONT_THIN_WIDTH * BFONT_HEIGHT / 8)

/** \brief Number of bytes to represent a single wide character within the BIOS font. */
#define BFONT_BYTES_PER_WIDE_CHAR   (BFONT_WIDE_WIDTH * BFONT_HEIGHT / 8)

/** \brief Number of bytes to represent a vmu icon within the BIOS font. */
#define BFONT_BYTES_PER_VMU_ICON    (BFONT_ICON_DIMEN * BFONT_ICON_DIMEN / 8)

/** @} */

/** \defgroup bfont_indicies Structure
    \brief    Structure of the Bios Font.
    @{
*/
/** \brief Start of Narrow Characters in Font Block */
#define BFONT_NARROW_START          0
#define BFONT_OVERBAR               BFONT_NARROW_START
#define BFONT_ISO_8859_1_33_126     (BFONT_NARROW_START + ( 1 * BFONT_BYTES_PER_CHAR))
#define BFONT_YEN                   (BFONT_NARROW_START + (95 * BFONT_BYTES_PER_CHAR))
#define BFONT_ISO_8859_1_160_255    (BFONT_NARROW_START + (96 * BFONT_BYTES_PER_CHAR))

/* JISX-0208 Rows 1-7 and 16-84 are encoded between BFONT_WIDE_START and BFONT_DREAMCAST_SPECIFIC.
    Only the box-drawing characters (row 8) are missing. */
/** \brief Size of a row for JISX-0208 characters */
#define JISX_0208_ROW_SIZE            94
/** \brief Start of Wide Characters in Font Block */
#define BFONT_WIDE_START              (288 * BFONT_BYTES_PER_CHAR)
/** \brief Start of JISX-0208 Rows 1-7 in Font Block */
#define BFONT_JISX_0208_ROW1          BFONT_WIDE_START
/** \brief Start of JISX-0208 Row 16-47 (Start of Level 1) in Font Block */
#define BFONT_JISX_0208_ROW16         (BFONT_WIDE_START + (658 * BFONT_BYTES_PER_WIDE_CHAR))
/** \brief JISX-0208 Row 48-84 (Start of Level 2) in Font Block */
#define BFONT_JISX_0208_ROW48         (BFONT_JISX_0208_ROW16 + ((32 * JISX_0208_ROW_SIZE) * BFONT_BYTES_PER_WIDE_CHAR))

/** \brief Start of DC Specific Icons in Font Block */
#define BFONT_DREAMCAST_SPECIFIC      (BFONT_WIDE_START + (7056 * BFONT_BYTES_PER_WIDE_CHAR))

/** \brief Start of VMU Specific Icons in Font Block */
#define BFONT_VMU_DREAMCAST_SPECIFIC  (BFONT_DREAMCAST_SPECIFIC+(22 * BFONT_BYTES_PER_WIDE_CHAR))

/** \brief Builtin DC Icons

    Builtin DC user icons.
    @{
*/
typedef enum bfont_dc_icon {
    BFONT_ICON_CIRCLECOPYRIGHT = 0x00,   /**< \brief Circle copyright */
    BFONT_CIRCLER              = 0x01,   /**< \brief Circle restricted */
    BFONT_ICON_TRADEMARK       = 0x02,   /**< \brief Trademark */
    BFONT_ICON_UPARROW         = 0x03,   /**< \brief Up arrow */
    BFONT_ICON_DOWNARROW       = 0x04,   /**< \brief Down arrow */
    BFONT_ICON_LEFTARROW       = 0x05,   /**< \brief Left arrow */
    BFONT_ICON_RIGHTARROW      = 0x06,   /**< \brief Right arrow */
    BFONT_ICON_UPRIGHTARROW    = 0x07,   /**< \brief Up right arrow */
    BFONT_ICON_DOWNRIGHTARROW  = 0x08,   /**< \brief Down right arrow */
    BFONT_ICON_DOWNLEFTARROW   = 0x09,   /**< \brief Down left arrow */
    BFONT_ICON_UPLEFTARROW     = 0x0A,   /**< \brief Up left arrow */
    BFONT_ICON_ABUTTON         = 0x0B,   /**< \brief A button */
    BFONT_ICON_BBUTTON         = 0x0C,   /**< \brief B button */
    BFONT_ICON_CBUTTON         = 0x0D,   /**< \brief C button */
    BFONT_ICON_DBUTTON         = 0x0E,   /**< \brief D button */
    BFONT_ICON_XBUTTON         = 0x0F,   /**< \brief X button */
    BFONT_ICON_YBUTTON         = 0x10,   /**< \brief Y button */
    BFONT_ICON_ZBUTTON         = 0x11,   /**< \brief Z button */
    BFONT_ICON_LTRIGGER        = 0x12,   /**< \brief L trigger */
    BFONT_ICON_RTRIGGER        = 0x13,   /**< \brief R trigger */
    BFONT_ICON_STARTBUTTON     = 0x14,   /**< \brief Start button */
    BFONT_ICON_VMU             = 0x15    /**< \brief VMU icon */
} bfont_dc_icon_t;
/** @} */

/** \brief Builtin VMU Icons

    Builtin VMU volume user icons. The Dreamcast's BIOS allows the 
    user to set these when formatting the VMU.
    @{
*/
typedef enum bfont_vmu_icon {
    BFONT_ICON_INVALID_VMU     = 0x00, /**< \brief Invalid */
    BFONT_ICON_HOURGLASS_ONE   = 0x01, /**< \brief Hourglass 1 */
    BFONT_ICON_HOURGLASS_TWO   = 0x02, /**< \brief Hourglass 2 */
    BFONT_ICON_HOURGLASS_THREE = 0x03, /**< \brief Hourglass 3 */
    BFONT_ICON_HOURGLASS_FOUR  = 0x04, /**< \brief Hourglass 4 */
    BFONT_ICON_VMUICON         = 0x05, /**< \brief VMU */
    BFONT_ICON_EARTH           = 0x06, /**< \brief Earth */
    BFONT_ICON_SATURN          = 0x07, /**< \brief Saturn */
    BFONT_ICON_QUARTER_MOON    = 0x08, /**< \brief Quarter moon */
    BFONT_ICON_LAUGHING_FACE   = 0x09, /**< \brief Laughing face */
    BFONT_ICON_SMILING_FACE    = 0x0A, /**< \brief Smiling face */
    BFONT_ICON_CASUAL_FACE     = 0x0B, /**< \brief Casual face */
    BFONT_ICON_ANGRY_FACE      = 0x0C, /**< \brief Angry face */
    BFONT_ICON_COW             = 0x0D, /**< \brief Cow */
    BFONT_ICON_HORSE           = 0x0E, /**< \brief Horse */
    BFONT_ICON_RABBIT          = 0x0F, /**< \brief Rabbit */
    BFONT_ICON_CAT             = 0x10, /**< \brief Cat */
    BFONT_ICON_CHICK           = 0x11, /**< \brief Chick */
    BFONT_ICON_LION            = 0x12, /**< \brief Lion */
    BFONT_ICON_MONKEY          = 0x13, /**< \brief Monkye */
    BFONT_ICON_PANDA           = 0x14, /**< \brief Panda */
    BFONT_ICON_BEAR            = 0x15, /**< \brief Bear */
    BFONT_ICON_PIG             = 0x16, /**< \brief Pig */
    BFONT_ICON_DOG             = 0x17, /**< \brief Dog */
    BFONT_ICON_FISH            = 0x18, /**< \brief Fish */
    BFONT_ICON_OCTOPUS         = 0x19, /**< \brief Octopus */
    BFONT_ICON_SQUID           = 0x1A, /**< \brief Squid */
    BFONT_ICON_WHALE           = 0x1B, /**< \brief Whale */
    BFONT_ICON_CRAB            = 0x1C, /**< \brief Crab */
    BFONT_ICON_BUTTERFLY       = 0x1D, /**< \brief Butterfly */
    BFONT_ICON_LADYBUG         = 0x1E, /**< \brief Ladybug */
    BFONT_ICON_ANGLER_FISH     = 0x1F, /**< \brief Angler fish */
    BFONT_ICON_PENGUIN         = 0x20, /**< \brief Penguin */
    BFONT_ICON_CHERRIES        = 0x21, /**< \brief Cherries */
    BFONT_ICON_TULIP           = 0x22, /**< \brief Tulip */
    BFONT_ICON_LEAF            = 0x23, /**< \brief Leaf */
    BFONT_ICON_SAKURA          = 0x24, /**< \brief Sakura */
    BFONT_ICON_APPLE           = 0x25, /**< \brief Apple */
    BFONT_ICON_ICECREAM        = 0x26, /**< \brief Ice cream */
    BFONT_ICON_CACTUS          = 0x27, /**< \brief Cactus */
    BFONT_ICON_PIANO           = 0x28, /**< \brief Piano */
    BFONT_ICON_GUITAR          = 0x29, /**< \brief Guitar */
    BFONT_ICON_EIGHTH_NOTE     = 0x2A, /**< \brief Eighth note */
    BFONT_ICON_TREBLE_CLEF     = 0x2B, /**< \brief Treble clef */
    BFONT_ICON_BOAT            = 0x2C, /**< \brief Boat */
    BFONT_ICON_CAR             = 0x2D, /**< \brief Car */
    BFONT_ICON_HELMET          = 0x2E, /**< \brief Helmet */
    BFONT_ICON_MOTORCYCLE      = 0x2F, /**< \brief Motorcycle */
    BFONT_ICON_VAN             = 0x30, /**< \brief Van */
    BFONT_ICON_TRUCK           = 0x31, /**< \brief Truck */
    BFONT_ICON_CLOCK           = 0x32, /**< \brief Clock */
    BFONT_ICON_TELEPHONE       = 0x33, /**< \brief Telephone */
    BFONT_ICON_PENCIL          = 0x34, /**< \brief Pencil */
    BFONT_ICON_CUP             = 0x35, /**< \brief Cup */
    BFONT_ICON_SILVERWARE      = 0x36, /**< \brief Silverware */
    BFONT_ICON_HOUSE           = 0x37, /**< \brief House */
    BFONT_ICON_BELL            = 0x38, /**< \brief Bell */
    BFONT_ICON_CROWN           = 0x39, /**< \brief Crown */
    BFONT_ICON_SOCK            = 0x3A, /**< \brief Sock */
    BFONT_ICON_CAKE            = 0x3B, /**< \brief cake */
    BFONT_ICON_KEY             = 0x3C, /**< \brief Key */
    BFONT_ICON_BOOK            = 0x3D, /**< \brief Book */
    BFONT_ICON_BASEBALL        = 0x3E, /**< \brief Baseball */
    BFONT_ICON_SOCCER          = 0x3F, /**< \brief Soccer */
    BFONT_ICON_BULB            = 0x40, /**< \brief Bulb */
    BFONT_ICON_TEDDY_BEAR      = 0x41, /**< \brief Teddy bear */
    BFONT_ICON_BOW_TIE         = 0x42, /**< \brief Bow tie */
    BFONT_ICON_BOW_ARROW       = 0x43, /**< \brief Bow and arrow */
    BFONT_ICON_SNOWMAN         = 0x44, /**< \brief Snowman */
    BFONT_ICON_LIGHTNING       = 0x45, /**< \brief Lightning */
    BFONT_ICON_SUN             = 0x46, /**< \brief Sun */
    BFONT_ICON_CLOUD           = 0x47, /**< \brief Cloud */
    BFONT_ICON_UMBRELLA        = 0x48, /**< \brief Umbrella */
    BFONT_ICON_ONE_STAR        = 0x49, /**< \brief One star */
    BFONT_ICON_TWO_STARS       = 0x4A, /**< \brief Two stars */
    BFONT_ICON_THREE_STARS     = 0x4B, /**< \brief Three stars */
    BFONT_ICON_FOUR_STARS      = 0x4C, /**< \brief Four stars */
    BFONT_ICON_HEART           = 0x4D, /**< \brief Heart */
    BFONT_ICON_DIAMOND         = 0x4E, /**< \brief Diamond */
    BFONT_ICON_SPADE           = 0x4F, /**< \brief Spade */
    BFONT_ICON_CLUB            = 0x50, /**< \brief Club */
    BFONT_ICON_JACK            = 0x51, /**< \brief Jack */
    BFONT_ICON_QUEEN           = 0x52, /**< \brief Queen */
    BFONT_ICON_KING            = 0x53, /**< \brief King */
    BFONT_ICON_JOKER           = 0x54, /**< \brief Joker */
    BFONT_ICON_ISLAND          = 0x55, /**< \brief Island */
    BFONT_ICON_0               = 0x56, /**< \brief `0` digit */
    BFONT_ICON_1               = 0x57, /**< \brief `1` digit */
    BFONT_ICON_2               = 0x58, /**< \brief `2` digit */
    BFONT_ICON_3               = 0x59, /**< \brief `3` digit */
    BFONT_ICON_4               = 0x5A, /**< \brief `4` digit */
    BFONT_ICON_5               = 0x5B, /**< \brief `5` digit */
    BFONT_ICON_6               = 0x5C, /**< \brief `6` digit */
    BFONT_ICON_7               = 0x5D, /**< \brief `7` digit */
    BFONT_ICON_8               = 0x5E, /**< \brief `8` digit */
    BFONT_ICON_9               = 0x5F, /**< \brief `9` digit */
    BFONT_ICON_A               = 0x60, /**< \brief `A` letter */
    BFONT_ICON_B               = 0x61, /**< \brief `B` letter */
    BFONT_ICON_C               = 0x62, /**< \brief `C` letter */
    BFONT_ICON_D               = 0x63, /**< \brief `D` letter */
    BFONT_ICON_E               = 0x64, /**< \brief `E` letter */
    BFONT_ICON_F               = 0x65, /**< \brief `F` letter */
    BFONT_ICON_G               = 0x66, /**< \brief `G` letter */
    BFONT_ICON_H               = 0x67, /**< \brief `H` letter */
    BFONT_ICON_I               = 0x68, /**< \brief `I` letter */
    BFONT_ICON_J               = 0x69, /**< \brief `J` letter */
    BFONT_ICON_K               = 0x6A, /**< \brief `K` letter */
    BFONT_ICON_L               = 0x6B, /**< \brief `L` letter */
    BFONT_ICON_M               = 0x6C, /**< \brief `M` letter */
    BFONT_ICON_N               = 0x6D, /**< \brief `N` letter */
    BFONT_ICON_O               = 0x6E, /**< \brief `O` letter */
    BFONT_ICON_P               = 0x6F, /**< \brief `P` letter */
    BFONT_ICON_Q               = 0x70, /**< \brief `Q` letter */
    BFONT_ICON_R               = 0x71, /**< \brief `R` letter */
    BFONT_ICON_S               = 0x72, /**< \brief `S` letter */
    BFONT_ICON_T               = 0x73, /**< \brief `T` letter */
    BFONT_ICON_U               = 0x74, /**< \brief `U` letter */
    BFONT_ICON_V               = 0x75, /**< \brief `V` letter */
    BFONT_ICON_W               = 0x76, /**< \brief `W` letter */
    BFONT_ICON_X               = 0x77, /**< \brief `X` letter */
    BFONT_ICON_Y               = 0x78, /**< \brief `Y` letter */
    BFONT_ICON_Z               = 0x79, /**< \brief `Z` letter */
    BFONT_ICON_CHECKER_BOARD   = 0x7A, /**< \brief Checker board */
    BFONT_ICON_GRID            = 0x7B, /**< \brief Grid */
    BFONT_ICON_LIGHT_GRAY      = 0x7C, /**< \brief Light gray */
    BFONT_ICON_DIAG_GRID       = 0x7D, /**< \brief Diagonal grid */
    BFONT_ICON_PACMAN_GRID     = 0x7E, /**< \brief Pacman grid */
    BFONT_ICON_DARK_GRAY       = 0x7F, /**< \brief Dark gray */
    BFONT_ICON_EMBROIDERY      = 0x80  /**< \brief Embroidery */
} bfont_vmu_icon_t;
/** @} */

/** @} */

/** \name  Coloring
    \brief Methods for modifying the text color.
    @{
*/

/** \brief   Set the font foreground color.

    This function selects the foreground color to draw when a pixel is opaque in
    the font. The value passed in for the color should be in whatever pixel
    format that you intend to use for the image produced.

    \param  c               The color to use.
    \return                 The old foreground color.

    \sa bfont_set_background_color()
*/
uint32_t bfont_set_foreground_color(uint32_t c);

/** \brief   Set the font background color.

    This function selects the background color to draw when a pixel is drawn in
    the font. This color is only used for pixels not covered by the font when
    you have selected to have the font be opaque.

    \param  c               The color to use.
    \return                 The old background color.

    \sa bfont_set_foreground_color()
*/
uint32_t bfont_set_background_color(uint32_t c);

/** @} */

/* Constants for the function below */
typedef enum bfont_code {
    BFONT_CODE_ISO8859_1 = 0,   /**< \brief ISO-8859-1 (western) charset */
    BFONT_CODE_EUC       = 1,   /**< \brief EUC-JP charset */
    BFONT_CODE_SJIS      = 2,   /**< \brief Shift-JIS charset */
    BFONT_CODE_RAW       = 3   /**< \brief Raw indexing to the BFONT */
} bfont_code_t;

/** \brief   Set the font encoding.

    This function selects the font encoding that is used for the font. This
    allows you to select between the various character sets available.

    \param  enc             The character encoding in use
*/
void bfont_set_encoding(bfont_code_t enc);

/** \name Character Lookups
    \brief Methods for finding various font characters and icons.
    @{
*/

/** \brief   Find an ISO-8859-1 character in the font.

    This function retrieves a pointer to the font data for the specified
    character in the font, if its available. Generally, you will not have to
    use this function, use one of the bfont_draw_* functions instead.

    \param  ch              The character to look up
    \return                 A pointer to the raw character data
*/
const uint8_t *bfont_find_char(uint32_t ch);

/** \brief   Find an full-width Japanese character in the font.

    This function retrieves a pointer to the font data for the specified
    character in the font, if its available. Generally, you will not have to
    use this function, use one of the bfont_draw_* functions instead.

    This function deals with full-width kana and kanji.

    \param  ch              The character to look up
    \return                 A pointer to the raw character data
*/
uint8_t *bfont_find_char_jp(uint32_t ch);

/** \brief   Find an half-width Japanese character in the font.

    This function retrieves a pointer to the font data for the specified
    character in the font, if its available. Generally, you will not have to
    use this function, use one of the bfont_draw_* functions instead.

    This function deals with half-width kana only.

    \param  ch              The character to look up
    \return                 A pointer to the raw character data
*/
uint8_t *bfont_find_char_jp_half(uint32_t ch);

/** \brief   Find a VMU icon.

    This function retrieves a pointer to the icon data for the specified VMU
    icon in the bios, if its available. The icon data is flipped both vertically
    and horizontally. Each vmu icon has dimensions 32x32 pixels and is 128 bytes
    long.

    \param  icon            The VMU icon index to look up.
    \return                 A pointer to the raw icon data or NULL if icon value
                            is incorrect.
*/
uint8_t *bfont_find_icon(bfont_vmu_icon_t icon);

/** \brief   Find a DC icon.

    This function retrieves a pointer to the icon data for the specified DC
    icon in the bios, if its available. Each dc icon has dimensions 24x24 pixels 
    and is 72 bytes long.

    \param  icon            The DC icon index to look up.
    \return                 A pointer to the raw icon data or NULL if icon value
                            is incorrect.
*/
uint8_t *bfont_find_dc_icon(bfont_dc_icon_t icon);

/** @} */

/** \defgroup bfont_char Character Drawing
    \brief Methods for rendering characters/icons.
    @{
*/

/** \brief   Draw a single character of any sort to a buffer.

    This function draws a single character in the set encoding to the given
    buffer. This function sits under draw, draw_thin, and draw_wide, while
    exposing the colors and bitdepths desired. This will allow the writing
    of bfont characters to paletted textures.

    \param  buffer          The buffer to draw to
    \param  bufwidth        The width of the buffer in pixels
    \param  fg              The foreground color to use
    \param  bg              The background color to use
    \param  bpp             The number of bits per pixel in the buffer
    \param  opaque          If true, overwrite background areas with black,
                            otherwise do not change them from what they are
    \param  c               The character to draw
    \param  wide            Draw a wide character
    \param  iskana          Draw a half-width kana character
    \return                 Amount of width covered in bytes
*/
size_t bfont_draw_ex(void *buffer, uint32_t bufwidth, uint32_t fg,
                     uint32_t bg, uint8_t bpp, bool opaque, uint32_t c,
                     bool wide, bool iskana);

/** \brief   Draw a single character to a buffer.

    This function draws a single character in the set encoding to the given
    buffer. Calling this is equivalent to calling bfont_draw_thin() with 0 for
    the final parameter.

    \param  buffer          The buffer to draw to (at least 12 x 24 pixels)
    \param  bufwidth        The width of the buffer in pixels
    \param  opaque          If true, overwrite blank areas with black,
                            otherwise do not change them from what they are
    \param  c               The character to draw
    \return                 Amount of width covered in bytes.
*/
size_t bfont_draw(void *buffer, uint32_t bufwidth, bool opaque, uint32_t c);

/** \brief   Draw a single thin character to a buffer.

    This function draws a single character in the set encoding to the given
    buffer. This only works with ISO-8859-1 characters and half-width kana.

    \param  buffer          The buffer to draw to (at least 12 x 24 pixels)
    \param  bufwidth        The width of the buffer in pixels
    \param  opaque          If true, overwrite blank areas with black,
                            otherwise do not change them from what they are
    \param  c               The character to draw
    \param  iskana          Set to 1 if the character is a kana, 0 if ISO-8859-1
    \return                 Amount of width covered in bytes.
*/
size_t bfont_draw_thin(void *buffer, uint32_t bufwidth, bool opaque,
                       uint32_t c, bool iskana);

/** \brief   Draw a single wide character to a buffer.

    This function draws a single character in the set encoding to the given
    buffer. This only works with full-width kana and kanji.

    \param  buffer          The buffer to draw to (at least 24 x 24 pixels)
    \param  bufwidth        The width of the buffer in pixels
    \param  opaque          If true, overwrite blank areas with black,
                            otherwise do not change them from what they are
    \param  c               The character to draw
    \return                 Amount of width covered in bytes.
*/
size_t bfont_draw_wide(void *buffer, uint32_t bufwidth, bool opaque,
                       uint32_t c);

/** \brief   Draw a VMU icon to the buffer.

    This function draws a 32x32 VMU icon to the given buffer, supporting 
    multiple color depths (4, 8, 16, and 32 bits per pixel).

    \param  buffer          The buffer to draw to (at least 32 x 32 pixels)
    \param  bufwidth        The width of the buffer in pixels
    \param  fg              The foreground color to use
    \param  bg              The background color to use
    \param  bpp             The number of bits per pixel in the buffer
    \param  opaque          If true, overwrite background areas with black,
                            otherwise do not change them from what they are
    \param  icon            The VMU icon to draw
    \return                 Amount of width covered in bytes
*/
size_t bfont_draw_vmu_icon_ex(void *buffer, uint32_t bufwidth, uint32_t fg,
                              uint32_t bg, uint8_t bpp, bool opaque, 
                              bfont_vmu_icon_t icon);

/** \brief   Draw a VMU icon to a buffer.

    This function draws a 32x32 VMU icon to the given buffer.

    \param  buffer          The buffer to draw to (at least 32 x 32 pixels)
    \param  bufwidth        The width of the buffer in pixels
    \param  opaque          If true, overwrite blank areas with black,
                            otherwise do not change them from what they are
    \param  icon            The VMU icon to draw
    \return                 Amount of width covered in bytes.
*/
size_t bfont_draw_vmu_icon(void *buffer, uint32_t bufwidth, bool opaque, 
                           bfont_vmu_icon_t icon);

/** \brief   Draw a DC icon to the buffer.

    This function draws a 24x24 DC icon to the given buffer, supporting 
    multiple color depths (4, 8, 16, and 32 bits per pixel).

    \param  buffer          The buffer to draw to (at least 24 x 24 pixels)
    \param  bufwidth        The width of the buffer in pixels
    \param  fg              The foreground color to use
    \param  bg              The background color to use
    \param  bpp             The number of bits per pixel in the buffer
    \param  opaque          If true, overwrite background areas with black,
                            otherwise do not change them from what they are
    \param  icon            The DC icon to draw
    \return                 Amount of width covered in bytes
*/
size_t bfont_draw_dc_icon_ex(void *buffer, uint32_t bufwidth, uint32_t fg,
                             uint32_t bg, uint8_t bpp, bool opaque, 
                             bfont_dc_icon_t icon);

/** \brief   Draw a DC icon to a buffer.

    This function draws a 24x24 DC icon to the given buffer.

    \param  buffer          The buffer to draw to (at least 24 x 24 pixels)
    \param  bufwidth        The width of the buffer in pixels
    \param  opaque          If true, overwrite blank areas with black,
                            otherwise do not change them from what they are
    \param  icon            The DC icon to draw
    \return                 Amount of width covered in bytes.
*/
size_t bfont_draw_dc_icon(void *buffer, uint32_t bufwidth, bool opaque, 
                          bfont_dc_icon_t icon);

/** @} */

/** \defgroup bfont_string String Drawing
    \brief    Methods for rendering formatted text/icons.

    @{
*/

/** brief Macros for Builtin DC Icons

    These macros are provided for use in string concatenation for printf
    and other functions requiring inline representation of DC icons.
    @{
*/
#define DI_CIRCLECOPYRIGHT     "\\di00"
#define DI_CIRCLER             "\\di01"
#define DI_TRADEMARK           "\\di02"
#define DI_UPARROW             "\\di03"
#define DI_DOWNARROW           "\\di04"
#define DI_LEFTARROW           "\\di05"
#define DI_RIGHTARROW          "\\di06"
#define DI_UPRIGHTARROW        "\\di07"
#define DI_DOWNRIGHTARROW      "\\di08"
#define DI_DOWNLEFTARROW       "\\di09"
#define DI_UPLEFTARROW         "\\di0A"
#define DI_ABUTTON             "\\di0B"
#define DI_BBUTTON             "\\di0C"
#define DI_CBUTTON             "\\di0D"
#define DI_DBUTTON             "\\di0E"
#define DI_XBUTTON             "\\di0F"
#define DI_YBUTTON             "\\di10"
#define DI_ZBUTTON             "\\di11"
#define DI_LTRIGGER            "\\di12"
#define DI_RTRIGGER            "\\di13"
#define DI_STARTBUTTON         "\\di14"
#define DI_VMU                 "\\di15"
/** @} */

/** brief Macros for Builtin VMU Icons

    These macros are provided for use in string concatenation for printf
    and other functions requiring inline representation of VMU icons.
    @{
*/
#define VI_INVALID_VMU         "\\vi00"
#define VI_HOURGLASS_ONE       "\\vi01"
#define VI_HOURGLASS_TWO       "\\vi02"
#define VI_HOURGLASS_THREE     "\\vi03"
#define VI_HOURGLASS_FOUR      "\\vi04"
#define VI_VMUICON             "\\vi05"
#define VI_EARTH               "\\vi06"
#define VI_SATURN              "\\vi07"
#define VI_QUARTER_MOON        "\\vi08"
#define VI_LAUGHING_FACE       "\\vi09"
#define VI_SMILING_FACE        "\\vi0A"
#define VI_CASUAL_FACE         "\\vi0B"
#define VI_ANGRY_FACE          "\\vi0C"
#define VI_COW                 "\\vi0D"
#define VI_HORSE               "\\vi0E"
#define VI_RABBIT              "\\vi0F"
#define VI_CAT                 "\\vi10"
#define VI_CHICK               "\\vi11"
#define VI_LION                "\\vi12"
#define VI_MONKEY              "\\vi13"
#define VI_PANDA               "\\vi14"
#define VI_BEAR                "\\vi15"
#define VI_PIG                 "\\vi16"
#define VI_DOG                 "\\vi17"
#define VI_FISH                "\\vi18"
#define VI_OCTOPUS             "\\vi19"
#define VI_SQUID               "\\vi1A"
#define VI_WHALE               "\\vi1B"
#define VI_CRAB                "\\vi1C"
#define VI_BUTTERFLY           "\\vi1D"
#define VI_LADYBUG             "\\vi1E"
#define VI_ANGLER_FISH         "\\vi1F"
#define VI_PENGUIN             "\\vi20"
#define VI_CHERRIES            "\\vi21"
#define VI_TULIP               "\\vi22"
#define VI_LEAF                "\\vi23"
#define VI_SAKURA              "\\vi24"
#define VI_APPLE               "\\vi25"
#define VI_ICECREAM            "\\vi26"
#define VI_CACTUS              "\\vi27"
#define VI_PIANO               "\\vi28"
#define VI_GUITAR              "\\vi29"
#define VI_EIGHTH_NOTE         "\\vi2A"
#define VI_TREBLE_CLEF         "\\vi2B"
#define VI_BOAT                "\\vi2C"
#define VI_CAR                 "\\vi2D"
#define VI_HELMET              "\\vi2E"
#define VI_MOTORCYCLE          "\\vi2F"
#define VI_VAN                 "\\vi30"
#define VI_TRUCK               "\\vi31"
#define VI_CLOCK               "\\vi32"
#define VI_TELEPHONE           "\\vi33"
#define VI_PENCIL              "\\vi34"
#define VI_CUP                 "\\vi35"
#define VI_SILVERWARE          "\\vi36"
#define VI_HOUSE               "\\vi37"
#define VI_BELL                "\\vi38"
#define VI_CROWN               "\\vi39"
#define VI_SOCK                "\\vi3A"
#define VI_CAKE                "\\vi3B"
#define VI_KEY                 "\\vi3C"
#define VI_BOOK                "\\vi3D"
#define VI_BASEBALL            "\\vi3E"
#define VI_SOCCER              "\\vi3F"
#define VI_BULB                "\\vi40"
#define VI_TEDDY_BEAR          "\\vi41"
#define VI_BOW_TIE             "\\vi42"
#define VI_BOW_ARROW           "\\vi43"
#define VI_SNOWMAN             "\\vi44"
#define VI_LIGHTNING           "\\vi45"
#define VI_SUN                 "\\vi46"
#define VI_CLOUD               "\\vi47"
#define VI_UMBRELLA            "\\vi48"
#define VI_ONE_STAR            "\\vi49"
#define VI_TWO_STARS           "\\vi4A"
#define VI_THREE_STARS         "\\vi4B"
#define VI_FOUR_STARS          "\\vi4C"
#define VI_HEART               "\\vi4D"
#define VI_DIAMOND             "\\vi4E"
#define VI_SPADE               "\\vi4F"
#define VI_CLUB                "\\vi50"
#define VI_JACK                "\\vi51"
#define VI_QUEEN               "\\vi52"
#define VI_KING                "\\vi53"
#define VI_JOKER               "\\vi54"
#define VI_ISLAND              "\\vi55"
#define VI_0                   "\\vi56"
#define VI_1                   "\\vi57"
#define VI_2                   "\\vi58"
#define VI_3                   "\\vi59"
#define VI_4                   "\\vi5A"
#define VI_5                   "\\vi5B"
#define VI_6                   "\\vi5C"
#define VI_7                   "\\vi5D"
#define VI_8                   "\\vi5E"
#define VI_9                   "\\vi5F"
#define VI_A                   "\\vi60"
#define VI_B                   "\\vi61"
#define VI_C                   "\\vi62"
#define VI_D                   "\\vi63"
#define VI_E                   "\\vi64"
#define VI_F                   "\\vi65"
#define VI_G                   "\\vi66"
#define VI_H                   "\\vi67"
#define VI_I                   "\\vi68"
#define VI_J                   "\\vi69"
#define VI_K                   "\\vi6A"
#define VI_L                   "\\vi6B"
#define VI_M                   "\\vi6C"
#define VI_N                   "\\vi6D"
#define VI_O                   "\\vi6E"
#define VI_P                   "\\vi6F"
#define VI_Q                   "\\vi70"
#define VI_R                   "\\vi71"
#define VI_S                   "\\vi72"
#define VI_T                   "\\vi73"
#define VI_U                   "\\vi74"
#define VI_V                   "\\vi75"
#define VI_W                   "\\vi76"
#define VI_X                   "\\vi77"
#define VI_Y                   "\\vi78"
#define VI_Z                   "\\vi79"
#define VI_CHECKER_BOARD       "\\vi7A"
#define VI_GRID                "\\vi7B"
#define VI_LIGHT_GRAY          "\\vi7C"
#define VI_DIAG_GRID           "\\vi7D"
#define VI_PACMAN_GRID         "\\vi7E"
#define VI_DARK_GRAY           "\\vi7F"
#define VI_EMBROIDERY          "\\vi80"
/** @} */

/** \brief   Draw a full string of any sort to any sort of buffer.

    This function draws a NUL-terminated string in the set encoding to the given
    buffer. This will automatically handle mixed half and full-width characters
    if the encoding is set to one of the Japanese encodings. Colors and bitdepth
    can be set.

    \param  buffer          The buffer to draw to.
    \param  width           The width of the buffer in pixels.
    \param  fg              The foreground color to use.
    \param  bg              The background color to use.
    \param  bpp             The number of bits per pixel in the buffer.
    \param  opaque          If true, overwrite background areas with black,
                            otherwise do not change them from what they are.
    \param str              The string to draw.

    \sa bfont_draw_str_ex_fmt(), bfont_draw_str_ex_va()
*/
void bfont_draw_str_ex(void *buffer, uint32_t width, uint32_t fg, uint32_t bg,
                       uint8_t bpp, bool opaque, const char *str);

/** \brief   Draw a full formatted string of any sort to any sort of buffer.

    This function is equivalent to bfont_draw_str_ex(), except that the string
    is formatted as with the `printf()` function.

    \param  buffer          The buffer to draw to.
    \param  width           The width of the buffer in pixels.
    \param  fg              The foreground color to use.
    \param  bg              The background color to use.
    \param  bpp             The number of bits per pixel in the buffer.
    \param  opaque          If true, overwrite background areas with black,
                            otherwise do not change them from what they are.
    \param  fmt             The printf-style format string to draw.
    \param  ...             Additional printf-style variadic arguments

    \sa bfont_draw_str_ex_vfmt()
*/
void bfont_draw_str_ex_fmt(void *buffer, uint32_t width, uint32_t fg, uint32_t bg,
                           uint8_t bpp, bool opaque, const char *fmt, ...)
                           __printflike(7, 8);

/** \brief   Draw formatted string of any sort to buffer (with va_args).

    This function is equivalent to bfont_draw_str_ex_fmt(), except that the
    variadic argument list is passed via a pointer to a va_list.

    \param  buffer          The buffer to draw to.
    \param  width           The width of the buffer in pixels.
    \param  fg              The foreground color to use.
    \param  bg              The background color to use.
    \param  bpp             The number of bits per pixel in the buffer.
    \param  opaque          If true, overwrite background areas with black,
                            otherwise do not change them from what they are.
    \param  fmt             The printf-style format string to draw.
    \param  var_args        Additional printf-style variadic arguments

    \sa bfont_draw_str_ex_fmt()
*/
void bfont_draw_str_ex_vfmt(void *buffer, uint32_t width, uint32_t fg, uint32_t bg,
                            uint8_t bpp, bool opaque, const char *fmt,
                            va_list *var_args);

/** \brief   Draw a full string to a buffer.

    This function draws a NUL-terminated string in the set encoding to the given
    buffer. This will automatically handle mixed half and full-width characters
    if the encoding is set to one of the Japanese encodings. Draws pre-set
    16-bit colors.

    \param  buffer          The buffer to draw to.
    \param  width           The width of the buffer in pixels.
    \param  opaque          If true, overwrite blank areas with bfont_bgcolor,
                            otherwise do not change them from what they are.
    \param  str             The string to draw.
*/
void bfont_draw_str(void *buffer, uint32_t width, bool opaque, const char *str);

/** \brief   Draw a full formatted string to a buffer.

    This function is equvalent to bfont_draw_str(), except that the string is
    formatted as with the `printf()` function.

    \param  buffer          The buffer to draw to.
    \param  width           The width of the buffer in pixels.
    \param  opaque          If true, overwrite blank areas with bfont_bgcolor,
                            otherwise do not change them from what they are.
    \param  fmt             The printf-style format string to draw.
    \param  ...             Additional printf-style variadic arguments.
*/
void bfont_draw_str_fmt(void *buffer, uint32_t width, bool opaque, const char *fmt,
                        ...) __printflike(4, 5);

/** \brief   Draw a full formatted string to video ram (with va_args).

    This function is equivalent to bfont_draw_str_ex_vfmt(), except that
    the variadic argument list is passed via a pointer to a va_list.

    \param  x               The x position to start drawing at.
    \param  y               The y position to start drawing at.
    \param  fg              The foreground color to use.
    \param  bg              The background color to use.
    \param  opaque          If true, overwrite background areas with black,
                            otherwise do not change them from what they are.
    \param  fmt             The printf-style format string to draw.
    \param  var_args        Additional printf-style variadic arguments

    \sa bfont_draw_str_ex()
*/
void bfont_draw_str_vram_vfmt(uint32_t x, uint32_t y, uint32_t fg, uint32_t bg,
                              bool opaque, const char *fmt,
                              va_list *var_args);

/** \brief   Draw a full string to video ram.

    This function draws a NUL-terminated string in the set encoding to video
    ram. This will automatically handle mixed half and full-width characters
    if the encoding is set to one of the Japanese encodings. Draws pre-set
    16-bit colors.

    \param  x               The x position to start drawing at.
    \param  y               The y position to start drawing at.
    \param  opaque          If true, overwrite blank areas with bfont_bgcolor,
                            otherwise do not change them from what they are.
    \param  fmt             The printf-style format string to draw.
    \param  ...             Additional printf-style variadic arguments.
*/
void bfont_draw_str_vram_fmt(uint32_t x, uint32_t y, bool opaque, const char *fmt,
                             ...) __printflike(4, 5);

/** @} */

/** @} */

__END_DECLS

#endif  /* __DC_BIOSFONT_H */
