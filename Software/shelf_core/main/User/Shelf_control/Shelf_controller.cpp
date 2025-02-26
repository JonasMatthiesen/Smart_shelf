#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Shelf_controller.h"

const uint16_t SHELF_LOOP_TIME_MS = 300;

void shelf_task(void *arg)
{
    static bool started = false;

    if (!started)
    {
        shelf_start();
        started = true;
    }

    while (1)
    {
        shelf_loop();
        vTaskDelay(pdMS_TO_TICKS(SHELF_LOOP_TIME_MS));
    }
}

void shelf_start()
{
    printf("Shelf start\n");
}

void shelf_loop()
{
    printf("Shelf loop\n");
}