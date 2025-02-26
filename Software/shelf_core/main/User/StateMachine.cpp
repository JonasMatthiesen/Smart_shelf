#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "Message_control/Message_controller.h"
#include "Shelf_control/Shelf_controller.h"
#define TASK_PRIO_3         3
#define TASK_PRIO_2         2
extern "C"
{
    void state_start(void *arg)
    {
        static bool started = false;

        if (!started)
        {
            xTaskCreatePinnedToCore(message_loop, "Message Task", 4096, (void*)2, TASK_PRIO_2, NULL, tskNO_AFFINITY);
            xTaskCreatePinnedToCore(shelf_loop, "State Task", 4096, (void*)3, TASK_PRIO_3, NULL, tskNO_AFFINITY);
            started = true;
        }
        vTaskDelay(pdMS_TO_TICKS(150));
    }

}