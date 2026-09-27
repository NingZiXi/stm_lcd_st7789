# stm_lcd_st7789：STM32 HAL SPI 接入示例

本目录的 `example.h` / `example.c` 可直接复制到 CubeMX 生成的 **CM7 用户代码目录**，也可把 `example.c` 加入 CM7 目标源码。它演示阻塞 SPI、CS/DC、可选复位、芯片初始化与左上角 2×2 RGB565 测试块；**尚未经过实际屏幕验证**。不是完整的 CubeMX 工程，不包含 ST HAL/CMSIS。其他 MCU 系列请把 `stm32h7xx_hal.h` 换成对应 HAL 头文件。

1. 根据实物确认芯片确为 ST7789、分辨率、供电、CS/DC/RST 接线、颜色顺序和偏移；先在 CubeMX 初始化 SPI（8-bit）和 GPIO，软件控制 CS/DC。
2. 添加本组件并链接 `stm_lcd_st7789`，把 `example.c` 加入同一 CM7 应用目标。以实际生成的 SPI 句柄与引脚宏替换下列占位符；没有 RST 引脚时将 `rst_port` 设为 `NULL`。
3. 在应用入口调用（不要改写 CubeMX 自动生成区）：

```c
#include "example.h"
#include "spi.h"
#include "gpio.h"

static stm_lcd_st7789_t panel;
static stm_lcd_st7789_example_board_t panel_board;

void app_main(void)
{
    panel_board = (stm_lcd_st7789_example_board_t){
        .spi = &hspi_display, /* 换成实际 SPI 句柄 */
        .cs_port = LCD_CS_GPIO_Port, .cs_pin = LCD_CS_Pin,
        .dc_port = LCD_DC_GPIO_Port, .dc_pin = LCD_DC_Pin,
        .rst_port = LCD_RST_GPIO_Port, .rst_pin = LCD_RST_Pin,
        .width = PANEL_WIDTH, .height = PANEL_HEIGHT,
        .x_gap = 0, .y_gap = 0,
    };
    int rc = stm_lcd_st7789_example_start(&panel, &panel_board);
    if (rc != 0) { /* 用项目的日志接口记录 rc，停止后续绘图。 */ }
    /* 初始化成功后可继续调用 stm_lcd_st7789_draw_bitmap(&panel, ...)。 */
}
```

`panel_board` 与 `panel` 必须长期有效。`x2/y2` 为不包含的右下角坐标；像素采用 RGB565 高字节先发，若实物颜色反转应核对屏幕 MADCTL、模组色序和 LVGL 字节交换配置。SPI 与其他设备共用时，在 `transmit()` 中从拉低 CS 到拉高 CS 的整个事务外加互斥锁；不得从中断里使用本阻塞示例。SPI 发送失败会返回错误，不能继续绘图。背光 GPIO、供电与实际屏幕时序由板级代码实现。

在 CM7 的 `CMakeLists.txt` 把复制到 `App/` 的示例源码加入应用目标（具体目录名按工程调整）：

```cmake
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/App/example.c)
```

中文主页：[README.md](../../README.md)；完整接入说明：[显示与触摸组件接入指南](https://github.com/NingZiXi/stm32-hal-lib/blob/main/docs/display-components.md)。
