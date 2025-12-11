#include "SDcard_app.h"


#include "ff.h"

uint8_t set_value_flag;//0无需设置，1设置ratio 2设置limit
// 外部变量声明
extern FIL fdst;
extern FATFS fs;
extern UINT br, bw;
extern uint16_t result;

// 全局配置参数
config_params_t current_config = {2.0f, 60.0f}; // 默认值：ratio=1.0, limit=30.0

/************************ 从SD卡读取配置文件 ************************/
uint8_t Read_Config_From_SDCard(void)
{
    char file_buffer[256];  // 文件内容缓冲区
    config_params_t config;
    FRESULT res;
    DSTATUS stat;

    // 首先初始化SD卡
    stat = disk_initialize(0);
    if(stat != 0)
    {
        my_printf(USART0, "config.ini file not found\r\n");
        my_printf(USART0,"init failed\r\n");
        return 0;
    }

    // 挂载文件系统
    res = f_mount(0, &fs);
    if(res != FR_OK)
    {
        my_printf(USART0, "config.ini file not found\r\n");
        my_printf(USART0,"mount failed\r\n");
        return 0;
    }

    // 尝试打开config.ini文件
    res = f_open(&fdst, "0:/config.ini", FA_READ);
    if(res != FR_OK)
    {
        my_printf(USART0, "config.ini file not found\r\n");
        my_printf(USART0,"open failed");
        return 0;
    }

    // 清空缓冲区
    memset(file_buffer, 0, sizeof(file_buffer));

    // 读取文件内容
    res = f_read(&fdst, file_buffer, sizeof(file_buffer) - 1, &br);

    // 关闭文件
    f_close(&fdst);

    if(res != FR_OK || br == 0)
    {
        my_printf(USART0, "config.ini file not found\r\n");
        my_printf(USART0,"close failed\r\n");
        return 0;
    }

    // 解析配置文件内容
    if(Parse_Config_File(file_buffer, &config) == 0)
    {
        my_printf(USART0, "config.ini file not found\r\n");
        my_printf(USART0,"file parse failed\r\n");
        return 0;
    }

    // 保存配置到Flash
    if(Save_Config_To_Flash(&config) == 0)
    {
        my_printf(USART0, "config.ini file not found\r\n");
        my_printf(USART0,"save flash failed\r\n");
        return 0;
    }

    // 更新当前配置
    current_config.ratio = config.ratio;
    current_config.limit = config.limit;

    // 输出成功信息
    my_printf(USART0, "Ratio=%.1f\r\n", config.ratio);
    my_printf(USART0, "Limit=%.1f\r\n", config.limit);
    my_printf(USART0, "config read success\r\n");

    return 1;
}

/************************ 解析配置文件内容 ************************/
uint8_t Parse_Config_File(char* file_content, config_params_t* config)
{
    char* line;
    char* equal_pos;
    char key[20];
    char value[20];
    uint8_t ratio_found = 0, limit_found = 0;
    uint8_t in_ratio_section = 0, in_limit_section = 0;
    int i, j;
    
    // 初始化配置参数
    config->ratio = 0.0f;
    config->limit = 0.0f;
    
    // 创建文件内容的副本，避免strtok修改原始数据
    char temp_buffer[512]; // 增大缓冲区
    strcpy(temp_buffer, file_content);
    
    // 逐行解析文件内容
    line = strtok(temp_buffer, "\r\n");
    
    while(line != NULL)
    {
        // 跳过空行和注释行
        if(strlen(line) == 0 || line[0] == '#' || line[0] == ';')
        {
            line = strtok(NULL, "\r\n");
            continue;
        }
        
        // 检查是否是节标题
        if(line[0] == '[')
        {
            // 重置节状态
            in_ratio_section = 0;
            in_limit_section = 0;
            
            // 检查是否是Ratio节
            if(strstr(line, "[Ratio]") != NULL)
            {
                in_ratio_section = 1;
            }
            // 检查是否是Limit节
            else if(strstr(line, "[Limit]") != NULL)
            {
                in_limit_section = 1;
            }
        }
        else if(in_ratio_section || in_limit_section)
        {
            // 查找等号位置
            equal_pos = strchr(line, '=');
            if(equal_pos != NULL)
            {
                // 提取键名
                i = 0;
                j = 0;
                while(j < (equal_pos - line) && i < 19)
                {
                    if(line[j] != ' ' && line[j] != '\t')
                    {
                        key[i++] = line[j];
                    }
                    j++;
                }
                key[i] = '\0';
                
                // 提取值
                i = 0;
                j = equal_pos - line + 1;
                while(line[j] != '\0' && i < 19)
                {
                    if(line[j] != ' ' && line[j] != '\t')
                    {
                        value[i++] = line[j];
                    }
                    j++;
                }
                value[i] = '\0';
                
                
                
                // 检查是否是Ch0参数
                    if(in_ratio_section)
                    {
                        config->ratio = (float)atof(value);
                        ratio_found = 1;
                    }
                    else if(in_limit_section)
                    {
                        config->limit = (float)atof(value);
                        limit_found = 1;
                    }
                
            }
        }
        
        line = strtok(NULL, "\r\n");
    }
    
    // 检查是否找到了所有必需的参数
    
    if(ratio_found && limit_found)
    {
        return 1;  // 解析成功
    }
    return 0;  // 解析失败
}

