# stm_lcd_st7789：Agent 阅读入口

修改前检查本组件状态与实际提交，保留用户改动。先读 [README.md](README.md)、[公开头文件](include/stm_lcd_st7789.h)、CMake、实现与原有测试；位于聚合仓库时合并上级 AGENTS.md，并遵循其 `docs/display-development.md`。

返回 `stm_lcd` 通用句柄；对象首成员嵌入清零 base，配置复制，IO/控制上下文仅借用。构造不访问硬件，base 最后初始化，输出最后发布。参考 AXS 的结构，不复制 AXS 协议、F407 接线或时序。不能在 port 增加芯片分支、伪造可选能力成功或删除旧协议测试。

按 [.clang-format](.clang-format) 使用 Allman、4 空格。C/H 文件头仅 file/brief，公开函数头使用 Doxygen；实现不写函数 Doxygen；struct/enum 成员使用同行右侧简短 `//`。保留 LICENSE 和既有来源。

[HAL 示例](examples/stm32_hal/README.md)只演示设备与 IO 接入，硬件初始化由消费工程提供。修改后运行 README 的主机测试、C11/C++17 头文件检查及适用的真实 HAL/port 集成；报告软件和硬件边界。未经当前用户确认不烧录、推送或发布，不修改消费工程生成代码。
