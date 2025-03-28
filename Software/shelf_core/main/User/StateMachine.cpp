#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "Message_control/Message_controller.h"
#include "Shelf_control/Shelf_controller.h"
#include "StateMachine.h"

#define TASK_PRIO_3         3
#define TASK_PRIO_2         2
const uint16_t STATEMACHINE_LOOP_TIME_MS = 1000;

void statemachine_task(void *arg)
{
    static bool started = false;
    
    if (!started)
    {
        statemachine_start();
        started = true;
    }

    while (1)
    {
        statemachine_loop();
        vTaskDelay(pdMS_TO_TICKS(STATEMACHINE_LOOP_TIME_MS));
    }

}

void statemachine_start()
{
    printf("Statemachine start\n");
    xTaskCreatePinnedToCore(message_task, "Message Task", 4096*2, (void*)2, TASK_PRIO_2, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(shelf_task, "Shelf Task", 4096, (void*)3, TASK_PRIO_3, NULL, tskNO_AFFINITY);
}

void statemachine_loop()
{

}



