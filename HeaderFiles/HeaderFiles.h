#ifndef __HEADERFILES_H
#define __HEADERFILES_H

/************************* ???? *************************/
/*GD??????*/
#include "gd32f4xx.h"
#include "gd32f4xx_libopt.h"
#include "gd32f4xx_dma.h"
#include "systick.h"
#include "Function.h"     

/*C?????????*/
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>


/*???????*/

#include "led_driver.h"
#include "key_driver.h"
#include "usart_driver.h"
#include "OLED.h"
#include "lfs_port.h"
#include "rtc_driver.h"
#include "adc_driver.h"

//?????
#include "scheduler.h"
#include "uart_app.h"
#include "SDcard_app.h"
#include "sampling_control.h"  // ��������ͷ�ļ�
#include "data_save.h"         // 数据存储头文件

/*???????*/
#include "ringbuffer.h"
#include "ff.h"
#include "diskio.h"
#include "sdcard.h"
#include "SPI_FLASH.h"


/*??????????*/
extern float ADC_value;

#endif

/****************************End*****************************/

