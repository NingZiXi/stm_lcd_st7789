#ifndef STM_LCD_ST7789_EXAMPLE_H
#define STM_LCD_ST7789_EXAMPLE_H
#include "stm32h7xx_hal.h"
#include "stm_lcd_st7789.h"

typedef struct {
    SPI_HandleTypeDef *spi;
    GPIO_TypeDef *cs_port, *dc_port, *rst_port;
    uint16_t cs_pin, dc_pin, rst_pin;
    uint16_t width, height, x_gap, y_gap;
} stm_lcd_st7789_example_board_t;

/* board 与 panel 在驱动使用期间都必须保持有效；GPIO/SPI 已由 CubeMX 初始化。 */
int stm_lcd_st7789_example_start(stm_lcd_st7789_t *panel, stm_lcd_st7789_example_board_t *board);
#endif
