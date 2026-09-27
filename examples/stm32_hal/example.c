#include "example.h"
#include <limits.h>

/* 阻塞发送；SPI 需配置为 8-bit，发送完成后才能释放 CS 或复用像素缓冲。 */
static int send_all(SPI_HandleTypeDef *spi, const void *data, size_t length)
{
    const uint8_t *bytes = (const uint8_t *)data;
    while (length != 0u) {
        uint16_t n = (uint16_t)(length > UINT16_MAX ? UINT16_MAX : length);
        if (HAL_SPI_Transmit(spi, (uint8_t *)bytes, n, 1000u) != HAL_OK) return -1;
        bytes += n;
        length -= n;
    }
    return 0;
}

static int transmit(void *io, uint8_t command, const void *data, size_t length)
{
    stm_lcd_st7789_example_board_t *board = (stm_lcd_st7789_example_board_t *)io;
    int result;
    HAL_GPIO_WritePin(board->cs_port, board->cs_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(board->dc_port, board->dc_pin, GPIO_PIN_RESET);
    result = send_all(board->spi, &command, 1u);
    if (result == 0 && length != 0u) {
        HAL_GPIO_WritePin(board->dc_port, board->dc_pin, GPIO_PIN_SET);
        result = send_all(board->spi, data, length);
    }
    HAL_GPIO_WritePin(board->cs_port, board->cs_pin, GPIO_PIN_SET);
    return result;
}

static int tx_param(void *io, uint8_t command, const uint8_t *data, size_t length)
{
    return transmit(io, command, data, length);
}

static int tx_color(void *io, uint8_t command, const void *pixels, size_t length)
{
    /* RAMWR 命令与所有像素共用一次 CS 有效期。共享 SPI 时在 transmit 两侧加锁。 */
    return transmit(io, command, pixels, length);
}

static void delay_ms(void *io, uint32_t ms)
{
    (void)io;
    HAL_Delay(ms);
}

static void reset(void *io, int high)
{
    stm_lcd_st7789_example_board_t *board = (stm_lcd_st7789_example_board_t *)io;
    HAL_GPIO_WritePin(board->rst_port, board->rst_pin, high ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

int stm_lcd_st7789_example_start(stm_lcd_st7789_t *panel, stm_lcd_st7789_example_board_t *board)
{
    /* RGB565，大端字节序：红、绿、蓝、白的 2x2 测试块。 */
    static const uint8_t pixels[] = { 0xF8, 0x00, 0x07, 0xE0, 0x00, 0x1F, 0xFF, 0xFF };
    stm_lcd_st7789_config_t cfg;
    int result;
    if (!panel || !board || !board->spi || !board->cs_port || !board->dc_port ||
        board->width < 2u || board->height < 2u) return -1;
    HAL_GPIO_WritePin(board->cs_port, board->cs_pin, GPIO_PIN_SET);
    cfg = (stm_lcd_st7789_config_t){
        .tx_param = tx_param, .tx_color = tx_color, .delay_ms = delay_ms,
        .reset = board->rst_port ? reset : NULL, .io = board,
        .width = board->width, .height = board->height,
        .x_gap = board->x_gap, .y_gap = board->y_gap,
    };
    result = stm_lcd_st7789_new_panel(panel, &cfg);
    if (result == 0) result = stm_lcd_st7789_reset(panel);
    if (result == 0) result = stm_lcd_st7789_init(panel);
    if (result == 0) result = stm_lcd_st7789_draw_bitmap(panel, 0u, 0u, 2u, 2u, pixels);
    return result;
}
