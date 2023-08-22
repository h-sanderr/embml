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
#include "cooler.h"
#include "cooler_data.h"
#include "iks01a2_motion_sensors.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
	LED_UNINITIALIZED,
	LED_OFF,
	LED_ON
} led_state_t;

#define ACC_SAMPLES (512)
#define ACC_SAMPLES_FLAT (3 * ACC_SAMPLES)
typedef struct {
	int16_t x[ACC_SAMPLES];
	int16_t y[ACC_SAMPLES];
	int16_t z[ACC_SAMPLES];
	int16_t flatten[ACC_SAMPLES_FLAT];
	uint16_t samples_cnt;
	uint16_t buffers_filled;
} acc_data_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BUFFERS_NEEDED (50)
#define CAPTURE_DATA_MODE 0

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define ITM_Port32(n) (*((volatile unsigned long *)(0xE0000000+4*n)))

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

int counter = 0;
led_state_t led_state = LED_UNINITIALIZED;
GPIO_InitTypeDef gpios = {
	.Pin = GPIO_PIN_5,
	.Mode = GPIO_MODE_OUTPUT_PP, // _OD?
	.Pull = GPIO_NOPULL,
	.Speed = GPIO_SPEED_FREQ_MEDIUM,
};
acc_data_t acc_data = {
	.x = {0},
	.y = {0},
	.z = {0},
	.flatten = {0},
	.samples_cnt = 0,
	.buffers_filled = 0,
};


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CRC_Init(void);
static void MX_USART2_UART_Init(void);
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
static ai_handle cooler = AI_HANDLE_NULL;

/* Global c-array to handle the activations buffer */
AI_ALIGNED(32)
static ai_u8 activations[AI_COOLER_DATA_ACTIVATIONS_SIZE];

/* Array to store the data of the input tensor */
AI_ALIGNED(32)
static ai_i16 in_data[AI_COOLER_IN_1_SIZE]; // size=384
/* or static ai_u8 in_data[AI_COOLER_IN_1_SIZE_BYTES]; */

/* c-array to store the data of the output tensor */
AI_ALIGNED(32)
static ai_float out_data[AI_COOLER_OUT_1_SIZE]; // size=2
/* static ai_u8 out_data[AI_COOLER_OUT_1_SIZE_BYTES]; */

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
  err = ai_cooler_create_and_init(&cooler, acts, NULL);
  if (err.type != AI_ERROR_NONE)
  {
	  printf("Error creating and initializing network (ai_cooler_create_and_init)\r\n");
  }

  /* Retrieve pointers to the model's input/output tensors */
  ai_input = ai_cooler_inputs_get(cooler, NULL);
  ai_output = ai_cooler_outputs_get(cooler, NULL);

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
  n_batch = ai_cooler_run(cooler, &ai_input[0], &ai_output[0]);
  if (n_batch != 1) {
	  err = ai_cooler_get_error(cooler);
      printf("Error running network (ai_cooler_run): err.type=%d err.code=%d\r\n", err.type, err.code);
  };

  return 0;
}
/* COPIED FROM
 * file:///C:/Users/Henrique/STM32Cube/Repository/Packs/STMicroelectronics/X-CUBE-AI/8.1.0/Documentation/embedded_client_api.html
 * END
 */

