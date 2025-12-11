#include "uart_app.h"
#include "data_app.h"
extern uint8_t set_value_flag;//0无需设置，1设置ratio 2设置limit

// 外部变量声明
extern file_counters_t g_file_counters;  // 文件计数器
// 测试函数声明
static uint8_t Test_Flash(void);
static uint8_t Test_SDCard(void);
static uint8_t Test_Flash_Silent(uint32_t* flash_id);
static uint8_t Test_SDCard_Silent(void);
static void Process_Test_Command(void);
//static void Process_Write_Config_Command(void); // 新增：写入配置命令

// 采样控制相关函数声明
static void Process_Sampling_Command(void); // 新增：采样控制命令

// 数据存储相关函数声明
static void Process_Storage_Command(void); // 新增：数据存储命令

// RTC相关函数声明
static void Process_RTC_Command(void);
static void Show_Current_Time(void);
static uint8_t Parse_And_Set_RTC_Time(char* time_str);

// 配置文件相关函数声明已移至SDcard_app.h

// 状态变量
static uint8_t rtc_config_mode = 0;  // 0: 正常模式, 1: 等待RTC配置数据

/************************ usartִ�к��� ************************/
void Uart_Proc(void)
{
    if(uart_flag == 0) return;

    // 检查环形缓冲区是否有数据
    if(uart_dma_buffer.itemCount > 0)
    {
        uint32_t read_size = uart_dma_buffer.itemCount;
        if(read_size > (UART_RX_BUFFER_SIZE - 1))  // 保留一个字节给字符串结束符
        {
            read_size = UART_RX_BUFFER_SIZE - 1;
        }

        // 从环形缓冲区读取数据
        if(ringbuffer_read(&uart_dma_buffer, uart_read_buffer, read_size) == 0)
        {
            uart_read_buffer[read_size] = '\0';  // 添加字符串结束符

            // 处理接收到的数据
            if(rtc_config_mode == 1)
            {
                // RTC配置模式，解析时间数据
                Parse_And_Set_RTC_Time((char*)uart_read_buffer);
                rtc_config_mode = 0;  // 退出配置模式
            }
            else
            {
				//Process_Write_Config_Command(); // 处理写入配置命令
                Process_Sampling_Command(); // 处理采样控制命令
                if(set_value_flag==0){
                Process_Sampling_Command(); // 处理采样控制命令
                Process_Storage_Command();  // 处理数据存储命令
                Process_Test_Command();
                Process_RTC_Command();
                Process_Config_Management((char*)uart_read_buffer);
                Process_Data_Command((char*)uart_read_buffer);}
                else if(set_value_flag==1)
                {
                    float nowvalue=atof((char*)uart_read_buffer);
                    Set_New_Ratio(nowvalue);
                    set_value_flag=0;
                }
                else if(set_value_flag==2)
                {
                    float nowvalue=atof((char*)uart_read_buffer);
                    Set_New_Limit(nowvalue);
                    set_value_flag=0;
                }
            }

            // 清空缓冲区
            memset(uart_read_buffer, 0, sizeof(uart_read_buffer));
        }
    }

    uart_flag = 0;
}

/************************ 处理测试命令 ************************/
static void Process_Test_Command(void)
{
    // 检查是否接收到"test"命令
    if(strstr((char*)uart_read_buffer, "test") != NULL)
    {
        uint8_t flash_result, sdcard_result;
        uint32_t flash_id,tf_card_size;
        uint8_t hour, min, sec, ampm;
        uint8_t year, month, date, week;

        // 记录test命令到日志
        Buffer_Log_Data("UART command: test");
        my_printf(USART0, "=======system selftest=======\r\n");

        // 测试Flash
        flash_result = Test_Flash_Silent(&flash_id);
        if(flash_result)
        {
            my_printf(USART0, "flash............ok\r\n");
        }
        else
        {
            my_printf(USART0, "flash............error\r\n");
        }

        // 测试SD卡
        sdcard_result = Test_SDCard_Silent();
        if(sdcard_result)
        {
            my_printf(USART0, "TF card............ok\r\n");
            // 获取SD卡容量
            tf_card_size = sd_card_capacity_get();
        }
        else
        {
            my_printf(USART0, "TF card............error\r\n");
            tf_card_size = 0;  // SD卡错误时设置容量为0
        }

        // 输出Flash ID
        my_printf(USART0, "flash ID: 0x%06X\r\n", flash_id);

        // 输出TF卡信息
        if(sdcard_result)
        {
            my_printf(USART0, "TF card memory: %lu KB\r\n", tf_card_size);
        }
        else
        {
            my_printf(USART0, "can not find TF card\r\n");
        }

        // 获取并输出RTC时间
        rtc_get_time(&hour, &min, &sec, &ampm);
        rtc_get_date(&year, &month, &date, &week);
        my_printf(USART0, "RTC: 20%02d-%02d-%02d %02d:%02d:%02d\r\n",
                  year, month, date, hour, min, sec);

        my_printf(USART0, "=======system selftest=======\r\n");
    }
}

