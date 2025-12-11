/************************************************************
 * ?????2025CIMC Copyright?? 
 * ?????led.c
 * ????: Lingyu Meng
 * ??: 2025CIMC IHD-V04
 * ?��: Lingyu Meng     2025/2/16     V0.01    original
************************************************************/

/************************* ???? *************************/

#include "led_driver.h"

/************************ ?????????? ************************/


/************************************************************ 
 * Function :       LED_Init
 * Comment  :       ????????LED???
 * Parameter:       null
 * Return   :       null
 * Author   :       Lingyu Meng
 * Date     :       2025-02-30 V0.1 original
************************************************************/

void LED_Init(void)
{

	rcu_periph_clock_enable(RCU_GPIOA);    // ?????GPIO_A???????
	rcu_periph_clock_enable(RCU_GPIOE);    // ?????GPIO_E???????

	// ��ʼ��LED1��LED2 (PE15��PE14)
	gpio_mode_set(GPIOE, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_15|GPIO_PIN_14);   			// GPIOģʽ����
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_15|GPIO_PIN_14);   // ���ѡ������
	gpio_bit_reset(GPIOE, GPIO_PIN_15|GPIO_PIN_14);  											// ��ʼ״̬Ϊ�͵�ƽ

	// ��ʼ��PA4 LED��������
	gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_4);   			// GPIOģʽ����
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_4);   // ���ѡ������
	gpio_bit_reset(GPIOA, GPIO_PIN_4);  											// ��ʼ״̬Ϊ�͵�ƽ

	// ��ʼ��LED3��LED4 (PA6��PA7)
	gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_6|GPIO_PIN_7);   			// GPIOģʽ����
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_6|GPIO_PIN_7);     // ���ѡ������
	gpio_bit_set(GPIOA, GPIO_PIN_6|GPIO_PIN_7);  										     	// ��ʼ״̬Ϊ�ߵ�ƽ


}


    



/****************************End*****************************/

