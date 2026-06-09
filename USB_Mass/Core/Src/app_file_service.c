#include "app_file_service.h"
#include "FreeRTOS.h"
#include "queue.h"

/* 队列句柄 — 在 FS_Init 中创建 */
static QueueHandle_t fs_queue = NULL;
static TaskHandle_t fs_task_handle = NULL;

/* 共享结果（单消费者 + 阻塞客户端模式，无竞争）*/
static FRESULT g_fs_result;

/* 文件服务任务函数 */
static void FileServiceTask(void *argument)
{
    (void)argument;
    FS_Request_t req;

    while (1)
    {
        if (xQueueReceive(fs_queue, &req, portMAX_DELAY) == pdTRUE)
        {
            switch (req.cmd)
            {
            case FS_CMD_OPENDIR:
                g_fs_result = f_opendir(req.dp, (const TCHAR *)req.path);
                break;
            case FS_CMD_READDIR:
                g_fs_result = f_readdir(req.dp, req.fip);
                break;
            case FS_CMD_CLOSEDIR:
                g_fs_result = f_closedir(req.dp);
                break;
            case FS_CMD_FOPEN:
                g_fs_result = f_open(req.fp, (const TCHAR *)req.path, req.flags);
                break;
            case FS_CMD_FREAD:
                g_fs_result = f_read(req.fp, req.buffer, req.btr, req.br);
                break;
            case FS_CMD_FCLOSE:
                g_fs_result = f_close(req.fp);
                break;
            case FS_CMD_FSTAT:
                g_fs_result = f_stat((const TCHAR *)req.path, req.fip);
                break;
            default:
                g_fs_result = FR_INVALID_PARAMETER;
                break;
            }

            /* 通知客户端结果已就绪 */
            xTaskNotifyGive(req.client);
        }
    }
}

/* 内部：发送请求并阻塞等待 */
static FRESULT FS_SendRequest(FS_Request_t *req)
{
    req->client = xTaskGetCurrentTaskHandle();
    xQueueSend(fs_queue, req, portMAX_DELAY);
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    return g_fs_result;
}

/*===== 同步包装函数 =====*/

FRESULT FS_OpenDir(const char *path, DIR *dp)
{
    FS_Request_t req = { .cmd = FS_CMD_OPENDIR, .dp = dp, .path = path };
    return FS_SendRequest(&req);
}

FRESULT FS_ReadDir(DIR *dp, FILINFO *fip)
{
    FS_Request_t req = { .cmd = FS_CMD_READDIR, .dp = dp, .fip = fip };
    return FS_SendRequest(&req);
}

FRESULT FS_CloseDir(DIR *dp)
{
    FS_Request_t req = { .cmd = FS_CMD_CLOSEDIR, .dp = dp };
    return FS_SendRequest(&req);
}

FRESULT FS_Open(const char *path, FIL *fp, uint8_t flags)
{
    FS_Request_t req = { .cmd = FS_CMD_FOPEN, .fp = fp, .path = path, .flags = flags };
    return FS_SendRequest(&req);
}

FRESULT FS_Read(FIL *fp, void *buff, UINT btr, UINT *br)
{
    FS_Request_t req = { .cmd = FS_CMD_FREAD, .fp = fp, .buffer = buff, .btr = btr, .br = br };
    return FS_SendRequest(&req);
}

FRESULT FS_Close(FIL *fp)
{
    FS_Request_t req = { .cmd = FS_CMD_FCLOSE, .fp = fp };
    return FS_SendRequest(&req);
}

FRESULT FS_Stat(const char *path, FILINFO *fip)
{
    FS_Request_t req = { .cmd = FS_CMD_FSTAT, .fip = fip, .path = path };
    return FS_SendRequest(&req);
}

void FS_Init(void)
{
    fs_queue = xQueueCreate(1, sizeof(FS_Request_t));
    xTaskCreate(FileServiceTask, "FILE_SVC", 120, NULL, 1, &fs_task_handle);
}

TaskHandle_t FS_GetTaskHandle(void)
{
    return fs_task_handle;
}