/************************ Flash测试函数 ************************/
static uint8_t Test_Flash(void)
{
    uint32_t flash_id = 0;

    my_printf(USART0, "Testing Flash...\r\n");

    // 读取Flash ID
    flash_id = spi_flash_read_id();

    my_printf(USART0, "Flash ID: 0x%06X\r\n", flash_id);

    // 检查Flash ID是否有效（非0x000000和0xFFFFFF）
    if(flash_id != 0x000000 && flash_id != 0xFFFFFF && flash_id != 0x000000)
    {
        my_printf(USART0, "Flash ID read successfully\r\n");
        return 1; // 测试通过
    }
    else
    {
        my_printf(USART0, "Flash ID read failed\r\n");
        return 0; // 测试失败
    }
}

/************************ Flash测试函数(静默版本) ************************/
static uint8_t Test_Flash_Silent(uint32_t* flash_id)
{
    // 读取Flash ID
    *flash_id = spi_flash_read_id();

    // 检查Flash ID是否有效（非0x000000和0xFFFFFF）
    if(*flash_id != 0x000000 && *flash_id != 0xFFFFFF)
    {
        return 1; // 测试通过
    }
    else
    {
        return 0; // 测试失败
    }
}

/************************ SD卡测试函数 ************************/
static uint8_t Test_SDCard(void)
{
    DSTATUS stat = 0;
    uint8_t retry_count = 3;

    my_printf(USART0, "Testing SD Card...\r\n");

    // 尝试初始化SD卡
    do
    {
        stat = disk_initialize(0);
        retry_count--;
    } while((stat != 0) && (retry_count > 0));

    my_printf(USART0, "SD Card Status: %d\r\n", stat);

    if(stat == 0)  // RES_OK
    {
        my_printf(USART0, "SD Card detected and initialized successfully\r\n");
        return 1; // 测试通过
    }
    else
    {
        my_printf(USART0, "SD Card not detected or initialization failed\r\n");
        return 0; // 测试失败
    }
}


/************************ RTC命令处理函数 ************************/
static void Process_RTC_Command(void)
{
    // 检查是否接收到"RTC Config"命令
    if(strstr((char*)uart_read_buffer, "RTC Config") != NULL)
    {
        // 记录RTC Config命令到日志
        Buffer_Log_Data("UART command: RTC Config");
        my_printf(USART0, "Input Datetime\r\n");
        rtc_config_mode = 1;  // 进入RTC配置模式
    }
    // 检查是否接收到"RTC now"命令
    else if(strstr((char*)uart_read_buffer, "RTC now") != NULL)
    {
        Show_Current_Time();
    }
}

/************************ 显示当前时间函数 ************************/
static void Show_Current_Time(void)
{
    uint8_t hour, min, sec, ampm;
    uint8_t year, month, date, week;

    // 获取当前时间和日期
    rtc_get_time(&hour, &min, &sec, &ampm);
    rtc_get_date(&year, &month, &date, &week);

    // 格式化输出：Current Time：2025-01-01 01-30-10
    my_printf(USART0, "Current Time: 20%02d-%02d-%02d %02d:%02d:%02d\r\n",
              year, month, date, hour, min, sec);
}

