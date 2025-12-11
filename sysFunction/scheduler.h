#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "HeaderFiles.h"


typedef struct {
    void (*task_func)(void);
    uint16_t rate_ms;
    uint32_t last_run;
}task_t;





void SD_test(void);

void Scheduler_Init(void);
void Scheduler_Run(void);
#endif
