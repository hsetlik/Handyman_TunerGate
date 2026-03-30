/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "BitstreamACF.h"
#include "Tuning.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "NoiseGate.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_adc_ex.h"
#include "stm32f4xx_hal_def.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_tim.h"
#include "stm32f4xx_it.h"
#include <string.h>
#include <stdlib.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define IN_TUNE_THRESH 4
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

/* USER CODE BEGIN PV */
// DMA fills this buffer with the ADC values
uint16_t tuningBuffer[TUNING_WINDOW_SIZE * 2];
uint16_t noiseGateBuffer[GATE_WINDOW_SIZE * 2];
// flags for tuner/noise gate modes
bool inTunerMode = false;
bool useNoiseGate = false;
volatile bool tunerDmaRunning = false;
volatile bool gateDmaRunning = false;
volatile bool noiseGateClosed = false;
uint32_t lastUpdateTick = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
// check the GPIOs to set the above `inTunerMode` and `useNoiseGate` flags
// call this once before the main while loop and again in the ISR for TIM3
// (which is 10x/second with current settings)
void checkModeSettings();

// draw the current tuning error to the display
void displayTuningError(tuning_error_t *err);
// draw a blank screen if we don't have valid tuning data
void displayBlankTuning();
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void checkModeSettings() {
  // 1. read the GPIOs to determine what state we should be in
  GPIO_PinState tuneState =
      HAL_GPIO_ReadPin(TunerMode_IN_GPIO_Port, TunerMode_IN_Pin);
  GPIO_PinState useGateState =
      HAL_GPIO_ReadPin(UseGate_IN_GPIO_Port, UseGate_IN_Pin);
  inTunerMode = tuneState == GPIO_PIN_SET;
  useNoiseGate = useGateState == GPIO_PIN_SET;
  const bool oledIsOn = ssd1306_GetDisplayOn() > 0;
  if (inTunerMode != oledIsOn) {
    ssd1306_SetDisplayOn((uint8_t)inTunerMode);
  } 
  setUseGateLED(useNoiseGate);
  if(inTunerMode && !tunerDmaRunning){
    if(gateDmaRunning){
      stopNoiseGateDMA();
    }
    startTunerDMA();
  } else if (!inTunerMode && !gateDmaRunning && useNoiseGate){
    if(tunerDmaRunning){
      stopTunerDMA();
    }
    startNoiseGateDMA();
  }
}

char *noteNames[12] = {"C",  "C#", "D",  "D#", "E",  "F",
                             "F#", "G",  "G#", "A",  "A#", "B"};

void displayTuningError(tuning_error_t *err) {
  char* noteName = noteNames[err->midiNote % 12];
  uint8_t xPos = 64 - (8 * strlen(noteName));
  // check if we're within the tuning threshold
  if(abs(err->errorCents) <= IN_TUNE_THRESH){
  ssd1306_Fill(White);
  ssd1306_SetCursor(xPos, 12);
  ssd1306_WriteString(noteName, Font_16x26, Black);
  } else {
  ssd1306_Fill(Black);
  ssd1306_SetCursor(xPos, 12);
  ssd1306_WriteString(noteName, Font_16x26, White);
    // draw the tuning error bar
    const uint8_t y0 = 40;
    const uint8_t y1 = 55;
    uint8_t barWidth = (uint8_t)(((float)abs(err->errorCents) / 50.0f) * 64.0f);
    uint8_t x0, x1;
    if(err->errorCents > 0){
      x0 = 64;
      x1 = 64 + barWidth;
    } else {
      x0 = 64 - barWidth;
      x1 = 64;
    }
    ssd1306_FillRectangle(x0, y0, x1, y1, White);
  }
  // 6. send the I2C data to update the screen
  ssd1306_UpdateScreen();
}


void displayBlankTuning(){
  ssd1306_Fill(Black);
  ssd1306_UpdateScreen();
}
// Implementations of shared stuff from main.h---------------------------------------------------

void startTunerDMA(){
  HAL_StatusTypeDef dmaStatus = HAL_ADC_Start_DMA(&hadc1, (uint32_t *)tuningBuffer, TUNING_WINDOW_SIZE * 2);
  if(dmaStatus != HAL_OK){
    Error_Handler();
  }
  HAL_StatusTypeDef timerStatus = HAL_TIM_Base_Start(&htim2);
  if(timerStatus != HAL_OK){
    Error_Handler();
  }
  tunerDmaRunning = true;
}


void stopTunerDMA(){
  HAL_StatusTypeDef timerStatus = HAL_TIM_Base_Stop(&htim2);
  if(timerStatus != HAL_OK){
    Error_Handler();
  }
  HAL_StatusTypeDef dmaStatus = HAL_ADC_Stop_DMA(&hadc1);
  if(dmaStatus != HAL_OK){
    Error_Handler();
  }
  tunerDmaRunning = false;
}

void startNoiseGateDMA(){
  HAL_StatusTypeDef dmaStatus = HAL_ADC_Start_DMA(&hadc1, (uint32_t *)noiseGateBuffer, GATE_WINDOW_SIZE * 2);
  if(dmaStatus != HAL_OK){
    Error_Handler();
  }
  HAL_StatusTypeDef timerStatus = HAL_TIM_Base_Start(&htim2);
  if(timerStatus != HAL_OK){
    Error_Handler();
  }
  gateDmaRunning = true;
}

void stopNoiseGateDMA(){
  HAL_StatusTypeDef timerStatus = HAL_TIM_Base_Stop(&htim2);
  if(timerStatus != HAL_OK){
    Error_Handler();
  }
  HAL_StatusTypeDef dmaStatus = HAL_ADC_Stop_DMA(&hadc1);
  if(dmaStatus != HAL_OK){
    Error_Handler();
  }
  gateDmaRunning = false;
}

