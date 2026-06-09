/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : json_app.c
  * @brief          : JSON parser application - file reading and test functions
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "json_app.h"
#include "json_parser.h"
#include "fatfs.h"
#include "usart.h"
#include "usbd_cdc_if.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Private variables ---------------------------------------------------------*/
static uint8_t json_pool[4096]; // json_parser arena 内存池
static char uart_buffer[256];  // 调试输出缓冲区

/* Private user code ---------------------------------------------------------*/

/**
 * @brief 从文件系统读取JSON文件并解析
 * @param filename 文件名
 * @return 0: 成功, 其他: 失败
 */
int read_and_parse_json_file(const char* filename) {
    FIL json_file;
    FRESULT fr;
    UINT bytes_read;
    char *file_buffer = NULL;
    DWORD file_size;

    // 1. 打开文件
    fr = f_open(&json_file, filename, FA_READ);
    if (fr != FR_OK) {
        snprintf(uart_buffer, sizeof(uart_buffer), "Failed to open file: %s, Error: %d\r\n", filename, fr);
        HAL_UART_Transmit(&huart4, (uint8_t*)uart_buffer, strlen(uart_buffer), 1000);
        return -1;
    }

    // 2. 获取文件大小
    file_size = f_size(&json_file);
    if (file_size == 0) {
        snprintf(uart_buffer, sizeof(uart_buffer), "File is empty: %s\r\n", filename);
        HAL_UART_Transmit(&huart4, (uint8_t*)uart_buffer, strlen(uart_buffer), 1000);
        f_close(&json_file);
        return -2;
    }

    // 3. 分配内存存储文件内容（+1是为了添加字符串结束符）
    file_buffer = (char*)malloc(file_size + 1);
    if (file_buffer == NULL) {
        snprintf(uart_buffer, sizeof(uart_buffer), "Memory allocation failed for file: %s\r\n", filename);
        HAL_UART_Transmit(&huart4, (uint8_t*)uart_buffer, strlen(uart_buffer), 1000);
        f_close(&json_file);
        return -3;
    }

    // 4. 读取文件内容
    fr = f_read(&json_file, file_buffer, file_size, &bytes_read);
    if (fr != FR_OK) {
        snprintf(uart_buffer, sizeof(uart_buffer), "Failed to read file: %s, Error: %d\r\n", filename, fr);
        HAL_UART_Transmit(&huart4, (uint8_t*)uart_buffer, strlen(uart_buffer), 1000);
        free(file_buffer);
        f_close(&json_file);
        return -4;
    }

    // 5. 关闭文件
    f_close(&json_file);

    // 6. 确保字符串以'\0'结尾
    file_buffer[bytes_read] = '\0';

    // 7. 解析JSON（json_parser 会原地修改 file_buffer）
    json_value_t *root = json_parse(file_buffer, json_pool, sizeof(json_pool), NULL);
    if (root == NULL) {
        snprintf(uart_buffer, sizeof(uart_buffer), "JSON parse error in file %s: %s\r\n", filename, json_get_error());
        HAL_UART_Transmit(&huart4, (uint8_t*)uart_buffer, strlen(uart_buffer), 1000);
        free(file_buffer);
        return -5;
    }

    // 8. 处理解析后的JSON数据
    snprintf(uart_buffer, sizeof(uart_buffer), "Successfully parsed JSON file: %s\r\n", filename);
    HAL_UART_Transmit(&huart4, (uint8_t*)uart_buffer, strlen(uart_buffer), 1000);

    // 示例：提取JSON数据
    json_value_t *name = json_object_get(root, "name");
    json_value_t *id = json_object_get(root, "id");
    json_value_t *config = json_object_get(root, "config");

    if (json_get_type(name) == JSON_STRING) {
        const char *val = json_get_string(name, NULL);
        snprintf(uart_buffer, sizeof(uart_buffer), "Name: %s\r\n", val);
        HAL_UART_Transmit(&huart4, (uint8_t*)uart_buffer, strlen(uart_buffer), 1000);
    }

    if (json_get_type(id) == JSON_NUMBER) {
        snprintf(uart_buffer, sizeof(uart_buffer), "ID: %d\r\n", (int)json_get_number(id));
        HAL_UART_Transmit(&huart4, (uint8_t*)uart_buffer, strlen(uart_buffer), 1000);
    }

    if (json_get_type(config) == JSON_OBJECT) {
        json_value_t *baudrate = json_object_get(config, "baudrate");
        json_value_t *enabled = json_object_get(config, "enabled");

        if (json_get_type(baudrate) == JSON_NUMBER) {
            snprintf(uart_buffer, sizeof(uart_buffer), "Baudrate: %d\r\n", (int)json_get_number(baudrate));
            HAL_UART_Transmit(&huart4, (uint8_t*)uart_buffer, strlen(uart_buffer), 1000);
        }

        if (json_get_type(enabled) == JSON_BOOL) {
            snprintf(uart_buffer, sizeof(uart_buffer), "Enabled: %s\r\n", json_get_bool(enabled) ? "Yes" : "No");
            HAL_UART_Transmit(&huart4, (uint8_t*)uart_buffer, strlen(uart_buffer), 1000);
        }
    }

    // 9. 清理资源（json_parser 使用 arena 池，无需释放解析树）
    free(file_buffer);

    return 0; // 成功
}

