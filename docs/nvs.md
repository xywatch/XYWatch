NVS (Non-Volatile Storage) 分区的数据读写主要通过 ESP32 的 NVS 库来实现。以下是基本用法：

1. **基本初始化**：
```cpp
#include <nvs_flash.h>
#include <nvs.h>

void setup() {
    // 初始化 NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS 分区被损坏或版本不匹配时擦除重新初始化
        nvs_flash_erase();
        err = nvs_flash_init();
    }
}
```

2. **写入数据**：
```cpp
void writeNVS() {
    nvs_handle_t handle;
    // 打开命名空间
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK) return;

    // 写入不同类型的数据
    nvs_set_i32(handle, "counter", 123);              // 整数
    nvs_set_str(handle, "name", "ESP32");            // 字符串
    nvs_set_blob(handle, "data", buffer, length);    // 二进制数据

    // 提交更改
    nvs_commit(handle);
    
    // 关闭句柄
    nvs_close(handle);
}
```

3. **读取数据**：
```cpp
void readNVS() {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &handle);
    if (err != ESP_OK) return;

    // 读取整数
    int32_t counter = 0;
    err = nvs_get_i32(handle, "counter", &counter);
    if (err == ESP_OK) {
        Serial.printf("Counter: %d\n", counter);
    }

    // 读取字符串
    char name[32];
    size_t length = sizeof(name);
    err = nvs_get_str(handle, "name", name, &length);
    if (err == ESP_OK) {
        Serial.printf("Name: %s\n", name);
    }

    nvs_close(handle);
}
```

4. **删除数据**：
```cpp
void deleteNVS() {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK) return;

    // 删除特定键
    nvs_erase_key(handle, "counter");
    
    // 或删除整个命名空间
    nvs_erase_all(handle);
    
    nvs_commit(handle);
    nvs_close(handle);
}
```

5. **注意事项**：
- 命名空间最长 15 字符
- 键名最长 15 字符
- 写入后需要调用 nvs_commit
- 使用完要关闭句柄
- 数据有大小限制（字符串和blob通常不超过4KB）

6. **错误处理**：
```cpp
esp_err_t err = nvs_get_i32(handle, "counter", &value);
switch (err) {
    case ESP_OK:
        Serial.println("Value read successfully");
        break;
    case ESP_ERR_NVS_NOT_FOUND:
        Serial.println("Value not found");
        break;
    default:
        Serial.printf("Error reading value: %d\n", err);
}
```

# nvs分区数据在程序烧录后会丢失吗

NVS 分区的数据在程序烧录时的处理取决于烧录方式：

1. **普通程序烧录**：
   - NVS 数据会保留
   - 只更新 app 分区
   - 配置数据（如WiFi密码）不会丢失

2. **擦除全片烧录**：
   - 选择 "Erase Flash: All Flash Contents" 时
   - NVS 数据会被清除
   - 所有分区都会被擦除
   - 需要重新写入配置数据

3. **特殊情况**：
   - 如果修改了分区表
   - 如果 NVS 分区损坏
   - 如果执行了 nvs_flash_erase()
   - 这些情况下 NVS 数据会丢失

4. **建议做法**：
   - 重要数据建议做备份
   - 程序中实现数据初始化逻辑
   - 检测并处理 NVS 数据缺失的情况
   - 必要时提供重置功能

所以正常的程序更新不会影响 NVS 数据，除非特意选择擦除或遇到特殊情况。
