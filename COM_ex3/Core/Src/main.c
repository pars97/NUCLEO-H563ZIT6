/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dac.h"
#include "gpdma.h"
#include "icache.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

COM_InitTypeDef BspCOMInit;

/* USER CODE BEGIN PV */
__IO uint32_t BspButtonState = BUTTON_RELEASED;

#define RX_BUF_SIZE 32

uint8_t rx_byte;
char rx_buffer[RX_BUF_SIZE];
uint8_t rx_index = 0;

uint8_t input_ready = 0;
uint8_t menu_state = 0;
uint8_t selected_option = 0;

volatile uint16_t adc1_buffer[2];
volatile uint16_t adc2_buffer[2];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

void UART_ProcessByte(void);
void PrintMenu(void);

void SetDAC(int value);
void SetPWM(TIM_HandleTypeDef *htim, uint32_t channel, int percent);

/* ADC read helpers (DMA shared buffer) */
//uint16_t Read_PF11(void);
//uint16_t Read_PF12(void);
//uint16_t Read_PF13(void);
//uint16_t Read_PF14(void);

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

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

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
  MX_GPDMA1_Init();
  MX_DAC1_Init();
  MX_ICACHE_Init();
  MX_TIM1_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();
  /* USER CODE BEGIN 2 */
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc1_buffer, 2);
  HAL_ADC_Start_DMA(&hadc2, (uint32_t*)adc2_buffer, 2);



  /* Start PWM outputs */
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);
  /* Enable main output for advanced timer */
  __HAL_TIM_MOE_ENABLE(&htim1);

  /* Start ADC DMA (4 samples total) */




  /* Print startup message */
  printf("System Ready\r\n");
  menu_state = 1;

  /* USER CODE END 2 */

  /* Initialize leds */
  BSP_LED_Init(LED_GREEN);
  BSP_LED_Init(LED_YELLOW);
  BSP_LED_Init(LED_RED);

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Initialize COM1 port (115200, 8 bits (7-bit data + 1 stop bit), no parity */
  BspCOMInit.BaudRate   = 115200;
  BspCOMInit.WordLength = COM_WORDLENGTH_8B;
  BspCOMInit.StopBits   = COM_STOPBITS_1;
  BspCOMInit.Parity     = COM_PARITY_NONE;
  BspCOMInit.HwFlowCtl  = COM_HWCONTROL_NONE;
  if (BSP_COM_Init(COM1, &BspCOMInit) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
    /* --- UART polling (menu input) --- */
    UART_ProcessByte();
    printf("ADC1=%u %u | ADC2=%u %u\r\n",
  		  adc1_buffer[0],
			  adc1_buffer[1],
			  adc2_buffer[0],
			  adc2_buffer[1]);

    /* --- Show menu when button triggers it --- */
    if (menu_state == 1)
    {
      PrintMenu();
      menu_state = 2;
      rx_index = 0;
    }

    /* --- Input received from UART --- */
    if (input_ready)
    {
      input_ready = 0;

      rx_buffer[rx_index] = '\0';
      int value = atoi(rx_buffer);
      rx_index = 0;

      if (menu_state == 2)
      {
        selected_option = value;

        printf("Selected: %d\r\n", selected_option);
        printf("Enter value:\r\n");

        menu_state = 3;
      }
      else if (menu_state == 3)
      {
        switch (selected_option)
        {
          case 1:
            SetDAC(value);
            break;

          case 2:
            SetPWM(&htim1, TIM_CHANNEL_1, value);
            break;

          case 3:
            SetPWM(&htim3, TIM_CHANNEL_1, value);
            break;

          case 4:
            SetPWM(&htim4, TIM_CHANNEL_1, value);
            SetPWM(&htim4, TIM_CHANNEL_2, value);
            break;

          default:
            printf("Invalid option\r\n");
            break;
        }

        menu_state = 0;
      }
    }

    /* --- periodic ADC print (debug) --- */
    static uint32_t lastTick = 0;

    if (HAL_GetTick() - lastTick > 5)
    {
      lastTick = HAL_GetTick();
      LL_ADC_REG_StartConversion(ADC1);
      LL_ADC_REG_StartConversion(ADC2);
      //HAL_Delay(5);


    }
//TIA1 = adc1_buffer[0] * 4;

  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Configure LSE Drive Capability
  *  Warning : Only applied when the LSE is disabled.
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLL1_SOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 250;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1_VCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1_VCORANGE_WIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the programming delay
  */
  __HAL_FLASH_SET_PROGRAM_DELAY(FLASH_PROGRAMMING_DELAY_2);
}

/* USER CODE BEGIN 4 */

void UART_ProcessByte(void)
{
  if (HAL_UART_Receive(&hcom_uart[COM1], &rx_byte, 1, 0) == HAL_OK)
  {
    if (rx_byte == '\r' || rx_byte == '\n')
    {
      rx_buffer[rx_index] = '\0';
      input_ready = 1;
    }
    else
    {
      if (rx_index < RX_BUF_SIZE - 1)
      {
        rx_buffer[rx_index++] = rx_byte;
      }
    }
  }
}

void PrintMenu(void)
{
  printf("\r\n--- MENU ---\r\n");
  printf("1: Set DAC\r\n");
  printf("2: TIM1 PWM\r\n");
  printf("3: TIM3 PWM\r\n");
  printf("4: TIM4 PWM\r\n");
}

void SetDAC(int value)
{
  if (value < 0) value = 0;
  if (value > 4095) value = 4095;

  HAL_DAC_SetValue(
      &hdac1,
      DAC_CHANNEL_2,
      DAC_ALIGN_12B_R,
      value
  );

  HAL_DAC_Start(&hdac1, DAC_CHANNEL_2);

  printf("DAC=%d\r\n", value);
}

void SetPWM(TIM_HandleTypeDef *htim, uint32_t channel, int percent)
{
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(htim);
    uint32_t pulse = (percent * (arr + 1)) / 100;

    __HAL_TIM_SET_COMPARE(htim, channel, pulse);

    printf("PWM set to %d%%\r\n", percent);
}

void BSP_PB_Callback(Button_TypeDef Button)
{
    if (Button == BUTTON_USER)
    {
        menu_state = 1;
    }
}


/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};
  MPU_Attributes_InitTypeDef MPU_AttributesInit = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region 0 and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x08FFF000;
  MPU_InitStruct.LimitAddress = 0x08FFFFFF;
  MPU_InitStruct.AttributesIndex = MPU_ATTRIBUTES_NUMBER0;
  MPU_InitStruct.AccessPermission = MPU_REGION_ALL_RO;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Attribute 0 and the memory to be protected
  */
  MPU_AttributesInit.Number = MPU_ATTRIBUTES_NUMBER0;
  MPU_AttributesInit.Attributes = INNER_OUTER(MPU_NOT_CACHEABLE);

  HAL_MPU_ConfigMemoryAttributes(&MPU_AttributesInit);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
