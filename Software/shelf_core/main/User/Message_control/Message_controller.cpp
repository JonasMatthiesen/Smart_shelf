#include <stdio.h>
#include <inttypes.h>
#include <iostream>
#include <sstream>
#include "sdkconfig.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "stdlib.h"
#include "string.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Message_controller.h"
#include "clock.h"
#include "freertos/semphr.h"
#include "core_mqtt.h"
#include "network_transport.h"
#include "freertos/queue.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_softap.h"
#include "json.hpp"
#include "../common.h"

#define WIFI_CONNECT_RETRIES (3)

const uint16_t MESSAGE_LOOP_TIME_MS = 1000;

static const char* TAG = "MESSAGE_CONTROLLER";
uint8_t wifi_connect_retries = 0;
bool re_provision = false;
bool ses_present = 0;
bool connect_message = false;
TaskHandle_t mqtt_task_handle; //mqtt task
 const char* topic_pub = "device/smart_shelf_3/pub";
 const char* topic_sub = "device/smart_shelf_3/sub";
 const char* thing_name = "smart_shelf_1_windows";
 const char* user_name = "user";
const char* endpoint = "a2hfupbrus69r5-ats.iot.eu-west-1.amazonaws.com";
uint8_t buffer[1024];
NetworkContext_t network_context;
TransportInterface_t transport;
MQTTConnectInfo_t connect_info;
MQTTFixedBuffer_t network_buffer; 
MQTTPublishInfo_t publish_info;
MQTTContext_t mqtt_context;
QueueHandle_t mqtt_aws_queue;
uint8_t connect_count = 0;
bool mqtt_task_started = false;
using json = nlohmann::json;

extern  "C"
{
 extern const char root_cert_auth_start[] asm(
        "_binary_root_cert_auth_crt_start");
    extern const char root_cert_auth_end[] asm(
        "_binary_root_cert_auth_crt_end");
    extern const char client_cert_start[] asm(
        "_binary_client_crt_start");
    extern const char client_cert_end[] asm(
        "_binary_client_crt_end");
    extern const char client_key_start[] asm(
        "_binary_client_key_start");
    extern const char client_key_end[] asm(
        "_binary_client_key_end");
}
/*******************************************************************/
MQTTStatus_t mqtt_subscribe_to(MQTTContext_t* mqtt_context,const char* topic,MQTTQoS_t qos_level)
{
    uint16_t pkt = MQTT_GetPacketId(mqtt_context);
    MQTTSubscribeInfo_t subscribe_topic ;
    subscribe_topic.qos = qos_level;
    subscribe_topic.pTopicFilter = topic;
    subscribe_topic.topicFilterLength = strlen(topic);
    return MQTT_Subscribe(mqtt_context,&subscribe_topic,1,pkt);
}
static void wifi_prov_handler(void *user_data, wifi_prov_cb_event_t event_base, void *event_data)
{
    switch(event_base)
    {
        case WIFI_PROV_START :
            ESP_LOGI(TAG, "--HANDLER ----Starting provisioning");
            // print_qr_code();
             break;
        case WIFI_PROV_CRED_RECV :
            ESP_LOGI(TAG, "Received credentials : \n SSID : %s \n Password : %s",((wifi_sta_config_t*)event_data)->ssid,((wifi_sta_config_t*)event_data)->password);
            break;
        case WIFI_PROV_CRED_SUCCESS :
            ESP_LOGI(TAG,"Credentials accepted");
            wifi_prov_mgr_stop_provisioning();
            break;
        case WIFI_PROV_CRED_FAIL :
            ESP_LOGE(TAG,"Credentials rejected, press RST and try again");
            wifi_prov_mgr_reset_sm_state_on_failure();
            break;
        case WIFI_PROV_END :
            ESP_LOGI(TAG,"Provisioning ended, deinitializing manager");
            wifi_prov_mgr_deinit();
            break;
        default : break;
    }
}
static void softap_prov_init(void)
{
    ESP_LOGI(TAG, "Initializing provisioning");
    wifi_prov_mgr_config_t mgr= 
    {
        .scheme = wifi_prov_scheme_softap,
        .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE,
        .app_event_handler = wifi_prov_handler,//WIFI_PROV_EVENT_HANDLER_NONE,
    };
    wifi_prov_mgr_init(mgr);
    bool is_prov = false;

    wifi_prov_mgr_is_provisioned(&is_prov);
    if(is_prov)
    {
        ESP_LOGI(TAG, "Already provisioned");
        wifi_prov_mgr_deinit();
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_start();
        Common::set_wifi_provisioned(true);  
    }
    else
    {
        ESP_LOGI(TAG,"Starting provisioning");
        esp_netif_create_default_wifi_ap();
        wifi_prov_mgr_disable_auto_stop(1000);
        wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_0,NULL,"PROV_ESP","password");
    }
}

