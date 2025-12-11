#include "data_save.h"
#include "data_app.h"
#include "SDcard_app.h"

// 全局变量定义
file_counters_t g_file_counters;
storage_mode_t g_storage_mode = STORAGE_MODE_NORMAL;
uint32_t g_boot_count = 0;
uint8_t g_storage_system_ready = 0; // 数据存储系统就绪标志
data_buffer_manager_t g_data_buffer_mgr; // 数据缓冲区管理器

// 文件系统变量定义
FATFS fs;       // 文件系统对象
FIL fdst;       // 文件对象

/************************ 数据存储系统初始化 ************************/
uint8_t Data_Save_Init(void)
{
    DSTATUS stat;
    FRESULT res;

    // 初始化文件计数器
    memset(&g_file_counters, 0, sizeof(file_counters_t));

    // 初始化数据缓冲区
    Data_Buffer_Init();

    // 初始化SD卡
//    my_printf(USART0,"DEBUG: Initializing SD card...\r\n");
    stat = disk_initialize(0);
    if(stat != 0) {
//        my_printf(USART0,"DEBUG: disk_initialize failed, stat: %d\r\n", stat);
        return 0; // SD卡初始化失败
    }
//    my_printf(USART0,"DEBUG: SD card initialized successfully\r\n");

    // 挂载文件系统
//    my_printf(USART0,"DEBUG: Mounting file system...\r\n");
    res = f_mount(0, &fs);
    if(res != FR_OK) {
//        my_printf(USART0,"DEBUG: mount failed, res: %d\r\n", res);
        return 0; // 文件系统挂载失败
    }
//    my_printf(USART0,"DEBUG: File system mounted successfully\r\n");

    // 创建存储文件夹
//    my_printf(USART0,"DEBUG: Creating storage folders...\r\n");
    if(!Create_Storage_Folders()) {
//        my_printf(USART0,"DEBUG: create folders failed\r\n");
        return 0; // 文件夹创建失败
    }
//    my_printf(USART0,"DEBUG: Storage folders created successfully\r\n");

    // 加载上电次数
    if(!Load_Boot_Count_From_Flash()) {
//        my_printf(USART0,"boot count from flash failed\r\n");
        g_boot_count = 0; // 如果加载失败，从0开始
    }

    // 先创建日志文件（使用当前boot count值）
    if(!Create_Log_File()) {
//        my_printf(USART0,"create log file failed\r\n");
        return 0; // 日志文件创建失败
    }

    // 然后增加上电次数并保存
    g_boot_count++;
    Save_Boot_Count_To_Flash(g_boot_count);

    // 验证并初始化RTC时间
    Verify_And_Init_RTC_Time();

    // 记录系统启动日志
    Save_Log_Data("System boot - Data storage initialized");

    // 设置系统就绪标志
    g_storage_system_ready = 1;

    return 1; // 初始化成功
}

/************************ 创建存储文件夹 ************************/
uint8_t Create_Storage_Folders(void)
{
    FRESULT res;

    // 创建sample文件夹
    res = f_mkdir(SAMPLE_FOLDER_PATH);
    if(res != FR_OK && res != FR_EXIST) {
        my_printf(USART0,"sample res:%d",res);
        return 0; // 创建失败且不是因为已存在
    }

    // 创建overLimit文件夹
    res = f_mkdir(OVERLIMIT_FOLDER_PATH);
    if(res != FR_OK && res != FR_EXIST) {
        my_printf(USART0,"overlimit res:%d",res);
        return 0;
    }

    // 创建log文件夹
    res = f_mkdir(LOG_FOLDER_PATH);
    if(res != FR_OK && res != FR_EXIST) {
        my_printf(USART0,"log res:%d",res);
        return 0;
    }

    // 创建hideData文件夹
    res = f_mkdir(HIDEDATA_FOLDER_PATH);
    if(res != FR_OK && res != FR_EXIST) {
        my_printf(USART0,"hideData res:%d",res);
        return 0;
    }

    return 1; // 所有文件夹创建成功
}

/************************ 从Flash加载上电次数 ************************/
uint8_t Load_Boot_Count_From_Flash(void)
{
    uint8_t data_buffer[4];

    // 从Flash读取上电次数数据
    
    spi_flash_buffer_read(data_buffer, BOOT_COUNT_FLASH_ADDR, 4);
//    my_printf(USART0, "Data_Buffer: %02X %02X %02X %02X\r\n", 
//          data_buffer[0], data_buffer[1], 
//          data_buffer[2], data_buffer[3]);

    // 检查数据有效性（非全0和全FF）
    if(data_buffer[0] == 0xFF && data_buffer[1] == 0xFF && data_buffer[2] == 0xFF && data_buffer[3] == 0xFF) {
        g_boot_count = 0;
        return 0; // 无有效数据
    }

    // 重组32位数据
    g_boot_count = (uint32_t)data_buffer[0] |
                   ((uint32_t)data_buffer[1] << 8) |
                   ((uint32_t)data_buffer[2] << 16) |
                   ((uint32_t)data_buffer[3] << 24);

    // Flash操作后验证文件计数器完整性
    Verify_File_Counters_Integrity();

    return 1; // 加载成功
}

/************************ 保存上电次数到Flash ************************/
uint8_t Save_Boot_Count_To_Flash(uint32_t boot_count)
{
    uint8_t data_buffer[4];

    // 将32位数据分解为4个字节
    data_buffer[0] = (uint8_t)(boot_count & 0xFF);
    data_buffer[1] = (uint8_t)((boot_count >> 8) & 0xFF);
    data_buffer[2] = (uint8_t)((boot_count >> 16) & 0xFF);
    data_buffer[3] = (uint8_t)((boot_count >> 24) & 0xFF);

    // 擦除Flash扇区
    spi_flash_sector_erase(BOOT_COUNT_FLASH_ADDR);
    delay_1ms(100); // 等待擦除完成

    // 写入数据到Flash
    spi_flash_buffer_write(data_buffer, BOOT_COUNT_FLASH_ADDR, 4);
    delay_1ms(50); // 等待写入完成

    // Flash操作后验证文件计数器完整性
    Verify_File_Counters_Integrity();

    return 1; // 保存成功
}