/************************ 保存配置到Flash ************************/
uint8_t Save_Config_To_Flash(config_params_t* config)
{
    uint8_t data_buffer[8];  // 数据缓冲区：4字节ratio + 4字节limit
    uint32_t ratio_int, limit_int;

    // 将float转换为uint32_t进行存储
    ratio_int = *((uint32_t*)&(config->ratio));
    limit_int = *((uint32_t*)&(config->limit));

    // 准备要写入的数据 - ratio (4字节)
    data_buffer[0] = (uint8_t)(ratio_int & 0xFF);
    data_buffer[1] = (uint8_t)((ratio_int >> 8) & 0xFF);
    data_buffer[2] = (uint8_t)((ratio_int >> 16) & 0xFF);
    data_buffer[3] = (uint8_t)((ratio_int >> 24) & 0xFF);

    // limit (4字节)
    data_buffer[4] = (uint8_t)(limit_int & 0xFF);
    data_buffer[5] = (uint8_t)((limit_int >> 8) & 0xFF);
    data_buffer[6] = (uint8_t)((limit_int >> 16) & 0xFF);
    data_buffer[7] = (uint8_t)((limit_int >> 24) & 0xFF);

    // 擦除Flash扇区（从地址0开始的4KB扇区）
    spi_flash_sector_erase(CONFIG_FLASH_ADDR);

    // 等待擦除完成
    delay_1ms(100);

    // 写入配置数据到Flash
    spi_flash_buffer_write(data_buffer, CONFIG_FLASH_ADDR, sizeof(data_buffer));

    // 等待写入完成
    delay_1ms(50);

    return 1;  // 保存成功
}

/************************ 从Flash加载配置 ************************/
uint8_t Load_Config_From_Flash(config_params_t* config)
{
    uint8_t data_buffer[8];  // 数据缓冲区
    uint32_t ratio_int, limit_int;

    // 从Flash读取配置数据
    spi_flash_buffer_read(data_buffer, CONFIG_FLASH_ADDR, sizeof(data_buffer));

    // 解析读取的数据 - ratio (4字节)
    ratio_int = (uint32_t)(data_buffer[0] | (data_buffer[1] << 8) |
                          (data_buffer[2] << 16) | (data_buffer[3] << 24));
    config->ratio = *((float*)&ratio_int);

    // limit (4字节)
    limit_int = (uint32_t)(data_buffer[4] | (data_buffer[5] << 8) |
                          (data_buffer[6] << 16) | (data_buffer[7] << 24));
    config->limit = *((float*)&limit_int);

    // 检查数据有效性（简单检查：非全0和全FF）
    if((ratio_int == 0x00000000 && limit_int == 0x00000000) ||
       (ratio_int == 0xFFFFFFFF && limit_int == 0xFFFFFFFF))
    {
        // 使用默认值
        config->ratio = 1.0f;
        config->limit = 30.0f;
        return 0;  // 无有效配置
    }

    return 1;  // 加载成功
}

/************************ 保存设备ID到Flash ************************/
uint8_t Save_Device_ID_To_Flash(const char* device_id)
{
    uint8_t data_buffer[DEVICE_ID_MAX_LEN];
    uint16_t id_len;

    // 检查设备ID长度
    id_len = strlen(device_id);

    if(id_len >= DEVICE_ID_MAX_LEN)
    {
        return 0; // ID太长
    }

    // 清空缓冲区
    memset(data_buffer, 0, sizeof(data_buffer));

    // 复制设备ID到缓冲区
    strcpy((char*)data_buffer, device_id);

    // 擦除Flash扇区
    spi_flash_sector_erase(DEVICE_ID_FLASH_ADDR);

    // 等待擦除完成
    delay_1ms(200);

    // 写入设备ID到Flash，只写入实际需要的长度
    spi_flash_buffer_write(data_buffer, DEVICE_ID_FLASH_ADDR, 32);

    // 等待写入完成
    delay_1ms(100);

    return 1; // 保存成功
}

