#include <stdio.h>
#include <unistd.h>
#include "Shelf_controller.h"
#include <sys/lock.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "Display.h"

const uint16_t SHELF_LOOP_TIME_MS = 300;
uint32_t last_time = 0;

uint32_t millis()
{
    return esp_timer_get_time() / 1000;
}

uint32_t elapsed_since(uint32_t last)
{
    return millis() - last;
}

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
    Display::start();
    Display::load_splash_screen();
}
bool splash_done = false;
bool shelf1_data_set = false;
bool shelf2_data_set = false;

void shelf_loop()
{
    if (elapsed_since(last_time) > 5000 && !splash_done)
    {
        Display::load_main_screen();
        splash_done = true;
    }

    if (elapsed_since(last_time) > 10000 && !shelf1_data_set)
    {
        Display::set_active_shelf(Display::ActiveShelf::SHELF_1);
        Display::set_shelf_data(Display::ActiveShelf::SHELF_1, "Screw", "22");
        shelf1_data_set = true;
    }

    if (elapsed_since(last_time) > 15000 && !shelf2_data_set)
    {
        Display::set_active_shelf(Display::ActiveShelf::SHELF_2);
        Display::set_shelf_data(Display::ActiveShelf::SHELF_2, "Bolt", "55");
        shelf2_data_set = true;
    }
}