/************************ 创建日志文件 ************************/
uint8_t Create_Log_File(void)
{
    FRESULT res;
    char log_filename[32];

    // 生成日志文件名：log{id}.txt
    sprintf(log_filename, "%s/log%lu.txt", LOG_FOLDER_PATH, g_boot_count);
    strcpy(g_file_counters.current_log_file, log_filename);

    // 创建日志文件
    res = f_open(&fdst, log_filename, FA_CREATE_ALWAYS | FA_WRITE);
    if(res != FR_OK) {
        return 0; // 文件创建失败
    }

    f_close(&fdst);
    return 1; // 文件创建成功
}

/************************ 验证并初始化RTC时间 ************************/
void Verify_And_Init_RTC_Time(void)
{
    uint8_t hour, min, sec, ampm;
    uint8_t year, month, date, week;

    // 获取当前RTC时间
    rtc_get_time(&hour, &min, &sec, &ampm);
    rtc_get_date(&year, &month, &date, &week);

//    my_printf(USART0, "DEBUG: Current RTC - Y:%02d M:%02d D:%02d H:%02d M:%02d S:%02d\r\n",
//              year, month, date, hour, min, sec);

    // 检查RTC时间是否有效
    if(year == 0 || year > 99 || month == 0 || month > 12 ||
       date == 0 || date > 31 || hour > 23 || min > 59 || sec > 59) {

//        my_printf(USART0, "DEBUG: Invalid RTC time detected, setting default time\r\n");

        // 设置默认时间：2025-01-01 12:00:00
        rtc_set_date(25, 1, 1, 2);  // 年(25=2025), 月(1), 日(1), 星期(2=周二)
        rtc_set_time(12, 0, 0, 0);  // 时(12), 分(0), 秒(0), AM/PM(0=24小时制)

        // 验证设置结果
        rtc_get_time(&hour, &min, &sec, &ampm);
        rtc_get_date(&year, &month, &date, &week);

//        my_printf(USART0, "DEBUG: RTC time set to: 20%02d-%02d-%02d %02d:%02d:%02d\r\n",
//                  year, month, date, hour, min, sec);
    } else {
//        my_printf(USART0, "DEBUG: RTC time is valid: 20%02d-%02d-%02d %02d:%02d:%02d\r\n",
//                  year, month, date, hour, min, sec);
    }
}

/************************ 生成14位时间戳字符串 ************************/
void Generate_DateTime_String(char* datetime_str)
{
    uint8_t hour, min, sec, ampm;
    uint8_t year, month, date, week;
    static uint32_t file_counter = 0; // 静态计数器，确保文件名唯一

    // 获取当前时间和日期
    rtc_get_time(&hour, &min, &sec, &ampm);
    rtc_get_date(&year, &month, &date, &week);

    // 添加调试信息
//    my_printf(USART0, "DEBUG: RTC values - Y:%02d M:%02d D:%02d H:%02d M:%02d S:%02d\r\n",
//              year, month, date, hour, min, sec);

    // 更严格的RTC时间有效性检查
    if(year == 0 || year > 99 || month == 0 || month > 12 ||
       date == 0 || date > 31 || hour > 23 || min > 59 || sec > 59) {
//        my_printf(USART0, "DEBUG: Invalid RTC time detected, using default + counter\r\n");

        // 使用默认时间加上计数器确保唯一性
        year = 25;    // 2025年
        month = 1;    // 1月
        date = 1;     // 1日
        hour = 12;    // 12时
        min = (file_counter / 60) % 60;      // 分钟基于计数器
        sec = file_counter % 60;             // 秒基于计数器
        file_counter++;

//        my_printf(USART0, "DEBUG: Using default time with counter: %02d:%02d:%02d (counter: %lu)\r\n",
//                  hour, min, sec, file_counter);
    }

    // 生成14位时间戳：YYYYMMDDHHMMSS
    sprintf(datetime_str, "20%02d%02d%02d%02d%02d%02d", year, month, date, hour, min, sec);

    // 验证生成的时间戳字符串
    if(strlen(datetime_str) != 14) {
//        my_printf(USART0, "DEBUG: Invalid datetime string length: %d, using fallback\r\n", strlen(datetime_str));
//        my_printf(USART0, "DEBUG: Original datetime string: '%s'\r\n", datetime_str);
        sprintf(datetime_str, "20250101120000"); // 使用固定的默认时间戳
    }

    // 确保字符串正确结束
    datetime_str[14] = '\0';

//    my_printf(USART0, "DEBUG: Generated datetime string: '%s' (length: %d)\r\n", datetime_str, strlen(datetime_str));
}

/************************ 生成sample文件名 ************************/
void Generate_Sample_Filename(char* filename)
{
    char datetime_str[DATETIME_STR_LEN];
    Generate_DateTime_String(datetime_str);
    sprintf(filename, "%s/sampleData%s.txt", SAMPLE_FOLDER_PATH, datetime_str);
}

/************************ 生成overLimit文件名 ************************/
void Generate_OverLimit_Filename(char* filename)
{
    char datetime_str[DATETIME_STR_LEN];
    Generate_DateTime_String(datetime_str);
    sprintf(filename, "%s/overLimit%s.txt", OVERLIMIT_FOLDER_PATH, datetime_str);
}

