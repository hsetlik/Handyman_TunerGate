/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
#define SAMPLE_RATE 48000.0f
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

// this starts the timer and dma stream for the audio input ADC
void startTunerDMA();

// stop the timer & DMA stream
void stopTunerDMA();

void startNoiseGateDMA();
void stopNoiseGateDMA();

// set the LED to indicate tuner/noise gate mode
void setUseGateLED(bool ledOn);

// set the GPIO pin that opens/closes the noise gate
void setNoiseGateClosed(bool gateClosed);

// poll for the injected ADC group (the pot readings)
void startPotADCConversion();

//check if it's time to clear the screen
bool readyToClearScreen();

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define DISP_CS_Pin GPIO_PIN_13
#define DISP_CS_GPIO_Port GPIOC
#define Audio_IN_Pin GPIO_PIN_1
#define Audio_IN_GPIO_Port GPIOA
#define Thresh_IN_Pin GPIO_PIN_2
#define Thresh_IN_GPIO_Port GPIOA
#define Release_IN_Pin GPIO_PIN_3
#define Release_IN_GPIO_Port GPIOA
#define GateClosed_OUT_Pin GPIO_PIN_4
#define GateClosed_OUT_GPIO_Port GPIOC
#define GateOpen_IN_Pin GPIO_PIN_2
#define GateOpen_IN_GPIO_Port GPIOB
#define TunerMode_IN_Pin GPIO_PIN_13
#define TunerMode_IN_GPIO_Port GPIOB
#define UseGate_IN_Pin GPIO_PIN_7
#define UseGate_IN_GPIO_Port GPIOC
#define UseGate_OUT_Pin GPIO_PIN_9
#define UseGate_OUT_GPIO_Port GPIOC
#define DISP_BKL_Pin GPIO_PIN_9
#define DISP_BKL_GPIO_Port GPIOA
#define DISP_CLK_Pin GPIO_PIN_3
#define DISP_CLK_GPIO_Port GPIOB
#define DISP_SDA_Pin GPIO_PIN_5
#define DISP_SDA_GPIO_Port GPIOB
#define DISP_RST_Pin GPIO_PIN_6
#define DISP_RST_GPIO_Port GPIOB
#define DISP_DC_Pin GPIO_PIN_9
#define DISP_DC_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
