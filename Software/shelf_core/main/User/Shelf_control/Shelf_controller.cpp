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
#include <math.h>       /* round, floor, ceil, trunc */

static const char *TAG = "SHLF_CTRL";
const uint16_t SHELF_LOOP_TIME_MS = 300;
const uint16_t CONNECTION_TIMEOUT = 20000;

uint32_t last_time = 0;

gpio_num_t SHELF1_HALL_PIN = GPIO_NUM_26;
gpio_num_t SHELF2_HALL_PIN = GPIO_NUM_27;
gpio_num_t SHELF3_HALL_PIN = GPIO_NUM_25; //Is actually 32, but weigth sensor is on 32 on breadboard

gpio_num_t HX711_SCK_PIN = GPIO_NUM_32; //Should be moved to 33 with botch
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
    Display::set_bootup_title("Calibration");
    ESP_LOGI(TAG, "Please empty shelves within 10 seconds" PRIi32);
    Display::set_bootup_message("Please empty shelves\n          within 10s");
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
    Display::set_bootup_message("Please put 1kg on the shelf\n             within 10s");
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

    ESP_LOGI(TAG, "Raw weight: %li \n", data);
    float weight = (data-shelf_data.calib_offset)*shelf_data.calib_scalar;
    ESP_LOGI(TAG, "Scaled weight: %f \n", weight);
    return weight;
}

//This function checks if the wifi is provisioned, and if the message controller is connected to AWS. It will wait CONNECTION_TIMEOUT time for each check
//If the wifi is not provisioned, it will wait until it is provisioned. If connection couldn't be established, it will reboot
void startup_checks_and_wait()
{
    //Check wifi connection
    uint32_t now = millis();
    ESP_LOGI(TAG, "Checking wifi provisioning" PRIi32);
    Display::set_bootup_message("Connecting to WIFI");
    vTaskDelay(pdMS_TO_TICKS(2000));
    while (elapsed_since(now) < CONNECTION_TIMEOUT)
    {
        vTaskDelay(pdMS_TO_TICKS(650));
        if (Common::get_wifi_provisioned())
        {
            ESP_LOGI(TAG, "Wifi provisioned" PRIi32);
            break;
        }
    }

    if (elapsed_since(now) >= CONNECTION_TIMEOUT)
    {
        //Make screen message that connection is lost and reboot?
        ESP_LOGE(TAG, "WIFI provisioning required" PRIi32);
        Display::set_bootup_message("WIFI needs to be provisioned");

        while (!Common::get_wifi_provisioned())
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        Display::set_bootup_message("Connected to WIFI");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    //Wait for connection to server
    now = millis();
    ESP_LOGI(TAG, "Waiting for message controller connection" PRIi32);
    Display::set_bootup_message("Connecting to server");
    vTaskDelay(pdMS_TO_TICKS(2000));
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
    Display::set_bootup_message("Retrieving data");
    vTaskDelay(pdMS_TO_TICKS(2000));
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
        ESP_ERROR_CHECK(ESP_FAIL);
    }
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
    gpio_set_direction(SHELF2_HALL_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(SHELF3_HALL_PIN, GPIO_MODE_INPUT);
    
    Display::start();
    Display::load_splash_screen();
    // initialize device
    ESP_ERROR_CHECK(hx711_init(&hx711));

    startup_checks_and_wait();
    
    shelf_data = Common::get_shelf_data();

    Display::set_shelf_data(Display::ActiveShelf::SHELF_1, shelf_data.s1_mpn, shelf_data.s1_qty);
    Display::set_shelf_data(Display::ActiveShelf::SHELF_2, shelf_data.s2_mpn, shelf_data.s2_qty);
    Display::set_shelf_data(Display::ActiveShelf::SHELF_3, shelf_data.s3_mpn, shelf_data.s3_qty);

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
            Display::set_bootup_message("Weight is not as expected");
            ESP_LOGE(TAG, "Weight is not as expected, please check the shelf" PRIi32);
        }
        else
        {
            Display::load_main_screen();
            Common::set_init_complete(true);
        }
    }
}

bool splash_done = false;
bool shelf1_data_set = false;
bool shelf2_data_set = false;