/************************ 生成hideData文件名 ************************/
void Generate_HideData_Filename(char* filename)
{
    char datetime_str[DATETIME_STR_LEN];
    Generate_DateTime_String(datetime_str);
    sprintf(filename, "%s/hideData%s.txt", HIDEDATA_FOLDER_PATH, datetime_str);
}

/************************ 检查并创建新文件 ************************/
uint8_t Check_And_Create_New_File(const char* folder_path, char* current_filename, uint8_t* record_count, const char* prefix)
{
    FRESULT res;
    char new_filename[64];
    char datetime_str[DATETIME_STR_LEN];

//    my_printf(USART0, "DEBUG: Check_And_Create_New_File - count: %d, filename: %s\r\n",
//              *record_count, current_filename);
//    my_printf(USART0, "DEBUG: folder_path: %s, prefix: %s\r\n", folder_path, prefix);

    // 如果记录数达到最大值或当前文件名为空，创建新文件
    if(*record_count >= MAX_RECORDS_PER_FILE || strlen(current_filename) == 0) {
        Generate_DateTime_String(datetime_str);

        // 验证输入参数
        if(folder_path == NULL || prefix == NULL || strlen(folder_path) == 0 || strlen(prefix) == 0) {
//            my_printf(USART0, "DEBUG: Invalid parameters for file creation\r\n");
            return 0;
        }

        // 构建完整文件路径（支持长文件名）
        sprintf(new_filename, "%s/%s%s.txt", folder_path, prefix, datetime_str);

//        my_printf(USART0, "DEBUG: File creation details:\r\n");
//        my_printf(USART0, "DEBUG: - folder_path: '%s'\r\n", folder_path);
//        my_printf(USART0, "DEBUG: - prefix: '%s'\r\n", prefix);
//        my_printf(USART0, "DEBUG: - datetime_str: '%s' (len: %d)\r\n", datetime_str, strlen(datetime_str));
//        my_printf(USART0, "DEBUG: - new_filename: '%s' (len: %d)\r\n", new_filename, strlen(new_filename));

        // 创建新文件（现在支持长文件名）
        // my_printf(USART0, "DEBUG: Attempting to create file: '%s'\r\n", new_filename);
        res = f_open(&fdst, new_filename, FA_CREATE_ALWAYS | FA_WRITE);
        if(res != FR_OK) {
            // my_printf(USART0, "DEBUG: File creation failed, res: %d, filename: '%s'\r\n", res, new_filename);

            // 尝试使用短文件名作为备用方案
            char short_filename[32];
            sprintf(short_filename, "%s/%s%lu.txt", folder_path, prefix, (uint32_t)(datetime_str[10] - '0') * 10 + (datetime_str[11] - '0'));
            // my_printf(USART0, "DEBUG: Trying short filename: '%s'\r\n", short_filename);

            res = f_open(&fdst, short_filename, FA_CREATE_ALWAYS | FA_WRITE);
            if(res != FR_OK) {
                // my_printf(USART0, "DEBUG: Short filename also failed, res: %d\r\n", res);
                return 0;
            }
            f_close(&fdst);

            // 使用短文件名
            strcpy(new_filename, short_filename);
//            my_printf(USART0, "DEBUG: Using short filename: '%s'\r\n", new_filename);
        } else {
            f_close(&fdst);
//            my_printf(USART0, "DEBUG: Long filename created successfully: '%s'\r\n", new_filename);
        }

        // 更新文件名和重置计数（使用安全的字符串复制）
        // 先清空目标缓冲区
        memset(current_filename, 0, 64);
        // 使用strncpy限制复制长度，避免缓冲区溢出
        strncpy(current_filename, new_filename, 63);
        current_filename[63] = '\0'; // 确保字符串结束
        *record_count = 0;

//        my_printf(USART0, "DEBUG: Updated filename: %s\r\n", current_filename);
    }

    return 1; // 成功
}

/************************ 重置文件计数器 ************************/
void Reset_File_Counters(void)
{
//    my_printf(USART0, "DEBUG: Resetting file counters\r\n");

    // 清零所有计数器
    g_file_counters.sample_count = 0;
    g_file_counters.overlimit_count = 0;
    g_file_counters.hidedata_count = 0;

    // 清空所有文件名（现在支持64字符长文件名）
    memset(g_file_counters.current_sample_file, 0, sizeof(g_file_counters.current_sample_file));
    memset(g_file_counters.current_overlimit_file, 0, sizeof(g_file_counters.current_overlimit_file));
    memset(g_file_counters.current_hidedata_file, 0, sizeof(g_file_counters.current_hidedata_file));
    memset(g_file_counters.current_log_file, 0, sizeof(g_file_counters.current_log_file));

//    my_printf(USART0, "DEBUG: File counters reset completed\r\n");
}

