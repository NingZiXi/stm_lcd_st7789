/**
 * @file test_panel.c
 * @brief 验证通用接口和芯片协议。
 */
#include "stm_lcd_st7789.h"
#include "test_allocator.h"
#include "stm_lcd_impl.h"
#include <assert.h>
#include <string.h>

typedef struct
{
    stm_err_t color_error; // 注入像素错误。
    unsigned commands, colors, fail_at, reset_calls, reset_fail_at,
        delays;                                  // 协议与生命周期调用计数。
    uint32_t delay_values[16];                   // 延时与复位电平记录。
    int reset_values[16];                        // 复位电平记录。
    size_t bytes;                                // 像素字节数与窗口记录。
    uint8_t xwindow[4], ywindow[4], first, last; // 窗口参数与像素首末字节。
} mock_t;

static stm_err_t cmd(void *io, uint8_t command, const uint8_t *data, size_t len)
{
    mock_t *m = io;
    if (!m->commands)
    {
        m->first = command;
    }
    ++m->commands;
    m->last = command;
    if (command == 0x2a && len == 4)
    {
        memcpy(m->xwindow, data, 4);
    }
    if (command == 0x2b && len == 4)
    {
        memcpy(m->ywindow, data, 4);
    }
    return m->commands == m->fail_at ? STM_ERR_TIMEOUT : STM_OK;
}

static stm_err_t color(void *io, uint8_t command, const void *data, size_t len)
{
    mock_t *m = io;
    assert(command == 0x2c && data);
    ++m->colors;
    m->bytes = len;
    return m->color_error;
}

static void delay(void *io, uint32_t ms)
{
    mock_t *m = io;
    if (m->delays < 16)
    {
        m->delay_values[m->delays] = ms;
    }
    ++m->delays;
}

static stm_err_t reset(void *io, int high)
{
    mock_t *m = io;
    if (m->reset_calls < 16)
    {
        m->reset_values[m->reset_calls] = high;
    }
    ++m->reset_calls;
    return m->reset_calls == m->reset_fail_at ? STM_ERR_IO : STM_OK;
}

static const stm_lcd_io_ops_t mock_ops = {.tx_param = cmd, .tx_color = color};
static const stm_lcd_io_ops_t invalid_ops = {0};

