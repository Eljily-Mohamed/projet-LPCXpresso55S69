/*
 * (C) 2023, E Boucharé
 */

/* Includes ------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "lcd.h"
#include "lcd_private.h"
#include "ili9341.h"

// to add extra debug, uncomment this line
//#define LCD_DEBUG

void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
	uint8_t caset[4];
	uint8_t paset[4];
	caset[0] = (uint8_t)(x0 >> 8);
	caset[1] = (uint8_t)(x0 & 0xff);
	caset[2] = (uint8_t)(x1 >> 8);
	caset[3] = (uint8_t)(x1 & 0xff);
	paset[0] = (uint8_t)(y0 >> 8);
	paset[1] = (uint8_t)(y0 & 0xff);
	paset[2] = (uint8_t)(y1 >> 8);
	paset[3] = (uint8_t)(y1 & 0xff);

	LCD_DPY_CMD;
	spi_write_byte(LCD_SPI, ILI9341_CASET);		// Column addr set
	LCD_DPY_DATA;
	spi_write(LCD_SPI, caset, 4);

	LCD_DPY_CMD;
	spi_write_byte(LCD_SPI, ILI9341_PASET);		// Row addr set
	LCD_DPY_DATA;
	spi_write(LCD_SPI, paset, 4);

	LCD_DPY_CMD;
	spi_write_byte(LCD_SPI, ILI9341_RAMWR);		// write to RAM
	LCD_DPY_DATA;
}

//clear the lcd with the specified color.
void lcd_clear_screen(uint16_t color)  
{
	lcd_fill_rect(0,0,disp.width,disp.height,color);
}

//draw a point on the lcd with the specified color.
//hwXpos specify x position.
//hwYpos specify y position.
//hwColor color of the point.
void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color) 
{
	if (x >= disp.width || y >= disp.height) {
		return;
	}
	LCD_DPY_EN;
	lcd_set_window(x,y,x,y);
	spi_write16_n(LCD_SPI, color, 1);
	LCD_DPY_DIS;
}

/****************************************************************
 * Display management
 ****************************************************************/
/*      mode     | MY MX MV ML BGR MH  0  0 |  hex
   PORTRAIT      |  0  1  0  0   1  0  0  0 | 0x48
   LANDSCAPE     |  0  0  1  0   1  0  0  0 | 0x28 
   PORTRAIT_INV  |  1  0  0  0   1  0  0  0 | 0x88
   LANDSCAPE_INV |  1  1  1  0   1  0  0  0 | 0xE8
 */
const uint8_t disp_modes[] = {0x48, 0x28, 0x88, 0xE8};


void lcd_set_display_orientation(Orientation orientation)
{
	if (disp.orientation == orientation) return;
	
	disp.orientation = orientation;
	/* change orientation on display controller */
	LCD_DPY_EN;
	LCD_DPY_CMD;
	spi_write_byte(LCD_SPI, ILI9341_MADCTL);		// set Memory Access Control
	LCD_DPY_DATA;
	spi_write_byte(LCD_SPI, disp_modes[orientation]);
	
	switch (orientation) {
	case PORTRAIT:
	case PORTRAIT_INV:
		disp.width  = LCD_WIDTH;
		disp.height = LCD_HEIGHT;
		break;
	case LANDSCAPE:
	case LANDSCAPE_INV:
		disp.width  = LCD_HEIGHT;
		disp.height = LCD_WIDTH;
		break;
	}
	lcd_set_window(0,0,disp.width-1,disp.height-1);
	
	/* send redraw event */
//	lcd_redraw();
	
	LCD_DPY_DIS;
}

uint16_t lcd_get_display_width(void)
{
	return disp.width;
}

uint16_t lcd_get_display_height(void)
{
	return disp.height;
}

/****************************************************************
 * Drawing context (DC) management
 ****************************************************************/
/* get a copy of the default drawing context */
void lcd_get_default_DC(DC *dc)
{
	*dc = disp.dc;
}

const Font* lcd_set_font(DC *dc, const Font *font)
{
	const Font *f=dc->font;
	dc->font = font;
	
	return f;
}

Color lcd_set_foreground(DC *dc, Color color)
{
	Color c = dc->fcolor;
	dc->fcolor = color;
	return c;
}

Color lcd_set_background(DC *dc, Color color)
{
	Color c = dc->bcolor;
	dc->bcolor = color;
	return c;
}

uint32_t lcd_set_alignment(DC *dc, uint32_t alignment)
{
	uint32_t a = dc->tflags & ALIGN_MSK;
	dc->tflags = (dc->tflags & ~ALIGN_MSK) | alignment;
	return a;
}

uint32_t lcd_set_direction(DC *dc, uint32_t direction)
{
	uint32_t d = dc->tflags & DIR_MSK;
	dc->tflags = (dc->tflags & ~DIR_MSK) | direction;
	return d;
}

/****************************************************************
 * Graphic functions
 *  drawing routines adapted from GraphApp library
 ****************************************************************/

void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
  /*! \todo: should change endianess in Interface control (0xF6)? */
//	color = (color>>8)|(color<<8); /* swap */
	if (!w) w=1;
	if (!h) h=1;
	LCD_DPY_EN;
	lcd_set_window(x, y, x+w-1, y+h-1);
	spi_write16_n(LCD_SPI, color, w*h);
	LCD_DPY_DIS;
}


void lcd_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
	if (!w) w=1;
	if (!h) h=1;
	LCD_DPY_EN;
	lcd_set_window(x, y, x+w-1, y);
	spi_write16_n(LCD_SPI, color, w);
	lcd_set_window(x, y+h-1, x+w-1, y+h-1);
	spi_write16_n(LCD_SPI, color, w);
	lcd_set_window(x, y+1, x, y+h-1);
	spi_write16_n(LCD_SPI, color, h-1);
	lcd_set_window(x+w-1, y+1, x+w-1, y+h-1);
	spi_write16_n(LCD_SPI, color, h-1);
	LCD_DPY_DIS;
}
/*
 *  Run-length slice line drawing:
 *
 *  This is based on Bresenham's line-slicing algorithm, which is
 *  faster than the traditional Bresenham's line drawing algorithm,
 *  and better suited for drawing filled rectangles instead of
 *  individual pixels.
 *
 *  It essentially reverses the ordinary Bresenham's logic;
 *  instead of keeping an error term which counts along the
 *  direction of travel (the major axis), it keeps an error
 *  term perpendicular to the major axis, to determine when
 *  to step to the next run of pixels.
 *
 *  See Michael Abrash's Graphics Programming Black Book on-line
 *  at http://www.ddj.com/articles/2001/0165/0165f/0165f.htm
 *  chapter 36 (and 35 and 37) for more details.
 *
 *  The algorithm can also draw lines with a thickness greater
 *  than 1 pixel. In that case, the line hangs below and to
 *  the right of the end points.
 */
