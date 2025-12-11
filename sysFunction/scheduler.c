#include "scheduler.h"
#include "sampling_control.h"  // ????????????????
/*************************????**************************/
uint8_t task_num;
uint32_t nowtime;
//key
uint8_t Key_Down,Key_Up,Key_Old,Key_Val;


//usart
uint8_t uart_dma_rx_buffer[UART_RX_BUFFER_SIZE];
uint8_t uart_read_buffer[UART_RX_BUFFER_SIZE];
ringbuffer_t uart_dma_buffer;
uint8_t uart_flag;

//adc
uint16_t adc_value[1];
float ADC_value;

//rtc
uint8_t hour, min, sec, ampm;
uint8_t year, month, date, week;

//flash
lfs_t lfs;             // lfs ?????????
lfs_file_t file;       // lfs ???????
struct lfs_config cfg; // lfs ????????????

//sdcard
uint16_t  count, result = 0;
extern FIL fdst;    // 外部声明，实际定义在data_save.c中
extern FATFS fs;    // 外部声明，实际定义在data_save.c中
UINT br, bw;
BYTE textfilebuffer[2048] = "GD32MCU FATFS TEST!\r\n";
BYTE buffer[128];
BYTE filebuffer[128];

/************************ led??????? ************************/



/************************ key??????? ************************/

void Key_Proc(void)
{
    Key_Val=Key_Read();
    Key_Down=Key_Val&(Key_Val^Key_Old);
    Key_Up=~Key_Val&(Key_Val^Key_Old);
    Key_Old=Key_Val;

    
    // ����KEY2���£�PE4���ţ�- 5������
    if(Key_Down == 1)  // KEY2����
    {
        my_printf(USART0, "sample cycle adjust: 5s\r\n");
        Buffer_Log_Data("KEY2 pressed - Set 5s period");
        Set_Sampling_Period(0);  // ����5������
    }

    // ����KEY3���£�PE5���ţ�- 10������
    if(Key_Down == 2)  // KEY3����
    {
        my_printf(USART0, "sample cycle adjust: 10s\r\n");
        Buffer_Log_Data("KEY3 pressed - Set 10s period");
        Set_Sampling_Period(1);  // ����10������
    }

    // ����KEY4���£�PE6���ţ�- 15������
    if(Key_Down == 3)  // KEY4����
    {
        my_printf(USART0, "sample cycle adjust: 15s\r\n");
        Buffer_Log_Data("KEY3 pressed - Set 15s period");
        Set_Sampling_Period(2);  // ����15������
    }

    // ����KEY1���£�PE3���ţ�- ��������
    if(Key_Down == 4)  // KEY1����
    {
        Buffer_Log_Data("KEY1 pressed - Toggle sampling");
        Sampling_Control_Toggle();  // �л�����״̬
    }
}

/************************ oled??????? ************************/

void Oled_Proc(void)
{
    // ??????????????????????????
    if(!Is_Sampling_Running())
    {
        Oled_Printf(0,0,"system idle   ");
        Oled_Printf(0,16,"              ");  // ????????????????????????
        OLED_Refresh(); 									/* ?????????OLED */
    }
}





/************************ adc??????? ************************/

void Adc_Proc(void)
{
    ADC_value=(float)adc_value[0]/819.0f;
}

/************************ rtc??????? ************************/

void Rtc_Proc(void)
{
	rtc_get_time(&hour, &min, &sec, &ampm);
	rtc_get_date(&year, &month, &date, &week);
}

/************************ flash??????? ************************/
void TestLFS(const struct lfs_config *cfg, lfs_t *lfs, lfs_file_t *file) {
    // mount the filesystem
    int err = lfs_mount(lfs, cfg);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err) {
        lfs_format(lfs, cfg);
        lfs_mount(lfs, cfg);
    }

    // read current count
    uint32_t boot_count = 0;
    lfs_file_open(lfs, file, "boot_count", LFS_O_RDWR | LFS_O_CREAT);
    lfs_file_read(lfs, file, &boot_count, sizeof(boot_count));

    // update boot count
    boot_count += 1;
    lfs_file_rewind(lfs, file);
    lfs_file_write(lfs, file, &boot_count, sizeof(boot_count));

    // remember the storage is not updated until the file is closed successfully
    lfs_file_close(lfs, file);

    // release any resources we were using
    lfs_unmount(lfs);

    // print the boot count
    my_printf(USART0,"boot_count: %d\n", boot_count);
}