void mqtt_decode_recieved(const char*  payload)
{
    ESP_LOGI(TAG,"received: %s", payload);
    json mqtt_package = json::parse(payload);

    ShelfData sdata;
    sdata.calib_scalar = mqtt_package.value("calibrate_scalar", 0.0);
    sdata.calib_offset = mqtt_package.value("calibrate_offset", 0);

    std::string shelf_name_str = mqtt_package.value("smart_shelf_1_name", "");
    sdata.s1_mpn = strdup(shelf_name_str.c_str());
    sdata.s1_qty = mqtt_package.value("items_1", 0);
    sdata.s1_weight_per_item = mqtt_package.value("weight_item_1", 0.0);
    sdata.s1_qty_limit = mqtt_package.value("limit_1", 0);

    shelf_name_str = mqtt_package.value("smart_shelf_2_name", "");
    sdata.s2_mpn = strdup(shelf_name_str.c_str());
    sdata.s2_qty = mqtt_package.value("items_2", 0);
    sdata.s2_weight_per_item = mqtt_package.value("weight_item_2", 0.0);
    sdata.s2_qty_limit = mqtt_package.value("limit_2", 0);

    shelf_name_str = mqtt_package.value("smart_shelf_3_name", "");
    sdata.s3_mpn = strdup(shelf_name_str.c_str());
    sdata.s3_qty = mqtt_package.value("items_3", 0);
    sdata.s3_weight_per_item = mqtt_package.value("weight_item_3", 0.0);
    sdata.s3_qty_limit = mqtt_package.value("limit_3", 0);

    re_provision = mqtt_package.value("wifi", 0);
    sdata.total_weight = 1000;

    Common::set_shelf_data(sdata);
    if (!Common::get_shelf_data_initialized())
    {
        Common::set_shelf_data_initialized(true);
    }
}