/************************ 验证文件计数器完整性 ************************/
void Verify_File_Counters_Integrity(void)
{
    uint8_t integrity_ok = 1;

//    my_printf(USART0, "DEBUG: Verifying file counters integrity\r\n");

    // 检查计数器是否在合理范围内
    if(g_file_counters.sample_count > MAX_RECORDS_PER_FILE) {
//        my_printf(USART0, "DEBUG: sample_count corrupted: %d\r\n", g_file_counters.sample_count);
        g_file_counters.sample_count = 0;
        integrity_ok = 0;
    }

    if(g_file_counters.overlimit_count > MAX_RECORDS_PER_FILE) {
//        my_printf(USART0, "DEBUG: overlimit_count corrupted: %d\r\n", g_file_counters.overlimit_count);
        g_file_counters.overlimit_count = 0;
        integrity_ok = 0;
    }

    if(g_file_counters.hidedata_count > MAX_RECORDS_PER_FILE) {
//        my_printf(USART0, "DEBUG: hidedata_count corrupted: %d\r\n", g_file_counters.hidedata_count);
        g_file_counters.hidedata_count = 0;
        integrity_ok = 0;
    }

    // 检查文件名是否包含有效路径
    if(strlen(g_file_counters.current_overlimit_file) > 0) {
        if(strstr(g_file_counters.current_overlimit_file, OVERLIMIT_FOLDER_PATH) == NULL) {
//            my_printf(USART0, "DEBUG: overlimit filename corrupted: %s\r\n", g_file_counters.current_overlimit_file);
            memset(g_file_counters.current_overlimit_file, 0, sizeof(g_file_counters.current_overlimit_file));
            g_file_counters.overlimit_count = 0;
            integrity_ok = 0;
        }
    }

    if(strlen(g_file_counters.current_sample_file) > 0) {
        if(strstr(g_file_counters.current_sample_file, SAMPLE_FOLDER_PATH) == NULL) {
//            my_printf(USART0, "DEBUG: sample filename corrupted: %s\r\n", g_file_counters.current_sample_file);
            memset(g_file_counters.current_sample_file, 0, sizeof(g_file_counters.current_sample_file));
            g_file_counters.sample_count = 0;
            integrity_ok = 0;
        }
    }

    if(strlen(g_file_counters.current_hidedata_file) > 0) {
        if(strstr(g_file_counters.current_hidedata_file, HIDEDATA_FOLDER_PATH) == NULL) {
//            my_printf(USART0, "DEBUG: hidedata filename corrupted: %s\r\n", g_file_counters.current_hidedata_file);
            memset(g_file_counters.current_hidedata_file, 0, sizeof(g_file_counters.current_hidedata_file));
            g_file_counters.hidedata_count = 0;
            integrity_ok = 0;
        }
    }

    if(integrity_ok) {
//        my_printf(USART0, "DEBUG: File counters integrity OK\r\n");
    } else {
//        my_printf(USART0, "DEBUG: File counters integrity FAILED - corrected\r\n");
    }
}

/************************ 安全创建超限文件 ************************/
uint8_t Create_OverLimit_File_Safe(void)
{
    FRESULT res;
    char new_filename[64];
    char datetime_str[DATETIME_STR_LEN];

//    my_printf(USART0, "DEBUG: Creating OverLimit file safely\r\n");

    // 生成新的时间戳
    Generate_DateTime_String(datetime_str);

    // 构建完整文件路径
    sprintf(new_filename, "%s/overLimit%s.txt", OVERLIMIT_FOLDER_PATH, datetime_str);

//    my_printf(USART0, "DEBUG: New OverLimit file: %s\r\n", new_filename);

    // 创建新文件
    res = f_open(&fdst, new_filename, FA_CREATE_ALWAYS | FA_WRITE);
    if(res != FR_OK) {
//        my_printf(USART0, "DEBUG: OverLimit file creation failed, res: %d\r\n", res);
        return 0;
    }
    f_close(&fdst);

    // 安全更新文件名（现在支持64字符长文件名）
    memset(g_file_counters.current_overlimit_file, 0, sizeof(g_file_counters.current_overlimit_file));
    strncpy(g_file_counters.current_overlimit_file, new_filename, 63);
    g_file_counters.current_overlimit_file[63] = '\0';
    g_file_counters.overlimit_count = 0;

//    my_printf(USART0, "DEBUG: OverLimit file created safely: %s\r\n", g_file_counters.current_overlimit_file);

    return 1;
}

/************************ 将采样数据编码为HEX字符串 ************************/
void Encode_Sample_Data_To_Hex(sample_data_t* data, char* hex_string)
{
    uint32_t timestamp = data->timestamp;
    float voltage = data->voltage;

    // 分离电压的整数部分和小数部分
    uint16_t voltage_int = (uint16_t)voltage;                    // 整数部分
    uint16_t voltage_frac = (uint16_t)((voltage - voltage_int) * 65536); // 小数部分

    // 调试信息
    // my_printf(USART0, "DEBUG: Encoding - timestamp: %lu, voltage: %.3f\r\n", timestamp, voltage);
    // my_printf(USART0, "DEBUG: Voltage parts - int: %d (0x%04X), frac: %d (0x%04X)\r\n",
    //           voltage_int, voltage_int, voltage_frac, voltage_frac);

    // 生成8字节HEX字符串：时间戳(4字节) + 电压整数部分(2字节) + 电压小数部分(2字节)
    sprintf(hex_string, "%08lX%04X%04X", timestamp, voltage_int, voltage_frac);
           // my_printf(USART0,"%s\r\n",hex_string);
    // 如果超限，添加*标记
    if(data->over_limit) {
        strcat(hex_string, "*");
        // my_printf(USART0, "DEBUG: Added * for over-limit data\r\n");
    }
    my_printf(USART0,"%s\r\n",hex_string);

    // my_printf(USART0, "DEBUG: Generated HEX string: %s\r\n", hex_string);
}

