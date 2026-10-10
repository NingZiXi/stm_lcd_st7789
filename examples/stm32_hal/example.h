/**
 * @file example.h
 * @brief SPI 接入，HAL 与 GPIO 由应用初始化。
 */
#ifndef STM_LCD_ST7789_EXAMPLE_H
#define STM_LCD_ST7789_EXAMPLE_H
#include "stm32h7xx_hal.h"
#include "stm_lcd_st7789.h"
#include "stm_lcd_impl.h"

typedef struct
{
    struct stm_lcd_io io;             // 板级持有，首次使用必须清零。
    SPI_HandleTypeDef *spi;           // 借用的已初始化 HAL。
    GPIO_TypeDef *cs_port;            // SPI 片选。
    GPIO_TypeDef *dc_port;            // 命令和数据选择。
    GPIO_TypeDef *rst_port;           // 可选复位口。
    uint16_t cs_pin, dc_pin, rst_pin; // 对应 GPIO 引脚。
    uint16_t width, height;           // 可见区域尺寸。
    uint16_t x_gap, y_gap;            // 模组窗口偏移。
} lcd_st7789_example_board_t;

/**
 * @brief 建立板级 IO 并创建、复位设备，面板完成初始化。
 * @param device 初始为空的通用句柄地址
 * @param board 已清零 IO 且 HAL 已初始化的板级配置，须持续有效
 * @return STM_OK；失败回收本次创建资源，底层错误原样传递。
 */
stm_err_t lcd_st7789_example_start(stm_lcd_panel_handle_t *device,
                                   lcd_st7789_example_board_t *board);
/**
 * @brief 删除设备后清除板级 IO，不释放 HAL 或帧缓冲。
 * @param device 已先删除 port 的句柄地址，成功清空
 * @param board 创建时使用的板级配置
 * @return STM_OK；在途、被借用或停止失败时保留资源供重试。
 */
stm_err_t lcd_st7789_example_stop(stm_lcd_panel_handle_t *device,
                                  lcd_st7789_example_board_t *board);
#endif
