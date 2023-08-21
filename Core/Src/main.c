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
// #include "app_x-cube-ai.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h> // to use printf()
#include <string.h> // to use memcpy()
#include "cooler_nn.h"
#include "cooler_nn_data.h"
#include "iks01a2_motion_sensors.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
	LED_UNINITIALIZED,
	LED_OFF,
	LED_ON
} led_state_t;

#define ACC_SAMPLES (128)
#define ACC_SAMPLES_FLAT (3 * ACC_SAMPLES)
typedef struct {
	int32_t x[ACC_SAMPLES];
	int32_t y[ACC_SAMPLES];
	int32_t z[ACC_SAMPLES];
	int32_t flatten[ACC_SAMPLES_FLAT];
	uint8_t samples_cnt;
	uint8_t buffers_filled;
} acc_data_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BUFFERS_NEEDED (50)
#define CAPTURE_DATA_MODE 1

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define ITM_Port32(n) (*((volatile unsigned long *)(0xE0000000+4*n)))

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc;

UART_HandleTypeDef huart2;

acc_data_t acc_data = {
	.x = {0},
	.y = {0},
	.z = {0},
	.flatten = {0},
	.samples_cnt = 0,
	.buffers_filled = 0,
};

/* USER CODE BEGIN PV */
int counter = 0;
led_state_t led_state = LED_UNINITIALIZED;
GPIO_InitTypeDef gpios = {
	.Pin = GPIO_PIN_5,
	.Mode = GPIO_MODE_OUTPUT_PP, // _OD?
	.Pull = GPIO_NOPULL,
	.Speed = GPIO_SPEED_FREQ_MEDIUM,
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_CRC_Init(void);

/* USER CODE BEGIN PFP */
static void toggle_led_on(void);
static void toggle_led_off(void);
static void led_init(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* COPIED FROM
 * file:///C:/Users/Henrique/STM32Cube/Repository/Packs/STMicroelectronics/X-CUBE-AI/8.1.0/Documentation/embedded_client_api.html
 * BEGIN
 */

/* Global handle to reference the instantiated C-model */
static ai_handle cooler_nn = AI_HANDLE_NULL;

/* Global c-array to handle the activations buffer */
AI_ALIGNED(32)
static ai_u8 activations[AI_COOLER_NN_DATA_ACTIVATIONS_SIZE];

/* Array to store the data of the input tensor */
AI_ALIGNED(32)
static ai_i32 in_data[AI_COOLER_NN_IN_1_SIZE]; // size=384
/* or static ai_u8 in_data[AI_COOLER_NN_IN_1_SIZE_BYTES]; */

/* c-array to store the data of the output tensor */
AI_ALIGNED(32)
static ai_float out_data[AI_COOLER_NN_OUT_1_SIZE]; // size=2
/* static ai_u8 out_data[AI_COOLER_NN_OUT_1_SIZE_BYTES]; */

/* Array of pointer to manage the model's input/output tensors */
static ai_buffer *ai_input;
static ai_buffer *ai_output;

/*
 * Bootstrap
 */
int aiInit(void) {
  ai_error err;

  /* Create and initialize the c-model */
  const ai_handle acts[] = { activations };
  err = ai_cooler_nn_create_and_init(&cooler_nn, acts, NULL);
  if (err.type != AI_ERROR_NONE)
  {
	  printf("Error creating and initializing network (ai_cooler_nn_create_and_init)\r\n");
  }

  /* Retrieve pointers to the model's input/output tensors */
  ai_input = ai_cooler_nn_inputs_get(cooler_nn, NULL);
  ai_output = ai_cooler_nn_outputs_get(cooler_nn, NULL);

  return 0;
}

/*
 * Run inference
 */
int aiRun(const void *in_data, void *out_data) {
  ai_i32 n_batch;
  ai_error err;

  /* 1 - Update IO handlers with the data payload */
  ai_input[0].data = AI_HANDLE_PTR(in_data);
  ai_output[0].data = AI_HANDLE_PTR(out_data);

  /* 2 - Perform the inference */
  n_batch = ai_cooler_nn_run(cooler_nn, &ai_input[0], &ai_output[0]);
  if (n_batch != 1) {
	  err = ai_cooler_nn_get_error(cooler_nn);
      printf("Error running network (ai_cooler_nn_run): err.code=%"PRIu32"\r\n", err.code);
  };

  return 0;
}
/* COPIED FROM
 * file:///C:/Users/Henrique/STM32Cube/Repository/Packs/STMicroelectronics/X-CUBE-AI/8.1.0/Documentation/embedded_client_api.html
 * END
 */

//int dummy = 0;
static void acquire_and_process_data_0(ai_i32 *in_data)
{
	printf("inside acquire_and_process_data_0()\r\n");
/*	ai_float balanced_in_data[] = {6.18482, 6.7943, 3.2689, 3.61665, 5.65359, 6.67005, 4.14315, 2.53531, 2.46291, 2.08165, 1.08845, 1.47574, 1.31225, 1.91796, 1.89142, 1.80857, 2.50883, 3.04302, 6.10126, 11.483, 15.5497, 16.0198, 13.561, 33.1542, 338.898, 1135.62, 891.49, 109.639, 20.2495, 7.49063, 3.51428, 3.84007, 2.39719, 1.82079, 1.76208, 1.96168, 1.61265, 1.07893, 1.21799, 1.84341, 1.07527, 1.15663, 1.50784, 2.09119, 1.87186, 1.40829, 1.22169, 1.32609, 1.86947, 1.45887, 4.77592, 5.6412, 1.92681, 6.16927, 5.5969, 1.97672, 1.25945, 0.848169, 1.10336, 1.46117, 1.54671, 1.27181, 0.614126, 1.19784, 1.6678, 1.30936, 1.70309, 1.77956, 1.94978, 1.12279, 1.54935, 1.36988, 1.07687, 1.5687, 1.79161, 2.79403, 5.17233, 2.941, 7.87023, 11.2608, 4.18702, 1.3587, 1.50813, 1.28973, 1.34841, 1.95224, 1.95753, 1.95546, 1.17236, 1.24915, 1.29134, 1.12929, 1.35094, 1.64719, 1.89014, 2.63823, 2.78076, 1.92639, 1.51213, 1.08821, 0.83912, 1.3183, 2.05485, 2.70797, 2.34265, 1.58861, 3.82429, 4.76799, 2.62857, 1.61344, 1.661, 1.97032, 1.97715, 1.48831, 1.83861, 2.05044, 2.34211, 2.41057, 1.74208, 0.98258, 1.23972, 1.0362, 0.889157, 1.03089, 1.65075, 1.60964, 2.96936, 4.13738, 3.72726, 6.51575, 8.30232, 14.9337, 18.1471, 21.5101, 12.3178, 3.44464, 1.76185, 1.6529, 1.71066, 3.12216, 3.52537, 3.43736, 2.98961, 3.27648, 2.34573, 2.71277, 2.69933, 1.6351, 2.57236, 2.44345, 2.23621, 3.47062, 28.7593, 96.6899, 76.0434, 8.80498, 3.96753, 2.75118, 3.70485, 3.58765, 1.36441, 1.87307, 2.5533, 2.33005, 2.40098, 4.62231, 4.85904, 4.51905, 3.27324, 2.64881, 2.49518, 2.72829, 2.41072, 3.08279, 3.08386, 5.12014, 6.31653, 3.04081, 10.8036, 13.2066, 3.43947, 4.31833, 4.12693, 2.2054, 1.1792, 2.02345, 2.23604, 2.41553, 2.14249, 1.80836, 1.85747, 1.81757, 1.49466, 1.64679, 1.47092, 1.79781, 1.87575, 1.3651, 1.64297, 1.99691, 2.30551, 2.99073, 3.01436, 1.74045, 5.01221, 4.23247, 6.37314, 8.07407, 3.29012, 1.5713, 1.72381, 1.51071, 1.70132, 1.75595, 1.76848, 2.06579, 1.2683, 1.84225, 1.78096, 1.34657, 1.93799, 1.44338, 1.17413, 1.90522, 2.33302, 2.13382, 1.87105, 1.55902, 2.06397, 5.59124, 4.45962, 2.97995, 3.71947, 2.41992, 1.6663, 1.91814, 1.54872, 1.42581, 1.00515, 0.940399, 1.41083, 1.70517, 1.77032, 1.8569, 1.29124, 1.10791, 1.28883, 1.43195, 1.43388, 1.29574, 0.904247, 1.12513, 1.18133, 1.23208, 1.28418, 1.26705, 126.328, 160.963, 53.6186, 20.8359, 10.4748, 9.30982, 8.28412, 8.91881, 12.8541, 10.3365, 5.58753, 3.21508, 2.23636, 2.29081, 2.61632, 2.63412, 2.19788, 1.76955, 1.85646, 3.9175, 6.46511, 7.46549, 5.84768, 13.059, 127.826, 428.364, 335.441, 40.2626, 8.27095, 4.13355, 2.19098, 1.75192, 2.40951, 2.61311, 1.7269, 1.88295, 2.1782, 2.78349, 2.20964, 2.07178, 1.80153, 2.28829, 2.10019, 2.28283, 3.1017, 2.71574, 2.83713, 4.79589, 5.49255, 12.4251, 85.2143, 102.793, 31.3474, 8.09443, 8.9923, 4.82188, 5.80016, 5.62106, 5.76268, 6.21577, 4.85884, 3.19726, 2.41727, 2.70534, 3.05192, 2.79541, 1.94624, 1.76744, 1.71635, 2.16868, 2.3371, 2.86562, 2.11599, 4.99351, 3.68841, 6.54551, 11.1265, 8.02252, 20.58, 29.5628, 12.3767, 2.96576, 2.4117, 2.22828, 2.14877, 2.28036, 2.06182, 2.01116, 2.2495, 2.49757, 1.62594, 1.89005, 2.26568, 2.81633, 3.5496, 4.20582, 4.13049, 4.87911, 4.19652, 3.61051, 5.32947, 17.1177, 12.3888, 7.29922, 15.5132, 9.86398, 5.58601, 4.92638, 3.43368, 3.13617, 2.759, 3.41232, 3.18632, 3.16368, 3.96178, 3.02068, 1.96712, 2.31186, 2.74439, 2.63176, 2.72869, 2.64181, 2.98639, 2.1743, 2.53918, 1.5804, 4.67098, 5.17431};
	ai_float unbalanced_in_data[] = {4.43908, 5.39348, 2.38369, 3.40773, 6.17758, 4.6939, 2.03993, 1.75095, 1.79598, 2.05004, 2.31489, 1.73622, 1.90182, 2.38788, 3.56594, 2.84402, 2.63376, 2.22295, 3.00589, 2.97768, 4.06246, 6.45848, 6.41503, 22.6758, 1178.26, 2185.44, 1024.77, 14.0889, 3.99851, 3.08149, 3.09621, 3.07699, 2.21513, 1.75305, 1.20664, 1.36989, 2.39287, 2.57449, 1.71123, 1.46208, 1.48264, 1.60662, 2.10179, 4.08237, 3.35806, 2.45719, 2.57157, 1.23914, 2.168, 43.1883, 74.4945, 32.2777, 1.38619, 1.68111, 2.65918, 2.98127, 6.25774, 5.31315, 1.82847, 1.72431, 1.05236, 1.79261, 1.78397, 2.42624, 2.09222, 1.55646, 1.5134, 1.72628, 3.08211, 3.32004, 1.69888, 1.15607, 2.23743, 7.18059, 121.739, 196.421, 80.1591, 3.43625, 2.04594, 1.80196, 5.30279, 15.9921, 13.3401, 2.58098, 1.21018, 1.91093, 1.94341, 2.08573, 2.25884, 1.7929, 1.1515, 0.968559, 2.0453, 4.90217, 4.63676, 2.24154, 2.74525, 1.50651, 2.26009, 25.4889, 39.2134, 15.9371, 1.30198, 1.94838, 1.59723, 9.08407, 22.9908, 17.2595, 2.82612, 1.48597, 1.45287, 2.01317, 3.24388, 4.01321, 2.36818, 1.23216, 2.00989, 2.98063, 3.4935, 2.79545, 1.7123, 1.63473, 1.42375, 6.74872, 61.8793, 87.4323, 31.6073, 2.30018, 3.82871, 4.51674, 3.77239, 15.8305, 28.1123, 18.0262, 4.55615, 2.44111, 1.54602, 1.45576, 1.52588, 1.6883, 1.83354, 1.66157, 4.04665, 4.07361, 2.98445, 2.52123, 1.43179, 1.73454, 3.82861, 6.02878, 3.60635, 4.77249, 195.863, 362.64, 170.299, 2.54506, 4.91445, 7.77002, 5.78719, 3.59369, 3.30616, 3.29633, 3.44312, 3.23091, 3.20378, 3.07707, 2.57964, 2.72041, 1.78114, 1.47587, 2.20742, 5.17482, 4.84564, 3.04223, 2.93071, 2.10032, 2.63793, 50.9466, 87.5493, 38.0944, 1.65012, 1.57305, 2.38073, 1.81495, 4.27231, 4.11821, 2.0192, 1.34573, 1.08161, 1.51168, 2.82247, 2.99955, 1.97695, 1.68905, 2.35827, 2.9876, 3.53247, 3.30143, 1.6742, 1.24358, 1.7219, 1.67938, 23.2946, 38.3687, 15.6257, 1.89668, 1.9767, 1.83991, 3.80786, 10.9091, 8.92, 2.18801, 1.17348, 1.25827, 2.0207, 2.69807, 3.42125, 2.26936, 0.584315, 1.83101, 2.94327, 4.89395, 4.23552, 1.80949, 1.17513, 1.37108, 1.10575, 3.23094, 4.62863, 2.04291, 1.39006, 0.971742, 1.17481, 2.8307, 8.0745, 6.06649, 1.53284, 1.03846, 1.18597, 1.61358, 2.04003, 2.28616, 1.73767, 1.44346, 1.27123, 1.83388, 1.81088, 1.83275, 1.704, 1.55957, 1.76227, 2.29281, 22.8349, 32.9401, 12.194, 1.41765, 126.872, 161.557, 53.5769, 20.9589, 12.6281, 8.2301, 5.15819, 3.19098, 3.85604, 2.90339, 2.83479, 1.78993, 1.26519, 1.22528, 1.64748, 1.21613, 0.980848, 1.8847, 4.63242, 4.09351, 2.95325, 2.03869, 2.07246, 6.69528, 348.132, 645.459, 302.845, 5.19256, 3.22704, 4.02428, 6.29848, 16.6674, 14.6649, 3.8881, 1.81155, 2.04806, 3.20056, 2.26002, 2.93865, 3.35866, 4.4141, 4.14317, 3.63891, 8.6413, 7.62239, 4.3637, 4.87299, 5.32941, 16.1513, 421.507, 731.429, 320.217, 10.2672, 4.74648, 6.83319, 8.76644, 23.6119, 21.2972, 6.89117, 3.07046, 3.22688, 3.58619, 5.61386, 5.38431, 4.13723, 3.47973, 3.56095, 5.63641, 12.3775, 9.76617, 5.01447, 2.79742, 5.2432, 14.8613, 272.16, 442.206, 181.341, 8.04965, 4.277, 3.82692, 45.7647, 140.687, 113.886, 17.8564, 4.17219, 3.91272, 5.07358, 7.4478, 8.3823, 4.99598, 3.47877, 4.11836, 7.88918, 13.4019, 11.0222, 5.26498, 3.78587, 3.70573, 5.3297, 34.1881, 50.1476, 18.0836, 5.45822, 4.53221, 5.21736, 37.238, 115.19, 88.0518, 13.1814, 3.68676, 5.4219, 5.75395, 5.72653, 5.91719, 5.35485, 2.95848, 4.07134, 6.46289, 9.53541, 8.1508, 5.44877, 3.27724, 4.16159, 14.4339, 138.732, 196.296, 70.0687, 3.67516};
	if (dummy == 0)
	{
		dummy = 1;
		memcpy(in_data, balanced_in_data, sizeof(balanced_in_data));
//		in_data = &balanced_in_data[0];
	}
	else
	{
		dummy = 0;
		memcpy(in_data, unbalanced_in_data, sizeof(balanced_in_data));
//		in_data = &unbalanced_in_data[0];
	}*/

	for (int i = 0; i < AI_COOLER_NN_IN_1_SIZE; i++)
	{
		in_data[i] = acc_data.flatten[i];
	}
}

static void post_process_0(ai_float *out_data)
{
	printf("inside post_process_0()\r\n");
	if (out_data[0] > out_data[1])
	{
		printf("result: balanced (out_data[0]=%.2f, out_data[1]=%.2f)\r\n", out_data[0], out_data[1]);
		toggle_led_off();
	}
	else
	{
		printf("result: unbalanced (out_data[0]=%.2f, out_data[1]=%.2f)\r\n", out_data[0], out_data[1]);
		toggle_led_on();
	}
}

static void toggle_led_off(void)
{
	if (led_state == LED_ON || led_state == LED_UNINITIALIZED)
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
		led_state = LED_OFF;
	}
}

static void toggle_led_on(void)
{
	if (led_state == LED_OFF || led_state == LED_UNINITIALIZED)
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
		led_state = LED_ON;
	}
}

