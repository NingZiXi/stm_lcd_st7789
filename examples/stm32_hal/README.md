# lcd_st7789：STM32 HAL 接入示例

本目录提供可复制到已有 HAL 应用的 `example.c` / `example.h`，不是完整 CubeMX 工程。先初始化板级时钟、GPIO 和总线，再传入实际 HAL 句柄/引脚。代码使用 STM32H7 HAL，其他系列自行替换头文件；示例不会自动编入组件库。

当前示例对应未发布的新 API；软件验证通过后仍需按实物回归。ST7789/ST7796 仅有主机验证，尚未实板验证。

## 接入步骤

1. 将组件和 stm_common 加入 CMake；LVGL port 先提供 LVGL 9 target。
2. 将本目录两个源码文件复制到应用，替换 HAL 头文件和实际板级参数。
3. 以 NULL 初始化句柄，按 example.h 的 start 接口创建；板级结构体必须持久有效。
4. 循环绘图/读取触点或调用 LVGL handler，检查每一步 `err != STM_OK`。
5. 停止所有访问后调用 `lcd_st7789_delete(&handle)`。

```cmake
target_sources(your_firmware PRIVATE App/example.c)
target_include_directories(your_firmware PRIVATE App)
target_link_libraries(your_firmware PRIVATE stm_lcd_st7789)
```

HAL_TIMEOUT 映射为 STM_ERR_TIMEOUT，HAL_ERROR/HAL_BUSY 映射为 STM_ERR_IO，start 失败保留首个错误并回收本次创建的对象，重复 start 不覆盖已有句柄。传输同步完成后才能复用缓冲；阻塞 API 不从中断调用。

## SPI 板级填写

配置 8-bit SPI，按实际模组填写 SPI 模式/频率、CS/DC/RST、宽高和 x_gap/y_gap。start 的两个参数为 `lcd_st7789_handle_t *` 和 `lcd_st7789_example_board_t *`；它按 create/reset/init/draw 绘制 2×2 RGB565 红绿蓝白测试块。CS 覆盖 RAMWR 和全部像素，分段发送不会提前释放 CS；共享总线须在整个 transmit 外加锁。线上 RGB565 大端字节序由此示例的像素数组体现，不对 LVGL 数据隐式交换。

完整 API、错误和资源契约见[中文主页](../../README.md)。
