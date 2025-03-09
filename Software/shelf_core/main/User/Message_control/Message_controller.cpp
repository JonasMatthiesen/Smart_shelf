#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Message_controller.h"
#include "../Common.h"
const uint16_t MESSAGE_LOOP_TIME_MS = 150;

ShelfData sdata;

#include "esp_timer.h"

inline uint32_t millis()
{
    return esp_timer_get_time() / 1000;
}

inline uint32_t elapsed_since(uint32_t last)
{
    return millis() - last;
}

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
bool sent = false;
void message_start()
{
    printf("Message start\n");
}

void message_loop()
{
    if (elapsed_since(0) >= 5000 && !sent) 
    {
        Common::set_connected(true);
        float weight_scaler = 0.095;
        int32_t weight_offset = 8600;
        sdata.calib_scalar = weight_scaler;
        sdata.calib_offset = weight_offset;
        sdata.s1_mpn = "Screw";
        sdata.s1_qty = "22";
        sdata.s1_weight_per_item = 50;
        sdata.s1_qty_limit = 100;
    
        sdata.s2_mpn = "Bolt";
        sdata.s2_qty = "55";
        sdata.s2_weight_per_item = 100;
        sdata.s2_qty_limit = 100;

        sdata.total_weight = 1000;
    
        Common::set_shelf_data(sdata);
        Common::set_shelf_data_initialized(true);
        sent = true;
    }
}