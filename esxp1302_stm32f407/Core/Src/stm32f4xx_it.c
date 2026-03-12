/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern SPI_HandleTypeDef hspi2;
extern SPI_HandleTypeDef hspi3;
extern TIM_HandleTypeDef htim6;

/* USER CODE BEGIN EV */
extern UART_HandleTypeDef huart1;
/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
  while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* LED0 快闪 2 次 (HardFault 信号) */
    for (int i = 0; i < 2; i++) {
      HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
      for (volatile uint32_t d = 0; d < 200000; d++);
      HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
      for (volatile uint32_t d = 0; d < 200000; d++);
    }
    for (volatile uint32_t d = 0; d < 1000000; d++);
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* LED0 快闪 3 次 (MemManage 信号) */
    for (int i = 0; i < 3; i++) {
      HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
      for (volatile uint32_t d = 0; d < 200000; d++);
      HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
      for (volatile uint32_t d = 0; d < 200000; d++);
    }
    for (volatile uint32_t d = 0; d < 1000000; d++);
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* LED0 快闪 4 次 (BusFault 信号) */
    for (int i = 0; i < 4; i++) {
      HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
      for (volatile uint32_t d = 0; d < 200000; d++);
      HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
      for (volatile uint32_t d = 0; d < 200000; d++);
    }
    for (volatile uint32_t d = 0; d < 1000000; d++);
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* LED0 快闪 5 次 (UsageFault 信号) */
    for (int i = 0; i < 5; i++) {
      HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
      for (volatile uint32_t d = 0; d < 200000; d++);
      HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
      for (volatile uint32_t d = 0; d < 200000; d++);
    }
    for (volatile uint32_t d = 0; d < 1000000; d++);
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles SPI2 global interrupt.
  */
void SPI2_IRQHandler(void)
{
  /* USER CODE BEGIN SPI2_IRQn 0 */

  /* USER CODE END SPI2_IRQn 0 */
  HAL_SPI_IRQHandler(&hspi2);
  /* USER CODE BEGIN SPI2_IRQn 1 */

  /* USER CODE END SPI2_IRQn 1 */
}

/**
  * @brief This function handles SPI3 global interrupt.
  */
void SPI3_IRQHandler(void)
{
  /* USER CODE BEGIN SPI3_IRQn 0 */

  /* USER CODE END SPI3_IRQn 0 */
  HAL_SPI_IRQHandler(&hspi3);
  /* USER CODE BEGIN SPI3_IRQn 1 */

  /* USER CODE END SPI3_IRQn 1 */
}

/**
  * @brief This function handles TIM6 global interrupt, DAC1 and DAC2 underrun error interrupts.
  */
void TIM6_DAC_IRQHandler(void)
{
  /* USER CODE BEGIN TIM6_DAC_IRQn 0 */

  /* USER CODE END TIM6_DAC_IRQn 0 */
  HAL_TIM_IRQHandler(&htim6);
  /* USER CODE BEGIN TIM6_DAC_IRQn 1 */

  /* USER CODE END TIM6_DAC_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/**
  * @brief This function handles USART1 global interrupt.
  *        Required for HAL_UART_Receive_IT (UART CLI receive).
  */
void USART1_IRQHandler(void)
{
  HAL_UART_IRQHandler(&huart1);
}

/* USER CODE END 1 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
