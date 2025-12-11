#include "key_driver.h"

uint8_t Key_Read(void)
{
    uint8_t temp=0;
    if(gpio_input_bit_get(GPIOE,GPIO_PIN_4)==RESET)temp=1;  // KEY2连接PE4引脚 - 5s周期
    if(gpio_input_bit_get(GPIOE,GPIO_PIN_5)==RESET)temp=2;  // KEY3连接PE5引脚 - 10s周期
    if(gpio_input_bit_get(GPIOE,GPIO_PIN_6)==RESET)temp=3;  // KEY4连接PE6引脚 - 15s周期
    if(gpio_input_bit_get(GPIOE,GPIO_PIN_3)==RESET)temp=4;  // KEY1连接PE3引脚 - 采样控制
    return temp;
}


void Key_Init()
{
    rcu_periph_clock_enable(RCU_GPIOE);
    gpio_mode_set(GPIOE,GPIO_MODE_INPUT,GPIO_PUPD_PULLUP,GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6);  // KEY1-4连接PE3-6引脚
}


