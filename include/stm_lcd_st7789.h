#ifndef STM_LCD_ST7789_H
#define STM_LCD_ST7789_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* All callbacks return 0 on success. tx_color sends its command (RAMWR) followed
 * by pixel data in one bus transaction; it must complete before returning.
 * RGB565 input must already be in the byte order expected by the panel. */
typedef struct {
    int (*tx_param)(void *io, uint8_t command, const uint8_t *data, size_t length);
    int (*tx_color)(void *io, uint8_t command, const void *pixels, size_t length);
    void (*delay_ms)(void *io, uint32_t milliseconds);
    void (*reset)(void *io, int high);
    void *io;
    uint16_t width, height, x_gap, y_gap;
} stm_lcd_st7789_config_t;
typedef struct { stm_lcd_st7789_config_t config; uint8_t created; uint8_t initialized; } stm_lcd_st7789_t;
int stm_lcd_st7789_new_panel(stm_lcd_st7789_t *panel, const stm_lcd_st7789_config_t *config);
int stm_lcd_st7789_reset(stm_lcd_st7789_t *panel);
int stm_lcd_st7789_init(stm_lcd_st7789_t *panel);
/* Rectangle end coordinates are exclusive; pixels are packed RGB565. */
int stm_lcd_st7789_draw_bitmap(stm_lcd_st7789_t *panel, uint16_t x1, uint16_t y1,
                           uint16_t x2, uint16_t y2, const void *pixels);
int stm_lcd_st7789_display_on_off(stm_lcd_st7789_t *panel, int on);
#ifdef __cplusplus
}
#endif
#endif
