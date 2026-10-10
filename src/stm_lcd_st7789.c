/**
 * @file stm_lcd_st7789.c
 * @brief 面板命令与实例生命周期。
 */
#include "stm_lcd_st7789.h"
#include "stm_lcd_impl.h"
#include <limits.h>
#include <stdlib.h>

typedef struct
{
    uint8_t cmd, size; // 命令及参数字节数。
    uint16_t delay_ms; // 命令后延时。
    uint8_t data[14];  // 保留模组原始参数。
} init_command_t;

/* Board vendor SPI example, experiment 50; values are module-specific defaults. */
static const init_command_t init_commands[] = {
    {0x11, 0, 120, {0}},
    {0x36, 1, 0, {0x00}},
    {0x3A, 1, 0, {0x55}},
    {0xB2, 5, 0, {0x0C, 0x0C, 0x00, 0x33, 0x33}},
    {0xB7, 1, 0, {0x56}},
    {0xBB, 1, 0, {0x20}},
    {0xC0, 1, 0, {0x2C}},
    {0xC2, 1, 0, {0x01}},
    {0xC3, 1, 0, {0x0F}},
    {0xC4, 1, 0, {0x20}},
    {0xC6, 1, 0, {0x0F}},
    {0xD0, 2, 0, {0xA4, 0xA1}},
    {0xD6, 1, 0, {0xA1}},
    {0xE0,
     14,
     0,
     {0xF0, 0x00, 0x06, 0x06, 0x07, 0x05, 0x30, 0x44, 0x48, 0x38, 0x11, 0x10, 0x2E, 0x34}},
    {0xE1,
     14,
     0,
     {0xF0, 0x0A, 0x0E, 0x0D, 0x0B, 0x27, 0x2F, 0x44, 0x47, 0x35, 0x12, 0x12, 0x2C, 0x32}},
    {0x35, 1, 0, {0x00}},
    {0x21, 0, 0, {0}},
    {0x29, 0, 0, {0}},
};

struct lcd_st7789_context
{
    struct stm_lcd_panel base;  // 首成员，框架管理借用和完成。
    lcd_st7789_config_t config; // 配置副本，不拥有外部资源。
    uint8_t initialized;        // 仅完整初始化成功后就绪。
};

static stm_err_t panel_reset(stm_lcd_panel_handle_t handle)
{
    struct lcd_st7789_context *panel = (struct lcd_st7789_context *)handle;
    if (!panel)
    {
        return STM_ERR_INVALID_ARG;
    }
    panel->initialized = 0;
    if (!panel->config.reset)
    {
        return STM_ERR_NOT_SUPPORTED;
    }
    stm_err_t err;
    err = panel->config.reset(panel->config.control_context, 1);
    if (err != STM_OK)
    {
        return err;
    }
    panel->config.delay_ms(panel->config.control_context, 10);
    err = panel->config.reset(panel->config.control_context, 0);
    if (err != STM_OK)
    {
        return err;
    }
    panel->config.delay_ms(panel->config.control_context, 50);
    err = panel->config.reset(panel->config.control_context, 1);
    if (err != STM_OK)
    {
        return err;
    }
    panel->config.delay_ms(panel->config.control_context, 200);
    return STM_OK;
}

static stm_err_t panel_init(stm_lcd_panel_handle_t handle)
{
    struct lcd_st7789_context *panel = (struct lcd_st7789_context *)handle;
    if (!panel)
    {
        return STM_ERR_INVALID_ARG;
    }
    panel->initialized = 0;
    stm_err_t err;
    for (size_t i = 0; i < sizeof(init_commands) / sizeof(init_commands[0]); ++i)
    {
        const init_command_t *op = &init_commands[i];
        err = stm_lcd_io_tx_param(panel->base.io, op->cmd, op->data, op->size);
        if (err != STM_OK)
        {
            return err;
        }
        if (op->delay_ms)
        {
            panel->config.delay_ms(panel->config.control_context, op->delay_ms);
        }
    }
    panel->initialized = 1;
    return STM_OK;
}