void lcd_draw_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, Color c)
//Graphics *g, Point p1, Point p2)
{
	int x, y, width, height;
	int temp, adj_up, adj_down, error_term, xadvance, dx, dy;
	int whole_step, initial_run, final_run, i, run_length;
	int w = 1;			// linewidth

	/* Figure out whether we're going left or right, and how
	 * far we're going horizontally */

	if ((dx = x2 - x1) < 0) {
		xadvance = -1;
		dx = -dx;
	} else {
		xadvance = 1;
	}

	/* We'll always draw top to bottom, to reduce the number of
	 * cases we have to handle */

	if ((dy = y2 - y1) < 0) {
		temp = y1;
		y1 = y2;
		y2 = temp;
		temp = x1;
		x1 = x2;
		x2 = temp;
		xadvance = -xadvance;
		dy = -dy;
	}

	/* Special-case horizontal, vertical, and diagonal lines,
	 * for speed and to avoid nasty boundary conditions and
	 * division by 0 */

	if (dx == 0) {						/* Vertical line */
		lcd_fill_rect(x1,y1,w,dy+1,c);
	} else if (dy == 0) {				/* Horizontal line */
		lcd_fill_rect(MIN(x1,x2),y1,dx+1,w,c);
	} else if (dx == dy) {				/* Diagonal line */
		x=x1, y=y1;
		for (i=0; i < dx+1; i++) {
			lcd_fill_rect(x,y,w,w,c);
			x += xadvance;
			y++;
		}
	} else if (dx >= dy) {
	/* Determine whether the line is more horizontal or vertical,
	 * and handle accordingly */

		/* More horizontal than vertical */

		if (xadvance < 0)
			x1++, x2++;

		/* Minimum # of pixels in a run in this line */
		whole_step = dx / dy;

		/* Error term adjust each time Y steps by 1; used to
		 * tell when one extra pixel should be drawn as part
		 * of a run, to account for fractional steps along
		 * the X axis per 1-pixel steps along Y */
		adj_up = (dx % dy) * 2;

		/* Error term adjust when the error term turns over,
		 * used to factor out the X step made at that time */
		adj_down = dy * 2;

		/* Initial error term; reflects an initial step of 0.5
		 * along the Y axis */
		error_term = (dx % dy) - (dy * 2);

		/* The initial and last runs are partial, because Y
		 * advances only 0.5 for these runs, rather than 1.
		 * Divide one full run, plus the initial pixel, between
		 * the initial and last runs */
		initial_run = (whole_step / 2) + 1;
		final_run = initial_run;

		/* If the basic run length is even and there's no
		 * fractional advance, we have one pixel that could
		 * go to either the initial or last partial run, which
		 * we'll arbitrarily allocate to the last run */
		if ((adj_up == 0) && ((whole_step & 0x01) == 0)) {
			initial_run--;
		}

		/* If there're an odd number of pixels per run, we
		 * have 1 pixel that can't be allocated to either
		 * the initial or last partial run, so we'll add 0.5
		 * to error term so this pixel will be handled by
		 * the normal full-run loop */
		if ((whole_step & 0x01) != 0) {
			error_term += dy;
		}

		/* Draw the first, partial run of pixels */
		x = x1;
		y = y1;
		height = w;
		width = initial_run;
		if (xadvance < 0) {
			x -= width;
			lcd_fill_rect(x,y,width,height,c);
		} else {
			lcd_fill_rect(x,y,width,height,c);
			x += width;
		}
		y ++;

		/* Draw all full runs */
		for (i=1; i < dy; i++) {
			run_length = whole_step;  /* at least */

			/* Advance the error term and add an extra
			 * pixel if the error term so indicates */
			if ((error_term += adj_up) > 0) {
				run_length++;
				error_term -= adj_down;   /* reset */
			}

			/* Draw this scan line's run */
			width = run_length;
			if (xadvance < 0) {
				x -= width;
				lcd_fill_rect(x,y,width,height,c);
			} else {
				lcd_fill_rect(x,y,width,height,c);
				x += width;
			}
			y ++;
		}
		/* Draw the final run of pixels */
		width = final_run;
		if (xadvance < 0)
			x -= width;
		lcd_fill_rect(x,y,width,height,c);
	} else {
		/* More vertical than horizontal */

		/* Minimum # of pixels in a run in this line */
		whole_step = dy / dx;

		/* Error term adjust each time X steps by 1; used to
		 * tell when 1 extra pixel should be drawn as part of
		 * a run, to account for fractional steps along the
		 * Y axis per 1-pixel steps along X */
		adj_up = (dy % dx) * 2;

		/* Error term adjust when the error term turns over,
		 * used to factor out the Y step made at that time */
		adj_down = dx * 2;

		/* Initial error term; reflects initial step of 0.5
		 * along the X axis */
		error_term = (dy % dx) - (dx * 2);

		/* The initial and last runs are partial, because
		 * X advances only 0.5 for these runs, rather than 1.
		 * Divide one full run, plus the initial pixel,
		 * between the initial and last runs */
		initial_run = (whole_step / 2) + 1;
		final_run = initial_run;

		/* If the basic run length is even and there's no
		 * fractional advance, we have 1 pixel that could
		 * go to either the initial or last partial run,
		 * which we'll arbitrarily allocate to the last run */
		if ((adj_up == 0) && ((whole_step & 0x01) == 0)) {
			initial_run--;
		}

		/* If there are an odd number of pixels per run, we
		 * have one pixel that can't be allocated to either
		 * the initial or last partial run, so we'll add 0.5
		 * to the error term so this pixel will be handled
		 * by the normal full-run loop */
		if ((whole_step & 0x01) != 0) {
			error_term += dx;
		}

		/* Draw the first, partial run of pixels */
		x = x1;
		y = y1;
		height = initial_run;
		width = w;
		lcd_fill_rect(x,y,width,height,c);
		x += xadvance;
		y += height;

		/* Draw all full runs */
		for (i=1; i < dx; i++) {
			run_length = whole_step;  /* at least */

			/* Advance the error term and add an extra
			 * pixel if the error term so indicates */
			if ((error_term += adj_up) > 0) {
				run_length++;
				error_term -= adj_down;   /* reset */
			}

			/* Draw this scan line's run */
			height = run_length;
			lcd_fill_rect(x,y,width,height,c);
			x += xadvance;
			y += height;
		}
		/* Draw the final run of pixels */
		height = final_run;
		lcd_fill_rect(x,y,width,height,c);
	}
}

