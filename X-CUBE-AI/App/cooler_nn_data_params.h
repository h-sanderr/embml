/**
  ******************************************************************************
  * @file    cooler_nn_data_params.h
  * @author  AST Embedded Analytics Research Platform
  * @date    Fri Aug 18 10:23:35 2023
  * @brief   AI Tool Automatic Code Generator for Embedded NN computing
  ******************************************************************************
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  ******************************************************************************
  */

#ifndef COOLER_NN_DATA_PARAMS_H
#define COOLER_NN_DATA_PARAMS_H
#pragma once

#include "ai_platform.h"

/*
#define AI_COOLER_NN_DATA_WEIGHTS_PARAMS \
  (AI_HANDLE_PTR(&ai_cooler_nn_data_weights_params[1]))
*/

#define AI_COOLER_NN_DATA_CONFIG               (NULL)


#define AI_COOLER_NN_DATA_ACTIVATIONS_SIZES \
  { 1736, }
#define AI_COOLER_NN_DATA_ACTIVATIONS_SIZE     (1736)
#define AI_COOLER_NN_DATA_ACTIVATIONS_COUNT    (1)
#define AI_COOLER_NN_DATA_ACTIVATION_1_SIZE    (1736)



#define AI_COOLER_NN_DATA_WEIGHTS_SIZES \
  { 77408, }
#define AI_COOLER_NN_DATA_WEIGHTS_SIZE         (77408)
#define AI_COOLER_NN_DATA_WEIGHTS_COUNT        (1)
#define AI_COOLER_NN_DATA_WEIGHT_1_SIZE        (77408)



#define AI_COOLER_NN_DATA_ACTIVATIONS_TABLE_GET() \
  (&g_cooler_nn_activations_table[1])

extern ai_handle g_cooler_nn_activations_table[1 + 2];



#define AI_COOLER_NN_DATA_WEIGHTS_TABLE_GET() \
  (&g_cooler_nn_weights_table[1])

extern ai_handle g_cooler_nn_weights_table[1 + 2];


#endif    /* COOLER_NN_DATA_PARAMS_H */
