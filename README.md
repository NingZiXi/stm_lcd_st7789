# stm_lcd_st7789

Independent ST7789 SPI controller driver. It does not initialize STM32 HAL, GPIO, or LVGL. Inspired by Espressif's panel-IO + panel-driver construction: configure SPI/CS/DC/reset in the BSP, pass synchronous `tx_param` and `tx_color(command, pixels, bytes)` callbacks, call `new_panel`, `reset`, `init`, and `draw_bitmap`. The `tx_color` callback sends `0x2C` (RAMWR) and pixel bytes in one bus transaction. Each IO callback must finish using its input buffer before it returns; wait for HAL DMA completion inside `tx_color` if necessary. Pixels must be RGB565 bytes in the panel's wire order; the driver never mutates the caller's buffer. `x2/y2` are exclusive. A nonzero IO callback result yields `-2`, invalid state/argument yields `-1`.

The init commands are transcribed from the H757 board vendor's experiment 50 and are **not yet tested on hardware**. Check the exact module variant and required MADCTL, inversion, color order, and offsets before turning on the backlight. ID read is not implemented and a successful host test is not evidence of an attached panel.

```cmake
add_subdirectory(Lib/stm_lcd_st7789)
target_link_libraries(app PRIVATE stm_lcd_st7789)
```