/* Cohen-Sutherland clipping algorithm
 *
 *  tbrl (top bottom right left)
 *
 *             xlc        xrc
 *       1001   |   1000   |   1010
 *              |          |
 *   ytc -------+----------+-------
 *              | cliprect |
 *       0001   |   0000   |   0010
 *              |          |
 *   ybc -------+----------+-------
 *              |          |
 *       0101   |   0100   |   0110
 */
 
#define INSIDE		0
#define	LEFT		1
#define RIGHT		2
#define	BOTTOM		4
#define TOP			8

static int xlc=0;		// x left clip
static int ytc=0;		// y top clip
static int xrc=0;		// x right clip
static int ybc=0;		// y bottom clip
static int clipped=0;		// clippiing active

/* clip: set the clipping rectangle
 *   win     : window
 *   x1, y1  : top left point coordinates
 *   x2, y2  : bottom right point coordinates
 */
void lcd_clip(int x1, int y1, int x2, int y2)
{
	xlc = x1;
	ytc = y1;
	xrc = x2;
	ybc = y2;
	clipped=1;
//	lcd_draw_rect(xlc,ytc,xrc-xlc,ybc-ytc,BLACK);
}

void lcd_unclip(void)
{
	clipped=0;
}

/* rgn_code: returns the region code */
static int rgn_code(int x, int y)
{
	int code = INSIDE;
	
	if (x < xlc) {				// to the left of rectangle
		code |= LEFT;
	} else if (x > xrc) {		// to the right of rectangle
		code |= RIGHT;
	}
	
	if (y > ybc) {				// below the rectangle
		code |= BOTTOM;
	} else if (y < ytc) {		// above the rectangle
		code |= TOP;
	}
	return code;
}

/* line: draw a line
 *   win     : window
 *   x1, y1  : top left point coordinates
 *   x2, y2  : bottom right point coordinates
 */
void lcd_line(int x1, int y1, int x2, int y2, Color c)
{
	if (!clipped) {
		lcd_draw_line(x1, y1, x2, y2, c);
	} else {
		int accept=0;
		
	    // Compute region codes for P1, P2
	    int code1 = rgn_code(x1, y1);
	    int code2 = rgn_code(x2, y2);
	
		for ( ; ; ) {
			if ((code1 == 0) && (code2 == 0)) {
				// both endpoints lie within rectangle
				accept=1;
				break;
			} else if (code1 & code2) {
				// both endpoints outside the clipping rectangle
				// no rectangle crossing
				break;
			} else {
				// part of the segment gets through the rectangle
				int code, x=0, y=0;
				
				// At least one endpoint is outside the rectangle, pick it.
				code = code1 ?  code1 : code2;
				
				// identify the crossed boundary & find intersection point;
				if (code & TOP) {
					// point is above the clip rectangle
					x = x1 + (x2 - x1) * (ytc - y1) / (y2 - y1);
					y = ytc;
				} else if (code & BOTTOM) {
					// point is below the rectangle
					x = x1 + (x2 - x1) * (ybc - y1) / (y2 - y1);
					y = ybc;
				} else if (code & RIGHT) {
					// point is to the right of rectangle
					y = y1 + (y2 - y1) * (xrc - x1) / (x2 - x1);
					x = xrc;
				} else if (code & LEFT) {
					// point is to the left of rectangle
					y = y1 + (y2 - y1) * (xlc - x1) / (x2 - x1);
					x = xlc;
				}
				
				// Now intersection point x, y is found
				// We replace point outside rectangle
				// by intersection point
				if (code == code1) {
					x1 = x;
					y1 = y;
					code1 = rgn_code(x1, y1);
				} else {
					x2 = x;
					y2 = y;
					code2 = rgn_code(x2, y2);
				}
			}
		}
		
	    if (accept) {
	    	lcd_draw_line(x1, y1, x2, y2, c);
	    }
    }
}

/* draw a sequence of disconnected segments, each segment being
 * described by its two endpoints
 */
void lcd_draw_segments(SPoint *p, int n, Color c)
{
	for (int i=0; i<n; i++) {
		lcd_line(p[2*i].x, p[2*i].y, p[2*i+1].x, p[2*i+1].y, c);
	}
}

/*
 *  Drawing an ellipse with a certain line thickness.
 *  Use an inner and and outer ellipse and fill the spaces between.
 *  The inner ellipse uses all UPPERCASE letters, the outer lowercase.
 *
 *  This algorithm is based on the fill_ellipse algorithm presented
 *  above, but uses two ellipse calculations, and some fix-up code
 *  to avoid pathological cases where the inner ellipse is almost
 *  the same size as the outer (in which case the border of the
 *  elliptical curve might otherwise have appeared broken).
 */
