#ifndef APP_FILE_SERVICE_H
#define APP_FILE_SERVICE_H

#include "ff.h"
#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 文件服务命令 */
#define FS_CMD_OPENDIR     1
#define FS_CMD_READDIR     2
#define FS_CMD_CLOSEDIR    3
#define FS_CMD_FOPEN       4
#define FS_CMD_FREAD       5
#define FS_CMD_FCLOSE      6
#define FS_CMD_FSTAT       7

/* 请求结构 — 客户端填充，文件服务任务处理 */
typedef struct {
    uint8_t      cmd;
    TaskHandle_t client;     /* 客户端任务句柄（用于通知）*/
    const char  *path;
    FIL         *fp;
    DIR         *dp;
    FILINFO     *fip;
    void        *buffer;
    UINT         btr;        /* 要读取的字节数 */
    UINT        *br;         /* [出] 实际读取的字节数 */
    uint8_t      flags;      /* 文件打开标志 */
} FS_Request_t;

/* 同步包装函数（发送请求 → 阻塞等待完成）*/
FRESULT FS_OpenDir(const char *path, DIR *dp);
FRESULT FS_ReadDir(DIR *dp, FILINFO *fip);
FRESULT FS_CloseDir(DIR *dp);
FRESULT FS_Open(const char *path, FIL *fp, uint8_t flags);
FRESULT FS_Read(FIL *fp, void *buff, UINT btr, UINT *br);
FRESULT FS_Close(FIL *fp);
FRESULT FS_Stat(const char *path, FILINFO *fip);

/* 获取任务句柄（用于外部测量栈水位等）*/
TaskHandle_t FS_GetTaskHandle(void);

/* 初始化 — 创建任务和队列（在 vTaskStartScheduler 前调用）*/
void FS_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_FILE_SERVICE_H */
