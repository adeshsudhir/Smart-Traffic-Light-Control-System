/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
/* Includes ------------------------------------------------------------------*/

#include "main.h"
#include "tim.h"
#include "gpio.h"

#include "string.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "lcd.h"
#include "tm1637.h"
#include "traffic_light.h"
#include "stdio.h"
#include "hcsr04.h"

// Keypad keymap

const char keymap[4][4] = {
	    {'1', '2', '3', 'A'},
	    {'4', '5', '6', 'B'},
	    {'7', '8', '9', 'C'},
	    {'*', '0', '#', 'D'}
	};

// Simple delay
void delayms(uint32_t dly)
{
  uint32_t i,j=0;
  for(i=0;i<dly;i++)
  for(j=0;j<16000;j++);
}

// Keypad Scan function
char scan_keypad(void) {
    for (int row = 0; row < 4; row++) {
        GPIOC->ODR |= 0x0F;              // Set all rows HIGH
        GPIOC->ODR &= ~(1 << row);       // Pull current row LOW

        for (volatile int d = 0; d < 1000; d++); // short delay

        for (int col = 0; col < 4; col++) {
            if ((GPIOC->IDR & (1 << (col + 4))) == 0) {
                return keymap[row][col]; // Return mapped key
            }
        }
    }
    return 0; // No key pressed
}

uint8_t lane1_cong, lane2_cong;


// Display1 Display 2 Pin assignment
TM1637_Display display1 = {
        .clk_port = GPIOA,
        .clk_pin = GPIO_PIN_4,
        .dio_port = GPIOA,
        .dio_pin = GPIO_PIN_5,
        .brightness = 0x07
    };

TM1637_Display display2 = {
        .clk_port = GPIOA,
        .clk_pin = GPIO_PIN_6,
        .dio_port = GPIOA,
        .dio_pin = GPIO_PIN_7,
        .brightness = 0x07
    };
/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void setup_sensors(void);
uint8_t traffic_congetion(uint8_t *lane1_cong, uint8_t *lane2_cong);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/


