/*
 * 2023 (C) E. Boucharé
 */

#include "pin_mux.h"
#include "board.h"

#include "fsl_gpio.h"
#include "fsl_common.h"
#include "fsl_power.h"
#define FOR_TEST
#ifdef FOR_TEST
#include "lcd.h"
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/


#ifndef FOR_TEST
#define LED_INIT()                   \
    gpio_pin_config_t led_config = { \
        kGPIO_DigitalOutput,         \
        0,                           \
    };                               \
    GPIO_PinInit(GPIO, BOARD_LED_RED_GPIO_PORT, BOARD_LED_RED_GPIO_PIN, &led_config);
#define LED_TOGGLE() GPIO_PortToggle(GPIO, BOARD_LED_RED_GPIO_PORT, 1u << BOARD_LED_RED_GPIO_PIN);

#else

typedef enum {
	L_SOLID, L_DOTTED, L_DASHED, L_COMB, L_ARROW, L_BAR, L_FBAR,
	L_STEP, L_FSTEP, L_NONE
} line_t;

typedef enum {
	M_CROSS, M_PLUS, M_DOT, M_STAR, M_CIRCLE, M_FCIRCLE,
	M_SQUARE, M_FSQUARE, M_DIAMOND, M_FDIAMOND, M_ARROW,
	M_TRIANGLE, M_FTRIANGLE, M_NONE
} marker_t;

line_t		ltype=L_SOLID;
marker_t	mtype=M_NONE;
int			msize=6;

#endif
/*******************************************************************************
 * Mailbox and events
 ******************************************************************************/
#define EVT_MASK						(0xFF<<24)

#define EVT_NONE						0
#define EVT_CORE_UP						(1U<<24)
#define EVT_RETVAL						(2U<<24)
#define EVT DRAWPOINT					(3U<<24)
#define EVT_DRAWLINE					(4U<<24)
#define EVT_DRAWRECT					(5U<<24)
#define EVT_DRAWRNDRECT					(6U<<24)
#define EVT_DRAWCIRCLE					(7U<<24)
#define EVT_DRAWELLISPSE				(8U<<24)
#define EVT_DRAWLINES					(9U<<24)
#define EVT_DRAWSEGMENTS				(10U<<24)
#define EVT_FILLRECT					(11U<<24)
#define EVT_CLIP						(12U<<24)
#define EVT_UNCLIP						(13U<<24)
#define EVT_DRAWPATH					(14U<<24)

#define EVT_FORECOLOR					(15U<<24)
#define EVT_BACKCOLOR					(16U<<24)
#define EVT_SETFONT						(17U<<24)

#define EVT_GETBUFFER					(30U<<24)
#define EVT_SETALIGN					    (31U<<24)
#define EVT_DIRECTION					    (32U<<24)
#define EVT_DRAWSTRING					    (33U<<24)

/* mailbox communication between cores */
void mb_init(void)
{
//  Init already done by core0
	MAILBOX->MBOXIRQ[0].IRQCLR=0xFFFFFFFF;
    NVIC_SetPriority(MAILBOX_IRQn, 2);
    NVIC_EnableIRQ(MAILBOX_IRQn);
}

/* pop event from CPU0 */
/* pop event from CPU0 */

uint32_t mb_pop_evt(void)
{
	uint32_t evt=MAILBOX->MBOXIRQ[0].IRQ;
	MAILBOX->MBOXIRQ[0].IRQCLR=evt;
	return evt;
}

/* send event to CPU1, wait if there is already a pending event, unless force is set */
bool mb_push_evt(uint32_t evt, bool force)
{
	if (MAILBOX->MBOXIRQ[1].IRQ && !force) {
		return false;
	}
	MAILBOX->MBOXIRQ[1].IRQSET=evt;
	return true;
}/* event queue */

#define MAX_EVT_DATA				5
typedef struct _Event {
	uint32_t	event;
	uint32_t	data[MAX_EVT_DATA];
} Event;

/* event queue handling */
#define MAX_EVTS					20

volatile Event evq[MAX_EVTS];
volatile int evq_rd=0, evq_wr=0;

bool next_event(Event *evt)
{
	if (evq_rd==evq_wr) return false;

	*evt=evq[evq_rd];
	evq_rd=(evq_rd+1) % MAX_EVTS;
	return true;
}

void MAILBOX_IRQHandler(void)
{
	if (((evq_wr+1)%MAX_EVTS)!=evq_rd) {
		evq[evq_wr].event = mb_pop_evt();
		evq_wr=(evq_wr+1) % MAX_EVTS;
	}
}

/*******************************************************************************
 * shared buffer
 ******************************************************************************/

#define MAXPOINTS	2048
#define BUFFER_CAPACITY 256
#define TOTAL_BUFFERS 10

