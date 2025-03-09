#include <stdio.h>
#include <unistd.h>
#include "Shelf_controller.h"
#include <sys/lock.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../common.h"
#include "Display.h"
#include "driver/gpio.h"
#include "HX711.h"
#include "esp_log.h"
#include "esp_timer.h"

const bool DEBUG_MODE = false;

static const char *TAG = "SHLF_CTRL";
const uint16_t SHELF_LOOP_TIME_MS = 300;
const uint16_t CONNECTION_TIMEOUT = 20000;

uint32_t last_time = 0;

gpio_num_t SHELF1_HALL_PIN = GPIO_NUM_26;
gpio_num_t HX711_SCK_PIN = GPIO_NUM_32;
gpio_num_t HX711_DOUT_PIN = GPIO_NUM_35;

ShelfData shelf_data;

hx711_t hx711 = {
    .dout = HX711_DOUT_PIN,
    .pd_sck = HX711_SCK_PIN,
    .gain = HX711_GAIN_A_64
};

inline uint32_t millis()
{
    return esp_timer_get_time() / 1000;
}

inline uint32_t elapsed_since(uint32_t last)
{
    return millis() - last;
}

void calibrate_weight()
{
    //Get initial weight of shelf
    ESP_LOGI(TAG, "Calibration started" PRIi32);
    ESP_LOGI(TAG, "Please empty shelves within 10 seconds" PRIi32);
    vTaskDelay(pdMS_TO_TICKS(10000));

    esp_err_t r = hx711_wait(&hx711, 500);
    if (r != ESP_OK)
    {
        ESP_LOGE(TAG, "Device not found: %d (%s)\n", r, esp_err_to_name(r));
    }

    r = hx711_read_average(&hx711, 10, &shelf_data.calib_offset);
    if (r != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not read data: %d (%s)\n", r, esp_err_to_name(r));
    }
    else
    {
        ESP_LOGI(TAG, "Weight offset data: %li" PRIi32, shelf_data.calib_offset);
    }

    ESP_LOGI(TAG, "Please put 1kg on the shelf within 10 seconds" PRIi32);

    vTaskDelay(pdMS_TO_TICKS(10000));

    r = hx711_wait(&hx711, 500);
    if (r != ESP_OK)
    {
        ESP_LOGE(TAG, "Device not found: %d (%s)\n", r, esp_err_to_name(r));
    }

    int32_t weight_1kg;
    r = hx711_read_average(&hx711, 10, &weight_1kg);
    if (r != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not read data: %d (%s)\n", r, esp_err_to_name(r));
    }
    else
    {
        ESP_LOGI(TAG, "1kg data: %" PRIi32, weight_1kg);
    }

    shelf_data.calib_scalar = 1000.0 / (weight_1kg - shelf_data.calib_offset);

    ESP_LOGI(TAG, "Weight offset: %li\nWeight scalar:  %f" PRIi32, shelf_data.calib_offset, shelf_data.calib_scalar);
}

