# ST7789 STM32 HAL 接入示例

源码为 [example.c](example.c) 和 [example.h](example.h)，使用真实 STM32H7 HAL 头文件；消费工程需先完成 SPI、CS/DC、逻辑尺寸/窗口偏移、可选 RST。 示例不猜引脚、时钟或屏幕通道。

## 添加与启动

将 `example.c` 编入消费工程，并添加本目录 include，链接 `stm_lcd_st7789` 与实际 HAL target。组件间依赖按[组件 README](../../README.md)设置，使用本次迁移匹配的本地框架。

```c
static lcd_st7789_example_board_t board; // 初始化硬件字段；内嵌 IO 首次必须为零。
static stm_lcd_panel_handle_t device = NULL;
// 填写 board 的 HAL、引脚、尺寸等实际配置后：
stm_err_t err = lcd_st7789_example_start(&device, &board);
// 成功时把通用句柄传给 port；板级 board 不得离开作用域。
```

`example_start` 建立通用 IO、创建设备、可选硬复位；面板还执行 init。失败回收本次拥有的设备和 IO，HAL/引脚/帧缓冲归应用。HAL_TIMEOUT → TIMEOUT，HAL_ERROR/HAL_BUSY → IO，驱动不吞错误。

`example_start` 初始化后发送 2×2 红/绿/蓝/白紧密像素，测试 SPI 字节顺序。示例同步发送，不包含 DMA。LVGL 使用 PARTIAL，必要时设置 `rgb565_swap=1`，不要在驱动再次交换。

## 接入同一个 LVGL port 与退出

port 配置直接填写 `.io`、`.panel`、可选 `.touch`、外部缓冲和时钟，不增加芯片专用 LVGL 绘图/输入包装。删除顺序：先 `lvgl_port_delete(&port)`，然后 `lcd_st7789_example_stop(&device, &board)`；该函数删除设备后 deinit 内嵌 IO，不释放 HAL。在途/被借用/停止失败时保留资源，继续服务并重试，不清零有效对象。

主机测试与真实 HAL 示例编译属于软件验证。此次迁移不烧录，不把旧 tag 的板测结论作为本示例硬件验收。
