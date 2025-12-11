#include "sampling_control.h"
#include "led_driver.h"  // 包含LED驱动头文件

// 全局采样控制结构体
sampling_control_t sampling_ctrl;

/************************ 初始化采样控制 ************************/
void Sampling_Control_Init(void)
{
//    my_printf(USART0, "DEBUG: Sampling_Control_Init called\r\n");

    // 强制重置所有状态，确保系统处于已知状态
    sampling_ctrl.state = SAMPLING_STOPPED;
    sampling_ctrl.last_sample_time = 0;
    sampling_ctrl.led_blink_period_ms = 500;    // 0.5秒LED切换周期（1秒完整周期：0.5秒亮+0.5秒灭）
    sampling_ctrl.last_led_toggle_time = 0;
    sampling_ctrl.oled_update_period_ms = 500;  // 0.5秒OLED更新周期
    sampling_ctrl.last_oled_update_time = 0;
    sampling_ctrl.led_state = 0;
    sampling_ctrl.current_voltage = 0.0f;
    sampling_ctrl.key_control_locked = 0;       // 修改：初始状态不锁定按键控制
    sampling_ctrl.over_limit_status = 0;        // 初始无超限

    // 强制关闭所有LED
    LED1_OFF();
    LED2_OFF();

    // 初始化周期配置
    sampling_ctrl.period_config.period_5s = 5000;   // 5秒
    sampling_ctrl.period_config.period_10s = 10000; // 10秒
    sampling_ctrl.period_config.period_15s = 15000; // 15秒
    sampling_ctrl.period_config.current_period_index = 0; // 默认5秒

    // 尝试从Flash加载周期配置
    if(!Load_Period_Config_From_Flash())
    {
        // 如果加载失败，使用默认配置并保存到Flash
        Save_Period_Config_To_Flash();
    }

    // 设置当前采样周期
    switch(sampling_ctrl.period_config.current_period_index)
    {
        case 0: sampling_ctrl.sample_period_ms = sampling_ctrl.period_config.period_5s; break;
        case 1: sampling_ctrl.sample_period_ms = sampling_ctrl.period_config.period_10s; break;
        case 2: sampling_ctrl.sample_period_ms = sampling_ctrl.period_config.period_15s; break;
        default: sampling_ctrl.sample_period_ms = sampling_ctrl.period_config.period_5s; break;
    }
}

/************************ 启动采样 ************************/
void Sampling_Control_Start(void)
{
//    my_printf(USART0, "DEBUG: Sampling_Control_Start called, current state: %d\r\n", sampling_ctrl.state);

    if(sampling_ctrl.state == SAMPLING_STOPPED)
    {
//        my_printf(USART0, "DEBUG: Starting sampling...\r\n");

        sampling_ctrl.state = SAMPLING_RUNNING;
        sampling_ctrl.last_sample_time = Get_Tick();
        sampling_ctrl.last_led_toggle_time = Get_Tick();
        sampling_ctrl.last_oled_update_time = Get_Tick();

        my_printf(USART0, "Periodic Sampling\r\n");
        my_printf(USART0, "sample cycle: %lus\r\n", Get_Current_Period_Seconds());

        // 记录启动日志
        Buffer_Log_Data("Sampling started");

        // 清屏并初始化OLED显示
        OLED_Clear();

        // 暂时禁用立即数据输出，避免SD卡操作导致系统卡死
        // Output_Sample_Data();
        Update_OLED_Display();

//        my_printf(USART0, "DEBUG: Sampling started successfully, state: %d\r\n", sampling_ctrl.state);
    }
    else
    {
//        my_printf(USART0, "DEBUG: Sampling already running or invalid state\r\n");
    }
}

