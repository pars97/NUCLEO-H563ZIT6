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
#define RX_BUF_SIZE 32
#define ADC_INSTANCES 2
#define ADC_CHANNELS  2
#define TOP_N 4
#define IL_LUT_SIZE 401



/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

COM_InitTypeDef BspCOMInit;

/* USER CODE BEGIN PV */
__IO uint32_t BspButtonState = BUTTON_RELEASED;



uint8_t rx_byte;
char rx_buffer[RX_BUF_SIZE];
uint8_t rx_index = 0;

uint8_t input_ready = 0;
uint8_t menu_state = 0;
uint8_t selected_option = 0;



typedef struct
{
    uint16_t val[TOP_N];
    uint16_t pos[TOP_N];
} TopN_t;
TopN_t peak[ADC_INSTANCES][ADC_CHANNELS];
uint32_t avg_peak[ADC_INSTANCES][ADC_CHANNELS];
uint32_t gain[ADC_INSTANCES][ADC_CHANNELS];

typedef struct
{
    uint32_t buffer[8];
    uint32_t sum;
    uint8_t index;
} MovingAverage8_t;

typedef struct
{
    uint16_t peak;
    uint32_t gain;
    uint8_t enabled;
} ADC_ChannelCal_t;

typedef struct
{
    uint32_t G_T;
    uint32_t G_D;
} IL_Gain_t;

ADC_ChannelCal_t adc_cal[4];
uint16_t adc_norm[ADC_INSTANCES][ADC_CHANNELS];

volatile uint16_t adc1_buffer[2];
volatile uint16_t adc2_buffer[2];
uint16_t ADC1_CH0[8];
uint16_t ADC1_CH1[8];
uint16_t ADC2_CH0[8];
uint16_t ADC2_CH1[8];
MovingAverage8_t ADC1_CH0_Filter;
MovingAverage8_t ADC1_CH1_Filter;
MovingAverage8_t ADC2_CH0_Filter;
MovingAverage8_t ADC2_CH1_Filter;

uint16_t Factor;
uint16_t IL;
uint16_t Current_IL;
int16_t kp;
int16_t ki;
int16_t kd;
int16_t integral;
IL_Gain_t il_gain;
uint8_t il_ready = 0;
volatile uint8_t h =0;

const uint16_t IL_LUT[IL_LUT_SIZE] = {
    65535, 64785, 64043, 63310, 62585, 61869, 61161, 60461,
    59769, 59084, 58408, 57740, 57079, 56425, 55779, 55141,
    54510, 53886, 53269, 52659, 52056, 51460, 50871, 50289,
    49713, 49144, 48582, 48026, 47476, 46932, 46395, 45864,
    45339, 44820, 44307, 43800, 43299, 42803, 42313, 41829,
    41350, 40876, 40409, 39946, 39489, 39037, 38590, 38148,
    37711, 37280, 36853, 36431, 36014, 35602, 35194, 34792,
    34393, 34000, 33610, 33226, 32845, 32469, 32098, 31730,
    31367, 31008, 30653, 30302, 29955, 29612, 29273, 28938,
    28607, 28280, 27956, 27636, 27320, 27007, 26698, 26392,
    26090, 25791, 25496, 25204, 24916, 24631, 24349, 24070,
    23794, 23522, 23253, 22987, 22723, 22463, 22206, 21952,
    21701, 21452, 21207, 20964, 20724, 20487, 20252, 20020,
    19791, 19565, 19341, 19119, 18901, 18684, 18470, 18259,
    18050, 17843, 17639, 17437, 17237, 17040, 16845, 16652,
    16462, 16273, 16087, 15903, 15721, 15541, 15363, 15187,
    15013, 14841, 14671, 14504, 14337, 14173, 14011, 13851,
    13692, 13535, 13381, 13227, 13076, 12926, 12778, 12632,
    12487, 12344, 12203, 12063, 11925, 11789, 11654, 11521,
    11389, 11258, 11129, 11002, 10876, 10752, 10629, 10507,
    10387, 10268, 10150, 10034, 9919, 9806, 9693, 9582,
    9473, 9364, 9257, 9151, 9046, 8943, 8840, 8739,
    8639, 8540, 8443, 8346, 8250, 8156, 8063, 7970,
    7879, 7789, 7700, 7612, 7524, 7438, 7353, 7269,
    7186, 7104, 7022, 6942, 6862, 6784, 6706, 6629,
    6554, 6478, 6404, 6331, 6259, 6187, 6116, 6046,
    5977, 5908, 5841, 5774, 5708, 5643, 5578, 5514,
    5451, 5389, 5327, 5266, 5206, 5146, 5087, 5029,
    4971, 4914, 4858, 4803, 4748, 4693, 4640, 4586,
    4534, 4482, 4431, 4380, 4330, 4280, 4231, 4183,
    4135, 4088, 4041, 3995, 3949, 3904, 3859, 3815,
    3771, 3728, 3685, 3643, 3601, 3560, 3519, 3479,
    3439, 3400, 3361, 3323, 3285, 3247, 3210, 3173,
    3137, 3101, 3065, 3030, 2996, 2961, 2927, 2894,
    2861, 2828, 2796, 2764, 2732, 2701, 2670, 2639,
    2609, 2579, 2550, 2520, 2492, 2463, 2435, 2407,
    2379, 2352, 2325, 2299, 2272, 2246, 2221, 2195,
    2170, 2145, 2121, 2096, 2072, 2049, 2025, 2002,
    1979, 1956, 1934, 1912, 1890, 1868, 1847, 1826,
    1805, 1784, 1764, 1744, 1724, 1704, 1685, 1665,
    1646, 1627, 1609, 1590, 1572, 1554, 1536, 1519,
    1501, 1484, 1467, 1450, 1434, 1417, 1401, 1385,
    1369, 1354, 1338, 1323, 1308, 1293, 1278, 1263,
    1249, 1234, 1220, 1206, 1193, 1179, 1165, 1152,
    1139, 1126, 1113, 1100, 1088, 1075, 1063, 1051,
    1039, 1027, 1015, 1003, 992, 981, 969, 958,
    947, 936, 926, 915, 905, 894, 884, 874,
    864, 854, 844, 835, 825, 816, 806, 797,
    788, 779, 770, 761, 752, 744, 735, 727,
    719, 710, 702, 694, 686, 678, 671, 663,
    655
};


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

