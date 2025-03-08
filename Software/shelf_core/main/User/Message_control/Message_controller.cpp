#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Message_controller.h"

const uint16_t MESSAGE_LOOP_TIME_MS = 150;

void message_task(void *arg)
{
    static bool started = false;

    if (!started)
    {
        message_start();
        started = true;
    }

    while (1)
    {
        message_loop();
        vTaskDelay(pdMS_TO_TICKS(MESSAGE_LOOP_TIME_MS));
    }
    
}

void message_start()
{
    printf("Message start\n");
}

void message_loop()
{

}