void lcd_draw_ellipse(Rect r, Colour c, int lwidth)
{
	/* Outer ellipse: e(x,y) = b*b*x*x + a*a*y*y - a*a*b*b */

	int a = r.width / 2;
	int b = r.height / 2;
	int x = 0;
	int y = b;
	long a2 = a*a;
	long b2 = b*b;
	long xcrit = (3 * a2 / 4) + 1;
	long ycrit = (3 * b2 / 4) + 1;
	long t = b2 + a2 - 2*a2*b;	/* t = e(x+1,y-1) */
	long dxt = b2*(3+x+x);
	long dyt = a2*(3-y-y);
	int d2xt = b2+b2;
	int d2yt = a2+a2;

	int w = lwidth;		//line_width;

	/* Inner ellipse: E(X,Y) = B*B*X*X + A*A*Y*Y - A*A*B*B */

	int A = a-w > 0 ? a-w : 0;
	int B = b-w > 0 ? b-w : 0;
	int X = 0;
	int Y = B;
	long A2 = A*A;
	long B2 = B*B;
	long XCRIT = (3 * A2 / 4) + 1;
	long YCRIT = (3 * B2 / 4) + 1;
	long T = B2 + A2 - 2*A2*B;	/* T = E(X+1,Y-1) */
	long DXT = B2*(3+X+X);
	long DYT = A2*(3-Y-Y);
	int D2XT = B2+B2;
	int D2YT = A2+A2;

	int movedown, moveout;
	int innerX = 0, prevx, prevy, W;
	Rect r1, r2;

	if ((r.width <= 2) || (r.height <= 2))
		lcd_fill_rect(r.x,r.y,r.width,r.height,c);

	r1.x = r.x + a;
	r1.y = r.y;
	r1.width = r.width & 1; /* i.e. if width is odd */
	r1.height = 1;

	r2 = r1;
	r2.y = r.y + r.height - 1;

	prevx = r1.x;
	prevy = r1.y;

	while (y > 0) {
		while (Y == y) {
			innerX = X;

			if (T + A2*Y < XCRIT) {			/* E(X+1,Y-1/2) <= 0 */
				/* move outwards to encounter edge */
				X += 1;
				T += DXT;
				DXT += D2XT;
			} else if (T - B2*X >= YCRIT) {	/* e(x+1/2,y-1) > 0 */
				/* drop down one line */
				Y -= 1;
				T += DYT;
				DYT += D2YT;
			} else {
				/* drop diagonally down and out */
				X += 1;
				Y -= 1;
				T += DXT + DYT;
				DXT += D2XT;
				DYT += D2YT;
			}
		}

		movedown = moveout = 0;

		W = x - innerX;
		if (r1.x + W < prevx)
			W = prevx - r1.x;
		if (W < w)
			W = w;

		if (t + a2*y < xcrit) {				/* e(x+1,y-1/2) <= 0 */
			/* move outwards to encounter edge */
			x += 1;
			t += dxt;
			dxt += d2xt;

			moveout = 1;
		} else if (t - b2*x >= ycrit) {		/* e(x+1/2,y-1) > 0 */
			/* drop down one line */
			y -= 1;
			t += dyt;
			dyt += d2yt;

			movedown = 1;
		} else {
			/* drop diagonally down and out */
			x += 1;
			y -= 1;
			t += dxt + dyt;
			dxt += d2xt;
			dyt += d2yt;

			movedown = 1;
			moveout = 1;
		}

		if (movedown) {
			if (r1.width == 0) {
				r1.x -= 1; r1.width += 2;
				r2.x -= 1; r2.width += 2;
				moveout = 0;
			}

			if (r1.x < r.x)
				r1.x = r2.x = r.x;
			if (r1.width > r.width)
				r1.width = r2.width = r.width;
			if (r1.y == r2.y-1) {
				r1.x = r2.x = r.x;
				r1.width = r2.width = r.width;
			}

			if ((r1.y < r.y+w) || (r1.x+W >= r1.x+r1.width-W)) {
				lcd_fill_rect(r1.x,r1.y,r1.width,r1.height,c);
				lcd_fill_rect(r2.x,r2.y,r2.width,r2.height,c);
			//	result &= app_fill_rect(g, r1);
			//	result &= app_fill_rect(g, r2);

				prevx = r1.x;
				prevy = r1.y;
			} else if (r1.y+r1.height < r2.y) {
				/* draw distinct rectangles */
				lcd_fill_rect(r1.x,r1.y,W,1,c);
				lcd_fill_rect(r1.x+r1.width-W,r1.y,W,1,c);
				lcd_fill_rect(r2.x,r2.y,W,1,c);
				lcd_fill_rect(r2.x+r2.width-W,r2.y,W,1,c);
				/*result &= app_fill_rect(g, rect(r1.x,r1.y,
						W,1));
				result &= app_fill_rect(g, rect(
						r1.x+r1.width-W,r1.y,W,1));
				result &= app_fill_rect(g, rect(r2.x,
						r2.y,W,1));
				result &= app_fill_rect(g, rect(
						r2.x+r2.width-W,r2.y,W,1));
				*/
				prevx = r1.x;
				prevy = r1.y;
			}

			/* move down */
			r1.y += 1;
			r2.y -= 1;
		}

		if (moveout) {
			/* move outwards */
			r1.x -= 1; r1.width += 2;
			r2.x -= 1; r2.width += 2;
		}
	}
	if ((x <= a) && (prevy < r2.y)) {
		/* draw final line */
		r1.height = r1.y+r1.height-r2.y;
		r1.y = r2.y;

		W = w;
		if (r.x + W != prevx)
			W = prevx - r.x;
		if (W < w)
			W = w;

		if (W+W >= r.width) {
			lcd_fill_rect(r.x,r1.y,r.width,r1.height,c);
			//result &= app_fill_rect(g, rect(r.x, r1.y,
			//	r.width, r1.height));
			return;
		}
		lcd_fill_rect(r.x,r1.y,W,r1.height,c);
		lcd_fill_rect(r.x+r.width-W,r1.y,W,r1.height,c);
		//result &= app_fill_rect(g, rect(r.x, r1.y, W, r1.height));
		//result &= app_fill_rect(g, rect(r.x+r.width-W, r1.y,
		//	W, r1.height));
	}
}

