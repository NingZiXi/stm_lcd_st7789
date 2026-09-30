/** @file stm_lcd_st7789.h @brief 芯片独立接口，统一错误码与不透明句柄。 */
#ifndef STM_LCD_ST7789_H
#define STM_LCD_ST7789_H
#include <stddef.h>
#include <stdint.h>
#include "stm_err.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct lcd_st7789_context *lcd_st7789_handle_t;
typedef struct {
    stm_err_t (*tx_param)(void *io, uint8_t command, const uint8_t *data, size_t length); /**< 同步传输；length 是字节数，0 时 data 可空。 */
    stm_err_t (*tx_color)(void *io, uint8_t command, const void *pixels, size_t length); /**< RAMWR 与像素在一笔独占事务内同步完成。 */
    void (*delay_ms)(void *io, uint32_t milliseconds); /**< 必需，阻塞延时。 */
    stm_err_t (*reset)(void *io, int high); /**< 可选，设置 RST 电平，原样传递错误。 */
    void *io; /**< 借用上下文，允许 NULL。 */
    uint16_t width, height, x_gap, y_gap; /**< 逻辑尺寸与模组偏移，地址范围不得溢出 16 位。 */
} lcd_st7789_config_t;
/**
 * @brief 创建控制对象，不初始化总线或接管板级资源。
 * @param config 配置被复制，io/回调上下文与外部缓冲必须在实例存续期有效。
 * @param out 输出句柄地址；*out 必须为 NULL，非空返回 INVALID_STATE 并保持原值。
 * @return STM_OK、INVALID_ARG、INVALID_CONFIG 或 NO_MEM；失败不改变 *out。
 * @note 芯片 create 不访问硬件。使用 calloc/free，仅允许应用串行线程调用。
 */
stm_err_t lcd_st7789_create(const lcd_st7789_config_t *config, lcd_st7789_handle_t *out);
/** @brief 仅释放拥有的控制对象；成功清空 *handle，空句柄也成功。
 * @note 调用前停止并发访问；其他别名不自动清空，删除后禁止使用。
 */
stm_err_t lcd_st7789_delete(lcd_st7789_handle_t *handle);
/** @brief 执行可选硬件复位；开始后进入未初始化状态，失败可重试。 */
stm_err_t lcd_st7789_reset(lcd_st7789_handle_t panel);
/** @brief 执行既有模组命令表；失败保持未初始化，调用者可重新初始化。
 * @note init 不隐式复位，接入时按 reset -> init 调用；不自动重试。
 */
stm_err_t lcd_st7789_init(lcd_st7789_handle_t panel);
/** @brief 发送开关显示命令；on 仅允许 0/1，要求初始化成功。 */
stm_err_t lcd_st7789_display_on_off(lcd_st7789_handle_t panel, int on);
/** @brief 同步绘制 [x1,x2)×[y1,y2)，每像素 2 字节、按行紧密排列。
 * @param pixels 调用者保证至少 (x2-x1)*(y2-y1)*2 字节，板级负责线上 RGB565 字节序。
 * @return 空/倒置矩形 INVALID_ARG，越界 OUT_OF_RANGE，未初始化 INVALID_STATE，传输错误原样返回。
 * @note 失败可能已写入部分像素，不回滚、不自动重试；实例保留可重试。DMA/缓存由板级管理。
 */
stm_err_t lcd_st7789_draw_bitmap(lcd_st7789_handle_t panel, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, const void *pixels);

#ifdef __cplusplus
}
#endif
#endif