void UART_ProcessByte(void);
void PrintMenu(void);

void SetDAC_1(int value);
void SetDAC_2(int value);
void SetPWM(TIM_HandleTypeDef *htim, uint32_t channel, int percent);

void MovingAverage8_Init(MovingAverage8_t *filt);
uint32_t MovingAverage8_Update(MovingAverage8_t *filt, uint32_t sample);
void TopN_Insert(TopN_t *t, uint16_t value, uint16_t pos);
uint32_t TopN_Average(TopN_t *t);
void Calibration_Run(volatile uint16_t *adc1_buffer,volatile uint16_t *adc2_buffer);
uint16_t Tune_IL_Gains(uint16_t adc_norm[2][2],TopN_t peak[2][2],IL_Gain_t *g,uint16_t x_centi_dB);
static inline uint16_t IL_LUT_Get(uint16_t il_centi_db);


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
  MovingAverage8_Init(&ADC1_CH0_Filter);
  MovingAverage8_Init(&ADC1_CH1_Filter);
  MovingAverage8_Init(&ADC2_CH0_Filter);
  MovingAverage8_Init(&ADC2_CH1_Filter);

  static uint32_t printTick = 0;
  int32_t calculation_MRR3 = 0;
  //int16_t calculation_MRR4 = 0;
  static uint16_t DAC_Value_MRR3 = 4095;
 // static uint16_t DAC_Value_MRR4 = 4095;
  memset(ADC1_CH0, 0, sizeof(ADC1_CH0));
  memset(ADC1_CH1, 0, sizeof(ADC1_CH1));
  memset(ADC2_CH0, 0, sizeof(ADC2_CH0));
  memset(ADC2_CH1, 0, sizeof(ADC2_CH1));
  uint32_t ADC1_CH0_filt = 0;
  uint32_t ADC1_CH1_filt = 0;
  uint32_t ADC2_CH0_filt = 0;
  uint32_t ADC2_CH1_filt = 0;
  uint16_t dac_value = 0;
  uint16_t calculate =0;
  //uint16_t db_IL=0;

  kp = 0;
  ki = 0;
  kd =0;
  IL = 0;
  Current_IL= 0;
  integral =0;
  h=0;

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
  //HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
  //HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED);

  //HAL_Delay(100);

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
  //static uint32_t lastTick = 0;

