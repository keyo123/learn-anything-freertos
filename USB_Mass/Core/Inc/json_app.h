/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : json_app.h
  * @brief          : JSON parser application header - JSON file reading and testing
  ******************************************************************************
  */
/* USER CODE END Header */
#ifndef __JSON_APP_H
#define __JSON_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Exported functions --------------------------------------------------------*/
int read_and_parse_json_file(const char *filename);
void test_json_parser(void);

#ifdef __cplusplus
}
#endif

#endif /* __JSON_APP_H */