#if 0
void app_draw_arc(Rect r, int start_angle, int end_angle)
{
	/* Outer ellipse: e(x,y) = b*b*x*x + a*a*y*y - a*a*b*b */

	int a = r.width / 2;
	int b = r.height / 2;
	int x = 0;
	int y = b;
	long a2 = a*a;
	long b2 = b*b;
	long xcrit = (3 * a2 / 4) + 1;
	long ycrit = (3 * b2 / 4) + 1;
	long t = b2 + a2 - 2*a2*b;	/* t = e(x+1,y-1) */
	long dxt = b2*(3+x+x);
	long dyt = a2*(3-y-y);
	int d2xt = b2+b2;
	int d2yt = a2+a2;

	int w = 1;			// line_width;

	/* Inner ellipse: E(X,Y) = B*B*X*X + A*A*Y*Y - A*A*B*B */

	int A = a-w > 0 ? a-w : 0;
	int B = b-w > 0 ? b-w : 0;
	int X = 0;
	int Y = B;
	long A2 = A*A;
	long B2 = B*B;
	long XCRIT = (3 * A2 / 4) + 1;
	long YCRIT = (3 * B2 / 4) + 1;
	long T = B2 + A2 - 2*A2*B;	/* T = E(X+1,Y-1) */
	long DXT = B2*(3+X+X);
	long DYT = A2*(3-Y-Y);
	int D2XT = B2+B2;
	int D2YT = A2+A2;

	/* arc rectangle calculations */
	int movedown, moveout;
	int innerX = 0, prevx, prevy, W;
	Rect r1, r2;
	int result = 1;

	/* line descriptions */
	Point p0, p1, p2;

	START_DEBUG();

	/* if angles differ by 360 degrees or more, close the shape */
	if ((start_angle + 360 <= end_angle) ||
	    (start_angle - 360 >= end_angle))
	{
		return app_draw_ellipse(g, r);
	}

	/* make start_angle >= 0 and <= 360 */
	while (start_angle < 0)
		start_angle += 360;
	start_angle %= 360;

	/* make end_angle >= 0 and <= 360 */
	while (end_angle < 0)
		end_angle += 360;
	end_angle %= 360;

	/* draw nothing if the angles are equal */
	if (start_angle == end_angle)
		return 1;

	/* find arc wedge line end points */
	p0 = pt(r.x + r.width/2, r.y + r.height/2);
	p1 = app_boundary_point(r, start_angle);
	p2 = app_boundary_point(r, end_angle);

	/* determine ellipse rectangles */
	r1.x = r.x + a;
	r1.y = r.y;
	r1.width = r.width & 1; /* i.e. if width is odd */
	r1.height = 1;

	r2 = r1;
	r2.y = r.y + r.height - 1;

	prevx = r1.x;
	prevy = r1.y;

	while (y > 0)
	{
		while (Y == y)
		{
			innerX = X;

			if (T + A2*Y < XCRIT) /* E(X+1,Y-1/2) <= 0 */
			{
				/* move outwards to encounter edge */
				X += 1;
				T += DXT;
				DXT += D2XT;
			}
			else if (T - B2*X >= YCRIT) /* e(x+1/2,y-1) > 0 */
			{
				/* drop down one line */
				Y -= 1;
				T += DYT;
				DYT += D2YT;
			}
			else {
				/* drop diagonally down and out */
				X += 1;
				Y -= 1;
				T += DXT + DYT;
				DXT += D2XT;
				DYT += D2YT;
			}
		}

		movedown = moveout = 0;

		W = x - innerX;
		if (r1.x + W < prevx)
			W = prevx - r1.x;
		if (W < w)
			W = w;

		if (t + a2*y < xcrit) /* e(x+1,y-1/2) <= 0 */
		{
			/* move outwards to encounter edge */
			x += 1;
			t += dxt;
			dxt += d2xt;

			moveout = 1;
		}
		else if (t - b2*x >= ycrit) /* e(x+1/2,y-1) > 0 */
		{
			/* drop down one line */
			y -= 1;
			t += dyt;
			dyt += d2yt;

			movedown = 1;
		}
		else {
			/* drop diagonally down and out */
			x += 1;
			y -= 1;
			t += dxt + dyt;
			dxt += d2xt;
			dyt += d2yt;

			movedown = 1;
			moveout = 1;
		}

		if (movedown) {
			if (r1.width == 0) {
				r1.x -= 1; r1.width += 2;
				r2.x -= 1; r2.width += 2;
				moveout = 0;
			}

			if (r1.x < r.x)
				r1.x = r2.x = r.x;
			if (r1.width > r.width)
				r1.width = r2.width = r.width;
			if (r1.y == r2.y-1) {
				r1.x = r2.x = r.x;
				r1.width = r2.width = r.width;
			}

			if ((r1.y < r.y+w) || (r1.x+W >= r1.x+r1.width-W))
			{
				result &= app_fill_arc_rect(g, r1,
						p0, p1, p2,
						start_angle, end_angle);
				result &= app_fill_arc_rect(g, r2,
						p0, p1, p2,
						start_angle, end_angle);

				prevx = r1.x;
				prevy = r1.y;
			}
			else if (r1.y+r1.height < r2.y)
			{
				/* draw distinct rectangles */
				result &= app_fill_arc_rect(g, rect(
						r1.x,r1.y,W,1),
						p0, p1, p2,
						start_angle, end_angle);
				result &= app_fill_arc_rect(g, rect(
						r1.x+r1.width-W,r1.y,W,1),
						p0, p1, p2,
						start_angle, end_angle);
				result &= app_fill_arc_rect(g, rect(
						r2.x,r2.y,W,1),
						p0, p1, p2,
						start_angle, end_angle);
				result &= app_fill_arc_rect(g, rect(
						r2.x+r2.width-W,r2.y,W,1),
						 p0, p1, p2,
						start_angle, end_angle);

				prevx = r1.x;
				prevy = r1.y;
			}

			/* move down */
			r1.y += 1;
			r2.y -= 1;
		}

		if (moveout) {
			/* move outwards */
			r1.x -= 1; r1.width += 2;
			r2.x -= 1; r2.width += 2;
		}
	}
	if ((x <= a) && (prevy < r2.y)) {
		/* draw final lines */
		r1.height = r1.y+r1.height-r2.y;
		r1.y = r2.y;

		W = w;
		if (r.x + W != prevx)
			W = prevx - r.x;
		if (W < w)
			W = w;

		if (W+W >= r.width) {
			while (r1.height > 0) {
				result &= app_fill_arc_rect(g, rect(r.x,
					r1.y, r.width, 1), p0, p1, p2,
					start_angle, end_angle);
				r1.y += 1;
				r1.height -= 1;
			}
			return result;
		}

		while (r1.height > 0) {
			result &= app_fill_arc_rect(g, rect(r.x, r1.y,
					W, 1), p0, p1, p2,
					start_angle, end_angle);
			result &= app_fill_arc_rect(g, rect(r.x+r.width-W,
					r1.y, W, 1), p0, p1, p2,
					start_angle, end_angle);
			r1.y += 1;
			r1.height -= 1;
		}
	}

	return result;
}
#endif
/**
 * \brief Helper function drawing rounded corners
 *
 * \param x0			The x-coordinate
 * \param y0			The y-coordinate
 * \param r				Radius
 * \param cornername	Corner (1, 2, 3, 4)
 * \param color			Color
 *
 * \return void
 */