int main(void)
{
    struct stm_lcd_io io = {0}, io2 = {0}, invalid_io = {0};

    mock_t m = {0}, m2 = {0};
    stm_lcd_panel_handle_t p = NULL, other = NULL;
    assert(stm_lcd_io_init(&io, &mock_ops, &m) == STM_OK);
    assert(stm_lcd_io_init(&io2, &mock_ops, &m2) == STM_OK);
    assert(stm_lcd_io_init(&invalid_io, &invalid_ops, &m) == STM_OK);
    lcd_st7789_config_t cfg = {.delay_ms = delay,
                               .reset = reset,
                               .io = &io,
                               .control_context = &m,
                               .width = 240,
                               .height = 320,
                               .x_gap = 2,
                               .y_gap = 3},
                        bad;
    assert(lcd_st7789_create(NULL, &p) == STM_ERR_INVALID_ARG && !p);
    assert(lcd_st7789_create(&cfg, NULL) == STM_ERR_INVALID_ARG);
    assert(stm_lcd_panel_delete(NULL) == STM_ERR_INVALID_ARG);
    assert(stm_lcd_panel_delete(&p) == STM_OK && !p);
    assert(stm_lcd_panel_init(NULL) == STM_ERR_INVALID_ARG);
    assert(stm_lcd_panel_reset(NULL) == STM_ERR_INVALID_ARG);
    assert(stm_lcd_panel_display_on_off(NULL, 0) == STM_ERR_INVALID_ARG);
    bad = cfg;
    bad.io = &invalid_io;
    assert(lcd_st7789_create(&bad, &p) == STM_ERR_INVALID_CONFIG && !p);
    bad = cfg;
    bad.delay_ms = NULL;
    assert(lcd_st7789_create(&bad, &p) == STM_ERR_INVALID_CONFIG && !p);
    bad = cfg;
    bad.width = 0;
    assert(lcd_st7789_create(&bad, &p) == STM_ERR_INVALID_CONFIG);
    bad = cfg;
    bad.x_gap = 65535;
    assert(lcd_st7789_create(&bad, &p) == STM_ERR_INVALID_CONFIG);
    test_alloc_fail = 1;
    assert(lcd_st7789_create(&cfg, &p) == STM_ERR_NO_MEM && !p && !test_alloc_live);
    test_alloc_fail = 0;
    assert(lcd_st7789_create(&cfg, &p) == STM_OK && p && m.commands == 0 && m.reset_calls == 0);
    stm_lcd_panel_handle_t saved = p;
    unsigned allocations = test_alloc_calls;
    assert(lcd_st7789_create(&cfg, &p) == STM_ERR_INVALID_STATE && p == saved &&
           allocations == test_alloc_calls);
    assert(stm_lcd_panel_display_on_off(p, 1) == STM_ERR_INVALID_STATE);
    assert(stm_lcd_panel_display_on_off(p, 2) == STM_ERR_INVALID_ARG);
    cfg.io = &io2;
    cfg.control_context = &m2;
    assert(lcd_st7789_create(&cfg, &other) == STM_OK && other != p && test_alloc_live == 2);
    cfg.io = NULL; /* 创建复制配置，后续修改不影响实例。 */
    assert(stm_lcd_panel_reset(p) == STM_OK);
    assert(m.reset_calls == 3 && m.delays == 3);
    assert(m.reset_values[0] == 1 && m.reset_values[1] == 0 && m.reset_values[2] == 1);
    assert(m.delay_values[0] == 10 && m.delay_values[1] == 50 && m.delay_values[2] == 200);
    m.fail_at = 3;
    assert(stm_lcd_panel_init(p) == STM_ERR_TIMEOUT && m.commands == 3);
    assert(stm_lcd_panel_display_on_off(p, 1) == STM_ERR_INVALID_STATE);
    m.fail_at = 0;
    m.commands = 0;
    assert(stm_lcd_panel_init(p) == STM_OK && m.commands > 5 && m.last == 0x29);
    assert(m2.commands == 0 && stm_lcd_panel_display_on_off(other, 1) == STM_ERR_INVALID_STATE);
    assert(stm_lcd_panel_init(other) == STM_OK);
    assert(stm_lcd_panel_display_on_off(p, 0) == STM_OK && m.last == 0x28);
    m.fail_at = m.commands + 1;
    assert(stm_lcd_panel_display_on_off(p, 1) == STM_ERR_TIMEOUT);
    m.fail_at = 0;
    uint8_t pixels[12] = {0};
    unsigned before = m.commands;
    assert(stm_lcd_panel_draw_bitmap(p, 0, 0, 2, 3, pixels) == STM_OK && m.colors == 1 &&
           m.bytes == 12);
    assert(m.xwindow[1] == 2 && m.xwindow[3] == 3 && m.ywindow[1] == 3 && m.ywindow[3] == 5);
    assert(stm_lcd_panel_draw_bitmap(p, 0, 0, 241, 3, pixels) == STM_ERR_OUT_OF_RANGE);
    assert(stm_lcd_panel_draw_bitmap(p, 1, 0, 1, 3, pixels) == STM_ERR_INVALID_ARG);
    assert(stm_lcd_panel_draw_bitmap(p, 0, 0, 1, 1, NULL) == STM_ERR_INVALID_ARG);
    assert(m.commands == before + 2);
    m.fail_at = m.commands + 2;
    before = m.commands;
    assert(stm_lcd_panel_draw_bitmap(p, 0, 0, 1, 1, pixels) == STM_ERR_TIMEOUT);
    assert(m.commands == before + 2 && m.colors == 1);
    m.fail_at = 0;
    assert(stm_lcd_panel_draw_bitmap(p, 239, 319, 240, 320, pixels) == STM_OK && m.bytes == 2);
    m.color_error = STM_ERR_CANCELLED;
    assert(stm_lcd_panel_draw_bitmap(p, 0, 0, 1, 1, pixels) == STM_ERR_CANCELLED);
    m.color_error = STM_OK;
    assert(stm_lcd_panel_reset(p) == STM_OK);
    assert(stm_lcd_panel_draw_bitmap(p, 0, 0, 1, 1, pixels) == STM_ERR_INVALID_STATE);
    assert(stm_lcd_panel_init(p) == STM_OK);
    m.reset_fail_at = m.reset_calls + 2;
    assert(stm_lcd_panel_reset(p) == STM_ERR_IO);
    assert(stm_lcd_panel_display_on_off(p, 1) == STM_ERR_INVALID_STATE);
    m.reset_fail_at = 0;
    assert(stm_lcd_panel_reset(p) == STM_OK && stm_lcd_panel_init(p) == STM_OK);
    assert(stm_lcd_panel_delete(&p) == STM_OK && !p && test_alloc_live == 1);
    assert(stm_lcd_panel_delete(&p) == STM_OK);
    cfg.io = &io;
    cfg.control_context = &m;
    assert(lcd_st7789_create(&cfg, &p) == STM_OK);
    assert(stm_lcd_panel_delete(&p) == STM_OK && stm_lcd_panel_delete(&other) == STM_OK &&
           !test_alloc_live);
    return 0;
}