//__attribute__ ((section(".shmem")))
//SPoint shdata[MAXPOINTS];				/* shared data buffer */
volatile extern SPoint __start_noinit_shmem[];
volatile SPoint *shdata=__start_noinit_shmem;
typedef struct {
    char data[BUFFER_CAPACITY];
} Buffer;
volatile Buffer *buffer_array ;
int current_buffer_idx;
void init_DataBuffer() {
	buffer_array = (Buffer *)(shdata + MAXPOINTS);
}

/*******************************************************************************
 * main
 ******************************************************************************/
int main(void)
{
    /* Init board hardware.*/
	init_DataBuffer();
#ifdef FOR_TEST
	lcd_init();
	lcd_switch_to(LCD_DPY);
#endif
    /* Initialize mailbox, send EVT_CORE_UP to notify core 0 that core 1 is up
     * and ready to work.
     */
	mb_init();
    mb_push_evt(EVT_CORE_UP,true);

#ifdef FOR_TEST
	Event ev;
	uint16_t x0, x1, y0, y1,w ,h,c;
 	uint32_t Z0;
 	int n;
   	DC dc;
    Font font;
   	lcd_get_default_DC(&dc);
   	for (;;) {
   	 while (!next_event(&ev)) {
   	        __WFI();
   	    }

    	switch (ev.event & EVT_MASK) {
    	case EVT_FORECOLOR:
    		dc.fcolor=(Color)(ev.event & 0x0000FFFF);
    		break;
    	case EVT_DRAWLINES:
    			n=ev.event&0x000007FF;
    		x0=shdata[0].x; y0=shdata[0].y;
    		for (int i=1;i<n;i++) {
    			x1=shdata[i].x; y1=shdata[i].y;
    			lcd_line(x0,y0,x1,y1,dc.fcolor);
    			x0=x1; y0=y1;
    		}
    		mb_push_evt(EVT_RETVAL,true);
    		break;
		case EVT_DRAWSEGMENTS:
				n=ev.event&0x000007FF;
				current_buffer_idx = (ev.event &0x00FF0000)>>16;

				x0 = (uint16_t)buffer_array[current_buffer_idx].data[0] | (uint16_t)buffer_array[current_buffer_idx].data[1]<< 8 ;
				lcd_draw_segments(shdata,n&0x7FF,(Color)x0);

				mb_push_evt(EVT_RETVAL,true);
				break;
		case EVT_DRAWPATH:
		    n = ev.event & 0x00FFFFFF;
		    uint16_t msize = (ev.event >> 20) & 0x0F; // msize
		    uint16_t mtype = (ev.event >> 16) & 0x0F; // mtype
		    uint16_t ltype = (ev.event >> 12) & 0x0F; // ltype

		    for (int i = 0; i < n; i++) {
		        uint16_t x = shdata[i].x;
		        uint16_t y = shdata[i].y;
//
//		        if (ltype == L_SOLID) {
//		        	lcd_line(x, y, dc.fcolor);
//		        } else if (ltype == L_DASHED) {
//		        	lcd_line(x, y, dc.fcolor);
//		        }
		    }
		    mb_push_evt(EVT_RETVAL, true); // Send return value
		    break;
    	case EVT_FILLRECT:
    		current_buffer_idx = (ev.event &0x00FF0000)>>16;
    		  x0 = (uint16_t)buffer_array[current_buffer_idx].data[0] | ((uint16_t)buffer_array[current_buffer_idx].data[1] << 8);
    		     y0 = (uint16_t)buffer_array[current_buffer_idx].data[2] | ((uint16_t)buffer_array[current_buffer_idx].data[3] << 8);
    		     w = (uint16_t)buffer_array[current_buffer_idx].data[4] | ((uint16_t)buffer_array[current_buffer_idx].data[5] << 8);
    		     h = (uint16_t)buffer_array[current_buffer_idx].data[6] | ((uint16_t)buffer_array[current_buffer_idx].data[7] << 8);
    		    uint16_t color = (uint16_t)buffer_array[current_buffer_idx].data[8] | ((uint16_t)buffer_array[current_buffer_idx].data[9] << 8);

    		    lcd_fill_rect(x0, y0, w, h, color);

        	    	    		mb_push_evt(EVT_RETVAL,true);
        	    	    		break;
    	case EVT_DRAWRECT:
    		current_buffer_idx = (ev.event &0x00FF0000)>>16;
									  x0 = (uint16_t)buffer_array[current_buffer_idx].data[0] | ((uint16_t)buffer_array[current_buffer_idx].data[1] << 8);
										 y0 = (uint16_t)buffer_array[current_buffer_idx].data[2] | ((uint16_t)buffer_array[current_buffer_idx].data[3] << 8);
										  w = (uint16_t)buffer_array[current_buffer_idx].data[4] | ((uint16_t)buffer_array[current_buffer_idx].data[5] << 8);
										  h = (uint16_t)buffer_array[current_buffer_idx].data[6] | ((uint16_t)buffer_array[current_buffer_idx].data[7] << 8);
										 c = (uint16_t)buffer_array[current_buffer_idx].data[8] | ((uint16_t)buffer_array[current_buffer_idx].data[9] << 8);

										lcd_draw_rect(x0, y0, w, h, c);

										mb_push_evt(EVT_RETVAL,true);
    	        	    	    		break;
     	case EVT_CLIP:
    		current_buffer_idx = (ev.event &0x00FF0000)>>16;
     		    int x1 = (int)buffer_array[current_buffer_idx].data[0] | ((int)buffer_array[current_buffer_idx].data[1] << 8);
     		    int y1 = (int)buffer_array[current_buffer_idx].data[2] | ((int)buffer_array[current_buffer_idx].data[3] << 8);
     		    int x2 = (int)buffer_array[current_buffer_idx].data[4] | ((int)buffer_array[current_buffer_idx].data[5] << 8);
     		    int y2 = (int)buffer_array[current_buffer_idx].data[6] | ((int)buffer_array[current_buffer_idx].data[7] << 8);
     		    lcd_clip(x1, y1, x2, y2);

        	        	    	    		mb_push_evt(EVT_RETVAL,true);
        	        	    	    		break;
     	case EVT_UNCLIP:
     	        	        	    	    		lcd_unclip();
     	        	        	    	    		mb_push_evt(EVT_RETVAL,true);
     	        	        	    	    		break;
    	case EVT_SETFONT:
    		current_buffer_idx = (ev.event &0x00FF0000)>>16;
    		font.data = (uint16_t)buffer_array[current_buffer_idx].data[0] | ((uint16_t)buffer_array[current_buffer_idx].data[1] << 8);
    		    font.height = (uint16_t)buffer_array[current_buffer_idx].data[2] | ((uint16_t)buffer_array[current_buffer_idx].data[3] << 8);
    		    font.width = (uint16_t)buffer_array[current_buffer_idx].data[4] | ((uint16_t)buffer_array[current_buffer_idx].data[5] << 8);
    		        	        	    	    		lcd_set_font(&dc,&font);

    		        	        	    	    		mb_push_evt(EVT_RETVAL,true);
         	        	        	    	    		break;
    	case EVT_SETALIGN:
    								current_buffer_idx = (ev.event &0x00FF0000)>>16;
									 Z0 = (uint32_t)buffer_array[current_buffer_idx].data[0] | ((uint32_t)buffer_array[current_buffer_idx].data[1] << 8) | ((uint32_t)buffer_array[current_buffer_idx].data[2] << 16) | ((uint32_t)buffer_array[current_buffer_idx].data[3] << 24);

										lcd_set_alignment(&dc, Z0);

             	        	        	    	    		mb_push_evt(EVT_RETVAL,true);
             	        	        	    	    		break;
    	case EVT_DIRECTION:
    		current_buffer_idx = (ev.event &0x00FF0000)>>16;
    		   Z0 = (uint32_t)buffer_array[current_buffer_idx].data[0] | ((uint32_t)buffer_array[current_buffer_idx].data[1] << 8) | ((uint32_t)buffer_array[current_buffer_idx].data[2] << 16) | ((uint32_t)buffer_array[current_buffer_idx].data[3] << 24);

    		    lcd_set_direction(&dc, Z0);

							mb_push_evt(EVT_RETVAL,true);
    	             	        	        	    	    		break;
    	case EVT_DRAWSTRING:
    		current_buffer_idx = (ev.event &0x00FF0000)>>16;
        	    														x0 = ((uint16_t)buffer_array[current_buffer_idx].data[1]<<8)|(uint16_t)buffer_array[current_buffer_idx].data[0];
        	    														y0 = ((uint16_t)buffer_array[current_buffer_idx].data[3]<<8)|(uint16_t)buffer_array[current_buffer_idx].data[2];
        	    														lcd_draw_string(&dc,x0,y0,&buffer_array[current_buffer_idx].data[4]);

        	    														mb_push_evt(EVT_RETVAL,true);
        	             	        	        	    	    		break;

    	default:
    		break;
    	}
    }
#else
    /* enable clock for GPIO */
    CLOCK_EnableClock(kCLOCK_Gpio0);
    CLOCK_EnableClock(kCLOCK_Gpio1);
    BOARD_InitBootPins();
   /* Make a noticable delay after the reset */
    /* Use startup parameter from the master core... */
    for (int i = 0; i < 2; i++)
    {
        SDK_DelayAtLeastUs(1000000U, 150000000L);
    }

    /* Configure LED */
    LED_INIT();

    for (;;)
    {
        SDK_DelayAtLeastUs(500000U, 150000000L);
        LED_TOGGLE();
    }
#endif
}
