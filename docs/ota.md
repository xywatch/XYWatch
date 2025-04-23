# ESP32 OTA

## 分区布局

ESP32有两个主要的应用程序分区：`factory`和`ota`。通常是这样分布的：
- `factory`: 原始固件分区（首次烧录的程序）
- `ota_0`: 第一个OTA分区
- `ota_1`: 第二个OTA分区

## 工作原理
```
[Boot]
   |
   ├── factory (原始固件)
   |
   ├── ota_0 (OTA分区A)
   |
   └── ota_1 (OTA分区B)
```

## OTA更新流程
- 假设当前运行在`ota_0`
- 调用`esp_ota_get_next_update_partition(NULL)`会返回`ota_1`
- 新固件会写入到`ota_1`
- 写入完成后，设置启动分区为`ota_1`
- 重启后从`ota_1`启动

## 代码中的关键操作
```cpp
// 1. 获取下一个可用的更新分区, 只要运行一次, 不能运行多次, 不然多次调用可能会返回不同的分区，导致写入混乱
update_partition = esp_ota_get_next_update_partition(NULL);

// 2. 开始OTA，准备写入新固件, 开始OTA时获取handle
esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);

// 3. 写入固件数据, 写入过程中使用这个handle
esp_ota_write(update_handle, data, len);

// 4. 完成写入, 结束时使用handle完成更新
esp_ota_end(update_handle);

// 5. 设置下次启动的分区
esp_ota_set_boot_partition(update_partition);
```

## 安全机制
- 如果新固件写入失败，不会影响当前运行的固件
- 如果新固件有问题，可以回滚到之前的分区
- 每个分区都有校验机制，确保固件完整性

这种机制确保了OTA更新的安全性，即使更新失败也不会导致设备变砖。 


# update_handle
update_handle 是OTA更新过程中的一个重要句柄，让我解释一下它的作用：
作用：
* 是OTA会话的唯一标识符
* 跟踪整个OTA更新过程
* 维护写入状态和校验信息

为什么需要handle：
* 确保写入的连续性
* 维护写入进度
* 进行数据校验
* 防止多个OTA过程混淆
* 在写入失败时能够正确清理

注意:
* handle必须在整个OTA过程中保持有效
* 不能中途改变handle的值
* 必须调用esp_ota_end来结束OTA会话
* 如果更新失败，需要调用esp_ota_abort清理资源


## esp_ota_begin

在ESP32的OTA（Over-the-Air，空中下载）更新过程中， esp_ota_begin 函数用于启动OTA更新会话。这两个调用的不同之处在于第二个参数，下面详细解释：

### esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle)
- 第二个参数 OTA_SIZE_UNKNOWN ：
  - 当使用 OTA_SIZE_UNKNOWN 作为第二个参数时，意味着在开始OTA更新时，你还不知道要下载的固件的具体大小。
  - 这种情况下，OTA系统会在写入数据的过程中动态地管理分区空间，直到数据写入完成。
  - 它适用于无法提前确定固件大小的场景，例如通过网络流式传输固件，在传输开始时无法得知完整固件的大小。
### esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle)
- 第二个参数 OTA_WITH_SEQUENTIAL_WRITES ：
  - OTA_WITH_SEQUENTIAL_WRITES 表示你将以顺序写入的方式更新固件。
  - 使用这个标志时，你需要确保后续调用 esp_ota_write 函数时，写入的数据是按照顺序连续的，不能跳过任何数据块。
  - 这种模式可以提高写入效率，因为OTA系统可以预先优化写入操作，减少不必要的擦除和写入操作。
  - 同时，使用这个标志时，你可能需要提前知道固件的大小，以便正确地管理写入过程。
### 总结
- OTA_SIZE_UNKNOWN ：适用于不知道固件大小的情况，OTA系统会动态管理分区空间。
- OTA_WITH_SEQUENTIAL_WRITES ：适用于可以按顺序连续写入数据的场景，能提高写入效率，但可能需要提前知道固件大小。
在实际使用中，你应该根据具体的应用场景和需求选择合适的标志。如果你能提前知道固件大小，并且可以保证顺序写入，那么使用 OTA_WITH_SEQUENTIAL_WRITES 可能会更高效；如果无法提前知道固件大小，那么使用 OTA_SIZE_UNKNOWN 会更合适。

# 错误 ESP_ERR_OTA_PARTITION_CONFLICT

ESP_ERR_OTA_PARTITION_CONFLICT

是的，很可能是因为使用了 `huge_app.csv` 分区表导致的问题。`huge_app.csv` 分区表主要用于大型应用，但它没有配置 OTA 分区。

建议改用支持 OTA 的分区表，比如：

```ini
[env:esp32dev]
# ... 其他配置 ...
board_build.partitions = default_ota.csv
```

不同分区表的区别：
1. **huge_app.csv**：
   - 单一大应用分区
   - 没有 OTA 分区
   - 最大程序空间
   - 不支持 OTA 更新

2. **default_ota.csv**：不行, 烧录不了 适合需要较大文件存储的应用
   - 包含两个 OTA 分区
   - 支持 OTA 更新
   - 每个分区较小
   - 标准配置

   nvs (0x5000 = 20KB)
   otadata (0x2000 = 8KB)
   app0 (0x140000 = 1.25MB)
   app1 (0x140000 = 1.25MB)
   spiffs (0x170000 = 1.5MB) 大空间用于存储文件

3. **min_spiffs.csv**： 适合需要OTA且程序较大的应用
   - 最小 SPIFFS 分区
   - 两个 OTA 分区
   - 更多程序空间
   - 支持 OTA

   nvs (0x5000 = 20KB) 非易失性存储, 存储配置数据、WiFi信息等
   otadata (0x2000 = 8KB)
   app0 (0x1E0000 = 1.875MB) 第一个应用程序分区
   app1 (0x1E0000 = 1.875MB) 第二个应用程序分区
   spiffs (0x30000 = 192KB) 存储文件