/************************ 停止采样 ************************/
void Sampling_Control_Stop(void)
{

    if(sampling_ctrl.state == SAMPLING_RUNNING)
    {
//        my_printf(USART0, "DEBUG: Stopping sampling...\r\n");

        sampling_ctrl.state = SAMPLING_STOPPED;

        // 关闭LED1（常灭）
        LED1_OFF();
        sampling_ctrl.led_state = 0;

        // 关闭LED2（超限指示）
        LED2_OFF();
        sampling_ctrl.over_limit_status = 0;

        // 按照要求设置OLED显示：第一行"system idle"，第二行为空
        OLED_Clear();
        OLED_ShowString(0, 0, (uint8_t*)"system idle", 16);  // 第一行显示"system idle"
        // 第二行为空，不显示任何内容
        OLED_Refresh();

        // 记录停止日志
        Buffer_Log_Data("Sampling stopped");

        // 按照图片要求的输出格式
        my_printf(USART0, "Periodic Sampling STOP\r\n");

//        my_printf(USART0, "DEBUG: Sampling stopped successfully, state: %d\r\n", sampling_ctrl.state);
    }
    else
    {
//        my_printf(USART0, "DEBUG: Sampling already stopped or invalid state\r\n");
    }
}

/************************ 采样控制任务 ************************/
void Sampling_Control_Task(void)
{
    uint32_t current_time = Get_Tick();
    
    if(sampling_ctrl.state == SAMPLING_RUNNING)
    {
        // 检查是否需要切换LED状态
        if((current_time - sampling_ctrl.last_led_toggle_time) >= sampling_ctrl.led_blink_period_ms)
        {
            Toggle_LED1();
            sampling_ctrl.last_led_toggle_time = current_time;
        }
        
        // 检查是否需要更新OLED显示
        if((current_time - sampling_ctrl.last_oled_update_time) >= sampling_ctrl.oled_update_period_ms)
        {
            Update_OLED_Display();
            sampling_ctrl.last_oled_update_time = current_time;
        }
        
        // 检查是否需要输出采样数据
        if((current_time - sampling_ctrl.last_sample_time) >= sampling_ctrl.sample_period_ms)
        {
            Output_Sample_Data();
            sampling_ctrl.last_sample_time = current_time;
        }
    }
}

/************************ 更新OLED显示 ************************/
void Update_OLED_Display(void)
{
    uint8_t hour, min, sec, ampm;
    uint8_t year, month, date, week;
    static char last_time_str[16] = {0};
    static char last_voltage_str[16] = {0};
    char time_str[16];
    char voltage_str[16];
    uint8_t need_update = 0;

    // 获取当前时间
    rtc_get_time(&hour, &min, &sec, &ampm);
    rtc_get_date(&year, &month, &date, &week);

    // 读取当前电压值
    sampling_ctrl.current_voltage = Read_ADC_Voltage();

    // 格式化时间字符串 (hh:mm:ss)
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", hour, min, sec);

    // 格式化电压字符串 (xx.xx V) - 确保格式正确
    snprintf(voltage_str, sizeof(voltage_str), "%.2f V      ", sampling_ctrl.current_voltage);

    // 检查是否需要更新显示（内容发生变化）
    if(strcmp(time_str, last_time_str) != 0)
    {
        strcpy(last_time_str, time_str);
        need_update = 1;
    }

    if(strcmp(voltage_str, last_voltage_str) != 0)
    {
        strcpy(last_voltage_str, voltage_str);
        need_update = 1;
    }

    // 只有内容变化时才更新显示，避免频繁刷新
    if(need_update)
    {
        // 使用Oled_Printf代替直接的OLED函数，避免清屏
        Oled_Printf(0, 0, "%s", time_str);      // 第一行显示时间
        Oled_Printf(0, 16, "%s", voltage_str);  // 第二行显示电压
        OLED_Refresh(); // 刷新显示
    }
}

/************************ 切换LED1状态 ************************/
void Toggle_LED1(void)
{
    if(sampling_ctrl.led_state == 0)
    {
        LED1_ON();
        sampling_ctrl.led_state = 1;
    }
    else
    {
        LED1_OFF();
        sampling_ctrl.led_state = 0;
    }
}

