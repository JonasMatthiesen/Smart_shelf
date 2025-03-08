set(srcs "main.cpp"
    "User/StateMachine.cpp"
    "User/Shelf_control/Shelf_controller.cpp"
    "User/Message_control/Message_controller.cpp"
    "User/Shelf_control/UI/ui.c"
    "User/Shelf_control/UI/ui_Screen1.c"
    "User/Shelf_control/UI/ui_Screen2.c"
    "User/Shelf_control/Display.cpp")

idf_component_register(SRCS ${srcs} 
                    PRIV_REQUIRES spi_flash
                    INCLUDE_DIRS "")