/************************ 解析并设置RTC时间函数 ************************/
static uint8_t Parse_And_Set_RTC_Time(char* time_str)
{
    uint16_t year;
    uint8_t month, date, hour, min, sec;
    uint8_t week;

    // 解析时间字符串格式：YYYY-MM-DD HH-MM-SS
    // 例如：2025-01-01 01-30-10
    int parsed = sscanf(time_str, "%4d-%2hhu-%2hhu %2hhu:%2hhu:%2hhu",
                       &year, &month, &date, &hour, &min, &sec);

    if(parsed != 6)
    {
        return 0; // 解析失败
    }

    // 验证数据范围
    if(year < 2000 || year > 2099 ||
       month < 1 || month > 12 ||
       date < 1 || date > 31 ||
       hour > 23 || min > 59 || sec > 59)
    {
        return 0; // 数据范围错误
    }

    // 计算星期几
    week = rtc_get_week(year, month, date);

    // 设置日期（年份转换为两位数）
    if(rtc_set_date(year - 2000, month, date, week) != 0)
    {
        return 0; // 设置日期失败
    }

    // 设置时间（24小时制，ampm=0）
    if(rtc_set_time(hour, min, sec, 0) != 0)
    {
        return 0; // 设置时间失败
    }
    my_printf(USART0,"RTC Config success\r\n");

    my_printf(USART0, "Time:%04d-%02d-%02d %02d:%02d:%02d\r\n",
              year, month, date, hour, min, sec);

    return 1; // 设置成功
}
/************************ 配置文件命令处理函数 ************************/
static void Process_Config_Command(void)
{
    // 检查是否接收到"conf"命令
    if(strstr((char*)uart_read_buffer, "conf") != NULL)
    {
        // 记录conf命令到日志
        Buffer_Log_Data("UART command: conf");

        // 调用SDcard_app中的配置读取函数
        // 函数内部已经处理了所有的错误情况和输出
        Read_Config_From_SDCard();
    }
}



/************************ 采样控制命令处理函数 ************************/
static void Process_Sampling_Command(void)
{
    // 检查是否接收到"start"命令
    if(strstr((char*)uart_read_buffer, "start") != NULL)
    {
        // my_printf(USART0, "DEBUG: UART: Received 'start' command\r\n");
        Buffer_Log_Data("UART command: start");
        Unlock_Sampling_Control();  // 先解锁
        Sampling_Control_Start();   // 再启动
    }
    // 检查是否接收到"stop"命令
    else if(strstr((char*)uart_read_buffer, "stop") != NULL)
    {
        // my_printf(USART0, "DEBUG: UART: Received 'stop' command\r\n");
        Buffer_Log_Data("UART command: stop");
        Sampling_Control_Stop();
    }
    // 检查是否接收到"reset"命令
    else if(strstr((char*)uart_read_buffer, "reset") != NULL)
    {
        // my_printf(USART0, "DEBUG: UART: Received 'reset' command\r\n");
        Buffer_Log_Data("UART command: reset");
        Sampling_Control_Reset();
    }
}