/************************ 保存加密数据 ************************/
uint8_t Save_Encrypted_Data(sample_data_t* data)
{
    FRESULT res;
    char hex_string[17]; // 16个HEX字符 + 结束符
    char line_buffer[32];

    // 检查存储系统是否就绪
    if(!g_storage_system_ready) {
//        my_printf(USART0, "DEBUG: Storage system not ready, skipping encrypted save\r\n");
        return 1; // 系统未就绪时直接返回成功，避免阻塞
    }

    // 检查并创建新的hideData文件
    if(!Check_And_Create_New_File(HIDEDATA_FOLDER_PATH,
                                  g_file_counters.current_hidedata_file,
                                  &g_file_counters.hidedata_count,
                                  "hideData")) {
        return 0; // 文件创建失败
    }
                                  


    // 编码数据为HEX字符串
    Encode_Sample_Data_To_Hex(data, hex_string);

    // 打开文件追加写入

    res = f_open(&fdst, g_file_counters.current_hidedata_file, FA_OPEN_ALWAYS | FA_WRITE);
    if(res != FR_OK) {
        // my_printf(USART0, "DEBUG: Encrypted file open failed, res: %d\r\n", res);
        return 0;
    }

    // 移动到文件末尾
    f_lseek(&fdst, f_size(&fdst));

    // 格式化数据行：HEX字符串 + 注释（可选）
    sprintf(line_buffer, "%s\r\n",
            hex_string);
    // 写入数据

    UINT bw;
    res = f_write(&fdst, line_buffer, strlen(line_buffer), &bw);
    f_close(&fdst);

    if(res == FR_OK && bw == strlen(line_buffer)) {
        g_file_counters.hidedata_count++;
        // my_printf(USART0, "DEBUG: Encrypted data saved successfully\r\n");
        return 1;
    } else {
        // my_printf(USART0, "DEBUG: Encrypted data write failed, res: %d, bw: %d\r\n", res, bw);
        return 0;
    }
}

/************************ 保存校验数据（未加密格式） ************************/
uint8_t Save_Verification_Data(sample_data_t* data)
{
    FRESULT res;
    char datetime_str[32];
    char line_buffer[128];
    char verify_filename[64];

    // 检查存储系统是否就绪
    if(!g_storage_system_ready) {
        // my_printf(USART0, "DEBUG: Storage system not ready, skipping verification save\r\n");
        return 1;
    }

    // 生成校验文件名（在hideData文件夹下）
    Generate_DateTime_String(datetime_str);
    sprintf(verify_filename, "%s/verify_%s.txt", HIDEDATA_FOLDER_PATH, datetime_str);

    // 转换时间戳为可读格式
    uint8_t hour, min, sec, year, month, date;
    Unix_To_DateTime(data->timestamp, &year, &month, &date, &hour, &min, &sec);

    // 打开校验文件（每次都创建新文件，便于对比）
    res = f_open(&fdst, verify_filename, FA_CREATE_ALWAYS | FA_WRITE);
    if(res != FR_OK) {
        // my_printf(USART0, "DEBUG: Verification file create failed, res: %d\r\n", res);
        return 0;
    }

    // 格式化校验数据（未加密的原始格式）
    sprintf(line_buffer, "Verification Data:\r\n");
    f_write(&fdst, line_buffer, strlen(line_buffer), NULL);

    sprintf(line_buffer, "Timestamp: %lu (Unix)\r\n", data->timestamp);
    f_write(&fdst, line_buffer, strlen(line_buffer), NULL);

    sprintf(line_buffer, "DateTime: 20%02d-%02d-%02d %02d:%02d:%02d\r\n",
            year, month, date, hour, min, sec);
    f_write(&fdst, line_buffer, strlen(line_buffer), NULL);

    sprintf(line_buffer, "Voltage: %.3f V\r\n", data->voltage);
    f_write(&fdst, line_buffer, strlen(line_buffer), NULL);

    sprintf(line_buffer, "Over_limit: %d\r\n", data->over_limit);
    f_write(&fdst, line_buffer, strlen(line_buffer), NULL);

    // 添加加密数据对照
    char hex_string[17];
    Encode_Sample_Data_To_Hex(data, hex_string);
    sprintf(line_buffer, "Encrypted: %s\r\n", hex_string);
    f_write(&fdst, line_buffer, strlen(line_buffer), NULL);

    f_close(&fdst);

    // my_printf(USART0, "DEBUG: Verification data saved to: %s\r\n", verify_filename);
    return 1;
}

/************************ 设置存储模式 ************************/
void Set_Storage_Mode(storage_mode_t mode)
{
    g_storage_mode = mode;

    // 记录模式切换日志（使用缓冲区方式）
    if(mode == STORAGE_MODE_ENCRYPT) {
        Buffer_Log_Data("Storage mode changed to ENCRYPT");
    } else {
        Buffer_Log_Data("Storage mode changed to NORMAL");
    }
}

/************************ 获取当前存储模式 ************************/
storage_mode_t Get_Storage_Mode(void)
{
    return g_storage_mode;
}

/************************ 保存采样数据 ************************/
uint8_t Save_Sample_Data(sample_data_t* data)
{
    FRESULT res;
    UINT bw;
    char data_line[128];
    uint16_t year;
    uint8_t month, day, hour, minute, second;

    // 检查存储系统是否就绪
    if(!g_storage_system_ready) {
        // my_printf(USART0, "DEBUG: Storage system not ready, skipping save\r\n");
        return 1; // 系统未就绪时直接返回成功，避免阻塞
    }

    // 如果是加密模式，不存储到sample文件夹
    if(g_storage_mode == STORAGE_MODE_ENCRYPT) {
        return 1; // 直接返回成功，不存储
    }

    // 检查数据指针有效性
    if(data == NULL) {
        return 0;
    }

    // 检查并创建新文件（添加错误处理）
    if(!Check_And_Create_New_File(SAMPLE_FOLDER_PATH, g_file_counters.current_sample_file,
                                  &g_file_counters.sample_count, "sampleData")) {
        return 0; // 文件创建失败，直接返回避免系统卡死
    }

    // 将Unix时间戳转换为日期时间
    Unix_To_DateTime(data->timestamp, &year, &month, &day, &hour, &minute, &second);

    // 格式化数据行：时间戳,电压值,超限标记
    sprintf(data_line, "%04d-%02d-%02d %02d:%02d:%02d,%.3f,%d\r\n",
            year, month, day, hour, minute, second, data->voltage, data->over_limit);

    // 打开文件追加写入（使用FA_OPEN_ALWAYS确保文件存在）
    res = f_open(&fdst, g_file_counters.current_sample_file, FA_OPEN_ALWAYS | FA_WRITE);
    if(res != FR_OK) {
        // 文件打开失败，可能SD卡有问题，直接返回避免系统卡死
        // my_printf(USART0, "DEBUG: Sample file open failed, res: %d\r\n", res);
        return 0;
    }

    // 移动到文件末尾
    res = f_lseek(&fdst, f_size(&fdst));
    if(res != FR_OK) {
        f_close(&fdst);
        return 0;
    }

    // 写入数据
    res = f_write(&fdst, data_line, strlen(data_line), &bw);
    f_close(&fdst); // 无论写入是否成功都要关闭文件

    if(res != FR_OK || bw != strlen(data_line)) {
        return 0; // 写入失败
    }

    // 增加记录计数
    g_file_counters.sample_count++;

    return 1; // 保存成功
}

