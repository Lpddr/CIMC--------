#ifndef ADC_APP_H
#define ADC_APP_H

#include "HeaderFiles.h"


/******************************************************************************************/
/* ADC及引脚 定义 */
/***************************************************************************************************************/

/* ADC */
#define ADC1_PORT       GPIOC
#define ADC1_CLK_PORT   RCU_GPIOC

#define ADC1_PIN        GPIO_PIN_0

extern uint16_t adc_value[1];

// FUNCTION
void bsp_adc_init(void);

/***************************************************************************************************************/



#endif