static void mqtt_event_cb(MQTTContext_t* pMQTTContext,MQTTPacketInfo_t* pPacketInfo, MQTTDeserializedInfo_t* pDeserializedInfo)
{
    ESP_LOGI(TAG,"MQTT CALL BACK EVENT: Packet info: %d", pPacketInfo->type);

   switch(pPacketInfo->type)
   {
    case MQTT_PACKET_TYPE_PUBLISH :  
        ESP_LOGI(TAG,"MQTT_PACKET_TYPE_PUBLISH");
        mqtt_decode_recieved((const char*) pDeserializedInfo->pPublishInfo->pPayload);
        break;
    case MQTT_PACKET_TYPE_SUBSCRIBE :
        ESP_LOGI(TAG,"MQTT_PACKET_TYPE_SUBSCRIBE");
        break;
    case MQTT_PACKET_TYPE_CONNECT  :
        ESP_LOGI(TAG,"MQTT_PACKET_TYPE_CONNECT");
        break;  
    case MQTT_PACKET_TYPE_CONNACK :
        ESP_LOGI(TAG,"MQTT_PACKET_TYPE_CONNACK");
        break;
    case MQTT_PACKET_TYPE_SUBACK:
        ESP_LOGI(TAG,"MQTT_PACKET_TYPE_SUBACK: %d", connect_count);
        ESP_LOGI(TAG,"mqtt_process_task started");
        mqtt_task_started = true;
        break;
    default:
        break;
   }

}
void mqtt_process_task()
{
    static bool started = false;
    if (!started)
    {
        connect_message = true;        
        ESP_LOGI(TAG,"MQTT Connected... subscribing to topics");
        //mqtt_subscribe_to(&mqtt_context,topic_pub,MQTTQoS0);
        mqtt_subscribe_to(&mqtt_context,topic_sub,MQTTQoS0);
        started = true;
    }

    MQTT_ProcessLoop(&mqtt_context);

    if (mqtt_task_started)
    {
        std::stringstream ss_mqtt;
        json message_json;
        if (connect_message)
        {
            ESP_LOGI(TAG,"Sending Connect Message");
            message_json = {
                {"smart_shelf_id", "smart_shelf_3"},
                {"startup", true}
            };
            std::string mqtt_string = message_json.dump();
            publish_info.pPayload = mqtt_string.c_str();
            publish_info.payloadLength = mqtt_string.length();
            uint16_t packet_id = MQTT_GetPacketId(&mqtt_context);
            MQTT_Publish(&mqtt_context,&publish_info,packet_id);
            connect_message = false;
        }
        else if (Common::get_shelf_data_updated())
        {
            ESP_LOGI(TAG,"Sending shelf data Message");
            ShelfData sdata = Common::get_shelf_data();
            message_json = {
                {"smart_shelf_id", "smart_shelf_3"},
                {"calibrate_offset", sdata.calib_offset},
                {"calibrate_scalar", sdata.calib_scalar},
                {"shelves", {
                    {
                        {"shelf_id", 1},
                        {"name", sdata.s1_mpn},
                        {"weight", sdata.s1_qty*sdata.s1_weight_per_item},
                        {"weight_of_one_item", sdata.s1_weight_per_item},
                        {"items", sdata.s1_qty},
                        {"limit", sdata.s1_qty_limit}
                    },
                    {
                        {"shelf_id", 2},
                        {"name", sdata.s2_mpn},
                        {"weight", sdata.s2_qty*sdata.s2_weight_per_item},
                        {"weight_of_one_item", sdata.s2_weight_per_item},
                        {"items", sdata.s2_qty},
                        {"limit", sdata.s2_qty_limit}
                    },
                    {
                        {"shelf_id", 3},
                        {"name", sdata.s3_mpn},
                        {"weight", sdata.s3_qty*sdata.s3_weight_per_item},
                        {"weight_of_one_item", sdata.s3_weight_per_item},
                        {"items", sdata.s3_qty},
                        {"limit", sdata.s3_qty_limit}
                    }
                }}
            };
            std::string mqtt_string = message_json.dump();
            publish_info.pPayload = mqtt_string.c_str();
            publish_info.payloadLength = mqtt_string.length();
            uint16_t packet_id = MQTT_GetPacketId(&mqtt_context);
            MQTT_Publish(&mqtt_context,&publish_info,packet_id);
            connect_message = false;
            Common::set_shelf_data_updated(false);

        }
        if (re_provision)
        {
            ESP_LOGI(TAG,"Starting provisioning");
            esp_netif_create_default_wifi_ap();
            wifi_prov_mgr_disable_auto_stop(1000);
            wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_0,NULL,"PROV_ESP","password");
            re_provision = false;
        }
    }
}
void set_wifi_status(bool connected)
{
    TlsTransportStatus_t xTlsRet = xTlsConnect(&network_context);


    if(xTlsRet == TLS_TRANSPORT_SUCCESS)
    {
        ESP_LOGI(TAG,"tls err : %d connection successful", xTlsRet);
    }
    else
    {
        ESP_LOGI(TAG,"tls err : %d connection failed", xTlsRet);
    }

    if (MQTT_Connect(&mqtt_context,&connect_info,NULL,2000,&ses_present) == MQTTSuccess)
    {
        ESP_LOGI(TAG,"MQTT Connect succes");
        Common::set_connected(true);
    }
    //xTaskCreate(mqtt_process_task,"mqtt_process_task", 2048 * 2,NULL,5,&mqtt_task_handle);
}
static void event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
     {
        esp_wifi_connect();
     } 
     else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
     {
         ESP_LOGI(TAG,"STA CONNECTED");
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        Common::set_wifi_connected(true);
        set_wifi_status(true);
    }
    else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGE(TAG,"STA DISCONNECTED");
        wifi_prov_mgr_reset_provisioning();
        //set_wifi_status(false);
    }
}
void wifi_init(void)
{
     esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
 

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                    ESP_EVENT_ANY_ID,
                                                    &event_handler,
                                                    NULL,
                                                    &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                    IP_EVENT_STA_GOT_IP,
                                                    &event_handler,
                                                    NULL,
                                                    &instance_got_ip));
    esp_netif_create_default_wifi_sta();
   wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