/************************ 保存超限数据 ************************/
uint8_t Save_OverLimit_Data(sample_data_t* data)
{extern config_params_t current_config; 
    FRESULT res;
    UINT bw;
    char data_line[128];
    uint16_t year;
    uint8_t month, day, hour, minute, second;

    // 检查存储系统是否就绪
    if(!g_storage_system_ready) {
        return 1; // 系统未就绪时直接返回成功，避免阻塞
    }

    // 检查数据指针有效性
    if(data == NULL) {
        my_printf(USART0,"data is NULL\r\n");
        return 0;
    }

    // 检查是否需要创建新文件
    if(g_file_counters.overlimit_count >= MAX_RECORDS_PER_FILE ||
       strlen(g_file_counters.current_overlimit_file) == 0 ||
       strstr(g_file_counters.current_overlimit_file, OVERLIMIT_FOLDER_PATH) == NULL) {

        // my_printf(USART0, "DEBUG: Need to create new OverLimit file\r\n");
        // my_printf(USART0, "DEBUG: Current count: %d, filename: %s\r\n",
        //           g_file_counters.overlimit_count, g_file_counters.current_overlimit_file);

        // 使用安全的文件创建方法
        if(!Create_OverLimit_File_Safe()) {
            // my_printf(USART0, "DEBUG: Safe OverLimit file creation failed\r\n");
            return 0;
        }
    }

    // 将Unix时间戳转换为日期时间
    Unix_To_DateTime(data->timestamp, &year, &month, &day, &hour, &minute, &second);

    // 格式化数据行：时间戳,电压值,超限标记
    sprintf(data_line, "%04d-%02d-%02d %02d:%02d:%02d,%.3f,%d\r\n",
            year, month, day, hour, minute, second, data->voltage*current_config.ratio, data->over_limit);

    // 打开文件追加写入（使用FA_OPEN_ALWAYS确保文件存在）
    res = f_open(&fdst, g_file_counters.current_overlimit_file, FA_OPEN_ALWAYS | FA_WRITE);
    if(res != FR_OK) {
        // 文件打开失败，可能SD卡有问题，直接返回避免系统卡死
        // my_printf(USART0, "DEBUG: OverLimit file open failed, res: %d, file: %s\r\n",
        //           res, g_file_counters.current_overlimit_file);
        return 0;
    }

    // 移动到文件末尾
    res = f_lseek(&fdst, f_size(&fdst));
    if(res != FR_OK) {
        f_close(&fdst);
        my_printf(USART0,"data is 1\r\n");
        return 0;
    }

    // 写入数据
    res = f_write(&fdst, data_line, strlen(data_line), &bw);
    f_close(&fdst); // 无论写入是否成功都要关闭文件

    if(res != FR_OK || bw != strlen(data_line)) {
        my_printf(USART0,"data is 1\r\n");
        return 0; // 写入失败
    }

    // 增加记录计数
    g_file_counters.overlimit_count++;

    return 1; // 保存成功
}

/************************ 保存加密数据 ************************/
uint8_t Save_HideData(sample_data_t* data)
{
    FRESULT res;
    UINT bw;
    char data_line[128];
    char hex_data[64];
    uint16_t year;
    uint8_t month, day, hour, minute, second;

    // 检查存储系统是否就绪
    if(!g_storage_system_ready) {
        return 1; // 系统未就绪时直接返回成功，避免阻塞
    }

    // 检查数据指针有效性
    if(data == NULL) {
        return 0;
    }

    // 检查并创建新文件（添加错误处理）
    if(!Check_And_Create_New_File(HIDEDATA_FOLDER_PATH, g_file_counters.current_hidedata_file,
                                  &g_file_counters.hidedata_count, "hideData")) {
        return 0; // 文件创建失败，直接返回避免系统卡死
    }

    // 将数据编码为十六进制字符串
    Encode_Sample_Data(data, hex_data);

    // 将Unix时间戳转换为日期时间（用于注释）
    Unix_To_DateTime(data->timestamp, &year, &month, &day, &hour, &minute, &second);

    // 格式化数据行：加密数据,时间注释
    sprintf(data_line, "%s,# %04d-%02d-%02d %02d:%02d:%02d\r\n",
            hex_data, year, month, day, hour, minute, second);

    // 打开文件追加写入（使用FA_OPEN_ALWAYS确保文件存在）
    res = f_open(&fdst, g_file_counters.current_hidedata_file, FA_OPEN_ALWAYS | FA_WRITE);
    if(res != FR_OK) {
        // 文件打开失败，可能SD卡有问题，直接返回避免系统卡死
        // my_printf(USART0, "DEBUG: HideData file open failed, res: %d\r\n", res);
        return 0;
    }

    // 移动到文件末尾
    res = f_lseek(&fdst, f_size(&fdst));
    if(res != FR_OK) {
        f_close(&fdst);
        return 0;
    }

    // 写入数据
    res = f_write(&fdst, data_line, strlen(data_line), &bw);
    f_close(&fdst); // 无论写入是否成功都要关闭文件

    if(res != FR_OK || bw != strlen(data_line)) {
        return 0; // 写入失败
    }

    // 增加记录计数
    g_file_counters.hidedata_count++;

    return 1; // 保存成功
}

