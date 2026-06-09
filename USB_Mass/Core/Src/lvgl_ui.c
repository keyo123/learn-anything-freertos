/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : lvgl_ui.c
  * @brief          : LVGL UI application - GUI creation and event handlers
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "lvgl_ui.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lcd.h"
#include "lcd_init.h"
#include "key.h"
#include "json_app.h"
#include "app_file_service.h"
#include <string.h>
#include <stdio.h>

/* Private define ------------------------------------------------------------*/
#define LED_ENABLE 0
#define BAR_ENABLE 0
#define ARC_ENABLE 0

/* Private variables ---------------------------------------------------------*/
lv_obj_t *text_led;
lv_obj_t *bar;
lv_obj_t *arc;
char longFileName[256];
static char fileShortName[16];
static uint16_t bytesread;
static char rData[1024];
static int fileLen;
static FIL fnew;
static uint32_t key1_count = 0;
static uint32_t key2_count = 0;

/* Private function prototypes -----------------------------------------------*/
static FRESULT get_actual_filename(const char *short_name, char *actual_name, UINT max_len);
static void set_value(void *bar, int32_t v);
static void set_angle(void *obj, int32_t v);
static void bar_event_cb(lv_event_t *e);

/* Private user code ---------------------------------------------------------*/

FRESULT get_actual_filename(const char *short_name, char *actual_name, UINT max_len)
{
    FILINFO fno;
    FRESULT res = FS_Stat(short_name, &fno);
    if (res == FR_OK) {
        strncpy(actual_name, fno.fname, max_len - 1);
        actual_name[max_len - 1] = '\0';
    }
    return res;
}

static void set_value(void *bar, int32_t v)
{
    lv_bar_set_value(bar, v, LV_ANIM_ON);
}

static void set_angle(void *obj, int32_t v)
{
    lv_arc_set_value(obj, v);
}

/* 按键事件处理回调 */
static void key_event_handler1(lv_event_t *e)
{
    lv_event_code_t code;
    code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e); // 获取触发事件的对象

    if (code == LV_EVENT_CLICKED)
    {
        FRESULT res;
        bool skip_file = false;

        do {
            res = FS_ReadDir(&DirInfo, &FileInfo);

            // 检查是否已到达目录末尾
            if(res == FR_OK && FileInfo.fname[0] == '\0')
            {
                // 已经读取完所有文件，关闭并重新打开目录以从头开始
                FS_CloseDir(&DirInfo);
                FS_OpenDir((const TCHAR *)"0:", &DirInfo);

                // 再次读取第一个文件
                res = FS_ReadDir(&DirInfo, &FileInfo);

                // 如果还是空，说明目录确实为空
                if(res == FR_OK && FileInfo.fname[0] == '\0') {
                    break;
                }
            }

            // 检查是否需要跳过当前文件
            skip_file = false;

            // 跳过系统卷信息文件夹
            if(strcmp(FileInfo.fname, "SYSTEM~1") == 0) {
                skip_file = true;
            }

            // 跳过其他系统隐藏文件/文件夹
            if(FileInfo.fattrib & AM_HID) {  // 隐藏文件
                skip_file = true;
            }

            if(FileInfo.fattrib & AM_SYS) {  // 系统文件
                skip_file = true;
            }

        } while(skip_file && res == FR_OK && FileInfo.fname[0] != '\0');

        // 只有在成功读取且不跳过的情况下才处理文件
        if(res == FR_OK && !skip_file && FileInfo.fname[0] != '\0') {

            if (get_actual_filename(FileInfo.fname, longFileName, sizeof(longFileName)) == FR_OK) {

                fileLen = strlen(longFileName);
                strcpy(fileShortName, longFileName);

                // 检查是否为JSON文件
                const char *file_ext = strrchr(longFileName, '.');
                if (file_ext && (strcmp(file_ext, ".json") == 0 || strcmp(file_ext, ".JSON") == 0)) {
                    // 解析JSON文件
                    read_and_parse_json_file(longFileName);
                } else {
                    // 处理普通文本文件
                    memset(rData, 0, sizeof(rData));
                    if (FS_Open(longFileName, &fnew, FA_READ) == FR_OK)
                    {
                        FS_Read(&fnew, rData, sizeof(rData) - 1, &bytesread);
                        FS_Close(&fnew);
                        rData[bytesread] = '\0'; // 确保字符串结束
                    }
                }
            }
        }

        // 截取 rData 的前几位显示在标签上
        char displayBuffer[50];  // 创建显示缓冲区
        int charsToShow = 30;    // 显示前30个字符
        const char *file_ext1 = strrchr(longFileName, '.');
        if (file_ext1 && (strcmp(file_ext1, ".json") == 0 || strcmp(file_ext1, ".JSON") == 0)) {
            // 对于JSON文件，显示文件名
            strncpy(displayBuffer, longFileName, charsToShow);
        } else {
            // 对于普通文件，显示内容
            strncpy(displayBuffer, rData, charsToShow);
        }
        displayBuffer[charsToShow] = '\0';

        // 当按钮被点击时，改变按钮的标签文本
        lv_led_set_color(text_led, lv_palette_main(LV_PALETTE_RED));
        key1_count++;
        if (key1_count > 10)
            key1_count = 0;
        lv_obj_t *label = lv_obj_get_child(btn, 0); // 获取按钮的第一个子对象（即标签）
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(label, 110); // 设置略小于按钮宽度
        lv_label_set_text(label, displayBuffer);
    }
}