static void lcdDrawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;
		if (cornername & 0x4) {
			lcd_draw_point(x0 + x, y0 + y, color);
			lcd_draw_point(x0 + y, y0 + x, color);
		}
		if (cornername & 0x2) {
			lcd_draw_point(x0 + x, y0 - y, color);
			lcd_draw_point(x0 + y, y0 - x, color);
		}
		if (cornername & 0x8) {
			lcd_draw_point(x0 - y, y0 + x, color);
			lcd_draw_point(x0 - x, y0 + y, color);
		}
		if (cornername & 0x1) {
			lcd_draw_point(x0 - y, y0 - x, color);
			lcd_draw_point(x0 - x, y0 - y, color);
		}
	}
}

/**
 * \brief Draws a rectangle with rounded corners specified by a coordinate pair, a width, and a height.
 *
 * \param x			The x-coordinate of the upper-left corner of the rectangle to draw
 * \param y			The y-coordinate of the upper-left corner of the rectangle to draw
 * \param w			Width of the rectangle to draw
 * \param h			Height of the rectangle to draw
 * \param r			Radius
 * \param color		Color
 *
 * \return void
 */
void lcd_draw_round_rect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color)
{
	// smarter version
	lcd_draw_line(x + r, y, x + w - r, y, color);
	lcd_draw_line(x + r, y + h - 1, x + w - r, y + h - 1, color);
	lcd_draw_line(x, y + r, x, y + h - r, color);
	lcd_draw_line(x + w - 1, y + r, x + w - 1, y + h - r, color);

	// draw four corners
	lcdDrawCircleHelper(x + r, y + r, r, 1, color);
	lcdDrawCircleHelper(x + w - r - 1, y + r, r, 2, color);
	lcdDrawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
	lcdDrawCircleHelper(x + r, y + h - r - 1, r, 8, color);
}

/**
 * \brief Draws an circle defined by a pair of coordinates and radius
 *
 * \param x0		The x-coordinate
 * \param y0		The y-coordinate
 * \param r			Radius
 * \param color		Color
 *
 * \return void
 */
void lcd_draw_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
	int16_t f = 1 - r;
	int16_t ddF_x = 1;
	int16_t ddF_y = -2 * r;
	int16_t x = 0;
	int16_t y = r;

	lcd_draw_point(x0, y0 + r, color);
	lcd_draw_point(x0, y0 - r, color);
	lcd_draw_point(x0 + r, y0, color);
	lcd_draw_point(x0 - r, y0, color);

	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		lcd_draw_point(x0 + x, y0 + y, color);
		lcd_draw_point(x0 - x, y0 + y, color);
		lcd_draw_point(x0 + x, y0 - y, color);
		lcd_draw_point(x0 - x, y0 - y, color);
		lcd_draw_point(x0 + y, y0 + x, color);
		lcd_draw_point(x0 - y, y0 + x, color);
		lcd_draw_point(x0 + y, y0 - x, color);
		lcd_draw_point(x0 - y, y0 - x, color);
	}
}

/****************************************************************
 * Text drawing functions
 ****************************************************************/
/* Draws a character at the specified coordinates
 *  dc			drawing context: holds font, back and fore colors	
 *  x, y		top-left char box coordinates
 *  c			character code
 */
