/** @file example.h @brief SPI 接入示例，HAL/GPIO 由应用初始化。 */
#ifndef STM_LCD_ST7789_EXAMPLE_H
#define STM_LCD_ST7789_EXAMPLE_H
#include "stm32h7xx_hal.h"
#include "stm_lcd_st7789.h"
typedef struct {
    SPI_HandleTypeDef *spi;
    GPIO_TypeDef *cs_port,*dc_port,*rst_port;
    uint16_t cs_pin,dc_pin,rst_pin;
    uint16_t width,height,x_gap,y_gap;
} lcd_st7789_example_board_t;
/** @brief 输出句柄须为空，失败回收新对象；board 在实例使用期间必须有效。 */
stm_err_t lcd_st7789_example_start(lcd_st7789_handle_t *panel,lcd_st7789_example_board_t *board);
#endif