// SET THE INSERTION LOSS, 300 means 3db, 455 means 4.55 db loss
  IL = 300;

  Calibration_Run(adc1_buffer, adc2_buffer);

  for (uint8_t a = 0; a < ADC_INSTANCES; a++)
  {
      for (uint8_t ch = 0; ch < ADC_CHANNELS; ch++)
      {
          adc_norm[a][ch] = gain[a][ch];
      }
  }
  printf("BEFORE IL GAINS \r\n");

  dac_value = Tune_IL_Gains(adc_norm,peak, &il_gain,IL);
  il_ready = 1;
  DAC_Value_MRR3 = dac_value;
  HAL_Delay(500);

  while (1)
  {
	  while(calculate<3)
	  {
		if (LL_ADC_REG_IsConversionOngoing(ADC1)==0&&LL_ADC_REG_IsConversionOngoing(ADC2)==0)
		{
		LL_ADC_REG_StartConversion(ADC1);
		LL_ADC_REG_StartConversion(ADC2);
		while(LL_ADC_REG_IsConversionOngoing(ADC1)||LL_ADC_REG_IsConversionOngoing(ADC2))
			{ }

		//ADC1_CH0_filt = MovingAverage8_Update(&ADC1_CH0_Filter, (adc1_buffer[0]*adc_norm[0][0]));
		//ADC1_CH1_filt = MovingAverage8_Update(&ADC1_CH1_Filter, (adc1_buffer[1]*(adc_norm[0][1]*il_gain.G_T)>>10));

		//ADC2_CH0_filt = MovingAverage8_Update(&ADC2_CH0_Filter, (adc2_buffer[0]*adc_norm[1][0]));
		//ADC2_CH1_filt = MovingAverage8_Update(&ADC2_CH1_Filter, (adc2_buffer[1]*(adc_norm[1][1]*il_gain.G_D)>>10));
		ADC1_CH1_filt = adc1_buffer[1]*(adc_norm[0][1]*il_gain.G_T)>>16;
		ADC2_CH1_filt = adc2_buffer[1]*(adc_norm[1][1]*il_gain.G_D)>>16;


		calculation_MRR3 += ADC2_CH1_filt-ADC1_CH1_filt;
		calculate++;
		HAL_Delay(5);
		}
	  }
		calculation_MRR3 = calculation_MRR3>>2;

		Current_IL = (avg_peak[0][1]<<16)/adc1_buffer[1];

		//calculation_MRR4 = ADC2_CH0_filt - (ADC1_CH1_filt>>1);
		//integral = integral + calculation;

		//kp=1;
		//ki=1;

		//DAC_Value = ((DAC_Value+kp*calculation+ki*integral)>>6);

		//if (DAC_Value>4096)
		//	DAC_Value = 4096;

		if (calculation_MRR3 >0)
		{
			if (DAC_Value_MRR3<4095)
				DAC_Value_MRR3 = DAC_Value_MRR3 +1;
			else
			{
				DAC_Value_MRR3 = 50;
			HAL_Delay(100);
			}
		}

		if (calculation_MRR3 <0)
		{
			if (DAC_Value_MRR3>0)
				DAC_Value_MRR3 = DAC_Value_MRR3 -1;
			else
			{
				DAC_Value_MRR3 = 4000;
			HAL_Delay(100);
			}
		}

		calculation_MRR3 =0;
		calculate =0;


		SetDAC_2(DAC_Value_MRR3);
		//SetDAC_1(DAC_Value_MRR4);


		if (HAL_GetTick() - printTick > 10)
		{
		    printf("ADC1=%lu %lu | ADC2=%lu %lu | DAC=%u IL=%u\r\n",
		           ADC1_CH0_filt,
		           ADC1_CH1_filt,
		           ADC2_CH0_filt,
		           ADC2_CH1_filt,
		           DAC_Value_MRR3,
				   Current_IL);

		    printf("DAC=%u IL = %u\r\n",
		           DAC_Value_MRR3,
				   Current_IL);

		    printTick = HAL_GetTick();
		}


//TIA1 = adc1_buffer[0] * 4;

  }

}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  /* USER CODE END 3 */


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
      //rx_index = 0;
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

void SetDAC_2(int value)
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
}

void SetDAC_1(int value)
{
  if (value < 0) value = 0;
  if (value > 4095) value = 4095;

  HAL_DAC_SetValue(
      &hdac1,
      DAC_CHANNEL_1,
      DAC_ALIGN_12B_R,
      value
  );

  HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
}

