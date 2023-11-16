/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "boot.h"
#include "proto.h"
#include "crc32.h"
#include "SEGGER_RTT.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
void normal_boot(void);
void bootloader_mode(void);
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_UART5_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint32_t start_time = 0;
  bool is_pressed = false;
  int ret = 0;

  SEGGER_RTT_printf(0, "bootloader version: %s \r\n", BOOTLOADER_VERSION);
  ret = boot_init();

  if (ret == -1) {
    SEGGER_RTT_printf(0, "boot_init: wrong partition \r\n");
    while (1);
  }

  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, GPIO_PIN_SET);
  // Logic to switch to bootloader mode
  start_time = HAL_GetTick();
  do {

    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {
      is_pressed = true;
    }

  } while ((HAL_GetTick() - start_time < 3000) && !is_pressed);

  // Perform normal boot.
  if (!is_pressed) {
    normal_boot();
  }

  // Enter into the bootloader mode.
  bootloader_mode();

  while (1)
  {
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void normal_boot(void)
{
  int ret = 0;

  SEGGER_RTT_printf(0, "normal boot: perform normal application boot\r\n");
  ret = boot_validate_appslot_bin();

  if (ret == -1) {
    SEGGER_RTT_printf(0, "normal boot: appslot validation failed\r\n");
    while (1); // unlimited wait
  }
  SEGGER_RTT_printf(0, "normal boot: validation successful\r\n");

  // jump to application address
  boot_goto_app();
}

void boot_newapp(void)
{
  int ret = 0;

  // validate the application binary
  ret = boot_validate_tempslot_bin(BOOT_TEMPSLOT1);
  if (ret == -1) {
    SEGGER_RTT_printf(0, "bootloader mode: validation failed, bootloader halt...!\r\n");
    while(1); // unlimited wait
  }

  boot_load_bin_to_appslot(BOOT_TEMPSLOT1);

  ret = boot_validate_appslot_bin();
  if (ret == -1) {
    SEGGER_RTT_printf(0, "bootloader mode: failed to load application, bootloader halt...!");
    while (1); // unlimited wait
  }

  SEGGER_RTT_printf(0, "bootloader mode: validation successful\r\n");
  // jump to application slot
  boot_goto_app();
}

void write_app_config(sbp_handle_t *sbp_handle)
{
  uint8_t version[8] = {0};

  memcpy(version, (uint8_t*)&sbp_handle->config.version, 4);
  SEGGER_RTT_printf(0, "bootloader mode: application version: ");
  SEGGER_RTT_printf(0, "%d.", version[0]);
  SEGGER_RTT_printf(0, "%d.", version[1]);
  SEGGER_RTT_printf(0, "%d",  version[2]);
  SEGGER_RTT_printf(0, "\r\n");
  SEGGER_RTT_printf(0, "bootloader mode: application size: %ld\r\n", sbp_handle->config.size);

  boot_write_config_version(sbp_handle->config.version);
  boot_write_config_size(sbp_handle->config.size);
  boot_write_config_crc(sbp_handle->config.crc);
  boot_write_config_slotno(BOOT_TEMPSLOT1);
}

void write_app_bin(sbp_handle_t *sbp_handle)
{
  boot_write_bin_to_tempslot(BOOT_TEMPSLOT1, sbp_handle->data.bytes, sbp_handle->data.size);
}

void bootloader_mode(void)
{
  sbp_handle_t sbp_handle;

  // Enter into the bootloader mode
  SEGGER_RTT_printf(0, "bootloader mode: entering bootloader mode\r\n");
  SEGGER_RTT_printf(0, "bootloader mode: erasing tempslot %d \r\n", BOOT_TEMPSLOT1 + 1);
  boot_erase_tempslot(BOOT_TEMPSLOT1);

  while (1) {
    proto_receive_packet(&sbp_handle);

    switch (sbp_handle.state) {

      case STATE_RESET:
        break;
      case STATE_DOWNLOAD_START:
        break;
      case STATE_DOWNLOAD_COMPLETE:
        boot_newapp();
        break;
      case STATE_CONF_PACKET_RECEIVED:
        write_app_config(&sbp_handle);
        break;
      case STATE_DATA_PACKET_RECEIVED:
        write_app_bin(&sbp_handle);
        break;

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
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
