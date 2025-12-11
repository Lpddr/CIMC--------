#include "ff.h"
#include <stdlib.h>  // 用于malloc和free
#include <string.h>  // 用于memset

#if _USE_LFN != 0

/* 将字符转换为大写 */
WCHAR ff_wtoupper(WCHAR chr)
{
    if (chr >= 'a' && chr <= 'z') {
        chr -= 0x20;
    }
    return chr;
}

/* 在字符编码之间转换代码点 */
WCHAR ff_convert(WCHAR chr, UINT dir)
{
    if (chr < 0x80) {
        /* ASCII字符 */
        return chr;
    }
    
    if (dir) {
        /* Unicode转OEM码 */
        return '?';  /* 不支持的字符用?替代 */
    } else {
        /* OEM码转Unicode */
        return chr;
    }
}

#endif /* _USE_LFN != 0 */

/* ======================== 静态缓冲区配置 ======================== */

#if _USE_LFN == 1
/* 当USE_LFN=1时，使用静态工作缓冲区，不需要动态内存分配函数 */
/* 静态缓冲区会自动在BSS段分配，大小为 (_MAX_LFN + 1) * 2 字节 */
#endif /* _USE_LFN == 1 */

#if _USE_LFN == 3
/* 如果将来需要切换回动态内存分配模式，可以启用以下函数 */

/* 使用标准库malloc/free */
void* ff_memalloc(UINT msize)
{
    void* ptr = malloc(msize);
    if (ptr != NULL) {
        memset(ptr, 0, msize);  // 清零分配的内存
    }
    return ptr;
}

void ff_memfree(void* mblock)
{
    if (mblock != NULL) {
        free(mblock);
    }
}

#endif /* _USE_LFN == 3 */
