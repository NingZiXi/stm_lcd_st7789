#include "stm_lcd_st7789.h"
#include <assert.h>
#include <string.h>
typedef struct { int commands, colors, fail; size_t bytes; uint8_t window[4]; } mock_t;
static int cmd(void *context, uint8_t command, const uint8_t *data, size_t length)
{
    mock_t *m = context;
    ++m->commands;
    if (command == 0x2A) { assert(length == 4); memcpy(m->window, data, 4); }
    if (command == 0x2C) assert(length == 0);
    return m->fail;
}
static int color(void *context, uint8_t command, const void *data, size_t length)
{ mock_t *m=context; assert(command==0x2C && data); ++m->colors; m->bytes=length; return m->fail; }
static void delay(void *context, uint32_t ms) { (void)context; assert(ms); }
int main(void)
{
    mock_t m={0};
    stm_lcd_st7789_t p={0};
    const stm_lcd_st7789_config_t cfg={cmd, color, delay, NULL, &m, 240, 320, 2, 3};
    uint8_t pixels[12]={0};
    assert(stm_lcd_st7789_new_panel(&p,&cfg)==0);
    assert(stm_lcd_st7789_draw_bitmap(&p,0,0,2,3,pixels)==-1);
    assert(stm_lcd_st7789_init(&p)==0);
    assert(m.commands>5);
    assert(stm_lcd_st7789_draw_bitmap(&p,0,0,2,3,pixels)==0);
    assert(m.colors==1 && m.bytes==12 && m.window[0]==0 && m.window[1]==2 && m.window[3]==3);
    assert(stm_lcd_st7789_draw_bitmap(&p,0,0,241,3,pixels)==-1);
    assert(stm_lcd_st7789_draw_bitmap(&p,2,0,2,3,pixels)==-1);
    m.fail=1;
    assert(stm_lcd_st7789_draw_bitmap(&p,0,0,1,1,pixels)==-2);
    assert(stm_lcd_st7789_reset(&p)==0);
    assert(stm_lcd_st7789_draw_bitmap(&p,0,0,1,1,pixels)==-1);
    return 0;
}
