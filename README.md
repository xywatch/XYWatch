参考 https://watchy.sqfmi.com/docs/legacy#simple-watchface-example

caution
Some users have reported problems with one of the supported RTC modules: The module PCF8563 seems to be supported during first boots, but their library is overridden by PlatformIO using a broken version - so you need to add an other repository (https://github.com/orbitalair/Rtc_Pcf8563.git) to prevent that.

lib_deps =
    sqfmi/Watchy ;
    https://github.com/tzapu/WiFiManager.git#v2.0.11-beta ; Pinned for the same reason
lib_ldf_mode = deep+
board_build.partitions = min_spiffs.csv

不应该添加 https://github.com/orbitalair/Rtc_Pcf8563.git, 这个库后来改了, 是stm32的, 要加也是要加https://github.com/elpaso/Rtc_Pcf8563

参考:
https://docs.platformio.org/en/latest/projectconf/sections/env/options/monitor/monitor_filters.html

# BMA
https://github.com/sethitow/mbed-pinetime/blob/master/drivers/BMA42x_Accelerometer/README.md
https://github.com/frederic/elaine-linux/tree/52f272b3d184933e84c4c85cbaa7cb90fe6c0199/drivers/iio/accel


bma421: 通过bma421来计步数
https://github.com/joaquimorg/MY-Time/blob/1e436f9efe639ce9034c0bf34705f8c3842a8d3a/src/smartwatch/modules/step_count/step_count.cpp#L47


1. **firmware.bin**：
   - 最终的应用程序二进制文件
   - 直接可以用于OTA更新
   - 不包含调试信息
   - 体积较小，适合传输和存储
   - 这就是实际烧录到ESP32分区中的文件

2. **firmware.elf**：
   - 包含调试信息的可执行文件
   - 包含符号表、行号等调试信息
   - 体积比.bin大很多
   - 主要用于开发调试
   - 可以用于GDB调试、崩溃分析等

3. **bootloader.bin**：
   - ESP32的引导加载程序
   - 负责系统初始化
   - 选择启动分区
   - 加载和运行应用程序
   - 处理OTA更新的分区切换
   - 通常只需烧录一次，除非要更新bootloader本身

编译过程：
```
源代码 -> 编译 -> firmware.elf -> 提取 -> firmware.bin
                   (带调试信息)        (纯二进制)
```

使用场景：
- OTA更新时只需要传输firmware.bin
- 调试问题时需要用到firmware.elf
- bootloader.bin通常在首次烧录时写入