float get_current_weight()
{
    if (shelf_data.calib_offset == 0 && shelf_data.calib_scalar == 0)
    {
        ESP_LOGI(TAG, "Not calibrated, measurement will be inaccurate" PRIi32);
    }

    esp_err_t r = hx711_wait(&hx711, 500);
    if (r != ESP_OK)
    {
        ESP_LOGE(TAG, "Device not found: %d (%s)\n", r, esp_err_to_name(r));
    }

    int32_t data;
    r = hx711_read_average(&hx711, 5, &data);
    if (r != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not read data: %d (%s)\n", r, esp_err_to_name(r));
    }

    float weight = (data-shelf_data.calib_offset)*shelf_data.calib_scalar;
    return weight;
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
    gpio_set_direction(SHELF1_HALL_PIN, GPIO_MODE_INPUT);
    Display::start();
    Display::load_splash_screen();
    // initialize device
    ESP_ERROR_CHECK(hx711_init(&hx711));

    if (DEBUG_MODE)
    {
        float weight_scaler = 0.095;
        int32_t weight_offset = 8600;
        shelf_data.calib_scalar = weight_scaler;
        shelf_data.calib_offset = weight_offset;

        Display::set_shelf_data(Display::ActiveShelf::SHELF_2, "Bolt", "55");
        Display::set_shelf_data(Display::ActiveShelf::SHELF_1, "Screw", "22");
    }
    else
    {
        //Wait for connection to server
        uint32_t now = millis();
        ESP_LOGI(TAG, "Waiting for message controller connection" PRIi32);
        while (elapsed_since(now) < CONNECTION_TIMEOUT)
        {
            vTaskDelay(pdMS_TO_TICKS(650));
            if (Common::get_connected())
            {
                ESP_LOGI(TAG, "Message controller connected" PRIi32);
                break;
            }
        }

        if (elapsed_since(now) >= CONNECTION_TIMEOUT)
        {
            //Make screen message that connection is lost and reboot?
            ESP_LOGE(TAG, "Failed connecting - Timed out" PRIi32);
        }

        //Wait for shelf data to be initialized
        now = millis();
        ESP_LOGI(TAG, "Waiting for shelf data update" PRIi32);
        while (elapsed_since(now) < CONNECTION_TIMEOUT)
        {
            vTaskDelay(pdMS_TO_TICKS(650));
            if (Common::get_shelf_data_initialized())
            {
                ESP_LOGI(TAG, "Shelf data initialized" PRIi32);
                break;
            }
        }

        if (elapsed_since(now) >= CONNECTION_TIMEOUT)
        {
            //Make screen message that connection is lost and reboot?
            ESP_LOGE(TAG, "Failed connecting - Timed out" PRIi32);
        }
        
        shelf_data = Common::get_shelf_data();

        Display::set_shelf_data(Display::ActiveShelf::SHELF_2, shelf_data.s1_mpn, shelf_data.s1_qty);
        Display::set_shelf_data(Display::ActiveShelf::SHELF_1, shelf_data.s2_mpn, shelf_data.s2_qty);

        //This means we have a clean start, and we need to calibrate the weight
        if (shelf_data.calib_offset == 0 || shelf_data.calib_scalar == 0)
        {
            calibrate_weight();
            Common::set_shelf_data(shelf_data);
            Common::set_shelf_data_updated(true);
            Display::load_main_screen();
            Common::set_init_complete(true);
        }
        else
        {
            //Sanity check to see if weight is correct compared to server value
            float current_weight = get_current_weight();

            if (current_weight < shelf_data.total_weight - 100 || current_weight > shelf_data.total_weight + 100)
            {
                //Display error message on screen
                ESP_LOGE(TAG, "Weight is not as expected, please check the shelf" PRIi32);
            }
            else
            {
                Display::load_main_screen();
                Common::set_init_complete(true);
            }
        }
    }



    //calibrate_weight();

    //Get inital weight of shelf (average wait over 3 seconds)

    //Get initial shelf data from server (mpn, qty, weight per item)

    //Compare measured weight with qty*weight per item and check if the weight is correct
}
bool splash_done = false;
bool shelf1_data_set = false;
bool shelf2_data_set = false;

void shelf_loop()
{
    if (Common::get_init_complete())
    {
        int s1_active = gpio_get_level(SHELF1_HALL_PIN);
    
        if (s1_active)
        {
            Display::set_active_shelf(Display::ActiveShelf::SHELF_1);
            ESP_LOGI(TAG, "Weight data: %f" PRIi32, get_current_weight());
        }
        else
        {
            Display::set_active_shelf(Display::ActiveShelf::NONE);
        }
    }
    //Check if any shelf is open, and wake up if any active

    //wait for shelf to close

    //compare weight with previous weight and find out how many items were taken/removed

    //update the display with the new data

    //put data in a place where it can be send to the server

    if (DEBUG_MODE)
    {
        if (elapsed_since(last_time) > 5000 && !splash_done)
        {
            Display::load_main_screen();
            splash_done = true;
        }
    
        int s1_active = gpio_get_level(SHELF1_HALL_PIN);
    
        if (s1_active)
        {
            Display::set_active_shelf(Display::ActiveShelf::SHELF_1);
            ESP_LOGI(TAG, "Weight data: %f" PRIi32, get_current_weight());
        }
        else
        {
            Display::set_active_shelf(Display::ActiveShelf::NONE);
        }
    }
}