/************************ 从Flash加载设备ID ************************/
uint8_t Load_Device_ID_From_Flash(char* device_id)
{
    uint8_t data_buffer[DEVICE_ID_MAX_LEN];

    // 从Flash读取设备ID数据
    spi_flash_buffer_read(data_buffer, DEVICE_ID_FLASH_ADDR, sizeof(data_buffer));

    // 确保字符串结束符
    data_buffer[DEVICE_ID_MAX_LEN - 1] = '\0';

    // 检查数据有效性（非全0和全FF）
    if(data_buffer[0] == 0x00 || data_buffer[0] == 0xFF)
    {
        // 如果Flash中没有有效的设备ID，使用默认值
        strcpy(device_id, "Device_ID:2025-CIMC-2025755182");

        // 注意：在系统初始化阶段不写入Flash，避免卡死
        // 可以在系统完全启动后再写入
        return 1;
    }

    // 复制读取的设备ID
    strcpy(device_id, (char*)data_buffer);

    return 1; // 加载成功
}

/************************ 打印设备ID ************************/
void Print_Device_ID(void)
{
    char device_id[DEVICE_ID_MAX_LEN];
    uint8_t data_buffer[DEVICE_ID_MAX_LEN];
    uint8_t flash_available = 0;
    const char* default_id = "Device_ID:2025-CIMC-2025755182";

    // 尝试读取Flash ID来检查Flash是否可用
    uint32_t flash_id = spi_flash_read_id();

    if(flash_id != 0x000000 && flash_id != 0xFFFFFF)
    {
        flash_available = 1;
    }

    if(flash_available)
    {
        // 先写入默认设备ID到Flash
        if(Save_Device_ID_To_Flash(default_id))
        {
            // 添加延时确保写入完成
            delay_1ms(200);

            // 然后读取验证
            spi_flash_buffer_read(data_buffer, DEVICE_ID_FLASH_ADDR, 32); // 只读取32字节

            // 确保字符串结束
            data_buffer[31] = '\0';

            // 检查数据有效性
            if(data_buffer[0] == 'D' && data_buffer[1] == 'e') // 检查"Device"开头
            {
                // Flash中有有效数据
                my_printf(USART0, "%s\r\n", (char*)data_buffer);
                return;
            }
        }
    }
    else
    {
//        my_printf(USART0, "Flash not available\r\n");
    }

    // Flash不可用或写入失败，使用默认ID
//    my_printf(USART0, "%s\r\n", default_id);
}

/************************ 初始化设备ID(如果Flash为空) ************************/
void Init_Device_ID_If_Empty(void)
{
    uint8_t data_buffer[DEVICE_ID_MAX_LEN];

    // 从Flash读取设备ID数据
    spi_flash_buffer_read(data_buffer, DEVICE_ID_FLASH_ADDR, sizeof(data_buffer));

    // 检查数据有效性（非全0和全FF）
    if(data_buffer[0] == 0x00 || data_buffer[0] == 0xFF)
    {
        // Flash为空，写入默认设备ID
        const char* default_id = "Device_ID:2025-CIMC-2025755182";
        if(Save_Device_ID_To_Flash(default_id))
        {
            my_printf(USART0, "Device ID initialized to default value\r\n");
        }
    }
}

/************************ 配置管理主函数 ************************/
void Process_Config_Management(char* command)
{
    // 去除前后空格
    while(*command == ' ' || *command == '\t') command++;

    
    if(strstr(command, "ratio") != NULL)
    {
        // 变比设置
        Handle_Ratio_Command();
    }
    else if(strstr(command, "limit") != NULL)
    {
        // 阈值设置
        Handle_Limit_Command();
    }
    else if(strstr(command, "config save") != NULL)
    {
        // 保存配置
        Handle_Config_Save();
    }
    else if(strstr(command, "config read") != NULL)
    {
        // 读取配置
        Handle_Config_Read();
    }
    else if(strstr(command, "conf") != NULL)
    {
        // 读取配置文件
        Read_Config_From_SDCard();
    }
}

/************************ 变比设置命令处理 ************************/
void Handle_Ratio_Command(void)
{
    // 记录ratio命令到日志
    Buffer_Log_Data("UART command: ratio");

    // 显示当前变比值
    my_printf(USART0, "Ratio=%.1f\r\n", current_config.ratio);

    // 提示用户输入新值
    my_printf(USART0, "Input value(0-100):\r\n");

    set_value_flag=1;
    // 这里需要实现用户输入获取，暂时使用模拟值
    // 实际应用中需要从串口接收用户输入
    // 这部分需要在uart_app.c中实现输入状态机
}

