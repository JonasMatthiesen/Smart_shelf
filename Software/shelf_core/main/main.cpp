/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "main.h"
#include "User/StateMachine.h"
#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"


extern "C" void app_main(void)
{
    printf("Program start\n");
    xTaskCreatePinnedToCore(statemachine_task, "State Task", 4096, (void*)0, 1, NULL, tskNO_AFFINITY);

    while(1)
    {
        //printf("Main loop\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