#if 0
uint16_t lcd_draw_char(DC *dc, int16_t x, int16_t y, char c)
{
	uint16_t cw = dc->font->width, ch = dc->font->height;
	
	if ((((uint16_t)x + cw) >= dc->clip.x+dc->clip.width) ||
		(((uint16_t)y + ch) >= dc->clip.y+dc->clip.height))
		return 0;

	uint16_t cache[cw*ch];
	uint16_t bcolor = dc->bcolor, fcolor = dc->fcolor;
	const uint8_t *p = dc->font->data+((uint8_t)c-0x20)*(((cw-1)>>3)+1)*ch;
	uint8_t b = *p;
	uint8_t msk = 0x80;
	int l=0;
	
	/* read data for the char and setup a char color buffer */
	for(int i = 0; i < ch; i++) {
		for(int j = 1; j <= cw; j++) {
			if(b & msk) {
				cache[l++]=fcolor;
			} else {
				cache[l++]=bcolor;
			}
			if (j & 0x7) {		/* j%8 != 0 */
				msk = msk>>1;
			} else {
				msk = 0x80;
				b = *(++p);
			}
		}
		msk = 0x80;
		b=*(++p);
	}
	
	/* send the char color buffer to the screen */
#if 0
	LCD_DPY_EN;
	lcd_set_window(x, y, x+cw-1, y+ch-1);
	spi_write16(LCD_SPI, cache, cw*ch);
	LCD_DPY_DIS;
#else
	/*      mode     | MY MX MV ML BGR MH  0  0 |  hex
	   PORTRAIT      |  0  1  0  0   1  0  0  0 | 0x48
	   LANDSCAPE     |  0  0  1  0   1  0  0  0 | 0x28 
	   PORTRAIT_INV  |  1  0  0  0   1  0  0  0 | 0x88
	   LANDSCAPE_INV |  1  1  1  0   1  0  0  0 | 0xE8
	 */
	uint8_t disp_modes[] = {0x48, 0x28, 0x88, 0xE8};
	LCD_DPY_EN;
	switch (dc->tflags & DIR_MSK) {
	case DIR_HORIZONTAL:
		lcd_set_window(x, y, x+cw-1, y+ch-1);
		spi_write16(LCD_SPI, cache, cw*ch);
		break;
	case DIR_HORIZONTAL_INV:
		LCD_DPY_CMD;
		spi_write_byte(LCD_SPI, ILI9341_MADCTL);		// set Memory Access Control
		LCD_DPY_DATA;
		spi_write_byte(LCD_SPI, disp_modes[2]);
		lcd_set_window(LCD_WIDTH-x-1, LCD_HEIGHT-y-1, LCD_WIDTH-x+cw-2, LCD_HEIGHT-y+ch-2);
		spi_write16(LCD_SPI, cache, cw*ch);
		LCD_DPY_CMD;
		spi_write_byte(LCD_SPI, ILI9341_MADCTL);		// set Memory Access Control
		LCD_DPY_DATA;
		spi_write_byte(LCD_SPI, disp_modes[0]);
		break;
		break;
	case DIR_VERTICAL:
		LCD_DPY_CMD;
		spi_write_byte(LCD_SPI, ILI9341_MADCTL);		// set Memory Access Control
		LCD_DPY_DATA;
		spi_write_byte(LCD_SPI, disp_modes[1]);
		lcd_set_window(y, LCD_WIDTH-x-1, y+cw-1, LCD_WIDTH-x+ch-2);
		spi_write16(LCD_SPI, cache, cw*ch);
		LCD_DPY_CMD;
		spi_write_byte(LCD_SPI, ILI9341_MADCTL);		// set Memory Access Control
		LCD_DPY_DATA;
		spi_write_byte(LCD_SPI, disp_modes[0]);
		break;
	case DIR_VERTICAL_INV:
		LCD_DPY_CMD;
		spi_write_byte(LCD_SPI, ILI9341_MADCTL);		// set Memory Access Control
		LCD_DPY_DATA;
		spi_write_byte(LCD_SPI, disp_modes[3]);
		lcd_set_window(LCD_HEIGHT-y-1, x, LCD_HEIGHT-y+cw-2, x+ch-2);
		spi_write16(LCD_SPI, cache, cw*ch);
		LCD_DPY_CMD;
		spi_write_byte(LCD_SPI, ILI9341_MADCTL);		// set Memory Access Control
		LCD_DPY_DATA;
		spi_write_byte(LCD_SPI, disp_modes[0]);
		break;
	}
	LCD_DPY_DIS;
#endif
	return cw;
}

void lcd_draw_string(DC *dc, int16_t x, int16_t y, const char *s)
{
	uint16_t width, height;
	
	lcd_get_string_size(dc, s, &width, &height);
	
	switch (dc->tflags & ALIGN_MSK) {
	case ALIGN_SE:
		/* nothing to do */
		break;
	case ALIGN_E:
		y -= height/2;
		break;
	case ALIGN_NE:
		y -= height;
		break;
	case ALIGN_N:
		x -= width/2;
		y -= height;
		break;
	case ALIGN_NW:
		x -= width;
		y -= height;
		break;
	case ALIGN_W:
		x -= width;
		y -= height/2;
		break;
	case ALIGN_SW:
		x -= width;
		break;
	case ALIGN_S:
		x -= width/2;
		break;
	case ALIGN_CENTER:
		x -= width/2;
		y -= height/2;
		break;
	}
	switch (dc->tflags & DIR_MSK) {
	case DIR_HORIZONTAL:
		while (*s) {
			x += lcd_draw_char(dc,x,y,*s++);
		}
		break;
	case DIR_HORIZONTAL_INV:
		while (*s) {
			x -= lcd_draw_char(dc,x,y,*s++);
		}
		break;
		break;
	case DIR_VERTICAL:
		while (*s) {
			y += lcd_draw_char(dc,x,y,*s++);
		}
		break;
	case DIR_VERTICAL_INV:
		while (*s) {
			y -= lcd_draw_char(dc,x,y,*s++);
		}
		break;
	}
}
#endif
static uint16_t lcd_draw_raw_char(DC *dc, int16_t x, int16_t y, char c)
{
	uint16_t cw = dc->font->width, ch = dc->font->height;
	
/*	if ((((uint16_t)x + cw) >= dc->clip.x+dc->clip.width) ||
		(((uint16_t)y + ch) >= dc->clip.y+dc->clip.height))
		return 0;
*/
	uint16_t cache[cw*ch];
	uint16_t bcolor = dc->bcolor, fcolor = dc->fcolor;
	const uint8_t *p = dc->font->data+((uint8_t)c-0x20)*(((cw-1)>>3)+1)*ch;
	uint8_t b = *p;
	uint8_t msk = 0x80;
	int l=0;
	
	/* read data for the char and setup a char color buffer */
	for(int i = 0; i < ch; i++) {
		for(int j = 1; j <= cw; j++) {
			if(b & msk) {
				cache[l++]=fcolor;
			} else {
				cache[l++]=bcolor;
			}
			if (j & 0x7) {		/* j%8 != 0 */
				msk = msk>>1;
			} else {
				msk = 0x80;
				b = *(++p);
			}
		}
		msk = 0x80;
		b=*(++p);
	}
	
	/* send the char color buffer to the screen */
	lcd_set_window(x, y, x+cw-1, y+ch-1);
	spi_write16(LCD_SPI, cache, cw*ch);
	
	return cw;
}

