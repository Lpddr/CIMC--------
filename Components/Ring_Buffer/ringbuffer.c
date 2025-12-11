#include "ringbuffer.h"

// 初始化环形缓冲区
void ringbuffer_init(ringbuffer_t *rb)
{
    // 初始化两个指针为0
    rb->r = 0;
    rb->w = 0;
    // 初始化缓冲区为0
    memset(rb->buffer, 0, sizeof(uint8_t) * RINGBUFFER_SIZE);
    // 初始化数据目计数为0
    rb->itemCount = 0;
}

// 判断环形缓冲区是否已满
uint8_t ringbuffer_is_full(ringbuffer_t *rb)
{
    // 如果数据目计数等于缓冲区大小，则返回1，否则返回0，表示未满
    return (rb->itemCount == RINGBUFFER_SIZE);
}

// 判断环形缓冲区是否为空
uint8_t ringbuffer_is_empty(ringbuffer_t *rb)
{
    // 如果数据目计数为0，则表示为空，返回1，否则返回0，表示不为空
    return (rb->itemCount == 0);
}

// 向环形缓冲区写入数据
int8_t ringbuffer_write(ringbuffer_t *rb, uint8_t *data, uint32_t num)
{
    // 如果缓冲区已满，返回-1
    if(ringbuffer_is_full(rb))
        return -1;
    
    // 计算实际可写入的数据量
    uint32_t available_space = RINGBUFFER_SIZE - rb->itemCount;
    uint32_t write_count = (num > available_space) ? available_space : num;
    
    // 将数据写入缓冲区
    for(uint32_t i = 0; i < write_count; i++)
    {
        rb->buffer[rb->w] = *data++;  // 写入数据并移动写指针
        rb->w = (rb->w + 1) % RINGBUFFER_SIZE;  // 写指针循环递增
        rb->itemCount++;  // 增加数据目计数
    }
    
    return (write_count == num) ? 0 : -1;  // 如果全部写入成功返回0，否则返回-1
}

// 从环形缓冲区读取数据
int8_t ringbuffer_read(ringbuffer_t *rb, uint8_t *data, uint32_t num)
{
    // 如果缓冲区为空，返回-1
    if(ringbuffer_is_empty(rb))
        return -1;
    
    // 从缓冲区读取数据
    while(num--)
    {
        *data++ = rb->buffer[rb->r];  // 读取数据并移动读指针
        rb->r = (rb->r + 1) % RINGBUFFER_SIZE;  // 读指针循环递增
        rb->itemCount--;  // 减少数据目计数
    }
    return 0;  // 读取成功返回0
}
