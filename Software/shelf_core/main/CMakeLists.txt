set(srcs "main.cpp"
    "User/StateMachine.cpp"
    "User/Shelf_control/Shelf_controller.cpp"
    "User/Message_control/Message_controller.cpp"
    "User/Shelf_control/UI/ui.c"
    "User/Shelf_control/UI/ui_Screen1.c"
    "User/Shelf_control/UI/ui_Screen2.c"
    "User/Shelf_control/Display.cpp"
    "User/Shelf_control/HX711.cpp"
    "User/common.cpp")

idf_component_register(SRCS ${srcs} 
                    PRIV_REQUIRES spi_flash
                    INCLUDE_DIRS ""
                    REQUIRES nvs_flash esp_wifi espressif__qrcode coreMQTT coreMQTT-Agent driver
                    wifi_provisioning)