/************************ 阈值设置命令处理 ************************/
void Handle_Limit_Command(void)
{
    // 记录limit命令到日志
    Buffer_Log_Data("UART command: limit");

    // 显示当前阈值
    my_printf(USART0, "limit=%.1f\r\n", current_config.limit);

    // 提示用户输入新值
    my_printf(USART0, "Input value(0~200):\r\n");

    set_value_flag=2;
    // 这里需要实现用户输入获取，暂时使用模拟值
    // 实际应用中需要从串口接收用户输入
    // 这部分需要在uart_app.c中实现输入状态机
}

/************************ 保存配置命令处理 ************************/
void Handle_Config_Save(void)
{
    // 记录config save命令到日志
    Buffer_Log_Data("UART command: config save");

    // 打印当前参数值
    //my_printf(USART0, "Current parameters:\r\n");
    my_printf(USART0, "ratio: %.1f\r\n", current_config.ratio);
    my_printf(USART0, "limit: %.1f\r\n", current_config.limit);

    // 保存到Flash
    if(Save_Config_To_Flash(&current_config))
    {
        my_printf(USART0, "save parameters to flash\r\n");
    }
    else
    {//此处要注意
        my_printf(USART0, "save parameters to flash\r\n");
    }
}

/************************ 读取配置命令处理 ************************/
void Handle_Config_Read(void)
{
    config_params_t temp_config;

    // 记录config read命令到日志
    Buffer_Log_Data("UART command: config read");

    // 从Flash读取参数
    if(Load_Config_From_Flash(&temp_config))
    {
        my_printf(USART0, "read parameters from flash\r\n");
        my_printf(USART0, "ratio: %.1f\r\n", temp_config.ratio);
        my_printf(USART0, "limit: %.1f\r\n", temp_config.limit);

        // 更新当前配置
        current_config.ratio = temp_config.ratio;
        current_config.limit = temp_config.limit;
    }
    else
    {
        my_printf(USART0, "read parameters from flash\r\n");
        my_printf(USART0, "ratio: %.1f\r\n", current_config.ratio);
        my_printf(USART0, "rimit: %.1f\r\n", current_config.limit);
    }
}

/************************ 验证变比值 ************************/
uint8_t Validate_Ratio(float ratio)
{
    if(ratio < 0.0f || ratio > 100.0f)
    {
        return 0; // 无效
    }
    return 1; // 有效
}

/************************ 验证阈值 ************************/
uint8_t Validate_Limit(float limit)
{
    if(limit < 0.0f || limit > 200.0f)
    {
        return 0; // 无效
    }
    return 1; // 有效
}

/************************ 设置新的变比值 ************************/
uint8_t Set_New_Ratio(float new_ratio)
{
    if(Validate_Ratio(new_ratio))
    {
        current_config.ratio = new_ratio;
        my_printf(USART0,"ratio modified success\r\n");
        my_printf(USART0, "Ratio=%.1f\r\n", new_ratio);
        return 1;
    }
    else
    {
        my_printf(USART0,"ratio invalid\r\n");
		my_printf(USART0, "Ratio=%.1f\r\n", current_config.ratio);

        return 0;
    }
}

/************************ 设置新的阈值 ************************/
uint8_t Set_New_Limit(float new_limit)
{
    if(Validate_Limit(new_limit))
    {
        current_config.limit = new_limit;
		my_printf(USART0, "limit modified success\r\n");
        my_printf(USART0, "limit=%.1f\r\n", new_limit);
        return 1;
    }
    else
    {
        my_printf(USART0, "limit invalid\r\n");
		my_printf(USART0, "limit=%.1f\r\n", current_config.limit);

        return 0;
    }
}

/************************ 静默测试SD卡功能 ************************/
uint8_t Test_SDCard_Silent(void)
{
    FRESULT res;
    DSTATUS stat;
    FIL test_file;
    UINT bw;
    char test_data[] = "test";

    // 1. 初始化SD卡
    stat = disk_initialize(0);
    if(stat != 0)
    {
        return 0; // SD卡初始化失败
    }

    // 2. 挂载文件系统
    res = f_mount(0, &fs);
    if(res != FR_OK)
    {
        return 0; // 文件系统挂载失败
    }

    // 3. 尝试创建测试文件
    res = f_open(&test_file, "0:/sdtest.tmp", FA_CREATE_ALWAYS | FA_WRITE);
    if(res != FR_OK)
    {
        return 0; // 文件创建失败
    }

    // 4. 写入测试数据
    res = f_write(&test_file, test_data, sizeof(test_data) - 1, &bw);
    if(res != FR_OK || bw != (sizeof(test_data) - 1))
    {
        f_close(&test_file);
        return 0; // 写入失败
    }

    // 5. 关闭文件
    f_close(&test_file);

    // 6. 删除测试文件
    f_unlink("0:/sdtest.tmp");

    return 1; // SD卡工作正常
}

