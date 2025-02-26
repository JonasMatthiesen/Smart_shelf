#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C"
{
    void shelf_loop(void *arg)
    {
        printf("SHELF TASK!\n");
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}