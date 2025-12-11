# 2025年CIMC中国智能制造挑战赛-工业嵌入式系统开发赛项

# Program：CIMC_GD32_fatfs_driver

## 程序简介
- 工程名称：GD32F470 fatfs 实验
- 实验平台: CIMC IHD V0.4
- MDK版本：5.25


## 板载资源

 - GD32F470VET6 MCU
 
 
## 功能简介

程序模板，可以用来拷贝建立工程


## 实验操作

启动后初始化tf卡（tf卡需提前格式化为fat32）
串口打印信息：Input data (press Enter to save):
此时，通过串口发送数据，系统会在文件系统中查找FATFS.TXT文件
如有，则直接打开；如没有，则创建该文件
打开后，向该文件写入串口发送的数据
写入完成后，打开该文件，进行读取。

## 引脚分配

PE2  --  SD_CD
PD2  --  SD_CMD
PC8  --  SD_DAT0
PC9  --  SD_DAT1
PC10 --  SD_DAT2
PC11 --  SD_DAT3
PC12 --  SD_CLK

## 程序版本

- 程序版本：V0.1
- 发布日期：2025-03-22

## 联系我们

- Copyright   : CIMC中国智能制造挑战赛
- Author      ：Lingyu Meng
- Website     ：www.siemenscup-cimc.org.cn
- Phone       ：15801122380

## 声明

**严禁商业用途，仅供学习使用。 **


## 目录结构

├─01 Readme		工程项目说明
├─CMSIS			内核驱动文件：Cortex Microcontroller Software Interface Standard
├─Function		用户程序
├─HardWare		硬件驱动
├─HeaderFiles	头文件集合
├─Library		库文件
│  ├─GD32F4xx_standard_peripheral
│  ├─GD32F4xx_usb_library
│  └─Third_Party
├─project		工程文件（含生成的连接文件）
├─Protocol		协议程序
├─Startup		启动文件
├─System		
└─User
