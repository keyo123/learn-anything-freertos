/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : lvgl_ui.h
  * @brief          : LVGL UI application header - GUI creation and event handlers
  ******************************************************************************
  */
/* USER CODE END Header */
#ifndef __LVGL_UI_H
#define __LVGL_UI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"

/* Exported variables --------------------------------------------------------*/
extern FILINFO FileInfo;
extern DIR DirInfo;

/* Exported functions --------------------------------------------------------*/
void create_test_ui(void);

#ifdef __cplusplus
}
#endif

#endif /* __LVGL_UI_H */