void shelf_loop()
{
    if (SHELF_CONTROL_DEBUG_MODE)
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

        float new_weight = get_current_weight();
    }
    else if (Common::get_init_complete())
    {
        int s1_active = gpio_get_level(SHELF1_HALL_PIN);
        int s2_active = gpio_get_level(SHELF2_HALL_PIN);
        int s3_active = gpio_get_level(SHELF3_HALL_PIN);

        if (s1_active)
        {
            Display::set_active_shelf(Display::ActiveShelf::SHELF_1);
            float prev_weight = get_current_weight();
            ESP_LOGI(TAG, "Previous weight: %f" PRIi32, prev_weight);

            uint32_t now = millis();
            while (gpio_get_level(SHELF1_HALL_PIN))
            {
                vTaskDelay(pdMS_TO_TICKS(100));
            }

            float new_weight = get_current_weight();
            ESP_LOGI(TAG, "New weight: %f" PRIi32, new_weight);

            float weight_diff = new_weight-prev_weight;
            float new_qty = weight_diff / shelf_data.s1_weight_per_item;

            ESP_LOGI(TAG, "Weight diff: %f" PRIi32, weight_diff);
            ESP_LOGI(TAG, "New qty: %f" PRIi32, floor(new_qty));

            shelf_data.s1_qty += floor(new_qty);
            shelf_data.total_weight = new_weight;
            
            Display::set_shelf_data(Display::ActiveShelf::SHELF_1, shelf_data.s1_mpn, shelf_data.s1_qty);
            Common::set_shelf_data(shelf_data);
            Common::set_shelf_data_updated(true);

            Display::set_active_shelf(Display::ActiveShelf::NONE);
        }
        else if (s2_active)
        {
                Display::set_active_shelf(Display::ActiveShelf::SHELF_2);
                float prev_weight = get_current_weight();
                ESP_LOGI(TAG, "Previous weight: %f" PRIi32, prev_weight);
    
                uint32_t now = millis();
                while (gpio_get_level(SHELF2_HALL_PIN))
                {
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
    
                float new_weight = get_current_weight();
                ESP_LOGI(TAG, "New weight: %f" PRIi32, new_weight);
    
                float weight_diff = new_weight-prev_weight;
                float new_qty = weight_diff / shelf_data.s2_weight_per_item;
    
                ESP_LOGI(TAG, "Weight diff: %f" PRIi32, weight_diff);
                ESP_LOGI(TAG, "New qty: %f" PRIi32, floor(new_qty));
    
                shelf_data.s2_qty += floor(new_qty);
                shelf_data.total_weight = new_weight;
                
                Display::set_shelf_data(Display::ActiveShelf::SHELF_2, shelf_data.s2_mpn, shelf_data.s2_qty);
                Common::set_shelf_data(shelf_data);
                Common::set_shelf_data_updated(true);
    
                Display::set_active_shelf(Display::ActiveShelf::NONE);
        }
        else if (s3_active)
        {
            Display::set_active_shelf(Display::ActiveShelf::SHELF_3);
            float prev_weight = get_current_weight();
            ESP_LOGI(TAG, "Previous weight: %f" PRIi32, prev_weight);

            uint32_t now = millis();
            while (gpio_get_level(SHELF3_HALL_PIN))
            {
                vTaskDelay(pdMS_TO_TICKS(100));
            }

            float new_weight = get_current_weight();
            ESP_LOGI(TAG, "New weight: %f" PRIi32, new_weight);

            float weight_diff = new_weight-prev_weight;
            float new_qty = weight_diff / shelf_data.s3_weight_per_item;

            ESP_LOGI(TAG, "Weight diff: %f" PRIi32, weight_diff);
            ESP_LOGI(TAG, "New qty: %f" PRIi32, floor(new_qty));

            shelf_data.s3_qty += floor(new_qty);
            shelf_data.total_weight = new_weight;
            
            Display::set_shelf_data(Display::ActiveShelf::SHELF_3, shelf_data.s3_mpn, shelf_data.s3_qty);
            Common::set_shelf_data(shelf_data);
            Common::set_shelf_data_updated(true);

            Display::set_active_shelf(Display::ActiveShelf::NONE);
        }
        else
        {
            
        }
    }
    //Check if any shelf is open, and wake up if any active

    //wait for shelf to close

    //compare weight with previous weight and find out how many items were taken/removed

    //update the display with the new data

    //put data in a place where it can be send to the server

}