/************************ 数据存储命令处理函数 ************************/
static void Process_Storage_Command(void)
{
    // 检查是否接收到"storage normal"命令
    if(strstr((char*)uart_read_buffer, "storage normal") != NULL)
    {
        Set_Storage_Mode(STORAGE_MODE_NORMAL);
        Buffer_Log_Data("Storage mode set to NORMAL");
        my_printf(USART0, "Storage mode set to NORMAL\r\n");
    }
    // 检查是否接收到"storage encrypt"命令
    else if(strstr((char*)uart_read_buffer, "storage encrypt") != NULL)
    {
        Set_Storage_Mode(STORAGE_MODE_ENCRYPT);
        Buffer_Log_Data("Storage mode set to ENCRYPT");
        my_printf(USART0, "Storage mode set to ENCRYPT\r\n");
    }
    // 检查是否接收到"storage status"命令
    else if(strstr((char*)uart_read_buffer, "storage status") != NULL)
    {
        storage_mode_t current_mode = Get_Storage_Mode();
        if(current_mode == STORAGE_MODE_NORMAL) {
            my_printf(USART0, "Current storage mode: NORMAL\r\n");
        } else {
            my_printf(USART0, "Current storage mode: ENCRYPT\r\n");
        }
        my_printf(USART0, "Boot count: %lu\r\n", g_boot_count);
    }
    // 检查是否接收到"storage test"命令
    else if(strstr((char*)uart_read_buffer, "storage test") != NULL)
    {
        my_printf(USART0, "Starting data storage system test...\r\n");
        Buffer_Log_Data("Storage system test started");
        if(Test_Data_Storage_System()) {
            my_printf(USART0, "Data storage system test PASSED\r\n");
            Buffer_Log_Data("Storage system test PASSED");
        } else {
            my_printf(USART0, "Data storage system test FAILED\r\n");
            Buffer_Log_Data("Storage system test FAILED");
        }
    }
    // 检查是否接收到"buffer status"命令
    else if(strstr((char*)uart_read_buffer, "buffer status") != NULL)
    {
        uint8_t data_count = Get_Buffer_Data_Count();
        uint8_t log_count = Get_Buffer_Log_Count();
        my_printf(USART0, "Buffer status:\r\n");
        my_printf(USART0, "Data buffer: %d/%d\r\n", data_count, DATA_BUFFER_SIZE);
        my_printf(USART0, "Log buffer: %d/%d\r\n", log_count, LOG_BUFFER_SIZE);
        my_printf(USART0, "Storage system ready: %s\r\n", g_storage_system_ready ? "YES" : "NO");
    }
    // 检查是否接收到"test save"命令
    else if(strstr((char*)uart_read_buffer, "test save") != NULL)
    {
        my_printf(USART0, "Testing data save functions...\r\n");

        // 创建测试数据
        sample_data_t test_data;
        test_data.timestamp = DateTime_To_Unix(2025, 1, 1, 12, 30, 45);
        test_data.voltage = 3.25f;
        test_data.over_limit = 0;

        // 测试缓冲区存储
        my_printf(USART0, "Testing buffer functions:\r\n");
        uint8_t result1 = Buffer_Sample_Data(&test_data);
        my_printf(USART0, "Buffer_Sample_Data: %d\r\n", result1);

        test_data.over_limit = 1;
        test_data.voltage = 5.0f;
        uint8_t result2 = Buffer_OverLimit_Data(&test_data);
        my_printf(USART0, "Buffer_OverLimit_Data: %d\r\n", result2);

        uint8_t result3 = Buffer_HideData(&test_data);
        my_printf(USART0, "Buffer_HideData: %d\r\n", result3);

        // 测试直接存储
        my_printf(USART0, "Testing direct save functions:\r\n");
        uint8_t result4 = Save_Sample_Data(&test_data);
        my_printf(USART0, "Save_Sample_Data: %d\r\n", result4);

        my_printf(USART0, "Test completed\r\n");
    }
    // 检查是否接收到"test overlimit"命令
    else if(strstr((char*)uart_read_buffer, "test overlimit") != NULL)
    {
        my_printf(USART0, "Testing OverLimit data save specifically...\r\n");

        // 创建超限测试数据
        sample_data_t test_data;
        test_data.timestamp = DateTime_To_Unix(2025, 1, 1, 12, 30, 45);
        test_data.voltage = 5.5f;  // 超限电压
        test_data.over_limit = 1;

        my_printf(USART0, "Test data: voltage=%.3f, over_limit=%d\r\n",
                  test_data.voltage, test_data.over_limit);

        // 测试直接存储超限数据
        my_printf(USART0, "Testing Save_OverLimit_Data directly...\r\n");
        uint8_t result = Save_OverLimit_Data(&test_data);
        my_printf(USART0, "Save_OverLimit_Data result: %d\r\n", result);

        // 测试缓冲区存储超限数据
        my_printf(USART0, "Testing Buffer_OverLimit_Data...\r\n");
        uint8_t result2 = Buffer_OverLimit_Data(&test_data);
        my_printf(USART0, "Buffer_OverLimit_Data result: %d\r\n", result2);

        my_printf(USART0, "OverLimit test completed\r\n");
    }
    // 检查是否接收到"set time"命令
    else if(strstr((char*)uart_read_buffer, "set time") != NULL)
    {
        my_printf(USART0, "Setting RTC time to default values...\r\n");

        // 设置默认时间：2025-01-01 12:00:00
        rtc_set_date(25, 1, 1, 2);  // 年(25=2025), 月(1), 日(1), 星期(2=周二)
        rtc_set_time(12, 0, 0, 0);  // 时(12), 分(0), 秒(0), AM/PM(0=24小时制)

        my_printf(USART0, "RTC time set to: 2025-01-01 12:00:00\r\n");

        // 验证设置结果
        uint8_t hour, min, sec, ampm;
        uint8_t year, month, date, week;
        rtc_get_time(&hour, &min, &sec, &ampm);
        rtc_get_date(&year, &month, &date, &week);

        my_printf(USART0, "Current RTC time: 20%02d-%02d-%02d %02d:%02d:%02d\r\n",
                  year, month, date, hour, min, sec);
    }
    // 检查是否接收到"test storage logic"命令
    else if(strstr((char*)uart_read_buffer, "test storage logic") != NULL)
    {
        my_printf(USART0, "Testing storage logic based on voltage limit...\r\n");

        extern config_params_t current_config;
        my_printf(USART0, "Current voltage limit: %.3f V\r\n", current_config.limit);

        // 测试正常电压数据（不超限）
        sample_data_t test_data1;
        test_data1.timestamp = DateTime_To_Unix(2025, 1, 1, 12, 30, 45);
        test_data1.voltage = current_config.limit - 0.5f;  // 低于限值
        test_data1.over_limit = 0;

        uint8_t result1 = Buffer_Sample_Data(&test_data1);

        // 测试超限电压数据
        sample_data_t test_data2;
        test_data2.timestamp = DateTime_To_Unix(2025, 1, 1, 12, 30, 50);
        test_data2.voltage = current_config.limit + 0.5f;  // 高于限值
        test_data2.over_limit = 1;


        uint8_t result2 = Buffer_OverLimit_Data(&test_data2);

    }
        // 检查是否接收到"unhide"命令
    else if(strstr((char*)uart_read_buffer, "unhide") != NULL)
    {
        Set_Storage_Mode(STORAGE_MODE_NORMAL);


        // 记录模式切换日志
        Buffer_Log_Data("Storage mode set to NORMAL");
    }
    
    // 检查是否接收到"hide"命令
    else if(strstr((char*)uart_read_buffer, "hide") != NULL)
    {
        Set_Storage_Mode(STORAGE_MODE_ENCRYPT);

        // 记录模式切换日志
        Buffer_Log_Data("Storage mode set to ENCRYPT");
    }

    // 检查是否接收到"test encrypt"命令
    else if(strstr((char*)uart_read_buffer, "test encrypt") != NULL)
    {
        my_printf(USART0, "Testing encryption data format...\r\n");

        // 创建测试数据
        sample_data_t test_data;
        test_data.timestamp = DateTime_To_Unix(2025, 1, 1, 12, 30, 45); // 示例时间戳
        test_data.voltage = 12.5f;  // 示例电压
        test_data.over_limit = 0;

        my_printf(USART0, "Test data: 2025-01-01 12:30:45, voltage: %.3f V\r\n", test_data.voltage);

        // 测试加密编码
        char hex_string[17];
        Encode_Sample_Data_To_Hex(&test_data, hex_string);
        my_printf(USART0, "Expected result: 6774C4F5000C8000\r\n");
        my_printf(USART0, "Actual result:   %s\r\n", hex_string);

        if(strcmp(hex_string, "6774C4F5000C8000") == 0) {
            my_printf(USART0, "Encryption test PASSED!\r\n");
        } else {
            my_printf(USART0, "Encryption test FAILED!\r\n");
        }

        // 测试保存加密数据
        my_printf(USART0, "Testing encrypted data save...\r\n");
        uint8_t result1 = Save_Encrypted_Data(&test_data);
        my_printf(USART0, "Save_Encrypted_Data result: %d\r\n", result1);

        my_printf(USART0, "Encryption test completed\r\n");
        my_printf(USART0, "Note: Only encrypted data is saved, no verification files\r\n");
    }
    // 检查是否接收到"verify counters"命令
    else if(strstr((char*)uart_read_buffer, "verify counters") != NULL)
    {
        my_printf(USART0, "Verifying file counters integrity...\r\n");

        // 显示当前文件计数器状态
        my_printf(USART0, "Current file counters:\r\n");
        my_printf(USART0, "Sample count: %d, file: %s\r\n",
                  g_file_counters.sample_count, g_file_counters.current_sample_file);
        my_printf(USART0, "OverLimit count: %d, file: %s\r\n",
                  g_file_counters.overlimit_count, g_file_counters.current_overlimit_file);
        my_printf(USART0, "HideData count: %d, file: %s\r\n",
                  g_file_counters.hidedata_count, g_file_counters.current_hidedata_file);
        my_printf(USART0, "Log file: %s\r\n", g_file_counters.current_log_file);

        // 验证完整性
        Verify_File_Counters_Integrity();

        my_printf(USART0, "File counters verification completed\r\n");
    }
    // 检查是否接收到"reset counters"命令
    else if(strstr((char*)uart_read_buffer, "reset counters") != NULL)
    {
        my_printf(USART0, "Resetting file counters...\r\n");

        Reset_File_Counters();

        my_printf(USART0, "File counters reset completed\r\n");
        my_printf(USART0, "Next data will create new files\r\n");
    }
    // 检查是否接收到"test filename"命令
    else if(strstr((char*)uart_read_buffer, "test filename") != NULL)
    {
        my_printf(USART0, "Testing filename length and creation...\r\n");

        // 测试长文件名生成
        char datetime_str[DATETIME_STR_LEN];
        char test_filename[64];

        Generate_DateTime_String(datetime_str);
        sprintf(test_filename, "%s/overLimit%s.txt", OVERLIMIT_FOLDER_PATH, datetime_str);

        my_printf(USART0, "Generated filename: '%s'\r\n", test_filename);
        my_printf(USART0, "Filename length: %d characters\r\n", strlen(test_filename));
        my_printf(USART0, "Buffer size: 64 characters\r\n");

        if(strlen(test_filename) < 64) {
            my_printf(USART0, "Filename length OK - fits in buffer\r\n");
        } else {
            my_printf(USART0, "WARNING: Filename too long for buffer!\r\n");
        }

        // 测试文件创建
        FIL test_file;  // 使用局部文件对象
        FRESULT res = f_open(&test_file, test_filename, FA_CREATE_ALWAYS | FA_WRITE);
        if(res == FR_OK) {
            f_close(&test_file);
            my_printf(USART0, "File creation test: SUCCESS\r\n");

            // 删除测试文件
            f_unlink(test_filename);
            my_printf(USART0, "Test file cleaned up\r\n");
        } else {
            my_printf(USART0, "File creation test: FAILED (res: %d)\r\n", res);
        }

        // 测试当前文件计数器状态
        my_printf(USART0, "Current overlimit filename: '%s'\r\n", g_file_counters.current_overlimit_file);
        my_printf(USART0, "Current overlimit filename length: %d\r\n", strlen(g_file_counters.current_overlimit_file));

        my_printf(USART0, "Filename test completed\r\n");
    }
    // 检查是否接收到"test limit"命令
    else if(strstr((char*)uart_read_buffer, "test limit") != NULL)
    {
        my_printf(USART0, "Testing limit parameter validation...\r\n");

        extern config_params_t current_config;
        my_printf(USART0, "Current limit: %.1f\r\n", current_config.limit);

        // 测试有效值
        my_printf(USART0, "\nTest 1 - Valid limit (150.0):\r\n");
        uint8_t result1 = Set_New_Limit(150.0f);
        my_printf(USART0, "Result: %d\r\n", result1);

        // 测试边界值 - 最大有效值
        my_printf(USART0, "\nTest 2 - Boundary valid limit (200.0):\r\n");
        uint8_t result2 = Set_New_Limit(200.0f);
        my_printf(USART0, "Result: %d\r\n", result2);

        // 测试边界值 - 最小有效值
        my_printf(USART0, "\nTest 3 - Boundary valid limit (0.0):\r\n");
        uint8_t result3 = Set_New_Limit(0.0f);
        my_printf(USART0, "Result: %d\r\n", result3);

        // 测试无效值 - 超出上限
        my_printf(USART0, "\nTest 4 - Invalid limit (200.1):\r\n");
        uint8_t result4 = Set_New_Limit(200.1f);
        my_printf(USART0, "Result: %d (should be 0)\r\n", result4);

        // 测试无效值 - 负值
        my_printf(USART0, "\nTest 5 - Invalid limit (-1.0):\r\n");
        uint8_t result5 = Set_New_Limit(-1.0f);
        my_printf(USART0, "Result: %d (should be 0)\r\n", result5);

        // 测试无效值 - 远超上限
        my_printf(USART0, "\nTest 6 - Invalid limit (500.0):\r\n");
        uint8_t result6 = Set_New_Limit(500.0f);
        my_printf(USART0, "Result: %d (should be 0)\r\n", result6);

        my_printf(USART0, "\nFinal limit: %.1f\r\n", current_config.limit);
        my_printf(USART0, "Limit validation test completed\r\n");
    }
    // 检查是否接收到"test log"命令
    else if(strstr((char*)uart_read_buffer, "test log") != NULL)
    {
        my_printf(USART0, "Testing command logging functionality...\r\n");

        // 显示当前日志缓冲区状态
        uint8_t log_count = Get_Buffer_Log_Count();
        my_printf(USART0, "Current log buffer count: %d\r\n", log_count);

        // 测试各种命令的日志记录
        my_printf(USART0, "Testing command log entries:\r\n");

        // 模拟记录各种命令
        Buffer_Log_Data("UART command: test (simulated)");
        Buffer_Log_Data("UART command: RTC Config (simulated)");
        Buffer_Log_Data("UART command: ratio (simulated)");
        Buffer_Log_Data("UART command: limit (simulated)");
        Buffer_Log_Data("UART command: config save (simulated)");
        Buffer_Log_Data("UART command: config read (simulated)");
        Buffer_Log_Data("UART command: conf (simulated)");

        // 显示更新后的日志缓冲区状态
        log_count = Get_Buffer_Log_Count();
        my_printf(USART0, "Updated log buffer count: %d\r\n", log_count);

        my_printf(USART0, "Command logging test completed\r\n");
        my_printf(USART0, "Note: Check log files in SD card for actual entries\r\n");
    }
    // 检查是否接收到"debug log"命令
    else if(strstr((char*)uart_read_buffer, "debug log") != NULL)
    {
        my_printf(USART0, "=== Log System Debug Information ===\r\n");

        // 检查存储系统状态
        extern uint8_t g_storage_system_ready;
        extern data_buffer_manager_t g_data_buffer_mgr;
        extern file_counters_t g_file_counters;

        my_printf(USART0, "Storage system ready: %s\r\n", g_storage_system_ready ? "YES" : "NO");
        my_printf(USART0, "Log buffer count: %d/%d\r\n", g_data_buffer_mgr.log_count, LOG_BUFFER_SIZE);
        my_printf(USART0, "Log write index: %d\r\n", g_data_buffer_mgr.log_write_index);
        my_printf(USART0, "Log read index: %d\r\n", g_data_buffer_mgr.log_read_index);
        my_printf(USART0, "Current log file: '%s'\r\n", g_file_counters.current_log_file);

        // 测试添加一条日志
        my_printf(USART0, "\nTesting log addition...\r\n");
        uint8_t before_count = g_data_buffer_mgr.log_count;
        uint8_t result = Buffer_Log_Data("DEBUG: Test log entry");
        uint8_t after_count = g_data_buffer_mgr.log_count;

        my_printf(USART0, "Before count: %d, After count: %d, Result: %d\r\n",
                  before_count, after_count, result);

        // 显示缓冲区内容
        my_printf(USART0, "\nLog buffer contents:\r\n");
        for(int i = 0; i < LOG_BUFFER_SIZE; i++) {
            if(g_data_buffer_mgr.log_buffer[i].valid) {
                my_printf(USART0, "[%d] Valid: '%s'\r\n", i, g_data_buffer_mgr.log_buffer[i].message);
            }
        }

        my_printf(USART0, "=== End Debug Information ===\r\n");
    }
}

/************************ SD卡测试函数(静默版本) ************************/
static uint8_t Test_SDCard_Silent(void)
{
    DSTATUS stat = 0;
    uint8_t retry_count = 3;

    // 尝试初始化SD卡
    do
    {
        stat = disk_initialize(0);
        retry_count--;
    } while((stat != 0) && (retry_count > 0));

    if(stat == 0)  // RES_OK
    {
        return 1; // 测试通过
    }
    else
    {
        return 0; // 测试失败
    }
}