int dummy = 0;
static void acquire_and_process_data_0(ai_i16 *in_data)
{
	//printf("inside acquire_and_process_data_0()\r\n");
/*	ai_i16 balanced_in_data[] = {-201,-201,-222,-222,-222,-113,-113,-113,-7,-7,-7,-7,-111,-111,-111,-239,-239,-239,-218,-218,-218,-89,-89,-89,-16,-16,-16,-207,-207,-207,-207,-185,-185,-185,-92,-92,-92,17,17,17,-92,-92,-92,-237,-237,-237,-237,-232,-232,-232,-87,-87,-87,-22,-22,-22,-194,-194,-194,-209,-209,-209,-209,-63,-63,-63,-43,-43,-43,-117,-117,-117,-258,-258,-258,-239,-239,-239,-75,-75,-75,-75,-13,-13,-13,-210,-210,-210,-171,-171,-171,-75,-75,-75,-14,-14,-14,-14,-93,-93,-93,-187,-187,-187,-210,-210,-210,13,13,13,-66,-66,-66,-172,-172,-172,-172,-193,-193,-193,-195,-195,-195,-101,-101,-101,-197,-197,-197,-252,-252,-252,-252,-227,-227,-227,-27,-27,-27,-54,-54,-54,-66,-66,-66,-86,-86,-86,-86,-65,-65,-65,-42,-42,-42,-89,-89,-89,-222,-222,-222,-274,-274,-274,-114,-114,-114,-114,-152,-152,-152,-99,-99,-99,-245,-245,-245,-168,-168,-168,-105,-105,-105,-105,-65,-65,-65,-214,-214,-214,-215,-215,-215,-121,-121,-121,1,1,1,1,22,22,22,-269,-269,-269,-107,-107,-107,-138,-138,-138,-127,-127,-127,-170,-170,-170,-170,-224,-224,-224,-137,-137,-137,5,5,5,-62,-62,-62,-226,-226,-226,-226,-189,-189,-189,-136,-136,-136,-11,-11,-11,-203,-203,-203,-213,-213,-213,-69,-69,-69,-69,-7,-7,-7,-80,-80,-80,-278,-278,-278,-205,-205,-205,-106,-106,-106,-106,-27,-27,-27,-199,-199,-199,-236,-236,-236,-110,-110,-110,-9,-9,-9,-9,-105,-105,-105,-237,-237,-237,-196,-196,-196,-87,-87,-87,16,16,16,-212,-212,-212,-212,-174,-174,-174,-68,-68,-68,-58,-58,-58,-128,-128,-128,-233,-233,-233,-233,-250,-250,-250,-43,-43,-43,-57,-57,-57,-213,-213,-213,-186,-186,-186,-111,-111,-111,-111,-34,-34,-34,-109,-109,-109,-227,-227,-227,-220,-220,-220,-50,-50,-50,-50,-83,-83,-83,-146,-146,-146,-180,-180,-180,-124,-124,-124,-44,-44,-44,-44,-136,-136,-136,-210,-210,-210,-234,-234,-234,-49,-49,-49,-116,-116,-116,-111,-111,-111,-111,-130,-130,-130,-131,-131,-131,-57,-57,-57,-88,-88,-88,-206,-206,-206,-206,-209,-209,-209,-54,-54,-54,-112,-112,-112,-65,-65,-65,-237,-237,-237,-204,-204,-204,-204,-92,-92,-92,-110,-110,-110,-181,-181,-181,-224,-224,-224,-92,-92,-92,-92,-38,-38,-38,9,9,9,-208,-208,-208,-126,-126,-126,-113,-113,-113,-113,-53,-53,-53,-210,-210,-210,-244,-244,-244,-138,-138,-138,-21,-21,-21,-77,-77,-77,-77,-259,-259,-259,-187,-187,-187,-95,-95,-95,-16,-16,-16,-171,-171,-171,-171,-193,-193,-193,-114,-114,-114,33,33,33,-122,-122,-122,-272,-272,-272,-209,-209,-209,-209,-190,-190,56,56,56,-6,-6,-6,-139,-139,-139,-139,-195,-195,-195,-136,-136,-136,-2,-2,-2,-51,-51,-51,-280,-280,-280,-229,-229,-229,-229,36,36,36,58,58,58,-122,-122,-122,-194,-194,-194,-124,-124,-124,-124,8,8,8,-44,-44,-44,-270,-270,-270,-229,-229,-229,34,34,34,34,61,61,61,-118,-118,-118,-180,-180,-180,-143,-143,-143,18,18,18,-43,-43,-43,-43,-255,-255,-255,-224,-224,-224,44,44,44,20,20,20,-83,-83,-83,-83,-212,-212,-212,-146,-146,-146,2,2,2,-12,-12,-12,-220,-220,-220,-238,-238,-238,-238,-20,-20,-20,-48,-48,-48,-167,-167,-167,-286,-286,-286,-272,-272,-272,-272,-46,-46,-46,53,53,53,-159,-159,-159,-80,-80,-80,174,174,174,174,127,127,127,40,40,40,-192,-192,-192,-270,-270,-270,-79,-79,-79,-102,-102,-102,-102,-333,-333,-333,-235,-235,-235,-61,-61,-61,-24,-24,-24,-29,-29,-29,-29,-220,-220,-220,-243,-243,-243,-1,-1,-1,44,44,44,-187,-187,-187,-187,-116,-116,-116,79,79,79,0,0,0,58,58,58,-201,-201,-201,-303,-303,-303,-303,-6,-6,-6,-74,-74,-74,-262,-262,-262,-206,-206,-206,-91,-91,-91,-91,-8,-8,-8,8,8,8,-248,-248,-248,-217,-217,-217,13,13,13,-25,-25,-25,-25,-159,-159,-159,-202,-202,-202,-81,-81,-81,51,51,51,-3,-3,-3,-3,-266,-266,-266,-238,-238,-238,0,0,0,6,6,6,-110,-110,-110,-110,-204,-204,-204,-123,-123,-123,48,48,48,-10,-10,-10,-277,-277,-277,-222,-222,-222,-222,12,12,12,15,15,15,-145,-145,-145,-207,-207,-207,-157,-157,-157,-157,-1,-1,-1,-40,-40,-40,-267,-267,-267,-235,-235,-235,51,51,51,37,37,37,37,-80,-80,-80,-157,-157,-157,-105,-105,-105,-6,-6,-6,5,5,5,5,-238,-238,-238,-254,-254,-254,44,44,44,-21,-21,-21,-66,-66,-66,-66,-221,-221,-221,-203,-203,-203,-49,-49,-49,-36,-36,-36,-251,-251,-251,-205,-205,-205,-205,13,13,13,12,12,12,7,7,7,-176,-176,-176,-177,-177,-177,-177,36,36,36,24,24,24,-251,-251,-251,-206,-206,-206,-50,-50,-50,-15,-15,-15,-15,-56,-56,-56,-217,-217,-217,-249,-249,-249,45,45,45,17,17,17,17,-171,-171,-171,-147,-147,-147,-34,-34,-34,2,2,2,-25,-25,-25,-25,-264,-264,-264,-283,-283,-283,-22,-22,-22,-60,-60,-60,-198,-198,-198,-214,-214,-214,-214,-92,-92,-92,54,54,54,41,41,41,-192,-192,-192,-178,-178,-178,-178,53,53,53,4,4,4,-196,-196,-196,-249,-249,-249,-159,-159,-159,-19,-19,-19,-19,16452,16452,16263,16263,16263,16232,16232,16232,16322,16322,16322,16322,16338,16338,16338,16451,16451,16451,16302,16302,16302,16308,16308,16308,16239,16239,16239,16474,16474,16474,16474,16186,16186,16186,16282,16282,16282,16332,16332,16332,16382,16382,16382,16380,16380,16380,16380,16256,16256,16256,16325,16325,16325,16312,16312,16312,16430,16430,16430,16129,16129,16129,16305,16305,16305,16305,16324,16324,16324,16410,16410,16410,16328,16328,16328,16305,16305,16305,16362,16362,16362,16362,16367,16367,16367,16343,16343,16343,16122,16122,16122,16417,16417,16417,16267,16267,16267,16267,16435,16435,16435,16275,16275,16275,16374,16374,16374,16322,16322,16322,16376,16376,16376,16205,16205,16205,16205,16208,16208,16208,16437,16437,16437,16287,16287,16287,16366,16366,16366,16280,16280,16280,16280,16404,16404,16404,16255,16255,16255,16423,16423,16423,16116,16116,16116,16333,16333,16333,16333,16402,16402,16402,16267,16267,16267,16337,16337,16337,16292,16292,16292,16466,16466,16466,16254,16254,16254,16254,16372,16372,16372,16139,16139,16139,16422,16422,16422,16375,16375,16375,16310,16310,16310,16310,16246,16246,16246,16364,16364,16364,16362,16362,16362,16269,16269,16269,16390,16390,16390,16390,16189,16189,16189,16503,16503,16503,16357,16357,16357,16248,16248,16248,16145,16145,16145,16464,16464,16464,16464,16360,16360,16360,16237,16237,16237,16422,16422,16422,16353,16353,16353,16374,16374,16374,16374,16268,16268,16268,16308,16308,16308,16149,16149,16149,16448,16448,16448,16355,16355,16355,16260,16260,16260,16260,16375,16375,16375,16342,16342,16342,16372,16372,16372,16216,16216,16216,16287,16287,16287,16287,16263,16263,16263,16482,16482,16482,16223,16223,16223,16319,16319,16319,16309,16309,16309,16309,16377,16377,16377,16380,16380,16380,16206,16206,16206,16324,16324,16324,16324,16324,16324,16468,16468,16468,16468,16177,16177,16177,16399,16399,16399,16270,16270,16270,16346,16346,16346,16282,16282,16282,16282,16301,16301,16301,16302,16302,16302,16415,16415,16415,16377,16377,16377,16084,16084,16084,16445,16445,16445,16445,16280,16280,16280,16393,16393,16393,16273,16273,16273,16389,16389,16389,16275,16275,16275,16275,16414,16414,16414,16213,16213,16213,16226,16226,16226,16429,16429,16429,16283,16283,16283,16283,16400,16400,16400,16245,16245,16245,16449,16449,16449,16256,16256,16256,16383,16383,16383,16112,16112,16112,16112,16396,16396,16396,16408,16408,16408,16321,16321,16321,16317,16317,16317,16305,16305,16305,16305,16455,16455,16455,16179,16179,16179,16416,16416,16416,16129,16129,16129,16461,16461,16461,16373,16373,16373,16373,16328,16328,16328,16259,16259,16259,16378,16378,16378,16367,16367,16367,16230,16230,16230,16230,16300,16300,16300,16295,16295,16295,16477,16477,16477,16295,16295,16295,16363,16363,16363,16177,16177,16177,16177,16465,16465,16465,16259,16259,16259,16239,16239,16239,16360,16360,16360,16278,16278,16278,16278,16487,16487,16487,16240,16240,16240,16329,16329,16329,16231,16231,16231,16479,16479,16479,16479,16204,16204,16204,16266,16266,16266,16328,16328,16328,16370,16370,16370,16405,16405,16405,16281,16281,16281,16281};
	ai_i16 unbalanced_in_data[] = {1097,7601,7601,7601,2838,2838,2838,-8137,-8137,-8137,-5194,-5194,-5194,6059,6059,6059,7955,7955,7955,7955,-5477,-5477,-5477,-7724,-7724,-7724,1671,1671,1671,7689,7689,7689,2165,2165,2165,2165,-8091,-8091,-8091,-4804,-4804,-4804,6244,6244,6244,7820,7820,7820,-5873,-5873,-5873,-7682,-7682,-7682,-7682,2022,2022,2022,7734,7734,7734,1658,1658,1658,-8074,-8074,-8074,-4409,-4409,-4409,-4409,6429,6429,6429,7588,7588,7588,-6345,-6345,-6345,-7617,-7617,-7617,2630,2630,2630,2630,7823,7823,7823,908,908,908,-8002,-8002,-8002,-4022,-4022,-4022,6591,6591,6591,7370,7370,7370,7370,-6662,-6662,-6662,-7561,-7561,-7561,3066,3066,3066,7901,7901,7901,232,232,232,232,-7911,-7911,-7911,-3515,-3515,-3515,6768,6768,6768,7024,7024,7024,-7043,-7043,-7043,-7043,-7464,-7464,-7464,3574,3574,3574,7973,7973,7973,-438,-438,-438,-7851,-7851,-7851,-3120,-3120,-3120,-3120,6884,6884,6884,6745,6745,6745,-7280,-7280,-7280,-7369,-7369,-7369,3945,3945,3945,3945,8050,8050,8050,-1142,-1142,-1142,-7808,-7808,-7808,-2491,-2491,-2491,7054,7054,7054,7054,6299,6299,6299,-7559,-7559,-7559,-7202,-7202,-7202,4351,4351,4351,8117,8117,8117,-1715,-1715,-1715,-1715,-7776,-7776,-7776,-2080,-2080,-2080,7171,7171,7171,5877,5877,5877,-7748,-7748,-7748,-7748,-6931,-6931,-6931,4733,4733,4733,8206,8206,8206,-2529,-2529,-2529,-7747,-7747,-7747,-7747,-1410,-1410,-1410,7299,7299,7299,5349,5349,5349,-7907,-7907,-7907,-6730,-6730,-6730,5020,5020,5020,5020,8248,8248,8248,-3122,-3122,-3122,-7745,-7745,-7745,-874,-874,-874,7375,7375,7375,7375,4783,4783,4783,-8018,-8018,-8018,-6379,-6379,-6379,5357,5357,5357,8236,8236,8236,8236,-3839,-3839,-3839,-7763,-7763,-7763,-164,-164,-164,7461,7461,7461,4166,4166,4166,-8107,-8107,-8107,-8107,-6036,-6036,-6036,5609,5609,5609,8183,8183,8183,-4399,-4399,-4399,-7737,-7737,-7737,-7737,531,531,531,7530,7530,7530,3395,3395,3395,-8134,-8134,-8134,-5559,-5559,-5559,-5559,5907,5907,5907,8088,8088,8088,-5041,-5041,-5041,-7754,-7754,-7754,1100,1100,1100,7619,7619,7619,7619,2718,2718,2718,-8132,-8132,-8132,-5099,-5099,-5099,6155,6155,6155,7906,7906,7906,7906,-5631,-5631,-5631,-7709,-7709,-7709,1896,1896,1896,7718,7718,7718,1764,1764,1764,1764,-8055,-8055,-8055,-4469,-4469,-4469,6379,6379,6379,7626,7626,7626,-6276,-6276,-6276,-7639,-7639,-7639,-7639,2518,2518,2518,7829,7829,7829,974,974,974,-7996,-7996,-7996,-3984,-3984,-3984,-3984,6617,6617,6617,7307,7307,7307,-6716,-6716,-6716,-7550,-7550,-7550,3194,3194,3194,3194,7924,7924,7924,-5,-5,-5,-7904,-7904,-7904,-3346,-3346,-3346,6855,6855,6855,6850,6850,6850,6850,-7161,-7161,-7161,-7405,-7405,-7405,3863,3863,3863,8045,8045,8045,-1000,-1000,-1000,-1000,-7815,-7815,-7815,-2634,-2634,-2634,7035,7035,7035,6294,6294,6294,-7534,-7534,-7534,-7162,-7162,-7162,-7162,4403,4403,4403,8151,8151,8151,-1916,-1916,-1916,-7772,-7772,-7772,-1890,-1890,-2184,-1170,-1170,-1170,-2306,-2306,-2306,3919,3919,3919,1261,1261,1261,-3440,-3440,-3440,-1348,-1348,-1348,-1348,1145,1145,1145,2829,2829,2829,-2454,-2454,-2454,-1035,-1035,-1035,-2152,-2152,-2152,-2152,3963,3963,3963,1082,1082,1082,-3314,-3314,-3314,-1557,-1557,-1557,1353,1353,1353,2822,2822,2822,2822,-2621,-2621,-2621,-974,-974,-974,-2074,-2074,-2074,3989,3989,3989,905,905,905,905,-3188,-3188,-3188,-1729,-1729,-1729,1700,1700,1700,2720,2720,2720,-2923,-2923,-2923,-2923,-870,-870,-870,-1901,-1901,-1901,3964,3964,3964,708,708,708,-3049,-3049,-3049,-1889,-1889,-1889,-1889,1906,1906,1906,2652,2652,2652,-3051,-3051,-3051,-821,-821,-821,-1673,-1673,-1673,-1673,3922,3922,3922,446,446,446,-2843,-2843,-2843,-2039,-2039,-2039,2187,2187,2187,2187,2566,2566,2566,-3240,-3240,-3240,-782,-782,-782,-1451,-1451,-1451,3822,3822,3822,222,222,222,222,-2656,-2656,-2656,-2186,-2186,-2186,2393,2393,2393,2495,2495,2495,-3364,-3364,-3364,-3364,-774,-774,-774,-1173,-1173,-1173,3723,3723,3723,-111,-111,-111,-2379,-2379,-2379,-2379,-2293,-2293,-2293,2713,2713,2713,2334,2334,2334,-3494,-3494,-3494,-796,-796,-796,-928,-928,-928,-928,3647,3647,3647,-362,-362,-362,-2243,-2243,-2243,-2315,-2315,-2315,2973,2973,2973,2973,2158,2158,2158,-3578,-3578,-3578,-811,-811,-811,-509,-509,-509,3419,3419,3419,3419,-780,-780,-780,-1971,-1971,-1971,-2408,-2408,-2408,3238,3238,3238,2015,2015,2015,-3617,-3617,-3617,-3617,-828,-828,-828,-240,-240,-240,3308,3308,3308,-1091,-1091,-1091,-1758,-1758,-1758,-1758,-2433,-2433,-2433,3486,3486,3486,1802,1802,1802,-3647,-3647,-3647,-940,-940,-940,-940,155,155,155,3138,3138,3138,-1468,-1468,-1468,-1516,-1516,-1516,-2427,-2427,-2427,3667,3667,3667,3667,1623,1623,1623,-3623,-3623,-3623,-1082,-1082,-1082,490,490,490,3063,3063,3063,3063,-1893,-1893,-1893,-1319,-1319,-1319,-2356,-2356,-2356,3856,3856,3856,1404,1404,1404,1404,-3537,-3537,-3537,-1235,-1235,-1235,859,859,859,2904,2904,2904,-2151,-2151,-2151,-1166,-1166,-1166,-1166,-2295,-2295,-2295,3947,3947,3947,1214,1214,1214,-3436,-3436,-3436,-1428,-1428,-1428,-1428,1203,1203,1203,2832,2832,2832,-2562,-2562,-2562,-976,-976,-976,-2064,-2064,-2064,-2064,3975,3975,3975,898,898,898,-3199,-3199,-3199,-1768,-1768,-1768,1643,1643,1643,2742,2742,2742,2742,-2857,-2857,-2857,-882,-882,-882,-1910,-1910,-1910,3973,3973,3973,710,710,710,710,-3026,-3026,-3026,-1899,-1899,-1899,1929,1929,1929,2653,2653,2653,-3108,-3108,-3108,-3108,-813,-813,-813,-1588,-1588,-1588,3905,3905,3905,325,325,325,-2749,-2749,-2749,-2120,-2120,-2120,-2120,2318,2318,2318,2515,2515,2515,-3348,-3348,-3348,-761,-761,-761,-1200,-1200,-1200,-1200,3750,3750,3750,-93,-93,-93,-2441,-2441,-2441,-2270,-2270,-2270,2697,2697,2697,2697,2350,2350,2350,-3504,-3504,-3504,-777,-777,-777,-805,-805,-805,3547,3547,3547,-455,-455,19107,10563,10563,10563,17993,17993,17993,16544,16544,16544,19788,19788,19788,14175,14175,14175,11877,11877,11877,11877,21071,21071,21071,15824,15824,15824,18866,18866,18866,10474,10474,10474,18535,18535,18535,18535,16032,16032,16032,20026,20026,20026,13799,13799,13799,12097,12097,12097,21020,21020,21020,16028,16028,16028,16028,18703,18703,18703,10396,10396,10396,18929,18929,18929,15720,15720,15720,20162,20162,20162,20162,13402,13402,13402,12424,12424,12424,20837,20837,20837,16412,16412,16412,18349,18349,18349,18349,10412,10412,10412,19419,19419,19419,15295,15295,15295,20239,20239,20239,13108,13108,13108,12720,12720,12720,12720,20701,20701,20701,16635,16635,16635,18095,18095,18095,10416,10416,10416,19824,19824,19824,19824,15029,15029,15029,20290,20290,20290,12674,12674,12674,13177,13177,13177,20453,20453,20453,20453,16980,16980,16980,17760,17760,17760,10462,10462,10462,20147,20147,20147,14838,14838,14838,20254,20254,20254,20254,12375,12375,12375,13611,13611,13611,20224,20224,20224,17211,17211,17211,17482,17482,17482,17482,10530,10530,10530,20454,20454,20454,14698,14698,14698,20182,20182,20182,12014,12014,12014,14192,14192,14192,14192,19772,19772,19772,17661,17661,17661,17029,17029,17029,10650,10650,10650,20669,20669,20669,20669,14660,14660,14660,20080,20080,20080,11733,11733,11733,14755,14755,14755,19378,19378,19378,19378,18003,18003,18003,16515,16515,16515,10849,10849,10849,20870,20870,20870,14739,14739,14739,14739,19933,19933,19933,11414,11414,11414,15372,15372,15372,18900,18900,18900,18398,18398,18398,16099,16099,16099,16099,10941,10941,10941,21023,21023,21023,14798,14798,14798,19744,19744,19744,11149,11149,11149,11149,16065,16065,16065,18284,18284,18284,18821,18821,18821,15570,15570,15570,11149,11149,11149,11149,21146,21146,21146,15007,15007,15007,19545,19545,19545,10872,10872,10872,16729,16729,16729,17723,17723,17723,17723,19171,19171,19171,15114,15114,15114,11345,11345,11345,21218,21218,21218,15234,15234,15234,15234,19341,19341,19341,10655,10655,10655,17510,17510,17510,16950,16950,16950,19609,19609,19609,19609,14551,14551,14551,11630,11630,11630,21193,21193,21193,15541,15541,15541,19087,19087,19087,10539,10539,10539,10539,18132,18132,18132,16443,16443,16443,19872,19872,19872,14036,14036,14036,11925,11925,11925,11925,21092,21092,21092,15900,15900,15900,18779,18779,18779,10390,10390,10390,18869,18869,18869,18869,15756,15756,15756,20167,20167,20167,13403,13403,13403,12382,12382,12382,20909,20909,20909,16329,16329,16329,16329,18438,18438,18438,10357,10357,10357,19376,19376,19376,15329,15329,15329,20295,20295,20295,20295,13024,13024,13024,12795,12795,12795,20691,20691,20691,16679,16679,16679,18037,18037,18037,18037,10392,10392,10392,19930,19930,19930,14940,14940,14940,20313,20313,20313,12524,12524,12524,13434,13434,13434,13434,20324,20324,20324,17128,17128,17128,17565,17565,17565,10489,10489,10489,20377,20377,20377,20377,14717,14717,14717,20222,20222,20222,12072,12072,12072,14154,14154,14154,19828,19828,19828,17635,17635,17635,17635,16978,16978,16978,10663,10663,10663,20733,20733,20733,14666,14666,14666,20076,20076};
	if (dummy == 0)
	{
		dummy = 1;
		memcpy(in_data, balanced_in_data, sizeof(balanced_in_data));
//		in_data = &balanced_in_data[0];
	}
	else
	{
		dummy = 0;
		memcpy(in_data, unbalanced_in_data, sizeof(unbalanced_in_data));
//		in_data = &unbalanced_in_data[0];
	}*/


	for (int i = 0; i < AI_COOLER_IN_1_SIZE; i++)
	{
		in_data[i] = acc_data.flatten[i];
	}
}

