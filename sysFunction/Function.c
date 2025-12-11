
/************************* ͷ�ļ� *************************/

#include "HeaderFiles.h"


/************************************************************ 
 * Function :       System_Init
 * Comment  :       ���ڳ�ʼ��MCU
 * Parameter:       null
 * Return   :       null
 * Author   :       Lingyu Meng
 * Date     :       2025-02-30 V0.1 original
************************************************************/


void nvic_config(void)
{
    nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);	// �����ж����ȼ�����
    nvic_irq_enable(SDIO_IRQn, 0, 0);					// ʹ��SDIO�жϣ����ȼ�Ϊ0
}


void System_Init(void)
{
	systick_config();     // ʱ������
    nvic_config();		//�����жϿ�����
}



/************************************************************ 
 * Function :       UsrFunction
 * Comment  :       �û�������
 * Parameter:       null
 * Return   :       null
 * Author   :       Liu Tao @ GigaDevice
 * Date     :       2025-05-10 V0.1 original
************************************************************/
void Erase_Boot_count(void)
{
    uint8_t data_buffer_earse[4]={0,0,0,0};
     spi_flash_buffer_write(data_buffer_earse, BOOT_COUNT_FLASH_ADDR, 4);
   /* my_printf(USART0, "Data: %02X %02X %02X %02X\r\n", 
          data_buffer_earse[0], data_buffer_earse[1], 
          data_buffer_earse[2], data_buffer_earse[3]);*/
}


void UsrFunction(void)
{
    delay_1ms(100);
    LED_Init();
    Key_Init();
    OLED_Init();
    Uart_Init();
    spi_flash_init();
    Scheduler_Init();
    bsp_adc_init();
   
    rtc_config();
    Sampling_Control_Init();  
    Sampling_Control_Reset(); 

    //Erase_Boot_count();
    if(Data_Save_Init()) {
//        my_printf(USART0,"Data storage system initialized\r\n");
    } else {
//        my_printf(USART0,"Data storage system init failed - system will continue without SD storage\r\n");
        // 即使数据存储初始化失败，系统也要继续运行
    }

    my_printf(USART0,"====system init====\r\n");

    // 打印设备ID
    Print_Device_ID();

    my_printf(USART0,"====system ready====\r\n");
    Init_Device_ID_If_Empty();
	while(1)
	{
        Scheduler_Run();
	}
}




/****************************End*****************************/

