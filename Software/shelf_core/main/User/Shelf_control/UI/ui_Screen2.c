// This file was generated by SquareLine Studio
// SquareLine Studio version: SquareLine Studio 1.5.1
// LVGL version: 8.3.11
// Project name: SquareLine_Project

#include "ui.h"

void main_screen_init()
{
    ui_Screen2 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen2, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_pos(ui_Screen2, 0, 0);   /// Position
    lv_obj_set_size(ui_Screen2, 240, 240);   /// Size
    lv_obj_set_style_radius(ui_Screen2 , LV_RADIUS_CIRCLE, 0);

    ui_TextArea1 = lv_textarea_create(ui_Screen2);
    lv_obj_set_width(ui_TextArea1, 245);
    lv_obj_set_height(ui_TextArea1, 80);
    lv_obj_set_x(ui_TextArea1, 0);
    lv_obj_set_y(ui_TextArea1, -80);
    lv_obj_set_align(ui_TextArea1, LV_ALIGN_CENTER);
    lv_textarea_set_text(ui_TextArea1, "			                      Shelf 1:\n			         MPN:\n			        QTY:");
    lv_textarea_set_placeholder_text(ui_TextArea1, "Placeholder...");
    lv_obj_set_style_bg_color(ui_TextArea1, lv_color_make(0xd3, 0xd3, 0xd3), 0);

    ui_TextArea2 = lv_textarea_create(ui_Screen2);
    lv_obj_set_width(ui_TextArea2, 245);
    lv_obj_set_height(ui_TextArea2, 80);
    lv_obj_set_x(ui_TextArea2, 0);
    lv_obj_set_y(ui_TextArea2, 0);
    lv_obj_set_align(ui_TextArea2, LV_ALIGN_CENTER);
    lv_textarea_set_text(ui_TextArea2, "			                      Shelf 2:\n			         MPN:\n			         QTY:");
    lv_textarea_set_placeholder_text(ui_TextArea2, "Placeholder...");
    lv_obj_set_style_bg_color(ui_TextArea2, lv_color_make(0xd3, 0xd3, 0xd3), 0);

    ui_TextArea3 = lv_textarea_create(ui_Screen2);
    lv_obj_set_width(ui_TextArea3, 245);
    lv_obj_set_height(ui_TextArea3, 80);
    lv_obj_set_x(ui_TextArea3, 0);
    lv_obj_set_y(ui_TextArea3, 80);
    lv_obj_set_align(ui_TextArea3, LV_ALIGN_CENTER);
    lv_textarea_set_text(ui_TextArea3, "			                      Shelf 3:\n			         MPN:\n			         QTY:");
    lv_textarea_set_placeholder_text(ui_TextArea3, "Placeholder...");
    lv_obj_set_style_bg_color(ui_TextArea3, lv_color_make(0xd3, 0xd3, 0xd3), 0);

    //lv_obj_set_style_bg_color(ui_TextArea2, lv_color_make(0x3f, 0x3f, 0x3f), 0);
    //lv_obj_set_style_text_color(ui_TextArea2, lv_color_make(0xff, 0xff, 0xff), 0);
}
