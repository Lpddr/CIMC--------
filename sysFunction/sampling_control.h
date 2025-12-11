#ifndef SAMPLING_CONTROL_H
#define SAMPLING_CONTROL_H

#include "HeaderFiles.h"
#include "usart_driver.h"  // 包含USART驱动头文件
#include "led_driver.h"    // 包含LED驱动头文件
#include "SDcard_app.h"    // 包含配置参数定义

// Flash地址定义
#define PERIOD_CONFIG_FLASH_ADDR  0x001000  // 周期配置存储地址(与CONFIG_FLASH_ADDR不同)

// 采样状态枚举
typedef enum {
    SAMPLING_STOPPED = 0,  // 停止状态
    SAMPLING_RUNNING = 1   // 运行状态
} sampling_state_t;

// 采样周期配置结构体
typedef struct {
    uint32_t period_5s;   // 5秒周期(毫秒)
    uint32_t period_10s;  // 10秒周期(毫秒)
    uint32_t period_15s;  // 15秒周期(毫秒)
    uint8_t current_period_index; // 当前周期索引(0=5s, 1=10s, 2=15s)
} sampling_period_config_t;

// 采样控制结构体
typedef struct {
    sampling_state_t state;        // 采样状态
    uint32_t sample_period_ms;     // 采样周期(毫秒)
    uint32_t last_sample_time;     // 上次采样时间
    uint32_t led_blink_period_ms;  // LED闪烁周期(毫秒)
    uint32_t last_led_toggle_time; // 上次LED切换时间
    uint32_t oled_update_period_ms; // OLED更新周期(毫秒)
    uint32_t last_oled_update_time; // 上次OLED更新时间
    uint8_t led_state;             // LED状态
    float current_voltage;         // 当前电压值
    uint8_t key_control_locked;    // 按键控制锁定标志
    uint8_t over_limit_status;     // 超限状态标志
    sampling_period_config_t period_config; // 周期配置
} sampling_control_t;

// 全局变量声明
extern sampling_control_t sampling_ctrl;

// 函数声明
void Sampling_Control_Init(void);           // 初始化采样控制
void Sampling_Control_Start(void);          // 启动采样
void Sampling_Control_Stop(void);           // 停止采样
void Sampling_Control_Task(void);           // 采样控制任务(在调度器中调用)
void Update_OLED_Display(void);             // 更新OLED显示
void Toggle_LED1(void);                     // 切换LED1状态
void Output_Sample_Data(void);              // 输出采样数据
float Read_ADC_Voltage(void);               // 读取ADC电压值
uint8_t Is_Sampling_Running(void);          // 检查采样是否运行中
void Sampling_Control_Toggle(void);         // 切换采样状态(按键控制)
void Sampling_Control_Reset(void);          // 重置采样控制状态
void Unlock_Sampling_Control(void);         // 解锁按键控制
void Set_Sampling_Period(uint8_t period_index); // 设置采样周期
uint8_t Save_Period_Config_To_Flash(void);  // 保存周期配置到Flash
uint8_t Load_Period_Config_From_Flash(void); // 从Flash加载周期配置
uint32_t Get_Current_Period_Seconds(void);  // 获取当前周期(秒)
uint8_t Check_Voltage_Over_Limit(float voltage); // 检查电压是否超限
void Control_LED2_Over_Limit(uint8_t over_limit); // 控制LED2超限指示

#endif
