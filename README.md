# stm_lcd_st7789

ST7789 SPI 屏幕芯片驱动。库不初始化 STM32 HAL、SPI、GPIO 或 LVGL；板级代码传入 IO、延时和可选复位回调。它只负责 ST7789 命令、坐标窗口和 RGB565 绘图，不与其他屏幕芯片共用一个实现。完整的 CubeMX、HAL SPI、LVGL 接入示例见[显示与触摸接入指南](https://github.com/NingZiXi/stm32-hal-lib/blob/main/docs/display-components.md)。

```c
stm_lcd_st7789_t panel = {0};
stm_lcd_st7789_config_t cfg = {
    .tx_param = board_tx_param, .tx_color = board_tx_color,
    .reset = board_reset, .delay_ms = board_delay_ms, .io = &board_io,
    .width = BOARD_LCD_WIDTH, .height = BOARD_LCD_HEIGHT,
    .x_gap = 0, .y_gap = 0,
};
int rc = stm_lcd_st7789_new_panel(&panel, &cfg);
if (rc == 0) rc = stm_lcd_st7789_reset(&panel);
if (rc == 0) rc = stm_lcd_st7789_init(&panel);
if (rc == 0) rc = stm_lcd_st7789_draw_bitmap(&panel, 0, 0, 1, 1, rgb565_pixel);
```

`tx_param(io, command, data, length)` 控制 DC 和 CS，发送命令与可选参数；`tx_color(io, command, pixels, length)` 在**同一笔独占事务**中发送 RAMWR 命令和全部像素，`length` 是**字节数**。回调返回 0 成功，非 0 失败，必须在返回前使用完输入缓冲。`x2/y2` 是**不包含**的终点；像素按行紧密排列，线上 RGB565 字节顺序由板级代码负责。初始化失败、参数不合法返回 -1，总线失败返回 -2。默认初始化表参考慧勤智远 STM32H757 实验 50，实际屏幕的分辨率、MADCTL、色序与偏移必须实板核对，目前仅通过主机测试、尚未完成屏幕实板测试。

```cmake
add_subdirectory(Lib/stm_lcd_st7789)
target_link_libraries(app PRIVATE stm_lcd_st7789) # app 改为你的实际目标名
```
