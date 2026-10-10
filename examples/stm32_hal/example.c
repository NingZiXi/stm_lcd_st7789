/**
 * @file example.c
 * @brief SPI 板级 IO 与通用面板接入。
 */
#include "example.h"
#include <limits.h>

static stm_err_t hal_result(HAL_StatusTypeDef status)
{
    if (status == HAL_OK)
    {
        return STM_OK;
    }
    return status == HAL_TIMEOUT ? STM_ERR_TIMEOUT : STM_ERR_IO;
}

static stm_err_t send_all(SPI_HandleTypeDef *spi, const void *data, size_t length)
{
    const uint8_t *bytes = data;
    while (length)
    {
        uint16_t n = (uint16_t)(length > UINT16_MAX ? UINT16_MAX : length);
        stm_err_t err = hal_result(HAL_SPI_Transmit(spi, (uint8_t *)bytes, n, 1000u));
        if (err != STM_OK)
        {
            return err;
        }
        bytes += n;
        length -= n;
    }
    return STM_OK;
}

static stm_err_t transmit(void *io, uint8_t command, const void *data, size_t length)
{
    lcd_st7789_example_board_t *board = io;
    // 共享 SPI 时在整笔事务外加锁，错误路径同样释放 CS 和锁。
    HAL_GPIO_WritePin(board->cs_port, board->cs_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(board->dc_port, board->dc_pin, GPIO_PIN_RESET);
    stm_err_t err = send_all(board->spi, &command, 1);
    if (err == STM_OK && length)
    {
        HAL_GPIO_WritePin(board->dc_port, board->dc_pin, GPIO_PIN_SET);
        err = send_all(board->spi, data, length);
    }
    HAL_GPIO_WritePin(board->cs_port, board->cs_pin, GPIO_PIN_SET);
    return err;
}

static stm_err_t tx_param(void *io, uint8_t cmd, const uint8_t *data, size_t len)
{
    return transmit(io, cmd, data, len);
}

static stm_err_t tx_color(void *io, uint8_t cmd, const void *pixels, size_t len)
{
    return transmit(io, cmd, pixels, len);
}

static void delay_ms(void *io, uint32_t ms)
{
    (void)io;
    HAL_Delay(ms);
}

static stm_err_t reset(void *io, int high)
{
    lcd_st7789_example_board_t *board = io;
    HAL_GPIO_WritePin(board->rst_port, board->rst_pin, high ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return STM_OK;
}

static const stm_lcd_io_ops_t io_ops = {.tx_param = tx_param, .tx_color = tx_color};

stm_err_t lcd_st7789_example_start(stm_lcd_panel_handle_t *panel, lcd_st7789_example_board_t *board)
{
    if (!panel || !board)
    {
        return STM_ERR_INVALID_ARG;
    }
    if (*panel || board->io.ops)
    {
        return STM_ERR_INVALID_STATE;
    }
    if (!board->spi || !board->cs_port || !board->dc_port || board->width < 2 || board->height < 2)
    {
        return STM_ERR_INVALID_CONFIG;
    }
    static const uint8_t pixels[] = {0xf8, 0x00, 0x07, 0xe0, 0x00, 0x1f, 0xff, 0xff};
    const lcd_st7789_config_t cfg = {
        .io = &board->io,
        .control_context = board,
        .delay_ms = delay_ms,
        .reset = board->rst_port ? reset : NULL,
        .width = board->width,
        .height = board->height,
        .x_gap = board->x_gap,
        .y_gap = board->y_gap,
    };
    stm_err_t err = stm_lcd_io_init(&board->io, &io_ops, board);
    if (err == STM_OK)
    {
        err = lcd_st7789_create(&cfg, panel);
    }
    if (err == STM_OK && board->rst_port)
    {
        err = stm_lcd_panel_reset(*panel);
    }
    if (err == STM_OK)
    {
        err = stm_lcd_panel_init(*panel);
    }
    if (err == STM_OK)
    {
        err = stm_lcd_panel_draw_bitmap(*panel, 0, 0, 2, 2, pixels);
    }
    if (err != STM_OK)
    {
        (void)stm_lcd_panel_delete(panel);
        if (board->io.ops)
        {
            (void)stm_lcd_io_deinit(&board->io);
        }
    }
    return err;
}

stm_err_t lcd_st7789_example_stop(stm_lcd_panel_handle_t *panel, lcd_st7789_example_board_t *board)
{
    if (!panel || !board)
    {
        return STM_ERR_INVALID_ARG;
    }
    stm_err_t err = stm_lcd_panel_delete(panel);
    if (err == STM_OK && board->io.ops)
    {
        err = stm_lcd_io_deinit(&board->io);
    }
    return err;
}
