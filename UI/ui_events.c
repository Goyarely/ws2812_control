#include "ui_events.h"
#include "ui.h"
#include "ws2812.h"

// 全局变量
static RGBColor_TypeDef current_color = RED;    // 当前选中颜色
static uint8_t current_brightness = 100;        // 当前亮度(10-100%)
static uint8_t breathing_mode = 0;              // 呼吸模式(0-关闭,1-慢,2-快,3-脉冲)
static lv_anim_t brightness_anim;               // 亮度动画
static bool is_breathing = false;               // 呼吸状态标志

// 应用亮度到颜色
static RGBColor_TypeDef apply_brightness(RGBColor_TypeDef color, uint8_t brightness) {
    RGBColor_TypeDef result;
    float factor = brightness / 100.0f;
    
    result.R = (uint8_t)(color.R * factor);
    result.G = (uint8_t)(color.G * factor);
    result.B = (uint8_t)(color.B * factor);
    
    return result;
}

// 更新颜色显示
static void update_color_display(void) {
    RGBColor_TypeDef adjusted_color = apply_brightness(current_color, current_brightness);
    
    // 更新WS2812B灯条颜色
    RGB_SetAll(Pixel_S1_NUM, adjusted_color);
    rgb_SendArray();
    
    // 更新亮度显示标签
    lv_label_set_text_fmt(ui_Label1, "Brightness: %d%%", current_brightness);
}

// 颜色按钮点击事件
static void color_btn_event_handler(lv_obj_t *obj, lv_event_t event) {
    if (event == LV_EVENT_CLICKED) {
        // 根据点击的按钮设置当前颜色
        if (obj == ui_btnRed) current_color = RED;
        else if (obj == ui_btnOrange) current_color = ORANGE;
        else if (obj == ui_btnYellow) current_color = YELLOW;
        else if (obj == ui_btnGreen) current_color = GREEN;
        else if (obj == ui_btnCyan) current_color = SKY;
        else if (obj == ui_btnBlue) current_color = BLUE;
        else if (obj == ui_btnPurple2) current_color = PURPLE;
        
        // 更新显示
        update_color_display();
        
        // 如果正在呼吸，重启呼吸动画
        if (is_breathing) {
            lv_anim_del(&current_brightness, NULL);
            start_breathing_animation();
        }
    }
}

// 滑块事件处理
static void slider_event_handler(lv_obj_t *obj, lv_event_t event) {
    if (event == LV_EVENT_VALUE_CHANGED) {
        // 停止呼吸动画
        if (is_breathing) {
            lv_anim_del(&current_brightness, NULL);
            is_breathing = false;
        }
        
        // 获取滑块值(0-100)，限制最小10%
        current_brightness = lv_slider_get_value(ui_Slider1);
        if (current_brightness < 10) current_brightness = 10;
        
        // 更新显示
        update_color_display();
    }
}

// 呼吸动画回调函数
static void breathing_anim_cb(void *var, int32_t value) {
    current_brightness = value;
    update_color_display();
    lv_slider_set_value(ui_Slider1, current_brightness, LV_ANIM_OFF);
}

// 呼吸动画结束回调
static void breathing_anim_end_cb(lv_anim_t *a) {
    // 反转动画方向，实现呼吸效果
    lv_anim_t reverse_anim;
    lv_anim_copy(&reverse_anim, a);
    
    if (reverse_anim.start == 10) {
        reverse_anim.start = 100;
        reverse_anim.end = 10;
    } else {
        reverse_anim.start = 10;
        reverse_anim.end = 100;
    }
    
    lv_anim_start(&reverse_anim);
}

// 开始呼吸动画
static void start_breathing_animation(void) {
    // 配置动画
    lv_anim_init(&brightness_anim);
    lv_anim_set_var(&brightness_anim, &current_brightness);
    lv_anim_set_exec_cb(&brightness_anim, breathing_anim_cb);
    lv_anim_set_end_cb(&brightness_anim, breathing_anim_end_cb);
    lv_anim_set_values(&brightness_anim, 10, 100);
    
    // 根据模式设置周期
    switch (breathing_mode) {
        case 1: lv_anim_set_time(&brightness_anim, 3000); break;  // 慢模式：3秒
        case 2: lv_anim_set_time(&brightness_anim, 1000); break;  // 快模式：1秒
        case 3: lv_anim_set_time(&brightness_anim, 500);  break;  // 脉冲模式：0.5秒
        default: return;
    }
    
    // 启动动画
    lv_anim_start(&brightness_anim);
    is_breathing = true;
}

// 下拉框事件处理
static void dropdown_event_handler(lv_obj_t *obj, lv_event_t event) {
    if (event == LV_EVENT_VALUE_CHANGED) {
        // 获取选中的模式
        breathing_mode = lv_dropdown_get_selected(ui_Dropdown1);
        
        // 停止当前动画
        if (is_breathing) {
            lv_anim_del(&current_brightness, NULL);
            is_breathing = false;
        }
        
        // 根据选择的模式执行相应操作
        if (breathing_mode == 0) {
            // 关闭呼吸模式，恢复当前亮度
            update_color_display();
        } else {
            // 启动对应模式的呼吸动画
            start_breathing_animation();
        }
    }
}

// 初始化UI事件
void ui_events_init(void) {
    // 初始化颜色按钮事件
    lv_obj_add_event_cb(ui_btnRed, color_btn_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_btnOrange, color_btn_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_btnYellow, color_btn_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_btnGreen, color_btn_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_btnCyan, color_btn_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_btnBlue, color_btn_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_btnPurple2, color_btn_event_handler, LV_EVENT_CLICKED, NULL);
    
    // 初始化滑块事件
    lv_obj_add_event_cb(ui_Slider1, slider_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    lv_slider_set_range(ui_Slider1, 10, 100);  // 设置滑块范围10-100%
    lv_slider_set_value(ui_Slider1, 100, LV_ANIM_OFF);  // 初始值100%
    
    // 初始化下拉框事件
    lv_obj_add_event_cb(ui_Dropdown1, dropdown_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    lv_dropdown_set_options(ui_Dropdown1, "Off\nSlow\nFast\nPulse");  // 设置模式选项
    lv_dropdown_set_selected(ui_Dropdown1, 0);  // 默认选择关闭
    
    // 初始化显示
    update_color_display();
}