/************************ 保存日志数据 ************************/
uint8_t Save_Log_Data(const char* log_message)
{
    FRESULT res;
    UINT bw;
    char log_line[256];
    uint8_t hour, min, sec, ampm;
    uint8_t year, month, date, week;

    // 检查存储系统是否就绪
    if(!g_storage_system_ready) {
        // my_printf(USART0, "DEBUG: Storage system not ready, skipping log: '%s'\r\n", log_message);
        return 1; // 系统未就绪时直接返回成功，避免阻塞
    }

    // 获取当前时间
    rtc_get_time(&hour, &min, &sec, &ampm);
    rtc_get_date(&year, &month, &date, &week);

    // 格式化日志行：[时间戳] 日志消息
    sprintf(log_line, "[20%02d-%02d-%02d %02d:%02d:%02d] %s\r\n",
            year, month, date, hour, min, sec, log_message);

    // 如果日志文件名为空，创建新的日志文件
    if(strlen(g_file_counters.current_log_file) == 0) {
        if(!Create_Log_File()) {
            return 0; // 日志文件创建失败
        }
    }

    // 打开日志文件追加写入（使用FA_OPEN_ALWAYS确保文件存在）
    res = f_open(&fdst, g_file_counters.current_log_file, FA_OPEN_ALWAYS | FA_WRITE);
    if(res != FR_OK) {
        // my_printf(USART0, "DEBUG: Log file open failed, res: %d\r\n", res);
        return 0; // 文件打开失败
    }

    // 移动到文件末尾
    f_lseek(&fdst, f_size(&fdst));

    // 写入日志
    res = f_write(&fdst, log_line, strlen(log_line), &bw);
    f_close(&fdst);

    if(res != FR_OK || bw != strlen(log_line)) {
        return 0; // 写入失败
    }

    return 1; // 保存成功
}

/************************ 数据存储系统测试函数 ************************/
uint8_t Test_Data_Storage_System(void)
{
    sample_data_t test_data;
    uint8_t test_result = 1;

    // 记录测试开始
    Save_Log_Data("Data storage system test started");

    // 测试1：正常模式数据存储
    Set_Storage_Mode(STORAGE_MODE_NORMAL);
    test_data.timestamp = DateTime_To_Unix(2025, 1, 1, 12, 30, 45);
    test_data.voltage = 2.5f;
    test_data.over_limit = 0;

    if(!Save_Sample_Data(&test_data)) {
        Save_Log_Data("Test failed: Normal mode sample data save");
        test_result = 0;
    }

    // 测试2：超限数据存储
    test_data.voltage = 5.0f;
    test_data.over_limit = 1;

    if(!Save_OverLimit_Data(&test_data)) {
        Save_Log_Data("Test failed: Over limit data save");
        test_result = 0;
    }

    // 测试3：加密模式数据存储
    Set_Storage_Mode(STORAGE_MODE_ENCRYPT);
    test_data.voltage = 3.3f;
    test_data.over_limit = 0;

    if(!Save_HideData(&test_data)) {
        Save_Log_Data("Test failed: Encrypt mode data save");
        test_result = 0;
    }

    // 恢复正常模式
    Set_Storage_Mode(STORAGE_MODE_NORMAL);

    // 记录测试结果
    if(test_result) {
        Save_Log_Data("Data storage system test completed successfully");
    } else {
        Save_Log_Data("Data storage system test failed");
    }

    return test_result;
}

/************************ 数据缓冲区管理函数 ************************/

/************************ 初始化数据缓冲区 ************************/
void Data_Buffer_Init(void)
{
    memset(&g_data_buffer_mgr, 0, sizeof(data_buffer_manager_t));
    g_data_buffer_mgr.data_write_index = 0;
    g_data_buffer_mgr.data_read_index = 0;
    g_data_buffer_mgr.log_write_index = 0;
    g_data_buffer_mgr.log_read_index = 0;
    g_data_buffer_mgr.data_count = 0;
    g_data_buffer_mgr.log_count = 0;
}

/************************ 添加数据到缓冲区 ************************/
uint8_t Add_Data_To_Buffer(sample_data_t* data, data_type_t type)
{
    if(data == NULL) {
        my_printf(USART0,"data is NULL\r\n");
        return 0;
    }

    // 检查缓冲区是否已满
    if(g_data_buffer_mgr.data_count >= DATA_BUFFER_SIZE) {
        my_printf(USART0,"data is full");
        return 0; // 缓冲区已满
    }

    // 添加数据到缓冲区
    g_data_buffer_mgr.data_buffer[g_data_buffer_mgr.data_write_index].data = *data;
    g_data_buffer_mgr.data_buffer[g_data_buffer_mgr.data_write_index].type = type;
    g_data_buffer_mgr.data_buffer[g_data_buffer_mgr.data_write_index].valid = 1;

    // 更新写入索引
    g_data_buffer_mgr.data_write_index = (g_data_buffer_mgr.data_write_index + 1) % DATA_BUFFER_SIZE;
    g_data_buffer_mgr.data_count++;

    return 1;
}