/************************ 输出采样数据 ************************/
void Output_Sample_Data(void)
{
    uint8_t hour, min, sec, ampm;
    uint8_t year, month, date, week;
    float voltage;
    uint8_t over_limit;
    extern config_params_t current_config;  // 引用全局配置
    extern uint8_t hide_mode;

//    my_printf(USART0, "DEBUG: Output_Sample_Data called\r\n");

    // 获取当前时间
    rtc_get_time(&hour, &min, &sec, &ampm);
    rtc_get_date(&year, &month, &date, &week);

//    my_printf(USART0, "DEBUG: RTC time: 20%02d-%02d-%02d %02d:%02d:%02d\r\n",
//              year, month, date, hour, min, sec);

    // 读取电压值
    voltage = Read_ADC_Voltage()*current_config.ratio;
    sampling_ctrl.current_voltage = voltage;

//    my_printf(USART0, "DEBUG: Voltage: %.3fV\r\n", voltage);

    // 检查是否超限
    over_limit = Check_Voltage_Over_Limit(voltage);
    sampling_ctrl.over_limit_status = over_limit;

//    my_printf(USART0, "DEBUG: Over limit: %d, Storage mode: %d\r\n",
//              over_limit, Get_Storage_Mode());

    // 控制LED2
    Control_LED2_Over_Limit(over_limit);

    // 准备数据存储结构
    sample_data_t sample_data;
    sample_data.timestamp = DateTime_To_Unix(2000 + year, month, date, hour, min, sec);
    sample_data.voltage = voltage;
    sample_data.over_limit = over_limit;

    // 使用非阻塞缓冲区方式存储数据
    if(Get_Storage_Mode() == STORAGE_MODE_NORMAL) {
        // 正常模式：根据是否超限决定存储位置
        if(over_limit) {
            // 超限数据存储到overLimit文件夹
            if(!Buffer_OverLimit_Data(&sample_data)) {
                // 缓冲区满时不影响采样继续进行
            }
//            my_printf(USART0, "DEBUG: Data stored to overLimit (voltage: %.3f > limit: %.3f)\r\n",
//                      voltage, current_config.limit);
        } else {
            // 正常数据存储到sample文件夹
            if(!Buffer_Sample_Data(&sample_data)) {
                // 缓冲区满时不影响采样继续进行
            }
//            my_printf(USART0, "DEBUG: Data stored to sample (voltage: %.3f <= limit: %.3f)\r\n",
//                      voltage, current_config.limit);
        }
    } else {
//        // 加密模式：特殊存储逻辑
//        my_printf(USART0, "DEBUG: Encrypt mode - processing data storage\r\n");

        // 1. 所有数据都存储加密格式到hideData文件夹
        if(!Save_Encrypted_Data(&sample_data)) {
//            my_printf(USART0, "DEBUG: Encrypted data save failed\r\n");
        } else {
//            my_printf(USART0, "DEBUG: Encrypted data saved successfully\r\n");
        }

        // 2. 如果超限，仍然要存储到overLimit文件夹（原有格式）
        if(over_limit) {
            if(!Buffer_OverLimit_Data(&sample_data)) {
                // 缓冲区满时不影响采样继续进行
            }
//            my_printf(USART0, "DEBUG: Over-limit data also stored to overLimit folder\r\n");
        }

//        my_printf(USART0, "DEBUG: Encrypt mode storage completed\r\n");
    }

    // 输出格式：根据是否超限决定输出格式
    if(hide_mode==0){
    if(over_limit)
    {
        // 超限时输出：2025-01-01 00:30:05 ch0=10.5V OverLimit (10.00) !
        my_printf(USART0, "20%02d-%02d-%02d %02d:%02d:%02d ch0=%.1fV OverLimit (%.2f) !\r\n",
                  year, month, date, hour, min, sec, voltage, current_config.limit);
    }
    else
    {
        // 正常时输出：2025-01-01 00:30:05 ch0=10.5V
        my_printf(USART0, "20%02d-%02d-%02d %02d:%02d:%02d ch0=%.1fV\r\n",
                  year, month, date, hour, min, sec, voltage);
    }}
}

/************************ 读取ADC电压值 ************************/
float Read_ADC_Voltage(void)
{
    uint16_t adc_raw_value;
    float voltage;

    // 读取ADC值 (使用外部定义的adc_value数组)
    extern uint16_t adc_value[1];
    adc_raw_value = adc_value[0];

    // 转换为电压值 (假设参考电压为3.3V，12位ADC)
    voltage = (float)adc_raw_value * 3.3f / 4095.0f;

    return voltage;
}

/************************ 检查采样是否运行中 ************************/
uint8_t Is_Sampling_Running(void)
{
    return (sampling_ctrl.state == SAMPLING_RUNNING) ? 1 : 0;
}