bool readyToClearScreen(){
  uint32_t now = HAL_GetTick();
  return (now - lastUpdateTick) > 600;
}

void setUseGateLED(bool ledOn){
  GPIO_PinState state = ledOn ? GPIO_PIN_SET : GPIO_PIN_RESET;
  HAL_GPIO_WritePin(UseGate_OUT_GPIO_Port, UseGate_OUT_Pin, state);
}

void setNoiseGateClosed(bool gateClosed){
  noiseGateClosed = gateClosed;
  GPIO_PinState state = noiseGateClosed ? GPIO_PIN_RESET : GPIO_PIN_SET;
  HAL_GPIO_WritePin(GateClosed_OUT_GPIO_Port, GateClosed_OUT_Pin, state);
}


void startPotADCConversion(){
  // 1. trigger the software injection
  HAL_StatusTypeDef startStatus = HAL_ADCEx_InjectedStart(&hadc1);
  if(startStatus != HAL_OK){
    Error_Handler();
  }
  // 2. block until the conversion is done
  HAL_StatusTypeDef pollStatus = HAL_ADCEx_InjectedPollForConversion(&hadc1, HAL_MAX_DELAY);
  if(pollStatus != HAL_OK){
    Error_Handler();
  }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  // Initialize the buffers for the BAC handling
  BAC_initBitArray();

  // initialize the OLED
  ssd1306_Init();
  ssd1306_Fill(White);
  ssd1306_UpdateScreen();
  HAL_Delay(200);



  // check the GPIO inputs for the first time and start the relevant DMA streams
  checkModeSettings();

  // start timer 3 for checking mode settings
  HAL_TIM_Base_Start_IT(&htim3);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    if (inTunerMode && BAC_isBitstreamLoaded()) {
      if(BAC_hasTuningSignal()){
        // 1. run the BAC algorithm
        const float currentHz = BAC_getCurrentHz();
        // 2. convert to an error value
        tuning_error_t err = Tune_getErrorForFreq(currentHz);
        // 3. display the error
        displayTuningError(&err);
        lastUpdateTick = HAL_GetTick();
      } else if (readyToClearScreen()) {
        displayBlankTuning();
      }
      // 4. Unset the bitstreamLoaded flag
      BAC_finishedWithCurrentHz();

    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 84;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};
  ADC_InjectionConfTypeDef sConfigInjected = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configures for the selected ADC injected channel its corresponding rank in the sequencer and its sample time
  */
  sConfigInjected.InjectedChannel = ADC_CHANNEL_2;
  sConfigInjected.InjectedRank = 1;
  sConfigInjected.InjectedNbrOfConversion = 2;
  sConfigInjected.InjectedSamplingTime = ADC_SAMPLETIME_3CYCLES;
  sConfigInjected.ExternalTrigInjecConvEdge = ADC_EXTERNALTRIGINJECCONVEDGE_NONE;
  sConfigInjected.ExternalTrigInjecConv = ADC_INJECTED_SOFTWARE_START;
  sConfigInjected.AutoInjectedConv = DISABLE;
  sConfigInjected.InjectedDiscontinuousConvMode = DISABLE;
  sConfigInjected.InjectedOffset = 0;
  if (HAL_ADCEx_InjectedConfigChannel(&hadc1, &sConfigInjected) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configures for the selected ADC injected channel its corresponding rank in the sequencer and its sample time
  */
  sConfigInjected.InjectedChannel = ADC_CHANNEL_3;
  sConfigInjected.InjectedRank = 2;
  if (HAL_ADCEx_InjectedConfigChannel(&hadc1, &sConfigInjected) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 1499;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 167;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 49999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GateClosed_OUT_Pin|UseGate_OUT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : GateClosed_OUT_Pin UseGate_OUT_Pin */
  GPIO_InitStruct.Pin = GateClosed_OUT_Pin|UseGate_OUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : GateOpen_IN_Pin TunerMode_IN_Pin */
  GPIO_InitStruct.Pin = GateOpen_IN_Pin|TunerMode_IN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : UseGate_IN_Pin */
  GPIO_InitStruct.Pin = UseGate_IN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(UseGate_IN_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
//========================================================================================
// Audio ADC callback
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
  // compute the second half of the buffer
  if (inTunerMode) {
  uint16_t* startPtr = &tuningBuffer[TUNING_WINDOW_SIZE];
    BAC_loadBitstream(startPtr);
    HAL_StatusTypeDef dmaStatus = HAL_ADC_Start_DMA(&hadc1, (uint32_t *)tuningBuffer, TUNING_WINDOW_SIZE * 2);
    if(dmaStatus != HAL_OK){
      Error_Handler();
    }
  } else if (useNoiseGate) {
    uint16_t* startPtr = &noiseGateBuffer[GATE_WINDOW_SIZE];
    Gate_processChunk(startPtr, GATE_WINDOW_SIZE);
    // update the pots here if it's time
    if(Gate_isAwaitingPotReadings()){
      startPotADCConversion();
      uint32_t threshVal = HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_1);
      uint32_t releaseVal = HAL_ADCEx_InjectedGetValue(&hadc1, ADC_INJECTED_RANK_2);
      Gate_updatePotReadings((uint16_t)threshVal, (uint16_t)releaseVal);
    }
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)noiseGateBuffer, GATE_WINDOW_SIZE * 2);
  }
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc) {
  // compute the first half of the buffer
  if(inTunerMode){
    BAC_loadBitstream(tuningBuffer);
  } else if (useNoiseGate) {
    Gate_processChunk(noiseGateBuffer, GATE_WINDOW_SIZE);
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim == &htim3) {
    checkModeSettings();
    if(useNoiseGate && !inTunerMode){
      Gate_requestPotReadings();
    }
  }
}


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
