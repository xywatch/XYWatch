#include "common.h"
#include "bma.h"

RTC_DATA_ATTR BMA423 sensor; // 必须要用RTC_DATA_ATTR，否则会唤醒后丢失
extern bool keep_on;

void bma_read_acc();
void getTemp();

byte getBma423Address()
{
    // scan for i2c devices
    byte error, address;

    Serial.println("Scanning...");
    for (address = 0x18; address <= 0x19; address++)
    {
        // The i2c_scanner uses the return value of
        // the Write.endTransmission to see if
        // a device did acknowledge to the address.
        Wire1.beginTransmission(address);
        error = Wire1.endTransmission();
        // error = Wire1.transfer(address, NULL, 0, NULL, 0);
        if (error == 0)
        {
            Serial.print("BMA423 device found at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.println(address, HEX);
            return address;
        }
        else if (error == 4)
        {
        }
    }
    Serial.println("No BMA423 devices found\n");
    return 0x18;
}

uint16_t _readRegister(uint8_t address, uint8_t reg, uint8_t *data,
                       uint16_t len)
{
    Wire1.beginTransmission(address);
    Wire1.write(reg);
    Wire1.endTransmission();
    Wire1.requestFrom((uint8_t)address, (uint8_t)len);
    uint8_t i = 0;
    while (Wire1.available())
    {
        data[i++] = Wire1.read();
    }
    return 0;
}

uint16_t _writeRegister(uint8_t address, uint8_t reg, uint8_t *data,
                        uint16_t len)
{
    Wire1.beginTransmission(address);
    Wire1.write(reg);
    Wire1.write(data, len);
    return (0 != Wire1.endTransmission());
}

bool bmaConfig()
{
    // getBma423Address();

    Serial.println("BMA _bmaConfig");
    // getBma423Address();
    byte address = 0x18; // getBma423Address(); // 接地 0x18; 1引角悬空了导致没有接地变成了0x19

    // delay(5000);

    if (sensor.begin(_readRegister, _writeRegister, delay, address) == false)
    {
        // fail to init BMA
        Serial.println("BMA ERROR");
        // getBma423Address();
        return false;
    }

    sensor.get_feature_config();

    // Accel parameter structure
    Acfg cfg;
    /*!
        Output data rate in Hz, Optional parameters:
            - BMA4_OUTPUT_DATA_RATE_0_78HZ
            - BMA4_OUTPUT_DATA_RATE_1_56HZ
            - BMA4_OUTPUT_DATA_RATE_3_12HZ
            - BMA4_OUTPUT_DATA_RATE_6_25HZ
            - BMA4_OUTPUT_DATA_RATE_12_5HZ
            - BMA4_OUTPUT_DATA_RATE_25HZ
            - BMA4_OUTPUT_DATA_RATE_50HZ
            - BMA4_OUTPUT_DATA_RATE_100HZ
            - BMA4_OUTPUT_DATA_RATE_200HZ
            - BMA4_OUTPUT_DATA_RATE_400HZ
            - BMA4_OUTPUT_DATA_RATE_800HZ
            - BMA4_OUTPUT_DATA_RATE_1600HZ
    */
    cfg.odr = BMA4_OUTPUT_DATA_RATE_100HZ;
    /*!
        G-range, Optional parameters:
            - BMA4_ACCEL_RANGE_2G
            - BMA4_ACCEL_RANGE_4G
            - BMA4_ACCEL_RANGE_8G
            - BMA4_ACCEL_RANGE_16G
    */
    cfg.range = BMA4_ACCEL_RANGE_2G;
    /*!
        Bandwidth parameter, determines filter configuration, Optional parameters:
            - BMA4_ACCEL_OSR4_AVG1
            - BMA4_ACCEL_OSR2_AVG2
            - BMA4_ACCEL_NORMAL_AVG4
            - BMA4_ACCEL_CIC_AVG8
            - BMA4_ACCEL_RES_AVG16
            - BMA4_ACCEL_RES_AVG32
            - BMA4_ACCEL_RES_AVG64
            - BMA4_ACCEL_RES_AVG128
    */
    cfg.bandwidth = BMA4_ACCEL_NORMAL_AVG4;

    /*! Filter performance mode , Optional parameters:
        - BMA4_CIC_AVG_MODE
        - BMA4_CONTINUOUS_MODE
    */
    cfg.perf_mode = BMA4_CONTINUOUS_MODE;

    bool ok;

    // Configure the BMA423 accelerometer
    if (!sensor.setAccelConfig(cfg))
    {
        Serial.println("sensor.setAccelConfig error");
    }
    else
    {
        // Acfg cfg2;
        // sensor.getAccelConfig(cfg2);
        // Serial.printf("cfg2.odr: %d\n", cfg2.odr);
        // Serial.printf("cfg2.range: %d\n", cfg2.range);
        // Serial.printf("cfg2.bandwidth: %d\n", cfg2.bandwidth);
        // Serial.printf("cfg2.perf_mode: %d\n", cfg2.perf_mode);
        // Serial.println("sensor.setAccelConfig OK");
    }

    // Enable BMA423 accelerometer
    // Warning : Need to use feature, you must first enable the accelerometer
    sensor.enableAccel();

    struct bma4_int_pin_config config;
    config.edge_ctrl = BMA4_LEVEL_TRIGGER;
    config.lvl = BMA4_ACTIVE_HIGH;
    // config.lvl = BMA4_ACTIVE_LOW;
    config.od = BMA4_PUSH_PULL;
    config.output_en = BMA4_OUTPUT_ENABLE;
    config.input_en = BMA4_INPUT_DISABLE;
    // The correct trigger interrupt needs to be configured as needed
    ok = sensor.setINTPinConfig(config, BMA4_INTR1_MAP); // 映射到ACC_INT1引脚
    // ok = sensor.setINTPinConfig(config, BMA4_INTR2_MAP); // 映射到ACC_INT2引脚
    if (!ok)
    {
        Serial.println("sensor.setINTPinConfig error");
    }

    // 轴映射, 为了配置抬腕
    // bottom layer, 0引角在右上
    // X = y axis
    // Y = x axis
    // Z = -z axis
    // x, y swapped, and z inverted
    struct bma423_axes_remap remap_data;

    // watchy
    // 1角在左上角, bottom layer
    remap_data.x_axis = 0;
    remap_data.x_axis_sign = 1;
    remap_data.y_axis = 1;
    remap_data.y_axis_sign = 0;
    remap_data.z_axis = 2;
    remap_data.z_axis_sign = 1;

    // go watch
    // 1角在左上角, top layer
    // X = -y axis Y = +x axis Z = +z axis
    // x, y swapped, and y inverted
    remap_data.x_axis = 1;
    remap_data.x_axis_sign = 1;
    remap_data.y_axis = 0;
    remap_data.y_axis_sign = 0;
    remap_data.z_axis = 2;
    remap_data.z_axis_sign = 0;

    // Need to raise the wrist function, need to set the correct axis
    if (!sensor.setRemapAxes(&remap_data))
    {
        Serial.println("setRemapAxes error");
    }

    // Enable BMA423 step counter feature
    if (!sensor.enableFeature(BMA423_STEP_CNTR, true))
    {
        Serial.println("enableFeature BMA423_STEP_CNTR error");
    }
    // Enable BMA423 wrist tilt feature 抬腕
    // if (!sensor.enableFeature(BMA423_TILT, appConfig.tiltWrist))
    // {
    //     Serial.println("enableFeature BMA423_TILT error");
    // }
    // Enable BMA423 wakeup feature 唤醒 double tap
    // if (!sensor.enableFeature(BMA423_WAKEUP, true))
    // {
    //     Serial.println("enableFeature BMA423_WAKEUP error");
    // }

    // Reset steps
    if (!sensor.resetStepCounter())
    {
        Serial.println("resetStepCounter error");
    }

    enableTiltWrist(appConfig.tiltWrist);
    enableDoubleTap(appConfig.doubleTap);

    // 中断配置

    // 检测步数
    // if (!sensor.enableStepCountInterrupt(true)) {
    //     Serial.println("enableStepCountInterrupt error");
    // }
    // 抬腕
    // if (!sensor.enableTiltInterrupt(appConfig.tiltWrist))
    // {
    //     Serial.println("enableTiltInterrupt error");
    // }
    // double tap
    // if (!sensor.enableWakeupInterrupt(true)) {
    //     Serial.println("enableWakeupInterrupt error");
    // }

    // uint8_t feature_config[BMA423_FEATURE_SIZE] = {1, 2, 3, 4, 5, 6, 7, 8};
    // if (bma4_write_regs(BMA4_FEATURE_CONFIG_ADDR, feature_config,
    //                             BMA423_FEATURE_SIZE, sensor.getDevFptr()) != BMA4_OK) {
    //     Serial.println("bma4_write_regs error");
    // }

    // Serial.println("xx");
    sensor.get_feature_config();

    // while (!sensor.getAccelEnable()) {
    //     Serial.println("BMA423 data not ready!");
    //     delay(200);
    // }
    // Serial.println("BMA423 data ready!");

    // bma_read_acc();

    if (!sensor.getAccelEnable())
    {
        Serial.println("BMA423 data not ready!");
        // return;
    }
    else
    {
        Serial.println("BMA423 data ready!");
    }

    // uint8_t data = 0;
    // bma4_read_regs(BMA4_POWER_CTRL_ADDR, &data, 1, sensor.getDevFptr());
    // Serial.printf("BMA423 power ctrl: %d\n", data); // 4

    // 可以disableAccel 证明可以写入数据
    // sensor.disableAccel();
    // bma4_read_regs(BMA4_POWER_CTRL_ADDR, &data, 1, sensor.getDevFptr());
    // Serial.printf("BMA423 power ctrl: %d\n", data); // 0

    appconfig_init_step_log();

    return true;
}

void enableTiltWrist (bool enable) {
    sensor.enableFeature(BMA423_TILT, enable);
    sensor.enableTiltInterrupt(enable);
}

void enableDoubleTap (bool enable) {
    sensor.enableFeature(BMA423_WAKEUP, enable);
    sensor.enableWakeupInterrupt(enable);
}

void resetStepCounter () {
    sensor.resetStepCounter();
}

void bma_read_acc()
{
    // 读取前先检查数据是否就绪, 一直是false
    if (!sensor.getAccelEnable())
    {
        Serial.println("BMA423 data not ready!");
        // return;
    }

    Accel bma_acc;
    if (!sensor.getAccel(bma_acc))
    {
        Serial.println("BMA423 read failed!");
        return;
    }

    // 打印原始值和实际g值
    Serial.printf("Raw - X: %d, Y: %d, Z: %d\n", bma_acc.x, bma_acc.y, bma_acc.z);

    // 转换为g值 (2g范围下，32768 = 2g)
    float x_g = bma_acc.x * 2.0 / 32768.0;
    float y_g = bma_acc.y * 2.0 / 32768.0;
    float z_g = bma_acc.z * 2.0 / 32768.0;
    Serial.printf("g - X: %.2f, Y: %.2f, Z: %.2f\n", x_g, y_g, z_g);
}

float bma423_get_temp()
{
    int8_t rslt;
    int32_t get_temp_C = 0;
    float actual_temp = 0;

    /* Get temperature in degree C */
    rslt = bma4_get_temperature(&get_temp_C, BMA4_DEG, sensor.getDevFptr());
    actual_temp = (float)get_temp_C / (float)BMA4_SCALE_TEMP;
    return actual_temp;
}


static long bma_previousMillis = 0;
static long bma_interval = 100;
static Accel bma_acc;

static bool bma_res = false;
static uint8_t bma_direction = 0;
static float bma_temp = 0;

static display_t draw()
{
    unsigned long currentMillis = millis();

    if (currentMillis - bma_previousMillis > bma_interval)
    {
        bma_previousMillis = currentMillis;

        // 切换到低速读取BMA423数据

        // 批量读取所有需要的数据
        bma_res = sensor.getAccel(bma_acc);
        bma_direction = sensor.getDirection();
        bma_temp = bma423_get_temp();

        // getTemp();

        // 处理读取到的数据
        // if (bma_res) {
        //     Serial.printf("X: %d, Y: %d, Z: %d\n", bma_acc.x, bma_acc.y, bma_acc.z);
        // }
    }
    if (bma_res == false)
    {
        draw_string("getAccel FAIL", false, 0, 0);
    }
    else
    {
        u8 y = 0;
        char buf[15];
        sprintf(buf, "X: %d", bma_acc.x);
        draw_string((char *)buf, false, 0, y);

        sprintf(buf, "Y: %d", bma_acc.y);
        draw_string((char *)buf, false, 0, y += 9);

        sprintf(buf, "Z: %d", bma_acc.z);
        draw_string((char *)buf, false, 0, y += 9);

        sprintf(buf, "Temp: %2.2fC", bma_temp);
        draw_string((char *)buf, false, 0, y += 9);

        y += 12;

        switch (bma_direction)
        {
        case DIRECTION_DISP_DOWN:
            draw_string_center("FACE DOWN", false, 0, 127, y);
            break;
        case DIRECTION_DISP_UP:
            draw_string_center("FACE UP", false, 0, 127, y);
            break;
        case DIRECTION_BOTTOM_EDGE:
            draw_string_center("BOTTOM EDGE", false, 0, 127, y);
            break;
        case DIRECTION_TOP_EDGE:
            draw_string_center("TOP EDGE", false, 0, 127, y);
            break;
        case DIRECTION_RIGHT_EDGE:
            draw_string_center("RIGHT EDGE", false, 0, 127, y);
            break;
        case DIRECTION_LEFT_EDGE:
            draw_string_center("LEFT EDGE", false, 0, 127, y);
            break;
        default:
            draw_string("ERROR!!!", false, 0, y);
            break;
        }
    }

    return DISPLAY_BUSY;
}

static bool btnExit()
{
    keep_on = false; // 如果在draw里一直刷新变量，变量在这里关闭不了，因为会draw线程会再次覆盖
    back_to_watchface();
    return true;
}

/* Declare an instance of the BMA4xy device */
struct bma4_dev dev;

uint16_t set_accel_config(void)
{
    uint16_t rslt = BMA4_OK;

    /* Initialize the device instance as per the initialization example */

    /* Enable the accelerometer */
    bma4_set_accel_enable(BMA4_ENABLE, &dev);

    /* Declare an accelerometer configuration structure */
    struct bma4_accel_config accel_conf;

    /* Assign the desired settings */
    accel_conf.odr = BMA4_OUTPUT_DATA_RATE_100HZ;
    accel_conf.range = BMA4_ACCEL_RANGE_2G;
    accel_conf.bandwidth = BMA4_ACCEL_NORMAL_AVG4;
    accel_conf.perf_mode = BMA4_CONTINUOUS_MODE;

    /* Set the configuration */
    rslt |= bma4_set_accel_config(&accel_conf, &dev);

    return rslt;
}

uint16_t read_accel_xyz(void)
{
    uint16_t rslt = BMA4_OK;

    /* Initialize the device instance as per the initialization example */

    /* Configure the accelerometer */

    /* Declare an instance of the sensor data structure */
    struct bma4_accel sens_data;

    /* Loop forever */
    // while (1) {
    /* Read the sensor data into the sensor data instance */
    rslt |= bma4_read_accel_xyz(&sens_data, &dev);

    /* Exit the program in case of a failure */
    if (rslt != BMA4_OK)
        return rslt;

    /* Use the data */
    printf("X: %d, Y: %d, Z: %d\n", sens_data.x, sens_data.y, sens_data.z);

    /* Pause for 10ms, 100Hz output data rate */
    // delay(500);
    // }

    return rslt;
}

void getTemp()
{
    int8_t rslt;
    int32_t get_temp_C = 0;
    int32_t get_temp_F = 0;
    int32_t get_temp_K = 0;
    float actual_temp = 0;

    /* Get temperature in degree C */
    rslt = bma4_get_temperature(&get_temp_C, BMA4_DEG, sensor.getDevFptr());
    /* Get temperature in degree F */
    rslt = bma4_get_temperature(&get_temp_F, BMA4_FAHREN, sensor.getDevFptr());
    /* Get temperature in degree K */
    rslt = bma4_get_temperature(&get_temp_K, BMA4_KELVIN, sensor.getDevFptr());

    /* Divide the temperature read with the scaling factor to get
    the actual temperature */
    if (rslt == BMA4_OK)
    {
        actual_temp = (float)get_temp_C / (float)BMA4_SCALE_TEMP;
        Serial.printf("Actual temperature in degree celsius is %10.2f degrees C\r\n", actual_temp);

        actual_temp = (float)get_temp_F / (float)BMA4_SCALE_TEMP;
        Serial.printf("Actual temperature in degree fahranheit is %10.2f degrees F\r\n", actual_temp);

        actual_temp = (float)get_temp_K / (float)BMA4_SCALE_TEMP;
        Serial.printf("Actual temperature in degree kelvin is %10.2f degrees K\r\n", actual_temp);

        /* 0x80 - temp read from the register and 23 is the ambient temp added.
         * If the temp read from register is 0x80, it means no valid
         * information is available */
        if (((get_temp_C - 23) / BMA4_SCALE_TEMP) == 0x80)
        {
            Serial.printf("No valid temperature information available\r\n");
        }
    }
}

uint16_t initBma423(void)
{
    uint16_t rslt = BMA4_OK;
    uint8_t init_seq_status = 0;

    /* Declare an instance of the BMA4xy device */
    // struct bma4_dev dev;

    /* Modify the parameters */
    dev.dev_addr = BMA4_I2C_ADDR_PRIMARY;
    //   dev.dev_addr = BMA4_I2C_ADDR_SECONDARY;
    dev.interface = BMA4_I2C_INTERFACE;
    dev.bus_read = _readRegister;
    dev.bus_write = _writeRegister;
    dev.delay = delay;
    dev.read_write_len = 8;
    dev.resolution = 12;
    dev.feature_len = BMA423_FEATURE_SIZE;

    /* a. Reading the chip id. */
    rslt |= bma423_init(&dev);
    if (rslt != BMA4_OK)
    {
        Serial.println("init error");
        Serial.println(dev.chip_id);
        return 0;
    }
    Serial.println("init ok");
    Serial.println(dev.chip_id);

    /* b. Performing initialization sequence.
      c. Checking the correct status of the initialization sequence.
    */
    rslt = bma423_write_config_file(&dev);

    if (rslt != BMA4_OK)
    {
        Serial.println("bma423_write_config_file error");
        Serial.println(rslt);
        Serial.println(rslt == BMA4_E_RD_WR_LENGTH_INVALID);
        Serial.println(rslt == BMA4_E_INVALID_SENSOR);
        // return 0;
    }
    Serial.println("bma423_write_config_file ok");
    set_accel_config();

    while (1)
    {
        read_accel_xyz();
        getTemp();
        delay(1000);
    }

    return rslt;
}

void showAccelerometer(void)
{
    // bmaConfig(); // 重新初始化试试
    // initBma423();
    // Serial.println("BMA423 init OK");
    sensor.get_feature_config();

    // uint8_t data = 0;
    // bma4_read_regs(BMA4_INT_MAP_1_ADDR, &data, 1, sensor.getDevFptr());
    // Serial.printf("BMA4_INT_MAP_1_ADDR: %d\n", data); // 40 = 101000

    // bma4_read_regs(0x5B, &data, 1, sensor.getDevFptr());
    // Serial.printf("0x5B: %d\n", data);

    display_setDrawFunc(draw);
    buttons_setFuncs(btnExit, btnExit, btnExit);
    showMeThenRun(NULL);
    keep_on = true;
}

uint8_t bma423_get_direction ()
{
    return sensor.getDirection();
}