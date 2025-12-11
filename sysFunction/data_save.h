#ifndef DATA_SAVE_H
#define DATA_SAVE_H

#include "HeaderFiles.h"

// 前向声明
typedef struct {
    uint32_t timestamp;    // Unix时间戳
    float voltage;         // 电压值
    uint8_t over_limit;    // 超限标记
} sample_data_t;

// 存储配置常量
#define MAX_RECORDS_PER_FILE    10      // 每个文件最大记录数
#define BOOT_COUNT_FLASH_ADDR   0x8000  // 上电次数存储地址
#define DATETIME_STR_LEN        15      // 时间戳字符串长度(14位+结束符)

// 数据缓冲区配置
#define DATA_BUFFER_SIZE        20      // 数据缓冲区大小
#define LOG_BUFFER_SIZE         50      // 日志缓冲区大小
#define MAX_LOG_MSG_LEN         128     // 最大日志消息长度

// 文件夹路径定义
#define SAMPLE_FOLDER_PATH      "0:/sample"
#define OVERLIMIT_FOLDER_PATH   "0:/overLimit"
#define LOG_FOLDER_PATH         "0:/log"
#define HIDEDATA_FOLDER_PATH    "0:/hideData"

// 文件计数器结构体
typedef struct {
    uint8_t sample_count;       // sample文件夹当前文件记录数
    uint8_t overlimit_count;    // overLimit文件夹当前文件记录数
    uint8_t hidedata_count;     // hideData文件夹当前文件记录数
    char current_sample_file[64];    // 当前sample文件名（增加到64字符支持长文件名）
    char current_overlimit_file[64]; // 当前overLimit文件名（增加到64字符支持长文件名）
    char current_hidedata_file[64];  // 当前hideData文件名（增加到64字符支持长文件名）
    char current_log_file[64];       // 当前log文件名（增加到64字符支持长文件名）
} file_counters_t;

// 存储模式枚举
typedef enum {
    STORAGE_MODE_NORMAL = 0,    // 正常模式：存储到sample文件夹
    STORAGE_MODE_ENCRYPT = 1    // 加密模式：存储到hideData文件夹
} storage_mode_t;

// 数据类型枚举
typedef enum {
    DATA_TYPE_SAMPLE = 0,       // 采样数据
    DATA_TYPE_OVERLIMIT = 1,    // 超限数据
    DATA_TYPE_HIDEDATA = 2      // 加密数据
} data_type_t;

// 缓冲区数据结构
typedef struct {
    sample_data_t data;         // 数据内容
    data_type_t type;           // 数据类型
    uint8_t valid;              // 数据有效标志
} buffered_data_t;

// 日志缓冲区结构
typedef struct {
    char message[MAX_LOG_MSG_LEN];  // 日志消息
    uint32_t timestamp;             // 时间戳
    uint8_t valid;                  // 有效标志
} buffered_log_t;

// 数据缓冲区管理结构
typedef struct {
    buffered_data_t data_buffer[DATA_BUFFER_SIZE];  // 数据缓冲区
    buffered_log_t log_buffer[LOG_BUFFER_SIZE];     // 日志缓冲区
    uint8_t data_write_index;                       // 数据写入索引
    uint8_t data_read_index;                        // 数据读取索引
    uint8_t log_write_index;                        // 日志写入索引
    uint8_t log_read_index;                         // 日志读取索引
    uint8_t data_count;                             // 缓冲区数据数量
    uint8_t log_count;                              // 缓冲区日志数量
} data_buffer_manager_t;

// 全局变量声明
extern file_counters_t g_file_counters;
extern storage_mode_t g_storage_mode;
extern uint32_t g_boot_count;
extern uint8_t g_storage_system_ready; // 数据存储系统就绪标志
extern data_buffer_manager_t g_data_buffer_mgr; // 数据缓冲区管理器

// 初始化函数
uint8_t Data_Save_Init(void);                          // 数据存储系统初始化
uint8_t Create_Storage_Folders(void);                  // 创建存储文件夹
uint8_t Load_Boot_Count_From_Flash(void);              // 从Flash加载上电次数
uint8_t Save_Boot_Count_To_Flash(uint32_t boot_count); // 保存上电次数到Flash
uint8_t Create_Log_File(void);                         // 创建日志文件
void Verify_And_Init_RTC_Time(void);                   // 验证并初始化RTC时间

// 加密数据存储相关函数
void Encode_Sample_Data_To_Hex(sample_data_t* data, char* hex_string); // 将采样数据编码为HEX字符串
uint8_t Save_Encrypted_Data(sample_data_t* data);      // 保存加密数据
uint8_t Save_Verification_Data(sample_data_t* data);   // 保存校验数据（未加密格式）

// 文件计数器管理函数
void Reset_File_Counters(void);                        // 重置文件计数器
void Verify_File_Counters_Integrity(void);             // 验证文件计数器完整性
uint8_t Create_OverLimit_File_Safe(void);              // 安全创建超限文件

// 时间戳和文件名生成函数
void Generate_DateTime_String(char* datetime_str);     // 生成14位时间戳字符串
void Generate_Sample_Filename(char* filename);         // 生成sample文件名
void Generate_OverLimit_Filename(char* filename);      // 生成overLimit文件名
void Generate_HideData_Filename(char* filename);       // 生成hideData文件名

// 缓冲区管理函数
void Data_Buffer_Init(void);                           // 初始化数据缓冲区
uint8_t Add_Data_To_Buffer(sample_data_t* data, data_type_t type); // 添加数据到缓冲区
uint8_t Add_Log_To_Buffer(const char* log_message);    // 添加日志到缓冲区
void Process_Data_Buffer(void);                        // 处理数据缓冲区
uint8_t Get_Buffer_Data_Count(void);                   // 获取缓冲区数据数量
uint8_t Get_Buffer_Log_Count(void);                    // 获取缓冲区日志数量

// 数据存储函数（直接存储，可能阻塞）
uint8_t Save_Sample_Data(sample_data_t* data);         // 保存采样数据
uint8_t Save_OverLimit_Data(sample_data_t* data);      // 保存超限数据
uint8_t Save_HideData(sample_data_t* data);            // 保存加密数据
uint8_t Save_Log_Data(const char* log_message);        // 保存日志数据

// 数据存储函数（缓冲区方式，非阻塞）
uint8_t Buffer_Sample_Data(sample_data_t* data);       // 缓冲采样数据
uint8_t Buffer_OverLimit_Data(sample_data_t* data);    // 缓冲超限数据
uint8_t Buffer_HideData(sample_data_t* data);          // 缓冲加密数据
uint8_t Buffer_Log_Data(const char* log_message);      // 缓冲日志数据

// 存储模式控制函数
void Set_Storage_Mode(storage_mode_t mode);            // 设置存储模式
storage_mode_t Get_Storage_Mode(void);                 // 获取当前存储模式

// 文件管理函数
uint8_t Check_And_Create_New_File(const char* folder_path, char* current_filename, uint8_t* record_count, const char* prefix); // 检查并创建新文件

// 测试函数
uint8_t Test_Data_Storage_System(void);            // 数据存储系统测试

#endif // DATA_SAVE_H