void lcd_draw_string(DC *dc, int16_t x, int16_t y, const char *s)
{
	uint16_t width = strlen(s)*dc->font->width;
	uint16_t height = dc->font->height;
	uint16_t temp;
	
	/* alter the current display orientation mode according to
	   the selected text orientation.
	 */
	Orientation orientation = disp.orientation;
	LCD_DPY_EN;
	switch (dc->tflags & DIR_MSK) {
	case DIR_HORIZONTAL:
		/* nothing to do */
		break;
	case DIR_HORIZONTAL_INV:
		orientation=(orientation+2) & 3;				// turn display current (or+2)%4
		x=LCD_WIDTH-x-1; y=LCD_HEIGHT-y-1;
		LCD_DPY_CMD;
		spi_write_byte(LCD_SPI, ILI9341_MADCTL);		// set Memory Access Control
		LCD_DPY_DATA;
		spi_write_byte(LCD_SPI, disp_modes[orientation]);
		break;
	case DIR_VERTICAL:
		orientation=(orientation+1) & 3;				// turn display current (or+2)%4
		temp=LCD_WIDTH-x-1; x=y; y=temp;
		LCD_DPY_CMD;
		spi_write_byte(LCD_SPI, ILI9341_MADCTL);		// set Memory Access Control
		LCD_DPY_DATA;
		spi_write_byte(LCD_SPI, disp_modes[orientation]);
		break;
	case DIR_VERTICAL_INV:
		orientation=(orientation+3) & 3;				// turn display current (or+2)%4
		temp=LCD_HEIGHT-y-1; y=x; x=temp;
		LCD_DPY_CMD;
		spi_write_byte(LCD_SPI, ILI9341_MADCTL);		// set Memory Access Control
		LCD_DPY_DATA;
		spi_write_byte(LCD_SPI, disp_modes[orientation]);
		break;
	}
	
	/* calculate the top-left coordinate of the string box 
	   according to selected alignment
	 */
	switch (dc->tflags & ALIGN_MSK) {
	case ALIGN_SE:
		/* nothing to do */
		break;
	case ALIGN_E:
		y -= height/2;
		break;
	case ALIGN_NE:
		y -= height;
		break;
	case ALIGN_N:
		x -= width/2;
		y -= height;
		break;
	case ALIGN_NW:
		x -= width;
		y -= height;
		break;
	case ALIGN_W:
		x -= width;
		y -= height/2;
		break;
	case ALIGN_SW:
		x -= width;
		break;
	case ALIGN_S:
		x -= width/2;
		break;
	case ALIGN_CENTER:
		x -= width/2;
		y -= height/2;
		break;
	}
	
#ifdef LCD_DEBUG
	uint16_t xo=x, yo=y;
#endif

	/* draw the string */
	while (*s) {
		x += lcd_draw_raw_char(dc,x,y,*s++);
	}
	
#ifdef LCD_DEBUG
	lcd_draw_rect(xo,yo,width,height,BLUE);
	LCD_DPY_EN;
#endif

	/* reset to the display orientation */
	if (orientation!=disp.orientation) {
		LCD_DPY_CMD;
		spi_write_byte(LCD_SPI, ILI9341_MADCTL);		// set Memory Access Control
		LCD_DPY_DATA;
		spi_write_byte(LCD_SPI, disp_modes[disp.orientation]);
	}
	LCD_DPY_DIS;
}

void lcd_get_string_size(DC *dc, const char *s, uint16_t *width, uint16_t *height)
{
	switch (dc->tflags & DIR_MSK) {
	case DIR_HORIZONTAL:
	case DIR_HORIZONTAL_INV:
		*width = strlen(s)*dc->font->width;
		*height = dc->font->height;
		break;
	case DIR_VERTICAL:
	case DIR_VERTICAL_INV:
		*width = dc->font->height;
		*height = strlen(s)*dc->font->width;
		break;
	}
}

int freq=0;

/****************************************************************
 * display initialization
 ****************************************************************/
//initialize the lcd.
//phwDevId pointer to device ID of lcd
void lcd_dpy_init(void)
{	
	uint8_t initlist[] = {
		0xEF, 3, 0x03, 0x80, 0x02,
		0xCF, 3, 0x00, 0xC1, 0x30,
		0xED, 4, 0x64, 0x03, 0x12, 0x81,
		0xE8, 3, 0x85, 0x00, 0x78,
		0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
		0xF7, 1, 0x20,
		0xEA, 2, 0x00, 0x00,
		ILI9341_PWCTR1  , 1, 0x23,				// Power control VRH[5:0]
		ILI9341_PWCTR2  , 1, 0x10,				// Power control SAP[2:0];BT[3:0]
		ILI9341_VMCTR1  , 2, 0x3e, 0x28,		// VCM control
		ILI9341_VMCTR2  , 1, 0x86,				// VCM control2
		ILI9341_MADCTL  , 1, disp_modes[disp.orientation],				// Memory Access Control
		ILI9341_VSCRSADD, 1, 0x00,				// Vertical scroll zero
		ILI9341_PIXFMT  , 1, 0x55,
		ILI9341_FRMCTR1 , 2, 0x00, 0x18,
		ILI9341_DFUNCTR , 3, 0x08, 0x82, 0x27,	// Display Function Control
		  0xF2, 1, 0x00,						// 3Gamma Function Disable
		ILI9341_GAMMASET , 1, 0x01,				// Gamma curve selected
		ILI9341_GMCTRP1 , 15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, // Set Gamma
		    0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
		ILI9341_GMCTRN1 , 15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, // Set Gamma
		    0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
		ILI9341_SLPOUT  , 0x80,					// Exit Sleep
		ILI9341_DISPON  , 0x80,					// Display on
		0x00									// Sentinel at the end of the list
	};
	uint8_t	cmd, x, n;

    lcd_switch_to(LCD_DPY);
	
	LCD_DPY_DIS;
	LCD_DPY_BL_ON;
	
	/* soft reset */
	LCD_DPY_EN;
	LCD_DPY_CMD;
	spi_write_byte(LCD_SPI, ILI9341_SWRESET);
	LCD_DPY_DIS;
	
	delay_ms(5);
	
	LCD_DPY_EN;
	uint8_t *p = initlist;
	while(*p != 0) { /* check for sentinel */
		cmd = *p++;
		x = *p++;
		n = x & 0x7F; /* mask out delay bit */
		LCD_DPY_CMD;
		spi_write_byte(LCD_SPI, cmd);
		LCD_DPY_DATA;
		if (n) {
			spi_write(LCD_SPI, p, n);
		}
		p += n;
		if (x & 0x80) { /* if delay bit is set */
			delay_ms(150);
		}
	}
	
	LCD_DPY_DIS;
	
	lcd_clear_screen(WHITE);
}


/*-------------------------------END OF FILE-------------------------------*/

