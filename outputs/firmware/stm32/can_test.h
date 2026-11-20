#ifndef CAN_TEST_H
#define CAN_TEST_H
#include "stm32f1xx_hal.h"

extern volatile uint32_t can_test_rx;
extern volatile uint32_t can_test_queued;
extern volatile uint32_t can_test_dropped;
extern volatile uint32_t can_test_error;
HAL_StatusTypeDef CAN_Test_Init(CAN_HandleTypeDef *handle);
void CAN_Test_Poll(void);
#endif