static stm_err_t panel_display_on_off(stm_lcd_panel_handle_t handle, int on)
{
    struct lcd_st7789_context *panel = (struct lcd_st7789_context *)handle;
    if (!panel || (on != 0 && on != 1))
    {
        return STM_ERR_INVALID_ARG;
    }
    if (!panel->initialized)
    {
        return STM_ERR_INVALID_STATE;
    }
    return stm_lcd_io_tx_param(panel->base.io, on ? 0x29 : 0x28, NULL, 0);
}

static stm_err_t panel_draw_bitmap(stm_lcd_panel_handle_t handle,
                                   uint16_t x1,
                                   uint16_t y1,
                                   uint16_t x2,
                                   uint16_t y2,
                                   const void *pixels)
{
    struct lcd_st7789_context *panel = (struct lcd_st7789_context *)handle;
    if (!panel || !pixels || x1 >= x2 || y1 >= y2)
    {
        return STM_ERR_INVALID_ARG;
    }
    if (x2 > panel->config.width || y2 > panel->config.height)
    {
        return STM_ERR_OUT_OF_RANGE;
    }
    size_t width = x2 - x1, height = y2 - y1;
    if (width > SIZE_MAX / height / 2u)
    {
        return STM_ERR_OUT_OF_RANGE;
    }
    if (!panel->initialized)
    {
        return STM_ERR_INVALID_STATE;
    }
    uint32_t xa = (uint32_t)x1 + panel->config.x_gap, xb = (uint32_t)x2 - 1u + panel->config.x_gap;
    uint32_t ya = (uint32_t)y1 + panel->config.y_gap, yb = (uint32_t)y2 - 1u + panel->config.y_gap;
    uint8_t x[4] = {(uint8_t)(xa >> 8), (uint8_t)xa, (uint8_t)(xb >> 8), (uint8_t)xb};
    uint8_t y[4] = {(uint8_t)(ya >> 8), (uint8_t)ya, (uint8_t)(yb >> 8), (uint8_t)yb};
    stm_err_t err = stm_lcd_io_tx_param(panel->base.io, 0x2A, x, sizeof x);
    if (err != STM_OK)
    {
        return err;
    }
    err = stm_lcd_io_tx_param(panel->base.io, 0x2B, y, sizeof y);
    if (err != STM_OK)
    {
        return err;
    }
    return stm_lcd_io_tx_color(panel->base.io, 0x2C, pixels, width * height * 2u);
}

static void panel_destroy(stm_lcd_panel_handle_t handle)
{
    free(handle);
}

static const stm_lcd_panel_ops_t panel_ops = {.reset = panel_reset,
                                              .init = panel_init,
                                              .display_on_off = panel_display_on_off,
                                              .draw = panel_draw_bitmap,

                                              .destroy = panel_destroy};

stm_err_t lcd_st7789_create(const lcd_st7789_config_t *config, stm_lcd_panel_handle_t *out)
{
    if (!config || !out)
    {
        return STM_ERR_INVALID_ARG;
    }
    if (*out)
    {
        return STM_ERR_INVALID_STATE;
    }
    if (!config->io || !config->io->ops || !config->io->ops->tx_param || !config->delay_ms ||
        !config->width || !config->height || !config->io->ops->tx_color ||
        (uint32_t)config->width + config->x_gap > 65536u ||
        (uint32_t)config->height + config->y_gap > 65536u)
    {
        return STM_ERR_INVALID_CONFIG;
    }
    struct lcd_st7789_context *panel = calloc(1, sizeof(*panel));
    if (!panel)
    {
        return STM_ERR_NO_MEM;
    }
    panel->config = *config;
    stm_err_t err = stm_lcd_panel_base_init(&panel->base, &panel_ops, config->io, config->width,
                                            config->height);
    if (err != STM_OK)
    {
        free(panel);
        return err;
    }
    *out = &panel->base;
    return STM_OK;
}