/************************ 添加日志到缓冲区 ************************/
uint8_t Add_Log_To_Buffer(const char* log_message)
{
    if(log_message == NULL) {
        return 0;
    }

    // 检查缓冲区是否已满
    if(g_data_buffer_mgr.log_count >= LOG_BUFFER_SIZE) {
        return 0; // 缓冲区已满
    }

    // 添加日志到缓冲区
    strncpy(g_data_buffer_mgr.log_buffer[g_data_buffer_mgr.log_write_index].message,
            log_message, MAX_LOG_MSG_LEN - 1);
    g_data_buffer_mgr.log_buffer[g_data_buffer_mgr.log_write_index].message[MAX_LOG_MSG_LEN - 1] = '\0';
    g_data_buffer_mgr.log_buffer[g_data_buffer_mgr.log_write_index].timestamp = Get_Tick();
    g_data_buffer_mgr.log_buffer[g_data_buffer_mgr.log_write_index].valid = 1;

    // 更新写入索引
    g_data_buffer_mgr.log_write_index = (g_data_buffer_mgr.log_write_index + 1) % LOG_BUFFER_SIZE;
    g_data_buffer_mgr.log_count++;

    return 1;
}

/************************ 获取缓冲区数据数量 ************************/
uint8_t Get_Buffer_Data_Count(void)
{
    return g_data_buffer_mgr.data_count;
}

/************************ 获取缓冲区日志数量 ************************/
uint8_t Get_Buffer_Log_Count(void)
{
    return g_data_buffer_mgr.log_count;
}

/************************ 处理数据缓冲区 ************************/
void Process_Data_Buffer(void)
{
    // 处理数据缓冲区
    while(g_data_buffer_mgr.data_count > 0) {
        buffered_data_t* buffered_data = &g_data_buffer_mgr.data_buffer[g_data_buffer_mgr.data_read_index];

        if(!buffered_data->valid) {
            break;
        }

        // 添加调试信息
        // my_printf(USART0, "DEBUG: Processing data buffer, type: %d, ready: %d\r\n",
        //           buffered_data->type, g_storage_system_ready);

        // 根据数据类型调用相应的存储函数
        uint8_t result = 0;
        switch(buffered_data->type) {
            case DATA_TYPE_SAMPLE:
                result = Save_Sample_Data(&buffered_data->data);
//                my_printf(USART0, "DEBUG: Save_Sample_Data result: %d\r\n", result);
                break;
            case DATA_TYPE_OVERLIMIT:
                result = Save_OverLimit_Data(&buffered_data->data);
//                my_printf(USART0, "DEBUG: Save_OverLimit_Data result: %d\r\n", result);
                break;
            case DATA_TYPE_HIDEDATA:
                result = Save_HideData(&buffered_data->data);
//                my_printf(USART0, "DEBUG: Save_HideData result: %d\r\n", result);
                break;
        }

        // 标记数据为已处理
        buffered_data->valid = 0;
        g_data_buffer_mgr.data_read_index = (g_data_buffer_mgr.data_read_index + 1) % DATA_BUFFER_SIZE;
        g_data_buffer_mgr.data_count--;

        // 如果存储失败，记录错误但继续处理下一条数据
        if(!result) {
            // my_printf(USART0, "DEBUG: Data storage failed\r\n");
        }

        // 每处理一条数据就退出，避免长时间阻塞
        break;
    }

    // 处理日志缓冲区
    while(g_data_buffer_mgr.log_count > 0) {
        buffered_log_t* buffered_log = &g_data_buffer_mgr.log_buffer[g_data_buffer_mgr.log_read_index];

        if(!buffered_log->valid) {
            // my_printf(USART0, "DEBUG: Log buffer entry invalid at index %d\r\n", g_data_buffer_mgr.log_read_index);
            break;
        }

        // my_printf(USART0, "DEBUG: Processing log: '%s', ready: %d\r\n",
        //           buffered_log->message, g_storage_system_ready);

        // 调用日志存储函数
        uint8_t result = Save_Log_Data(buffered_log->message);

        // 标记日志为已处理
        buffered_log->valid = 0;
        g_data_buffer_mgr.log_read_index = (g_data_buffer_mgr.log_read_index + 1) % LOG_BUFFER_SIZE;
        g_data_buffer_mgr.log_count--;

        // 如果存储失败，记录错误但继续处理下一条日志
        if(!result) {
            // my_printf(USART0, "DEBUG: Log save failed for: '%s'\r\n", buffered_log->message);
        } else {
            // my_printf(USART0, "DEBUG: Log saved successfully: '%s'\r\n", buffered_log->message);
        }

        // 每处理一条日志就退出，避免长时间阻塞
        break;
    }
}

/************************ 非阻塞数据存储函数 ************************/

/************************ 缓冲采样数据 ************************/
uint8_t Buffer_Sample_Data(sample_data_t* data)
{
    uint8_t result = Add_Data_To_Buffer(data, DATA_TYPE_SAMPLE);
    // my_printf(USART0, "DEBUG: Buffer_Sample_Data result: %d, voltage: %.3f\r\n", result, data->voltage);
    return result;
}

/************************ 缓冲超限数据 ************************/
uint8_t Buffer_OverLimit_Data(sample_data_t* data)
{
    uint8_t result = Add_Data_To_Buffer(data, DATA_TYPE_OVERLIMIT);
    // my_printf(USART0, "DEBUG: Buffer_OverLimit_Data result: %d, voltage: %.3f\r\n", result, data->voltage);
    return result;
}

/************************ 缓冲加密数据 ************************/
uint8_t Buffer_HideData(sample_data_t* data)
{
    uint8_t result = Add_Data_To_Buffer(data, DATA_TYPE_HIDEDATA);
    // my_printf(USART0, "DEBUG: Buffer_HideData result: %d, voltage: %.3f\r\n", result, data->voltage);
    return result;
}

/************************ 缓冲日志数据 ************************/
uint8_t Buffer_Log_Data(const char* log_message)
{
    uint8_t result = Add_Log_To_Buffer(log_message);
    // my_printf(USART0, "DEBUG: Buffer_Log_Data('%s') result: %d, count: %d\r\n",
    //           log_message, result, g_data_buffer_mgr.log_count);
    return result;
}