static void key_event_handler2(lv_event_t *e)
{
    lv_event_code_t code;
    code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e); // 获取触发事件的对象

    if (code == LV_EVENT_CLICKED)
    {
        // 当按钮被点击时，改变按钮的标签文本
        lv_led_set_color(text_led, lv_palette_main(LV_PALETTE_GREEN));
        key2_count++;
        lv_obj_t *label = lv_obj_get_child(btn, 0); // 获取按钮的第一个子对象（即标签）
        lv_label_set_text_fmt(label, "key2count: %u", key2_count);
    }
}

// bar事件回调
static void bar_event_cb(lv_event_t *e)
{
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_param(e);
    if (dsc->part != LV_PART_INDICATOR)
        return;

    lv_obj_t *obj = lv_event_get_target(e);

    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.font = LV_FONT_DEFAULT;

    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d", (int)lv_bar_get_value(obj));

    lv_point_t txt_size;
    lv_txt_get_size(&txt_size, buf, label_dsc.font, label_dsc.letter_space, label_dsc.line_space, LV_COORD_MAX, label_dsc.flag);

    lv_area_t txt_area;
    /*If the indicator is long enough put the text inside on the right*/
    if (lv_area_get_width(dsc->draw_area) > txt_size.x + 20)
    {
        txt_area.x2 = dsc->draw_area->x2 - 5;
        txt_area.x1 = txt_area.x2 - txt_size.x + 1;
        label_dsc.color = lv_color_white();
    }
    /*If the indicator is still short put the text out of it on the right*/
    else
    {
        txt_area.x1 = dsc->draw_area->x2 + 5;
        txt_area.x2 = txt_area.x1 + txt_size.x - 1;
        label_dsc.color = lv_color_black();
    }

    txt_area.y1 = dsc->draw_area->y1 + (lv_area_get_height(dsc->draw_area) - txt_size.y) / 2;
    txt_area.y2 = txt_area.y1 + txt_size.y - 1;

    lv_draw_label(dsc->draw_ctx, &label_dsc, &txt_area, buf, NULL);
}

void create_test_ui(void)
{
    // 获取当前屏幕
    lv_obj_t *scr = lv_scr_act();

    lv_color_t pink = lv_color_make(255, 128, 128);                     // 粉红色
    lv_color_t main_purple = lv_palette_main(LV_PALETTE_PURPLE);        // 获取调色板中的主紫色
    lv_color_t dark_purple = lv_palette_darken(LV_PALETTE_PURPLE, 2);   // 将紫色调深两级（0-4级）
    lv_color_t light_purple = lv_palette_lighten(LV_PALETTE_PURPLE, 2); // 将紫色调浅两级（0-4级）

    // led
#if LED_ENABLE
    text_led = lv_led_create(scr);
    lv_obj_set_pos(text_led, 120 - 40 / 2, 20);
    lv_obj_set_size(text_led, 40, 40);
    lv_led_set_color(text_led, lv_palette_main(LV_PALETTE_BLUE));
#endif

#if BAR_ENABLE
    // bar
    bar = lv_bar_create(scr);
    lv_obj_set_size(bar, 200, 20);
    lv_obj_center(bar);
    lv_obj_add_event_cb(bar, bar_event_cb, LV_EVENT_DRAW_MAIN_END, NULL);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, bar);
    lv_anim_set_values(&a, 0, 100);
    lv_anim_set_exec_cb(&a, set_value);
    lv_anim_set_time(&a, 2000);
    lv_anim_set_playback_time(&a, 2000);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);
#endif

#if ARC_ENABLE
    arc = lv_arc_create(lv_scr_act());
    lv_obj_set_size(arc, 200, 200);
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270);
    lv_arc_set_value(arc, 100);
    lv_obj_center(arc);
#endif

    // 创建一个测试按钮用于验证点击坐标映射
    lv_obj_t *test_btn1 = lv_btn_create(scr);
    lv_obj_set_pos(test_btn1, 100 - 100 / 2, 50);
    lv_obj_set_size(test_btn1, 140, 140);
    // 设置不同状态下的背景颜色
    lv_obj_set_style_bg_color(test_btn1, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_DEFAULT);  // 默认蓝色
    lv_obj_set_style_bg_color(test_btn1, pink, LV_STATE_PRESSED);                              // 按下红色
    lv_obj_set_style_bg_color(test_btn1, lv_palette_main(LV_PALETTE_GREY), LV_STATE_DISABLED); // 禁用灰色
    lv_obj_t *btn_label1 = lv_label_create(test_btn1);
    lv_label_set_text(btn_label1, "text btn");
    // 添加事件监听器
    lv_obj_add_event_cb(test_btn1, key_event_handler1, LV_EVENT_CLICKED, NULL);
}