/************************ 切换采样状态(按键控制) ************************/
void Sampling_Control_Toggle(void)
{
//    my_printf(USART0, "DEBUG: Sampling_Control_Toggle called, current state: %d\r\n", sampling_ctrl.state);

    // 检查按键控制是否被锁定
//    if(sampling_ctrl.key_control_locked)
//    {
////        my_printf(USART0, "KEY: Control locked, ignoring key press\r\n");
//        return;
//    }

    if(sampling_ctrl.state == SAMPLING_STOPPED)
    {
//        my_printf(USART0, "DEBUG: KEY: Starting sampling...\r\n");
        Sampling_Control_Start();
    }
    else if(sampling_ctrl.state == SAMPLING_RUNNING)
    {
//        my_printf(USART0, "DEBUG: KEY: Stopping sampling...\r\n");
        Sampling_Control_Stop();
    }
    else
    {
//        my_printf(USART0, "DEBUG: KEY: Invalid sampling state: %d\r\n", sampling_ctrl.state);
    }
}

/************************ 重置采样控制状态 ************************/
void Sampling_Control_Reset(void)
{
//    my_printf(USART0, "DEBUG: Sampling_Control_Reset called\r\n");

    // 强制停止采样
    sampling_ctrl.state = SAMPLING_STOPPED;

    // 关闭所有LED
    LED1_OFF();
    LED2_OFF();
    sampling_ctrl.led_state = 0;
    sampling_ctrl.over_limit_status = 0;

    // 重置时间戳
    sampling_ctrl.last_sample_time = 0;
    sampling_ctrl.last_led_toggle_time = 0;
    sampling_ctrl.last_oled_update_time = 0;

    // 解锁按键控制
    sampling_ctrl.key_control_locked = 0;

    // 设置OLED显示为idle状态
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t*)"system idle", 16);
    OLED_Refresh();

//    my_printf(USART0, "DEBUG: Sampling control reset completed\r\n");
}

/************************ 解锁按键控制 ************************/
void Unlock_Sampling_Control(void)
{
    sampling_ctrl.key_control_locked = 0;
//    my_printf(USART0, "DEBUG: KEY: Control unlocked\r\n");
}

/************************ 设置采样周期 ************************/
void Set_Sampling_Period(uint8_t period_index)
{
    if(period_index > 2) return; // 无效索引

    sampling_ctrl.period_config.current_period_index = period_index;

    // 更新当前采样周期
    switch(period_index)
    {
        case 0:
            sampling_ctrl.sample_period_ms = sampling_ctrl.period_config.period_5s;
//            my_printf(USART0, "sample cycle adjust: 5s\r\n");
            break;
        case 1:
            sampling_ctrl.sample_period_ms = sampling_ctrl.period_config.period_10s;
//            my_printf(USART0, "sample cycle adjust: 10s\r\n");
            break;
        case 2:
            sampling_ctrl.sample_period_ms = sampling_ctrl.period_config.period_15s;
//            my_printf(USART0, "sample cycle adjust: 15s\r\n");
            break;
    }

    // 保存配置到Flash
    Save_Period_Config_To_Flash();

    // 如果当前正在采样，立即输出一次数据以显示新周期
    if(sampling_ctrl.state == SAMPLING_RUNNING)
    {
        Output_Sample_Data();
        sampling_ctrl.last_sample_time = Get_Tick(); // 重置采样时间
    }
}

