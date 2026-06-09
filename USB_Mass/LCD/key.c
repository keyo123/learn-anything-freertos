#include "key.h"
#include "main.h"

#define DEBOUNCE_DELAY 20 // 消抖时间，单位ms


// 初始化按键状态
key_state_t key1_state = {0, 0, 0, 0};


uint8_t get_key_debounced(uint8_t raw_input, key_state_t *key, uint32_t current_time) {
    // 更新状态历史
    key->previous_state = key->current_state;
    key->current_state = raw_input;
    
    // 检查状态是否变化
    if(key->current_state != key->debounce_state) {
        // 状态变化，记录时间
        key->last_time = current_time;
        key->debounce_state = key->current_state;
    }
    
    // 检查是否已过消抖时间
    if((current_time - key->last_time) > DEBOUNCE_DELAY) {
        // 消抖时间已过，返回稳定状态
        return key->current_state;
    }
    
    // 仍在消抖期内，返回之前的状态
    return key->previous_state;
}

uint8_t read_raw_keys(void)
{
	if(!SELECT)
	return 0b01;
	else if(!OK)
	return 0b10;
	else
	return 0;
}

