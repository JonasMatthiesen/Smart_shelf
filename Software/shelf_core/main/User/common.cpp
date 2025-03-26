#include "common.h"
#include "esp_log.h"

ShelfData data;
bool server_connected = false;
bool wifi_connected = false;
bool wifi_provisioned = false;
bool init_complete = false;
bool shelf_data_initialized = false;
bool shelf_data_updated = false;

void Common::set_shelf_data(ShelfData _data)
{
    data = _data;
}

ShelfData Common::get_shelf_data()
{
/*     float weight_scaler = 0.10025;
    int32_t weight_offset = -3649;
    data.calib_scalar = weight_scaler;
    data.calib_offset = weight_offset;

    data.s1_mpn = "Screw big";
    data.s1_qty = 0;
    data.s1_weight_per_item = 26;
    data.s1_qty_limit = 100;

    data.s2_mpn = "Screw small";
    data.s2_qty = 0;
    data.s2_weight_per_item = 13;
    data.s2_qty_limit = 100;

    data.s3_mpn = "Bolt";
    data.s3_qty = 120;
    data.s3_weight_per_item = 20;
    data.s3_qty_limit = 100;

    data.total_weight = 0; */
    return data;
}

void Common::set_connected(bool connected)
{
    //Must only be set by the message controller task
    // TaskHandle_t xTaskGetCurrentTaskHandle();
    // if (xTaskGetCurrentTaskHandle() == xTaskGetHandle("Message Task"))
    // {
    //     connected = connected;
    // }
    // else
    // {
    //     ESP_LOGE("Common", "Connected can only be set by the message controller task");
    // }

    server_connected = connected;
}

bool Common::get_connected()
{
    return server_connected;
}

void Common::set_init_complete(bool complete)
{
    //Must only be set by the shelf controller task
    // TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    // if (current_task == xTaskGetHandle("Shelf Task"))
    // {
    //     init_complete = complete;
    // }

    init_complete = complete;
}

bool Common::get_init_complete()
{
    return init_complete;
}

void Common::set_shelf_data_initialized(bool data_initialized)
{
    //Must only be set by the message controller task
    // TaskHandle_t xTaskGetCurrentTaskHandle();
    // if (xTaskGetCurrentTaskHandle() == xTaskGetHandle("Message Task"))
    // {
    //     shelf_data_initialized = data_initialized;
    // }

    shelf_data_initialized = data_initialized;
}

bool Common::get_shelf_data_initialized()
{
    return shelf_data_initialized;
}

void Common::set_shelf_data_updated(bool data_updated)
{
    //Must only be set by the shelf controller task
    // TaskHandle_t xTaskGetCurrentTaskHandle();
    // if (xTaskGetCurrentTaskHandle() == xTaskGetHandle("Shelf Task"))
    // {
    //     shelf_data_updated = data_updated;
    // }

    shelf_data_updated = data_updated;
}

bool Common::get_shelf_data_updated()
{
    return shelf_data_updated;
}

bool Common::get_wifi_connected()
{
    return wifi_connected;
}

void Common::set_wifi_connected(bool connected)
{
    //Must only be set by the shelf controller task
    // TaskHandle_t xTaskGetCurrentTaskHandle();
    // if (xTaskGetCurrentTaskHandle() == xTaskGetHandle("Shelf Task"))
    // {
    //     shelf_data_updated = data_updated;
    // }

    wifi_connected = connected;
}

bool Common::get_wifi_provisioned()
{
    return wifi_provisioned;
}

void Common::set_wifi_provisioned(bool connected)
{
    //Must only be set by the shelf controller task
    // TaskHandle_t xTaskGetCurrentTaskHandle();
    // if (xTaskGetCurrentTaskHandle() == xTaskGetHandle("Shelf Task"))
    // {
    //     shelf_data_updated = data_updated;
    // }

    wifi_provisioned = connected;
}