void MovingAverage8_Init(MovingAverage8_t *filt)
{
    filt->sum = 0;
    filt->index = 0;

    for(uint8_t i = 0; i < 8; i++)
    {
        filt->buffer[i] = 0;
    }
}

uint32_t MovingAverage8_Update(MovingAverage8_t *filt, uint32_t sample)
{
    /* Remove oldest sample from sum */
    filt->sum -= filt->buffer[filt->index];

    /* Store new sample */
    filt->buffer[filt->index] = sample;

    /* Add new sample to sum */
    filt->sum += sample;

    /* Advance circular buffer index */
    filt->index = (filt->index + 1) & 0x07;

    /* Return average */
    return (uint32_t)(filt->sum >> 3);
}

uint32_t TopN_Average(TopN_t *t)
{
    return (t->val[0] + t->val[1] + t->val[2] + t->val[3]) >> 2;
}

void TopN_Insert(TopN_t *t, uint16_t value, uint16_t pos)
{
    for (int i = 0; i < TOP_N; i++)
    {
        if (value > t->val[i])
        {
            for (int j = TOP_N - 1; j > i; j--)
            {
                t->val[j] = t->val[j - 1];
                t->pos[j] = t->pos[j - 1];
            }

            t->val[i] = value;
            t->pos[i] = pos;
            return;
        }
    }
}

uint16_t Tune_IL_Gains(uint16_t adc_norm[2][2],TopN_t peak[2][2],  IL_Gain_t *g,uint16_t x_centi_dB)
{
	printf("TUNING IL GAIN\r\n");
	// Convert centi-dB to LUT index (5 centi-dB resolution)
	uint16_t idx = x_centi_dB / 5;
	uint32_t ratio = IL_LUT[idx];
	uint16_t dac_value = 3100;
	g -> G_T = 16384;
	g -> G_D = 16384;
	uint8_t Ongoing = 1;
	uint32_t ADC1_CH0_filt = 0;
	uint32_t ADC1_CH1_filt = 0;
	uint32_t ADC2_CH0_filt = 0;
	uint32_t ADC2_CH1_filt = 0;
	uint16_t dac = dac_value;


	uint32_t target = ((ratio*adc_norm[0][1]>>16))*(uint32_t)avg_peak[0][1];
	// Target is ratio(<<16)*adc_norm(<<10.

	while (Ongoing)
	{
	SetDAC_2(dac);
	HAL_Delay(10);
	printf("%u\r\n",dac);
		  while (h<8)
		  {
			LL_ADC_REG_StartConversion(ADC1);
			LL_ADC_REG_StartConversion(ADC2);
				while(LL_ADC_REG_IsConversionOngoing(ADC1)||LL_ADC_REG_IsConversionOngoing(ADC2))
				{ }
			ADC1_CH0_filt = MovingAverage8_Update(&ADC1_CH0_Filter, adc1_buffer[0]*adc_norm[0][0]);
			ADC1_CH1_filt = MovingAverage8_Update(&ADC1_CH1_Filter, adc1_buffer[1]*adc_norm[0][1]);

			ADC2_CH0_filt = MovingAverage8_Update(&ADC2_CH0_Filter, adc2_buffer[0]*adc_norm[1][0]);
			ADC2_CH1_filt = MovingAverage8_Update(&ADC2_CH1_Filter, adc2_buffer[1]*adc_norm[1][1]);

				h++;
		  }
	  printf("ADC1_CH1_filt %lu\r\n",ADC1_CH1_filt);
	  	  h =0;
	  	  dac--;

	  	if (((ADC1_CH1_filt-target)>>31)==1)
	  		  {
	  		  	  dac_value = dac;
	  		  	  printf("A VALUE! %u\r\n",dac);
	  		  	  Ongoing =0;
	  		  }
	  }
	printf("DAC VALUE FOUND: %u\r\n",dac_value);
	SetDAC_2(dac_value);
	Ongoing =1;
	int32_t calc=0;
	while (Ongoing)
		  {
			  if (LL_ADC_REG_IsConversionOngoing(ADC1)==0&&LL_ADC_REG_IsConversionOngoing(ADC2)==0)
			{
			LL_ADC_REG_StartConversion(ADC1);
			LL_ADC_REG_StartConversion(ADC2);
			while(LL_ADC_REG_IsConversionOngoing(ADC1)||LL_ADC_REG_IsConversionOngoing(ADC2))
				{ }

			ADC1_CH0_filt = MovingAverage8_Update(&ADC1_CH0_Filter, adc1_buffer[0]*adc_norm[0][0]);
			ADC1_CH1_filt = MovingAverage8_Update(&ADC1_CH1_Filter, (adc1_buffer[1]*(adc_norm[0][1]*g->G_T)>>16));

			ADC2_CH0_filt = MovingAverage8_Update(&ADC2_CH0_Filter, adc2_buffer[0]*adc_norm[1][0]);
			ADC2_CH1_filt = MovingAverage8_Update(&ADC2_CH1_Filter, (adc2_buffer[1]*(adc_norm[1][1]*g->G_D)>>16));
			printf("ADC1=%u %u | ADC2=%u %u\r\n",adc1_buffer[0],adc1_buffer[1],adc2_buffer[0],adc2_buffer[1]);

			}

			calc = ADC2_CH1_filt-ADC1_CH1_filt;

			if (calc>0)
				{g -> G_D -=1;
			printf("G_D = %u\r\n",g->G_D);
			HAL_Delay(10);}

			else
			Ongoing=0;
		  	  }

			printf("Gain T : %lu, Gain D: %lu\r\n",g->G_T,g->G_D);
			HAL_Delay(500);
			return(dac_value);


}