void Flash_Proc(void)
{
     //TestLFS(&cfg, &lfs, &file);
}

/************************ sdcard??????? ************************/
ErrStatus memory_compare(uint8_t* src, uint8_t* dst, uint16_t length) 
{
    while(length --){
        if(*src++ != *dst++)
            return ERROR;
    }
    return SUCCESS;
}



void SD_test(void)
{
    	uint16_t k = 5;
	DSTATUS stat = 0;

	
//	gd_eval_com_init(); //????????

	do
	{
		stat = disk_initialize(0); 			//?????SD?????????0??,?????????????,?????????????????????U ??????????????????????????
	}while((stat != 0) && (--k));			//???????????????????k????
    
    my_printf(USART0,"SD Card disk_initialize:%d\r\n",stat);
    f_mount(0, &fs);						 //????SD????????????????0????
    my_printf(USART0,"SD Card f_mount:%d\r\n",stat);

	if(RES_OK == stat)						 //???????????FR_OK ??????????
	{        
        my_printf(USART0,"\r\nSD Card Initialize Success!\r\n");
	 
        result = f_open(&fdst, "0:/FATFS.TXT", FA_CREATE_ALWAYS | FA_WRITE);		//??SD??????????FATFS.TXT??
	 
		//write_file();	//?????????

		result = f_write(&fdst, textfilebuffer, sizeof(textfilebuffer), &bw); 	//??textfilebuffer?????????????????
		//result = f_write(&fdst, filebuffer, sizeof(filebuffer), &bw);				//??filebuffer?????????????????
        
		/**********????????? begin****************/
		if(FR_OK == result)		
               my_printf(USART0,"FATFS FILE write Success!\r\n");
        else
		{
                my_printf(USART0,"FATFS FILE write failed!\r\n");
        }
		/**********????????? end****************/
		
        f_close(&fdst);//??????
		
		
        f_open(&fdst, "0:/FATFS.TXT", FA_OPEN_EXISTING | FA_READ);	//????????????????
        br = 1;
		
		/**********????????????? begin****************/
        for(;;)
		{
			// ????????
            for (count=0; count<128; count++)
			{
				buffer[count]=0;
			}
			// ???????????buffer
            result = f_read(&fdst, buffer, sizeof(buffer), &br);
            if ((0 == result)|| (0 == br))
			{
                break;
			}
        }
		/**********????????????? end****************/
		
		// ????????????????????????????
        if(SUCCESS == memory_compare(buffer, textfilebuffer, 128))
		{
			my_printf(USART0,"FATFS Read File Success!\r\nThe content is:%s\r\n",buffer);
		}
        else
		{
            my_printf(USART0,"FATFS FILE read failed!\n");            
        }
         f_close(&fdst);//??????
	} 
	

}


task_t Scheduler_Task[]={
    {Uart_Proc,100,0},
    {Key_Proc,10,0},
    {Oled_Proc,200,0},
    {Flash_Proc,2000,0},
    {Rtc_Proc,1000,0},
    {Adc_Proc,1000,0},
    {Sampling_Control_Task,50,0},  // ????????????50ms????
    {Process_Data_Buffer,200,0}    // 数据缓冲区处理任务，200ms执行一次
};


void Scheduler_Init(void)
{
    task_num=sizeof(Scheduler_Task)/sizeof(task_t);
}

void Scheduler_Run(void)
{
    uint8_t i;
    for(i=0;i<task_num;i++)
    {
        nowtime=Get_Tick();
        if(Scheduler_Task[i].last_run+Scheduler_Task[i].rate_ms<=nowtime)
        {
            Scheduler_Task[i].task_func();
            Scheduler_Task[i].last_run=nowtime;
        }
    }
}
