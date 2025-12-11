#ifndef SDCARD_APP_H
#define SDCARD_APP_H
#include "HeaderFiles.h"

// 配置参数结构体
typedef struct {
    float ratio;     // 变比值 (Ch0) 范围: 0-100
    float limit;     // 阈值 (Ch0) 范围: 0-100
} config_params_t;

// Flash存储地址定义
#define CONFIG_FLASH_ADDR    0x0000  // 配置参数在Flash中的存储地址
#define DEVICE_ID_FLASH_ADDR 0x1000  // 设备ID在Flash中的存储地址

// 设备ID相关定义
#define DEVICE_ID_MAX_LEN    64      // 设备ID最大长度

// 函数声明
uint8_t Read_Config_From_SDCard(void);
uint8_t Parse_Config_File(char* file_content, config_params_t* config);
uint8_t Save_Config_To_Flash(config_params_t* config);
uint8_t Load_Config_From_Flash(config_params_t* config);

// 设备ID相关函数
uint8_t Save_Device_ID_To_Flash(const char* device_id);
uint8_t Load_Device_ID_From_Flash(char* device_id);
void Print_Device_ID(void);
void Init_Device_ID_If_Empty(void);

// 配置管理函数
void Process_Config_Management(char* command);
void Handle_Ratio_Command(void);
void Handle_Limit_Command(void);
void Handle_Config_Save(void);
void Handle_Config_Read(void);
uint8_t Validate_Ratio(float ratio);
uint8_t Validate_Limit(float limit);
uint8_t Set_New_Ratio(float new_ratio);
uint8_t Set_New_Limit(float new_limit);

// SD卡测试函数
uint8_t Test_SDCard_Silent(void);

// 全局配置参数
extern config_params_t current_config;

#endif