static inline uint16_t IL_LUT_Get(uint16_t il_centi_db)
{
    uint16_t idx = il_centi_db / 5;

    if (idx >= IL_LUT_SIZE - 1)
        idx = IL_LUT_SIZE - 1;

    return IL_LUT[idx];

}

void Calibration_Run(volatile uint16_t *adc1_buffer,
                     volatile uint16_t *adc2_buffer)
{
    printf("Calibration Begin\r\n");

    /* reset */
    for (uint8_t a = 0; a < ADC_INSTANCES; a++)
    {
        for (uint8_t ch = 0; ch < ADC_CHANNELS; ch++)
        {
            for (uint8_t i = 0; i < TOP_N; i++)
            {
                peak[a][ch].val[i] = 0;
                peak[a][ch].pos[i] = 0;
            }
        }
    }

    for (uint16_t dac = 0; dac < 4095; dac++)
    {
        SetDAC_2(dac);

        LL_ADC_REG_StartConversion(ADC1);
        LL_ADC_REG_StartConversion(ADC2);

        while (LL_ADC_REG_IsConversionOngoing(ADC1) ||
               LL_ADC_REG_IsConversionOngoing(ADC2))
        {
        }

        uint16_t adc[ADC_INSTANCES][ADC_CHANNELS];

        adc[0][0] = adc1_buffer[0];
        adc[0][1] = adc1_buffer[1];

        adc[1][0] = adc2_buffer[0];
        adc[1][1] = adc2_buffer[1];

        for (uint8_t a = 0; a < ADC_INSTANCES; a++)
        {
            for (uint8_t ch = 0; ch < ADC_CHANNELS; ch++)
            {
                TopN_Insert(&peak[a][ch], adc[a][ch], dac);
            }
        }

        HAL_Delay(1);
    }

    /* compute averages */
    for (uint8_t a = 0; a < ADC_INSTANCES; a++)
    {
        for (uint8_t ch = 0; ch < ADC_CHANNELS; ch++)
        {
            avg_peak[a][ch] =
                (peak[a][ch].val[0] +
                 peak[a][ch].val[1] +
                 peak[a][ch].val[2] +
                 peak[a][ch].val[3]) >> 2;
        }
    }

    /* find global reference */
    uint32_t ref = 0;

    for (uint8_t a = 0; a < ADC_INSTANCES; a++)
    {
        for (uint8_t ch = 0; ch < ADC_CHANNELS; ch++)
        {
            if (avg_peak[a][ch] > ref)
                ref = avg_peak[a][ch];
        }
    }

    /* compute gains */
    for (uint8_t a = 0; a < ADC_INSTANCES; a++)
    {
        for (uint8_t ch = 0; ch < ADC_CHANNELS; ch++)
        {
            gain[a][ch] = ((uint32_t)ref << 10) / avg_peak[a][ch];
        }
    }

    printf("Calibration End\r\n");
    printf("MAXIMUM: %lu\r\n",ref);
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