/**
 * @brief 简易测试：验证 json_parser 能否正常工作
 *         使用硬编码 JSON 字符串，不依赖文件系统
 */
void test_json_parser(void)
{
    // 注意：json_parser 会原地修改输入缓冲，所以不能用 const 字符串
    char json_str[] = "{"
        "\"name\": \"STM32 Device\","
        "\"id\": 12345,"
        "\"pi\": 3.14159,"
        "\"enabled\": true,"
        "\"null_val\": null,"
        "\"config\": {"
        "    \"baudrate\": 115200,"
        "    \"parity\": \"none\","
        "    \"enabled\": false"
        "},"
        "\"tags\": [\"embedded\", \"stm32\", \"json\"]"
        "}";

    static uint8_t test_pool[2048];

    // ========== CDC 用法示例 ==========
    // 等待 PC 打开串口（DTR 置位），超时 5 秒
    uint32_t timeout = HAL_GetTick() + 5000;
    uint8_t ready = CDC_IsReady();
    while (!ready) {
        if (HAL_GetTick() >= timeout) {
            HAL_UART_Transmit(&huart4, (uint8_t*)"CDC timeout\r\n", 13, 1000);
            return;
        }
        ready = CDC_IsReady();
    }
    HAL_Delay(100); // 等 PC 串口完全就绪
#define TX(...)  CDC_Transmit_FS((uint8_t*)uart_buffer, strlen(uart_buffer))
    //========================================================

    // 当前使用 UART4 输出
//#define TX(...)  HAL_UART_Transmit(&huart4, (uint8_t*)uart_buffer, strlen(uart_buffer), 1000)

    snprintf(uart_buffer, sizeof(uart_buffer), "\r\n=== json_parser test ===\r\n");
    TX();

    json_value_t *root = json_parse(json_str, test_pool, sizeof(test_pool), NULL);
    if (root == NULL) {
        snprintf(uart_buffer, sizeof(uart_buffer), "Parse FAILED: %s\r\n", json_get_error());
        TX();
        return;
    }

    snprintf(uart_buffer, sizeof(uart_buffer), "Parse OK\r\n");
    TX();

    // 测试 string
    json_value_t *v = json_object_get(root, "name");
    if (json_get_type(v) == JSON_STRING) {
        snprintf(uart_buffer, sizeof(uart_buffer), "name = \"%s\"\r\n", json_get_string(v, NULL));
        TX();
    }

    // 测试 integer (JSON_NUMBER)
    v = json_object_get(root, "id");
    if (json_get_type(v) == JSON_NUMBER) {
        snprintf(uart_buffer, sizeof(uart_buffer), "id = %d (int)\r\n", (int)json_get_number(v));
        TX();
    }

    // 测试 float
    v = json_object_get(root, "pi");
    if (json_get_type(v) == JSON_NUMBER) {
        snprintf(uart_buffer, sizeof(uart_buffer), "pi = %.5f\r\n", json_get_number(v));
        TX();
    }

    // 测试 bool (true)
    v = json_object_get(root, "enabled");
    if (json_get_type(v) == JSON_BOOL) {
        snprintf(uart_buffer, sizeof(uart_buffer), "enabled = %s\r\n", json_get_bool(v) ? "true" : "false");
        TX();
    }

    // 测试 null
    v = json_object_get(root, "null_val");
    if (json_get_type(v) == JSON_NULL) {
        snprintf(uart_buffer, sizeof(uart_buffer), "null_val = null\r\n");
        TX();
    }

    // 测试嵌套对象
    v = json_object_get(root, "config");
    if (json_get_type(v) == JSON_OBJECT) {
        snprintf(uart_buffer, sizeof(uart_buffer), "config (object, %d keys):\r\n", (int)json_object_size(v));
        TX();

        json_value_t *bv = json_object_get(v, "baudrate");
        if (json_get_type(bv) == JSON_NUMBER) {
            snprintf(uart_buffer, sizeof(uart_buffer), "  baudrate = %d\r\n", (int)json_get_number(bv));
            TX();
        }

        bv = json_object_get(v, "parity");
        if (json_get_type(bv) == JSON_STRING) {
            snprintf(uart_buffer, sizeof(uart_buffer), "  parity = \"%s\"\r\n", json_get_string(bv, NULL));
            TX();
        }

        bv = json_object_get(v, "enabled");
        if (json_get_type(bv) == JSON_BOOL) {
            snprintf(uart_buffer, sizeof(uart_buffer), "  enabled = %s\r\n", json_get_bool(bv) ? "true" : "false");
            TX();
        }
    }

    // 测试数组
    v = json_object_get(root, "tags");
    if (json_get_type(v) == JSON_ARRAY) {
        snprintf(uart_buffer, sizeof(uart_buffer), "tags (array, %d items):\r\n", (int)json_array_size(v));
        TX();
        for (uint32_t i = 0; i < json_array_size(v); i++) {
            json_value_t *av = json_array_get(v, i);
            if (json_get_type(av) == JSON_STRING) {
                snprintf(uart_buffer, sizeof(uart_buffer), "  [%d] = \"%s\"\r\n", i, json_get_string(av, NULL));
                TX();
            }
        }
    }

    snprintf(uart_buffer, sizeof(uart_buffer), "=== test done ===\r\n");
    TX();
#undef TX
}
