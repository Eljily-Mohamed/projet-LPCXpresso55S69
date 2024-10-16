/*
 * (C) 2023, E Boucharé
 */

#ifndef _LCD_H_
#define _LCD_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* defines for lcd_switch_to */
#define	LCD_DPY			0
#define LCD_TS			1
#define LCD_SD			2

/* Screen orientation */
typedef enum {
	PORTRAIT,
	LANDSCAPE,
	PORTRAIT_INV,
	LANDSCAPE_INV
} Orientation;

/* TouchScreen state */
#define TS_DOWN						0
#define TS_UP						1

/* Exported types ------------------------------------------------------------*/
typedef	uint16_t	Colour;
typedef	uint16_t	Color;

typedef struct Point {
	int				x;
	int				y;
} Point;

typedef struct _SPoint {
	short	x, y;
} SPoint;

typedef	struct Rect {
	int				x;			/* left-most pixel inside rectangle */
	int				y;			/* top-most pixel inside rectangle */
	int				width;		/* in pixels */
	int				height;		/* in pixels */
} Rect;

typedef struct Font {	/* monospace font descriptor */
  const uint8_t *	data;		/* char data */
  uint16_t			width;		/* char width */
  uint16_t			height;		/* char height */
} Font;

typedef struct DC {
	Rect			clip;		/* clipping rectangle */
	Color			bcolor;		/* background color */
	Color			fcolor;		/* foreground color */
	const Font *	font;		/* font */
	uint32_t		tflags;		/* text flags: direction and alignment */
	uint32_t		linewidth;	/* linewidth */
} DC;	/* Drawing Context */

/* text flags: direction */ 
#define DIR_HORIZONTAL			(1<<0)
#define DIR_VERTICAL			(1<<1)
#define DIR_HORIZONTAL_INV		(1<<2)
#define DIR_VERTICAL_INV		(1<<3)

#define DIR_MSK					0x0000000F

/* text flags: alignment relative to coordinate */
#define ALIGN_NE				(1<<4)
#define ALIGN_N					(1<<5)
#define ALIGN_NW				(1<<6)
#define ALIGN_W					(1<<7)
#define ALIGN_SW				(1<<8)
#define ALIGN_S					(1<<9)
#define ALIGN_SE				(1<<10)
#define ALIGN_E					(1<<11)
#define ALIGN_LEFT				ALIGN_E
#define ALIGN_RIGHT				ALIGN_W
#define ALIGN_CENTER			(1<<12)

#define ALIGN_MSK				0x00001FF0


/* Exported constants --------------------------------------------------------*/
extern const Font fixed24;
extern const Font fixed20;
extern const Font fixed16;
extern const Font fixed12;
extern const Font fixed8;

/* Exported macro ------------------------------------------------------------*/

#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

#define LCD_WIDTH		240
#define LCD_HEIGHT		320

#define WHITE			0xFFFF
#define BLACK			0x0000	  
#define BLUE			0x001F  
#define BRED			0XF81F
#define GRED			0xFFE0
#define GBLUE			0x07FF
#define RED				0xF800
#define MAGENTA			0xF81F
#define GREEN			0x07E0
#define CYAN			0x7FFF
#define YELLOW			0xFFE0
#define BROWN			0xBC40 
#define BRRED			0xFC07 
#define GRAY			0x8430 

#define RGB(r,g,b)		((Colour)((r & 0xF8)<<8 | (g & 0xFC)<<3 | (b & 0xF8)>>3))

/* Exported functions ------------------------------------------------------- */
void lcd_ts_init(void);
void lcd_init(void);
void lcd_switch_to(uint32_t cfg);
void lcd_redraw(void);

/* Display management functions */
void lcd_set_display_orientation(Orientation orientation);
uint16_t lcd_get_display_width(void);
uint16_t lcd_get_display_height(void);

/* touchscreen API functions */
void    lcd_ts_id(uint8_t *ver, uint16_t *id);
uint8_t lcd_ts_get_data(uint16_t *x, uint16_t *y, uint8_t *z);
int     lcd_ts_touched(void);

/* Drawing context management functions */
void lcd_get_default_DC(DC *dc);

const Font* lcd_set_font(DC *dc, const Font *font);
Color lcd_set_foreground(DC *dc, Color color);
Color lcd_set_background(DC *dc, Color color);
uint32_t lcd_set_alignment(DC *dc, uint32_t alignment);
uint32_t lcd_set_direction(DC *dc, uint32_t direction);

/* graphic drawing functions */
void lcd_clear_screen(uint16_t color);

void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

void lcd_draw_point(uint16_t x, uint16_t y, Color c);
void lcd_draw_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, Color c);
void lcd_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, Color c);
void lcd_draw_round_rect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, Color c);
void lcd_draw_circle(int16_t x0, int16_t y0, int16_t r, Color c);
void lcd_draw_ellipse(Rect r, Color c, int lwidth);
void lcd_draw_segments(SPoint *p, int n, Color c);

void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/* draw lines with clipping */
void lcd_clip(int x1, int y1, int x2, int y2);
void lcd_unclip(void);
void lcd_line(int x1, int y1, int x2, int y2, Color c);

/* String drawing functions */
uint16_t lcd_draw_char(DC *dc, int16_t x, int16_t y, char c);
void lcd_draw_string(DC *dc, int16_t x, int16_t y, const char *s);

void lcd_get_string_size(DC *dc, const char *s, uint16_t *width, uint16_t *height);

#ifdef __cplusplus
}
#endif
#endif
/*-------------------------------END OF FILE-------------------------------*/