//     wifi_config_t wfcfg;
//     memset(&wfcfg, 0, sizeof(wfcfg));
//     strcpy(reinterpret_cast<char*>(wfcfg.sta.ssid), CONFIG_SSID);
//     strcpy(reinterpret_cast<char*>(wfcfg.sta.password),CONFIG_PASSWORD);
//     wfcfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

//    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
//    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wfcfg) );
//   ESP_ERROR_CHECK(esp_wifi_start());
}

void aws_init(void)
{
    network_context.pcHostname = endpoint;
    network_context.xPort = 8883;
    network_context.pxTls = NULL;
    network_context.xTlsContextSemaphore = xSemaphoreCreateMutex();
    network_context.disableSni = false;
    network_context.pcServerRootCA = root_cert_auth_start;
    network_context.pcServerRootCASize = root_cert_auth_end - root_cert_auth_start;
    network_context.pcClientCert = client_cert_start;
    network_context.pcClientCertSize = client_cert_end - client_cert_start;
    network_context.pcClientKey = client_key_start;
    network_context.pcClientKeySize = client_key_end - client_key_start;
    network_context.pAlpnProtos = NULL;
    transport.pNetworkContext = &network_context;
    transport.recv = espTlsTransportRecv;
    transport.send = espTlsTransportSend;
    transport.writev = NULL;
    network_buffer.pBuffer = buffer;
    network_buffer.size = sizeof(buffer);
    connect_info.cleanSession = true;
    connect_info.pClientIdentifier = thing_name;
    connect_info.clientIdentifierLength = ( uint16_t ) strlen(thing_name);
    connect_info.keepAliveSeconds = 60;
    connect_info.pUserName = user_name;
    connect_info.userNameLength = ( uint16_t ) strlen(user_name);
    publish_info.qos = MQTTQoS0;
    publish_info.pTopicName = topic_pub;
    publish_info.topicNameLength = strlen(topic_pub);
    MQTT_Init(&mqtt_context,&transport,Clock_GetTimeMs,&mqtt_event_cb,&network_buffer);
}

void message_start()
{
    ESP_LOGI(TAG,"-----------------------MESSAGE INIT ------------------"); 
    esp_log_level_set(TAG,ESP_LOG_VERBOSE);
   // mqtt_aws_queue = xQueueCreate(10,sizeof(uint32_t));
    wifi_init();
    softap_prov_init(); 
    aws_init(); 
   // set_wifi_status(1);
  //  xTaskCreate(user_main_task,"user_main_task",2048,NULL,5,NULL);;
}

void message_loop()
{
    mqtt_process_task();
}

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
        if (Common::get_connected())
        {
            static bool first = false;
            if(!first)
            {
                ESP_LOGI(TAG,"-----------------------MESSAGE LOOP STARTED ------------------"); 
                first = true;
            }
           
            message_loop();
        }
       
        vTaskDelay(pdMS_TO_TICKS(MESSAGE_LOOP_TIME_MS));
    }
}