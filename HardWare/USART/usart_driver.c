#include "usart_driver.h"


int my_printf(uint32_t usart_periph, const char *format, ...) {
    char buffer[512];
    va_list arg;
    int len;

    // 1. 格式化字符串
    va_start(arg, format);
    len = vsnprintf(buffer, sizeof(buffer), format, arg);
    va_end(arg);

    // 2. 通过GD32库发送数据
    if (len > 0) {
        for (uint16_t i = 0; i < len; i++) {
            // 等待发送缓冲区空
            while (RESET == usart_flag_get(usart_periph, USART_FLAG_TBE));
            // 发送单个字符
            usart_data_transmit(usart_periph, (uint8_t)buffer[i]);
        }
    }

    return len;
}

void USART0_Config(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);    // 使能GPIO时钟

    rcu_periph_clock_enable(RCU_USART0);   // 使能串口时钟
	
	  gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_9 | GPIO_PIN_10);
	//PA9Tx PA10Rx
		gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9);   // GPIO 模式设置
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);  // 输出参数设置
    
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_10);


    usart_deinit(USART0);    // 串口复位
    usart_word_length_set(USART0, USART_WL_8BIT);  // 字长
    usart_stop_bit_set(USART0, USART_STB_1BIT);    // 停止位
    usart_parity_config(USART0, USART_PM_NONE);
    usart_baudrate_set(USART0, 115200U);     // 波特率
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);     // 接收使能
	  usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);   // 发送使能
	  usart_hardware_flow_rts_config(USART0, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(USART0, USART_CTS_DISABLE);
    usart_enable(USART0);           // 串口使能
    

//    usart_dma_transmit_config(USART1, USART_RECEIVE_DMA_ENABLE);//打开串口DMA发送
	usart_dma_receive_config(USART0, USART_RECEIVE_DMA_ENABLE);//打开串口DMA接收
    usart_flag_clear(USART0, USART_FLAG_TC);
    usart_interrupt_enable(USART0, USART_INT_IDLE);
    nvic_irq_enable(USART0_IRQn,0,1);

}


void USART0_DMA_Config(void)
{
    dma_single_data_parameter_struct dma_init_struct;
    
    ringbuffer_init(&uart_dma_buffer);
    
    // 使能DMA时钟
    rcu_periph_clock_enable(RCU_DMA1);
    
    // DMA 去初始化
    dma_deinit(DMA1, DMA_CH2);  // USART0_RX 对应 DMA_CH2
    
    // 配置 DMA 接收参数
    dma_init_struct.periph_addr = (uint32_t)&USART_DATA(USART0);  // 外设地址
    dma_init_struct.memory0_addr = (uint32_t)uart_dma_rx_buffer;   // 内存地址
    dma_init_struct.direction = DMA_PERIPH_TO_MEMORY;              // 数据传输方向
    dma_init_struct.number = UART_RX_BUFFER_SIZE;                  // 传输数量
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;      // 外设地址不递增
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;       // 内存地址递增
    dma_init_struct.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;   // 传输数据宽度
    dma_init_struct.circular_mode = DMA_CIRCULAR_MODE_DISABLE;     // 保持普通模式（非循环）
    dma_init_struct.priority = DMA_PRIORITY_MEDIUM;                // 优先级
    
    // 初始化 DMA
    dma_single_data_mode_init(DMA1, DMA_CH2, &dma_init_struct);
    
    dma_channel_subperipheral_select(DMA1, DMA_CH2, DMA_SUBPERI4);
    // 使能 DMA 通道
    dma_channel_enable(DMA1, DMA_CH2);
    
    
    // 初始化标志
    uart_flag = 0;
}

// USART0中断处理函数
void USART0_IRQHandler(void)
{
    uint16_t dma_receive_size = 0;
    
    // 检查是否是空闲中断
    if(RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_IDLE))
    {
        // 清除空闲中断标志 - 先读STAT，再读DATA寄存器
        (void)USART_STAT0(USART0);  // 读取状态寄存器
        (void)USART_DATA(USART0);  // 读取数据寄存器，清除IDLE标志
        
        // 暂停DMA传输
        dma_channel_disable(DMA1, DMA_CH2);
        
        // 计算已接收的数据量
        dma_receive_size = UART_RX_BUFFER_SIZE - dma_transfer_number_get(DMA1, DMA_CH2);
        
        // 处理接收到的数据
        if(dma_receive_size > 0)
        {
            // 检查环形缓冲区是否有足够空间
            if((uart_dma_buffer.itemCount + dma_receive_size) <= RINGBUFFER_SIZE)
            {
                // 将DMA接收到的数据放入环形缓冲区
                ringbuffer_write(&uart_dma_buffer, uart_dma_rx_buffer, dma_receive_size);
                // 设置标志，表示有新数据可用
                uart_flag = 1;
            }
        }
        
        // 清空DMA接收缓冲区
        memset(uart_dma_rx_buffer, 0, UART_RX_BUFFER_SIZE);
        
        // 重新配置并启动DMA传输
        dma_memory_address_config(DMA1, DMA_CH2, DMA_MEMORY_0, (uint32_t)uart_dma_rx_buffer);
        dma_transfer_number_config(DMA1, DMA_CH2, UART_RX_BUFFER_SIZE);
        dma_flag_clear(DMA1, DMA_CH2, DMA_FLAG_FTF);
        dma_channel_enable(DMA1, DMA_CH2); 
    }
}



void Uart_Init(void)
{
    USART0_Config();
    USART0_DMA_Config();
}