// Main traffic light task
void TrafficLightTask(void *pvParameters)
{
    int green_duration1, green_duration2, yellow_duration, red_duration1, red_duration2;
    char buf1[5], buf2[5];


    while (1)
    {
        // Check congestion before each cycle
    	lane1_cong = 0;
    	lane2_cong = 0;
        traffic_congetion(&lane1_cong, &lane2_cong);

        // Adjust timings based on congestion
        if (lane1_cong && !lane2_cong)
        {
            green_duration1 = 20; // Extend Lane 1 green
            green_duration2 = 8;  // Shorten Lane 2 green
            red_duration1 = 11;   // Lane 1 red = Lane 2 green + yellow
            red_duration2 = 23;   // Lane 2 red = Lane 1 green + yellow
            yellow_duration = 3;
        }
        else if (!lane1_cong && lane2_cong)
        {
            green_duration1 = 8;  // Shorten Lane 1 green
            green_duration2 = 20; // Extend Lane 2 green
            red_duration1 = 23;   // Lane 1 red = Lane 2 green + yellow
            red_duration2 = 11;   // Lane 2 red = Lane 1 green + yellow
            yellow_duration = 3;
        }
        else if (lane1_cong && lane2_cong)
        {
            green_duration1 = 15; // Moderate green for both
            green_duration2 = 15;
            red_duration1 = 18;   // Lane 1 red = Lane 2 green + yellow
            red_duration2 = 18;   // Lane 2 red = Lane 1 green + yellow
            yellow_duration = 3;
        }
        else
        {
            green_duration1 = 12; // Default timings
            green_duration2 = 12;
            red_duration1 = 15;   // Lane 1 red = Lane 2 green + yellow
            red_duration2 = 15;   // Lane 2 red = Lane 1 green + yellow
            yellow_duration = 3;
        }

        // Phase 1: Lane 1 green, Lane 2 red
        HAL_GPIO_WritePin(LANE1_GREEN_PORT, LANE1_GREEN_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LANE1_RED_PORT, LANE1_RED_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LANE1_YELLOW_PORT, LANE1_YELLOW_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LANE2_RED_PORT, LANE2_RED_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LANE2_GREEN_PORT, LANE2_GREEN_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LANE2_YELLOW_PORT, LANE2_YELLOW_PIN, GPIO_PIN_RESET);
        for (int i = 0; i < green_duration1; i++)
        {
            sprintf(buf1, "%04d", green_duration1 - i);
            sprintf(buf2, "%04d", red_duration2 - i);
            TM1637_DisplayString(&display1, buf1);
            TM1637_DisplayString(&display2, buf2);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        // Phase 2: Lane 1 yellow, Lane 2 red
        HAL_GPIO_WritePin(LANE1_GREEN_PORT, LANE1_GREEN_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LANE1_YELLOW_PORT, LANE1_YELLOW_PIN, GPIO_PIN_SET);
        for (int i = 0; i < yellow_duration; i++)
        {
            sprintf(buf1, "%04d", yellow_duration - i);
            sprintf(buf2, "%04d", red_duration2 - green_duration1 - i);
            TM1637_DisplayString(&display1, buf1);
            TM1637_DisplayString(&display2, buf2);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        // Transition directly to Lane 2 green
        HAL_GPIO_WritePin(LANE1_YELLOW_PORT, LANE1_YELLOW_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LANE1_RED_PORT, LANE1_RED_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LANE2_RED_PORT, LANE2_RED_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LANE2_GREEN_PORT, LANE2_GREEN_PIN, GPIO_PIN_SET);

        // Phase 3: Lane 1 red, Lane 2 green
        for (int i = 0; i < green_duration2; i++)
        {
            sprintf(buf1, "%04d", red_duration1 - i);
            sprintf(buf2, "%04d", green_duration2 - i);
            TM1637_DisplayString(&display1, buf1);
            TM1637_DisplayString(&display2, buf2);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        // Phase 4: Lane 2 yellow, Lane 1 red
        HAL_GPIO_WritePin(LANE2_GREEN_PORT, LANE2_GREEN_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LANE2_YELLOW_PORT, LANE2_YELLOW_PIN, GPIO_PIN_SET);
        for (int i = 0; i < yellow_duration; i++)
        {
            sprintf(buf1, "%04d", red_duration1 - green_duration2 - i);
            sprintf(buf2, "%04d", yellow_duration - i);
            TM1637_DisplayString(&display1, buf1);
            TM1637_DisplayString(&display2, buf2);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        // Transition directly to Lane 1 green
        HAL_GPIO_WritePin(LANE2_YELLOW_PORT, LANE2_YELLOW_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LANE2_RED_PORT, LANE2_RED_PIN, GPIO_PIN_SET);
    }
}

// Task to check the password
void password_task(void *pv)
{
	char passkey[5] = {0};
	const char password[] = "1234";

	LcdFxn(0, 0x01); // Clear screen
	lprint(0x80, "Welcome");

	while (1) {
		char key = scan_keypad();
		if (key == '*') {
			LcdFxn(0, 0x01);
			lprint(0x80, "Enter PIN:");
			memset(passkey, 0, sizeof(passkey));
			int i = 0;

			while (1) {
				key = scan_keypad();
				if (key) {
					if (key == '#') {
						passkey[i] = '\0';
						LcdFxn(0, 0x01);

						if (strcmp(passkey, password) == 0) {
							lprint(0x80, "Access Granted");
							HAL_GPIO_WritePin(LANE1_RED_PORT, LANE1_RED_PIN, GPIO_PIN_SET);
							HAL_GPIO_WritePin(LANE2_RED_PORT, LANE2_RED_PIN, GPIO_PIN_SET);
							vTaskDelay(pdMS_TO_TICKS(10000));
							HAL_GPIO_WritePin(LANE1_RED_PORT, LANE1_RED_PIN, GPIO_PIN_RESET);
							HAL_GPIO_WritePin(LANE2_RED_PORT, LANE2_RED_PIN, GPIO_PIN_RESET);
							xTaskCreate(TrafficLightTask, "Traffic Light", 128, NULL, 1, NULL);

						} else {
							lprint(0x80, "Access Denied");
						}

						vTaskDelay(pdMS_TO_TICKS(1000));
						LcdFxn(0, 0x01);
						lprint(0x80, "Welcome");
						break;
					}

					if (i < 4) {
						passkey[i++] = key;
						lprint(0xC0 + i - 1, "*");
					}
					while (scan_keypad()); // Wait for release
					vTaskDelay(pdMS_TO_TICKS(100)); // Debounce
				}

				vTaskDelay(pdMS_TO_TICKS(10));
			}
		}

		vTaskDelay(pdMS_TO_TICKS(50)); // Polling delay
	}
}


// test function to observe the working of sensor

float a, b, c, d;

void testing(void *pv)
{
	while(1)
	{
		a = HCSR04_Read(&hcsr04_sensors[0]);
		b = HCSR04_Read(&hcsr04_sensors[1]);
		c = HCSR04_Read(&hcsr04_sensors[2]);
		d = HCSR04_Read(&hcsr04_sensors[3]);
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
  LcdInit();
  TM1637_Init(&display1);
  TM1637_Init(&display2);
  TM1637_DisplayDecimal(&display1, 0);
  TM1637_DisplayDecimal(&display2, 0);
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  // 10 second Red Toggle
//  for(uint8_t i = 0; i < 10; i++)
//  {
//	  HAL_GPIO_TogglePin(LANE1_RED_PORT, LANE1_RED_PIN);
//	  HAL_GPIO_TogglePin(LANE2_RED_PORT, LANE2_RED_PIN);
//	  HAL_Delay(10000);
//  }

  // Ultra sonic sensor setup
  setup_sensors();

  xTaskCreate(password_task, "password", 128, NULL, 1, NULL);
  xTaskCreate(testing, "test", 100, NULL, 1, NULL);
  vTaskStartScheduler();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void setup_sensors(void)
{
    extern TIM_HandleTypeDef htim2;

    HCSR04_InitSensor(&hcsr04_sensors[0], &htim2, L1_ULTR_A_Trig_GPIO_Port, L1_ULTR_A_Trig_Pin, L1_ULTR_A_Echo_GPIO_Port, L1_ULTR_A_Echo_Pin);
    HCSR04_InitSensor(&hcsr04_sensors[1], &htim2, L1_ULTR_B_Trig_GPIO_Port, L1_ULTR_B_Trig_Pin, L1_ULTR_B_Echo_GPIO_Port, L1_ULTR_B_Echo_Pin);
    HCSR04_InitSensor(&hcsr04_sensors[2], &htim2, L2_ULTR_A_Trig_GPIO_Port, L2_ULTR_A_Trig_Pin, L2_ULTR_A_Echo_GPIO_Port, L2_ULTR_A_Echo_Pin);
    HCSR04_InitSensor(&hcsr04_sensors[3], &htim2, L2_ULTR_B_Trig_GPIO_Port, L2_ULTR_B_Trig_Pin, L2_ULTR_B_Echo_GPIO_Port, L2_ULTR_B_Echo_Pin);
}

uint8_t traffic_congetion(uint8_t *lane1_cong, uint8_t *lane2_cong)
{
    float lane1_a = HCSR04_Read(&hcsr04_sensors[0]);
    float lane1_b = HCSR04_Read(&hcsr04_sensors[1]);
    float lane2_a = HCSR04_Read(&hcsr04_sensors[2]);
    float lane2_b = HCSR04_Read(&hcsr04_sensors[3]);

    // Threshold at 15 cm for reliable vehicle detection
    *lane1_cong = ((lane1_a < 15 && lane1_a > 0) && (lane1_b < 15 && lane1_b > 0)) ? 1 : 0;
    *lane2_cong = ((lane2_a < 15 && lane2_a > 0) && (lane2_b < 15 && lane2_b > 0)) ? 1 : 0;

    return (*lane1_cong || *lane2_cong) ? 1 : 0;
}

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM3 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM3) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
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
