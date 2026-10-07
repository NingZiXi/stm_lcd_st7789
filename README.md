# stm_lcd_st7789：ST7789 SPI 面板驱动

按芯片独立维护命令表、窗口和 RGB565 绘图，HAL SPI/GPIO 与线上字节顺序由板级负责。初始化默认值来自厂商实验 50，尺寸、偏移、MADCTL 和颜色须按实际模组核实。

## 最小调用

```c
lcd_st7789_handle_t panel = NULL;
lcd_st7789_config_t cfg = {
    .tx_param=board_tx_param, .tx_color=board_tx_color,
    .delay_ms=board_delay_ms, .reset=board_reset, .io=&board_io,
    .width=BOARD_LCD_WIDTH, .height=BOARD_LCD_HEIGHT, .x_gap=0, .y_gap=0,
};
stm_err_t err = lcd_st7789_create(&cfg, &panel);
if (err == STM_OK) err = lcd_st7789_reset(panel);
if (err == STM_OK) err = lcd_st7789_init(panel);
if (err == STM_OK) err = lcd_st7789_draw_bitmap(panel, 0, 0, 1, 1, rgb565_pixel);
if (err != STM_OK) lcd_st7789_delete(&panel);
/* 使用结束后同样调用 lcd_st7789_delete(&panel)。 */
```

## 错误与资源契约

所有操作和传输/复位回调返回 `stm_err_t`，成功为 `STM_OK`，失败检查 `err != STM_OK`，不能使用 `err < 0`。HAL 适配将 `HAL_TIMEOUT` 映射为 `STM_ERR_TIMEOUT`，`HAL_ERROR/HAL_BUSY` 映射为 `STM_ERR_IO`；组件原样传递回调错误，延时回调仍返回 void。

| 情况 | 错误 |
| --- | --- |
| 空参数、非法调用参数 | `STM_ERR_INVALID_ARG` |
| 缺少必需回调、尺寸或方向配置错误 | `STM_ERR_INVALID_CONFIG` |
| 输出句柄非空、面板未初始化 | `STM_ERR_INVALID_STATE` |
| 控制对象/LVGL 对象分配失败 | `STM_ERR_NO_MEM` |
| 绘图越界或像素长度计算溢出 | `STM_ERR_OUT_OF_RANGE` |
| GT9271 ID 不匹配 | `STM_ERR_NOT_SUPPORTED` |
| 触摸帧点数等数据校验失败 | `STM_ERR_VERIFY` |

`create(config, &handle)` 要求 handle 初始为 NULL；复制配置，用 calloc/free 管理小型控制对象，芯片 create 不访问硬件。创建失败保持输出为空；非空输出被拒绝且原值不变。`delete(&handle)` 仅回收拥有的对象，成功清空 handle，空句柄也成功；NULL 句柄地址是参数错误。删除前停止并发访问，其他别名不会被自动清空。

板级拥有 HAL、总线、GPIO、背光、外部缓冲和回调上下文；组件不释放或重新配置这些资源。实例使用期间上下文必须有效，可用 NULL io 表示无上下文。应用串行调用，组件不默认线程安全，不在中断中调用阻塞操作，不增加日志/RTT/RTOS 依赖。同步传输返回前必须用完输入缓冲；共享总线在整笔事务外加锁，DMA/DCache 一致性由板级管理。

## 绘图与失败状态

`tx_param(io, command, data, length)` 和 `tx_color` 长度均为字节；tx_color 必须在一笔独占事务中发送 RAMWR 与全部像素。绘图矩形为 `[x1,x2)×[y1,y2)`，RGB565 紧密按行排列，调用者提供足量像素，组件不转换字节序。

复位/初始化开始后未就绪，初始化成功才可绘图或开关显示。参数校验失败不访问硬件或改变实例。传输失败立即停止后续命令，可能已有部分像素改变，不回滚、不自动重试；保留实例供重试或重新初始化。无 reset 回调时 reset 仅使实例未就绪，板级必须保证实际硬件状态。

## CMake 与依赖

依赖 `stm_common` 的 `stm_err.h`，不复制公共错误码。优先复用已有 `stm_common` target，其次找同级源码；缺失时自动下载固定 v1.0.0 提交 `ce3d186dde2d374a8e9c7b9068a7b88f97d57dc1`。可设置 `STM_COMMON_FETCH=OFF` 禁止下载，`STM_COMMON_GIT_REPOSITORY=https://gitee.com/nzxhg/stm_common.git` 指定镜像，或 `FETCHCONTENT_SOURCE_DIR_STM_COMMON` 指定离线源码。已有 target/同级源码无需网络。

```cmake
add_subdirectory(Lib/stm_lcd_st7789)
target_link_libraries(your_firmware PRIVATE stm_lcd_st7789)
```

手动集成时添加组件 include/源码及 stm_common 头文件目录。LVGL port 还要求应用提前提供 LVGL 9 的 `lvgl` target 和配置。

## 从 v0.1.0 迁移

| 旧接口 | 当前接口 |
| --- | --- |
| `stm_lcd_st7789_t` 公开结构体 | `lcd_st7789_handle_t`，初始 NULL |
| `stm_lcd_st7789_config_t` | `lcd_st7789_config_t` |
| `new_panel(实例地址, config)` | `lcd_st7789_create(config, &handle)` |
| 直接访问结构体 / 无销毁接口 | `lcd_st7789_delete(&handle)` |
| int 与负数错误码 | `stm_err_t`，`err != STM_OK`，回调同步迁移 |

其他操作使用 lcd_<型号> 前缀。

ST7789/ST7796 仅有主机验证，尚未实板验证。

## 软件验证与发布状态

```sh
cmake -S tests -B build/tests -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/tests
ctest --test-dir build/tests --output-on-failure
```

主机测试覆盖参数/配置、分配失败、资源回收、多实例和错误传递，并编译 C11/C++17 公共头文件。测试分配器仅用于测试构建，不加入产品固件。中文 HAL 示例见 [examples/stm32_hal/README.md](examples/stm32_hal/README.md)。许可证见 [LICENSE](LICENSE)。

`v0.2.0` 采用不透明句柄、`create/delete` 和统一 `stm_err_t`，包含破坏性接口迁移，不保留旧接口包装。`v0.1.0` 继续保留；升级前按上表迁移类型、回调和生命周期。此版本的主机测试、C11/C++17 头文件、中文 HAL 示例及 H757 Debug/Release 集成构建已通过。

本轮未接入此器件实物，发布范围仅为软件契约与构建验证，硬件回归仍待完成。