/************************ 保存周期配置到Flash ************************/
uint8_t Save_Period_Config_To_Flash(void)
{
    uint8_t data_buffer[16];  // 数据缓冲区：4*4字节
    uint32_t temp;

    // 准备要写入的数据
    // period_5s (4字节)
    temp = sampling_ctrl.period_config.period_5s;
    data_buffer[0] = (uint8_t)(temp & 0xFF);
    data_buffer[1] = (uint8_t)((temp >> 8) & 0xFF);
    data_buffer[2] = (uint8_t)((temp >> 16) & 0xFF);
    data_buffer[3] = (uint8_t)((temp >> 24) & 0xFF);

    // period_10s (4字节)
    temp = sampling_ctrl.period_config.period_10s;
    data_buffer[4] = (uint8_t)(temp & 0xFF);
    data_buffer[5] = (uint8_t)((temp >> 8) & 0xFF);
    data_buffer[6] = (uint8_t)((temp >> 16) & 0xFF);
    data_buffer[7] = (uint8_t)((temp >> 24) & 0xFF);

    // period_15s (4字节)
    temp = sampling_ctrl.period_config.period_15s;
    data_buffer[8] = (uint8_t)(temp & 0xFF);
    data_buffer[9] = (uint8_t)((temp >> 8) & 0xFF);
    data_buffer[10] = (uint8_t)((temp >> 16) & 0xFF);
    data_buffer[11] = (uint8_t)((temp >> 24) & 0xFF);

    // current_period_index (4字节，为了对齐)
    temp = (uint32_t)sampling_ctrl.period_config.current_period_index;
    data_buffer[12] = (uint8_t)(temp & 0xFF);
    data_buffer[13] = (uint8_t)((temp >> 8) & 0xFF);
    data_buffer[14] = (uint8_t)((temp >> 16) & 0xFF);
    data_buffer[15] = (uint8_t)((temp >> 24) & 0xFF);

    // 先擦除扇区
    spi_flash_sector_erase(PERIOD_CONFIG_FLASH_ADDR);

    // 写入数据到Flash
    spi_flash_buffer_write(data_buffer, PERIOD_CONFIG_FLASH_ADDR, sizeof(data_buffer));

    return 1;
}

/************************ 从Flash加载周期配置 ************************/
uint8_t Load_Period_Config_From_Flash(void)
{
    uint8_t data_buffer[16];  // 数据缓冲区
    uint32_t temp;

    // 从Flash读取配置数据
    spi_flash_buffer_read(data_buffer, PERIOD_CONFIG_FLASH_ADDR, sizeof(data_buffer));

    // 检查数据有效性（简单检查：所有字节不能都是0xFF）
    uint8_t all_ff = 1;
    for(int i = 0; i < 16; i++)
    {
        if(data_buffer[i] != 0xFF)
        {
            all_ff = 0;
            break;
        }
    }

    if(all_ff) return 0; // 无有效数据

    // 解析读取的数据
    // period_5s (4字节)
    temp = (uint32_t)(data_buffer[0] | (data_buffer[1] << 8) |
                     (data_buffer[2] << 16) | (data_buffer[3] << 24));
    if(temp >= 1000 && temp <= 60000) // 有效范围检查
        sampling_ctrl.period_config.period_5s = temp;

    // period_10s (4字节)
    temp = (uint32_t)(data_buffer[4] | (data_buffer[5] << 8) |
                     (data_buffer[6] << 16) | (data_buffer[7] << 24));
    if(temp >= 1000 && temp <= 60000) // 有效范围检查
        sampling_ctrl.period_config.period_10s = temp;

    // period_15s (4字节)
    temp = (uint32_t)(data_buffer[8] | (data_buffer[9] << 8) |
                     (data_buffer[10] << 16) | (data_buffer[11] << 24));
    if(temp >= 1000 && temp <= 60000) // 有效范围检查
        sampling_ctrl.period_config.period_15s = temp;

    // current_period_index (4字节)
    temp = (uint32_t)(data_buffer[12] | (data_buffer[13] << 8) |
                     (data_buffer[14] << 16) | (data_buffer[15] << 24));
    if(temp <= 2) // 有效范围检查
        sampling_ctrl.period_config.current_period_index = (uint8_t)temp;

    return 1;
}

/************************ 获取当前周期(秒) ************************/
uint32_t Get_Current_Period_Seconds(void)
{
    return sampling_ctrl.sample_period_ms / 1000;
}

/************************ 检查电压是否超限 ************************/
uint8_t Check_Voltage_Over_Limit(float voltage)
{
    extern config_params_t current_config;  // 引用全局配置

    if(voltage > current_config.limit)
    {
        return 1; // 超限
    }
    return 0; // 正常
}

/************************ 控制LED2超限指示 ************************/
void Control_LED2_Over_Limit(uint8_t over_limit)
{
    if(over_limit)
    {
        LED2_ON();   // 点亮LED2
    }
    else
    {
        LED2_OFF();  // 熄灭LED2
    }
}
