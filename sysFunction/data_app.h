#ifndef DATA_APP_H
#define DATA_APP_H

#include "HeaderFiles.h"
#include "rtc_driver.h"
#include "data_save.h"  // 包含data_save.h来获取sample_data_t定义

// 数据转换相关函数
void Process_Data_Command(char* command);
void Handle_Hide_Command(void);
void Handle_Unhide_Command(char* hex_data);
uint32_t DateTime_To_Unix(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
void Unix_To_DateTime(uint32_t unix_time, uint16_t* year, uint8_t* month, uint8_t* day, uint8_t* hour, uint8_t* minute, uint8_t* second);
void Encode_Sample_Data(sample_data_t* data, char* hex_output);
uint8_t Decode_Hex_Data(char* hex_input, sample_data_t* data);
uint8_t Check_Voltage_Limit(float voltage);
float Get_Current_Voltage(void);

// 全局变量
extern sample_data_t current_sample;
extern float ADC_value; // 来自scheduler模块的ADC电压值

#endif // DATA_APP_H


