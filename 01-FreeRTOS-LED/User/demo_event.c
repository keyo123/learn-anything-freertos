#include "demos.h"
#include "event_groups.h"
#include <stdio.h>

#define BIT_SENSOR  (1 << 0)
#define BIT_CONFIRM (1 << 1)

static EventGroupHandle_t xEventGroup = NULL;

static void vTaskSensor(void *pvParams)
{
    (void)pvParams;
    int i;

    for (i = 1; i <= 5; i++)
    {
        vTaskDelay(pdMS_TO_TICKS(200));
        printfSafe("[Sensor]  Sample #%d\r\n", i);
        xEventGroupSetBits(xEventGroup, BIT_SENSOR);
    }

    vTaskDelete(NULL);
}

static void vTaskUser(void *pvParams)
{
    (void)pvParams;
    int i;

    for (i = 1; i <= 5; i++)
    {
        vTaskDelay(pdMS_TO_TICKS(300));
        printfSafe("[User]    Confirm #%d\r\n", i);
        xEventGroupSetBits(xEventGroup, BIT_CONFIRM);
    }

    vTaskDelete(NULL);
}

static void vTaskProcessor(void *pvParams)
{
    (void)pvParams;
    int count = 0;

    while (count < 5)
    {
        EventBits_t bits = xEventGroupWaitBits(
            xEventGroup,
            BIT_SENSOR | BIT_CONFIRM,
            pdTRUE,       /* 唤醒后自动清除 */
            pdTRUE,       /* AND 模式 */
            portMAX_DELAY
        );

        count++;
        printfSafe(">>> Processor: Process #%d (bits=0x%02x)\r\n", count, bits);
    }

    printfSafe("\r\n=== Event Group Demo Done ===\r\n");
    vTaskDelete(NULL);
}

void start_event_demos(void)
{
    xEventGroup = xEventGroupCreate();
    if (xEventGroup != NULL)
    {
        xTaskCreate(vTaskSensor,    "Sensor",    configMINIMAL_STACK_SIZE, NULL, 1, NULL);
        xTaskCreate(vTaskUser,      "User",      configMINIMAL_STACK_SIZE, NULL, 1, NULL);
        xTaskCreate(vTaskProcessor, "Processor", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    }
}
