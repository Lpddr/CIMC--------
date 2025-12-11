#include "data_app.h"
#include "SDcard_app.h"
uint8_t hide_mode;
// 全局变量
sample_data_t current_sample;

/************************ 数据命令处理主函数 ************************/
void Process_Data_Command(char* command)
{
    // 去除前后空格
    while(*command == ' ' || *command == '\t') command++;

      if(strstr(command, "unhide") != NULL)
    {
        // 数据解密命令
        char* hex_data = strstr(command, "unhide");
        hex_data += 6; // 跳过"unhide"
        while(*hex_data == ' ' || *hex_data == '\t') hex_data++; // 跳过空格
        hide_mode=0;
        Handle_Unhide_Command(hex_data);
    }
    else
  if(strstr(command, "hide") != NULL)
    {
        // 数据加密命令
        hide_mode=1;
        Handle_Hide_Command();
        
    }
  
}

/************************ 处理hide命令 ************************/
void Handle_Hide_Command(void)
{
    char hex_output[20]; // 16字符HEX + 可能的* + 结束符
    uint8_t hour, min, sec, ampm;
    uint8_t year, month, date, week;

    // 使用rtc_driver获取当前时间和日期
    rtc_get_time(&hour, &min, &sec, &ampm);
    rtc_get_date(&year, &month, &date, &week);

    // 转换为Unix时间戳 (year是两位数，需要加2000)
    current_sample.timestamp = DateTime_To_Unix(2000 + year, month, date, hour, min, sec);

    // 获取当前电压值
    current_sample.voltage = Get_Current_Voltage();

    // 检查是否超限
    current_sample.over_limit = Check_Voltage_Limit(current_sample.voltage);

    // 编码为十六进制格式
    Encode_Sample_Data(&current_sample, hex_output);

    // 输出结果
    if(current_sample.over_limit==1)
    my_printf(USART0, "%s*\r\n", hex_output);
    else
       my_printf(USART0, "%s\r\n", hex_output); 
}

/************************ 处理unhide命令 ************************/
void Handle_Unhide_Command(char* hex_data)
{
    sample_data_t decoded_data;
    uint16_t year;
    uint8_t month, day, hour, minute, second;

    // 解码十六进制数据
    if(Decode_Hex_Data(hex_data, &decoded_data))
    {
        // 转换Unix时间戳为日期时间
        Unix_To_DateTime(decoded_data.timestamp, &year, &month, &day, &hour, &minute, &second);

        // 输出原始格式
        my_printf(USART0, "%04d-%02d-%02d %02d:%02d:%02d ch0=%.2fV",
                  year, month, day, hour, minute, second, decoded_data.voltage);

        if(decoded_data.over_limit)
        {
            my_printf(USART0, " *OVER LIMIT*");
        }
        my_printf(USART0, "\r\n");
    }
    else
    {
        //my_printf(USART0, "Invalid hex data format\r\n");
    }
}

/************************ 日期时间转Unix时间戳 ************************/
uint32_t DateTime_To_Unix(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
    uint32_t days = 0;
    uint16_t i;

    // 计算从1970年到指定年份的天数
    for(i = 1970; i < year; i++)
    {
        if((i % 4 == 0 && i % 100 != 0) || (i % 400 == 0))
        {
            days += 366; // 闰年
        }
        else
        {
            days += 365; // 平年
        }
    }

    // 每月天数表（平年）
    uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    // 检查当前年是否为闰年
    if((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
    {
        days_in_month[1] = 29; // 闰年2月29天
    }

    // 计算当年已过月份的天数
    for(i = 1; i < month; i++)
    {
        days += days_in_month[i - 1];
    }

    // 加上当月已过天数
    days += (day - 1);

    // 转换为秒数
    return days * 86400 + hour * 3600 + minute * 60 + second;
}

/************************ Unix时间戳转日期时间 ************************/
void Unix_To_DateTime(uint32_t unix_time, uint16_t* year, uint8_t* month, uint8_t* day, uint8_t* hour, uint8_t* minute, uint8_t* second)
{
    uint32_t days = unix_time / 86400;
    uint32_t remaining_seconds = unix_time % 86400;

    // 计算时分秒
    *hour = remaining_seconds / 3600;
    *minute = (remaining_seconds % 3600) / 60;
    *second = remaining_seconds % 60;

    // 计算年份
    *year = 1970;
    while(1)
    {
        uint32_t days_in_year = 365;
        if((*year % 4 == 0 && *year % 100 != 0) || (*year % 400 == 0))
        {
            days_in_year = 366;
        }

        if(days >= days_in_year)
        {
            days -= days_in_year;
            (*year)++;
        }
        else
        {
            break;
        }
    }

    // 每月天数表
    uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if((*year % 4 == 0 && *year % 100 != 0) || (*year % 400 == 0))
    {
        days_in_month[1] = 29;
    }

    // 计算月份和日期
    *month = 1;
    while(days >= days_in_month[*month - 1])
    {
        days -= days_in_month[*month - 1];
        (*month)++;
    }

    *day = days + 1;
}

/************************ 编码采样数据为十六进制 ************************/
void Encode_Sample_Data(sample_data_t* data, char* hex_output)
{
    uint32_t timestamp = data->timestamp;
    uint16_t voltage_int = (uint16_t)data->voltage; // 整数部分
    float voltage_frac = data->voltage - voltage_int; // 小数部分
    uint16_t voltage_frac_hex = (uint16_t)(voltage_frac * 65536); // 小数部分转换

    // 格式化为16字符十六进制字符串
    sprintf(hex_output, "%08X%04X%04X", timestamp, voltage_int, voltage_frac_hex);

    // 如果超限，添加*标记
    if(data->over_limit)
    {
        strcat(hex_output, "*");
    }
}

/************************ 解码十六进制数据 ************************/
uint8_t Decode_Hex_Data(char* hex_input, sample_data_t* data)
{
    char temp_str[10];
    uint32_t timestamp;
    uint16_t voltage_int, voltage_frac;

    // 检查输入长度（至少16个字符）
    if(strlen(hex_input) < 16)
    {
        return 0;
    }

    // 检查是否有超限标记
    data->over_limit = (strchr(hex_input, '*') != NULL) ? 1 : 0;

    // 解析时间戳（前8个字符）
    memset(temp_str, 0, sizeof(temp_str));
    strncpy(temp_str, hex_input, 8);
    sscanf(temp_str, "%X", &timestamp);
    data->timestamp = timestamp;

    // 解析电压整数部分（第9-12个字符）
    memset(temp_str, 0, sizeof(temp_str));
    strncpy(temp_str, hex_input + 8, 4);
    sscanf(temp_str, "%X", (unsigned int*)&voltage_int);

    // 解析电压小数部分（第13-16个字符）
    memset(temp_str, 0, sizeof(temp_str));
    strncpy(temp_str, hex_input + 12, 4);
    sscanf(temp_str, "%X", (unsigned int*)&voltage_frac);

    // 重构电压值
    data->voltage = (float)voltage_int + ((float)voltage_frac / 65536.0f);

    return 1; // 解码成功
}

/************************ 检查电压是否超限 ************************/
uint8_t Check_Voltage_Limit(float voltage)
{
    // 获取当前阈值配置
    extern config_params_t current_config;

    if(voltage > current_config.limit)
    {
        return 1; // 超限
    }
    return 0; // 正常
}

/************************ 获取当前电压值 ************************/
float Get_Current_Voltage(void)
{
    // 从scheduler模块获取ADC转换后的电压值
    extern float ADC_value;

    return ADC_value;
}