static void led_init(void)
{
	HAL_GPIO_Init(GPIOA, &gpios);
	toggle_led_off();
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
  led_init();
  IKS01A2_MOTION_SENSOR_Init(IKS01A2_LSM6DSL_0, MOTION_ACCELERO);
  IKS01A2_MOTION_SENSOR_Enable(IKS01A2_LSM6DSL_0, MOTION_ACCELERO);
  IKS01A2_MOTION_SENSOR_Axes_t my_axes = {
		  .x = 0,
		  .y = 0,
		  .z = 0,
  };
  int32_t ret;
/*  ret = IKS01A2_MOTION_SENSOR_GetAxes(IKS01A2_LSM6DSL_0, MOTION_ACCELERO, &my_axes);
  printf("x=%"PRId32", y=%"PRId32", z=%"PRId32", ret=%"PRId32"\r\n", my_axes.x, my_axes.y, my_axes.z, ret);*/

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_CRC_Init(); // this line calls __HAL_RCC_CRC_CLK_ENABLE()
  aiInit();

  // MX_X_CUBE_AI_Init();
  /* USER CODE BEGIN 2 */
  printf(" ");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
/*	  printf("counter=%d\r\n", counter);
	  if (counter == 255)
		  counter = 0;
	  else
		  counter++;*/
//	  HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);

	  ret = IKS01A2_MOTION_SENSOR_GetAxes(IKS01A2_LSM6DSL_0, MOTION_ACCELERO, &my_axes);
//	  printf("x=%"PRId32", y=%"PRId32", z=%"PRId32", ret=%"PRId32"\r\n", my_axes.x, my_axes.y, my_axes.z, ret);
/*	  acc_data.x[acc_data.samples_cnt] = my_axes.x;
	  acc_data.y[acc_data.samples_cnt] = my_axes.y;
	  acc_data.z[acc_data.samples_cnt] = my_axes.z;*/
	  acc_data.flatten[acc_data.samples_cnt] = my_axes.x;
	  acc_data.flatten[acc_data.samples_cnt + ACC_SAMPLES] = my_axes.y;
	  acc_data.flatten[acc_data.samples_cnt + (2 * ACC_SAMPLES)] = my_axes.z;
	  acc_data.samples_cnt++;
	  if (acc_data.samples_cnt == ACC_SAMPLES)
	  {
		  acc_data.samples_cnt = 0;
#if CAPTURE_DATA_MODE
		  for (uint16_t i = 0; i < ACC_SAMPLES_FLAT; i++)
		  {
			  if (i != ACC_SAMPLES_FLAT - 1)
				  printf("%"PRId32",", acc_data.flatten[i]);
			  else
				  printf("%"PRId32"\r\n", acc_data.flatten[i]);
			  HAL_Delay(1);
		  }
		  acc_data.buffers_filled++;
		  if (acc_data.buffers_filled == BUFFERS_NEEDED)
		  {
			  printf("End of program\r\n");
		  	  return 0;
		  }
#endif
	  }
#if !CAPTURE_DATA_MODE
	  /* 1 - Acquire, pre-process and fill the input buffers */
	  acquire_and_process_data_0(in_data);

	  /* 2 - Call inference engine */
	  aiRun(in_data, out_data);

	  /* 3 - Post-process the predictions */
	  post_process_0(out_data);

	  /* USER CODE END WHILE */



	  // MX_X_CUBE_AI_Process();
	  /* USER CODE BEGIN 3 */
#endif
	  HAL_Delay(1);
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
int _write(int file, char *ptr, int len)
{
	ITM_Port32(31) = 1;
	int DataIdx;
	for (DataIdx = 0; DataIdx < len; DataIdx++)
	{
		ITM_SendChar(*ptr++);
//		HAL_Delay(5);
	}
	ITM_Port32(31) = 2;
	return len;
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