static void post_process_0(ai_float *out_data)
{
	//printf("inside post_process_0()\r\n");
	if (out_data[0] > out_data[1] && out_data[0] > 0.9 && out_data[1] < 0.9)
	{
		printf("result: balanced (out_data[0]=%.9f, out_data[1]=%.9f)\r\n", out_data[0], out_data[1]);
		//printf("result: balanced (out_data[0]=%d, out_data[1]=%d)\r\n", out_data[0], out_data[1]);
		toggle_led_off();
	}
	else if (out_data[1] > out_data[0] && out_data[1] > 0.9 && out_data[0] < 0.9)
	{
		printf("result: unbalanced (out_data[0]=%.9f, out_data[1]=%.9f)\r\n", out_data[0], out_data[1]);
		//printf("result: unbalanced (out_data[0]=%d, out_data[1]=%d)\r\n", out_data[0], out_data[1]);
		toggle_led_on();
	}
	else
		printf("result: inconclusive (out_data[0]=%.9f, out_data[1]=%.9f)\r\n", out_data[0], out_data[1]);
	out_data[0] = 0;
	out_data[1] = 0;
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
  IKS01A2_MOTION_SENSOR_AxesRaw_t my_axes = {
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
  MX_CRC_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */
  aiInit();
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

	  ret = IKS01A2_MOTION_SENSOR_GetAxesRaw(IKS01A2_LSM6DSL_0, MOTION_ACCELERO, &my_axes);
//	  printf("x=%"PRId32", y=%"PRId32", z=%"PRId32", ret=%"PRId32"\r\n", my_axes.x, my_axes.y, my_axes.z, ret);
/*	  acc_data.x[acc_data.samples_cnt] = my_axes.x;
	  acc_data.y[acc_data.samples_cnt] = my_axes.y;
	  acc_data.z[acc_data.samples_cnt] = my_axes.z;*/
	  acc_data.flatten[acc_data.samples_cnt] = my_axes.x;
	  acc_data.flatten[acc_data.samples_cnt + ACC_SAMPLES] = my_axes.y;
	  acc_data.flatten[acc_data.samples_cnt + (2 * ACC_SAMPLES)] = my_axes.z;
	  memset(&my_axes, 0, sizeof(my_axes));
	  acc_data.samples_cnt++;
	  if (acc_data.samples_cnt == ACC_SAMPLES)
	  {
		  acc_data.samples_cnt = 0;
#if CAPTURE_DATA_MODE
		  for (uint16_t i = 0; i < ACC_SAMPLES_FLAT; i++)
		  {
			  if (i != ACC_SAMPLES_FLAT - 1)
				  printf("%d,", acc_data.flatten[i]);
			  else
				  printf("%d\r\n", acc_data.flatten[i]);
			  HAL_Delay(1);
		  }
		  acc_data.buffers_filled++;
		  if (acc_data.buffers_filled == BUFFERS_NEEDED)
		  {
			  printf("End of program\r\n");
		  	  return 0;
		  }
		  uint16_t buffers_filled_temp = acc_data.buffers_filled;
		  memset(&acc_data, 0, sizeof(acc_data));
		  acc_data.buffers_filled = buffers_filled_temp;
#else
		  /* 1 - Acquire, pre-process and fill the input buffers */
		  acquire_and_process_data_0(in_data);

		  /* 2 - Call inference engine */
		  aiRun(in_data, out_data);

		  /* 3 - Post-process the predictions */
		  post_process_0(out_data);

		  memset(&acc_data, 0, sizeof(acc_data));

		  HAL_Delay(999);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
#endif
	  }
	  HAL_Delay(2);
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
