
DDEFS +=

LCD = lib/lcd

DINCDIR += $(LCD)

SRC += $(LCD)/lcd.c $(LCD)/lcd_private.c $(LCD)/lcd_dpy.c $(LCD)/lcd_ts.c \
       $(LCD)/fonts/fixed8.c $(LCD)/fonts/fixed12.c \
       $(LCD)/fonts/fixed16.c $(LCD)/fonts/fixed20.c
       