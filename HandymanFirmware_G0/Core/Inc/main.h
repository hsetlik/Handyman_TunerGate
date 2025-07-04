/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define TUNER_MODE_IN_Pin GPIO_PIN_1
#define TUNER_MODE_IN_GPIO_Port GPIOA
#define OPEN_IN_Pin GPIO_PIN_2
#define OPEN_IN_GPIO_Port GPIOA
#define OPEN_IN_EXTI_IRQn EXTI2_3_IRQn
#define TUNE_IN_Pin GPIO_PIN_3
#define TUNE_IN_GPIO_Port GPIOA
#define TUNE_IN_EXTI_IRQn EXTI2_3_IRQn
#define CLOSED_OUT_Pin GPIO_PIN_4
#define CLOSED_OUT_GPIO_Port GPIOA
#define GATE_LED_OUT_Pin GPIO_PIN_6
#define GATE_LED_OUT_GPIO_Port GPIOA
#define GATE_BYP_IN_Pin GPIO_PIN_7
#define GATE_BYP_IN_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */
//extern volatile uint32_t* TICK_COUNT;
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
