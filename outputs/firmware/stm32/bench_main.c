#include "can_test.h"

static CAN_HandleTypeDef hcan;

static void Error_Handler(void)
{
    /* Fast LED blink distinguishes setup failure from the normal heartbeat. */
    for (;;) {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        HAL_Delay(100);
    }
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
    osc.PLL.PLLMUL = RCC_PLL_MUL16;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) Error_Handler();

    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK) Error_Handler();
}

void HAL_CAN_MspInit(CAN_HandleTypeDef *handle)
{
    if (handle->Instance != CAN1) return;
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_CAN1_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_11; /* CN10-14: CAN_RX from transceiver RX. */
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_12; /* CN10-12: CAN_TX to transceiver TX. */
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

int main(void)
{
    HAL_Init();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef led = {0};
    led.Pin = GPIO_PIN_5; /* Nucleo LD2. */
    led.Mode = GPIO_MODE_OUTPUT_PP;
    led.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &led);

    GPIO_InitTypeDef buzzer = {0};
    buzzer.Pin = GPIO_PIN_8; /* Arduino D7: active-low buzzer signal. */
    buzzer.Mode = GPIO_MODE_INPUT; /* High impedance is completely silent. */
    buzzer.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &buzzer);

    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef button = {0};
    button.Pin = GPIO_PIN_13; /* Nucleo blue USER button, active low. */
    button.Mode = GPIO_MODE_INPUT;
    button.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &button);

    SystemClock_Config();
    hcan.Instance = CAN1;
    if (CAN_Test_Init(&hcan) != HAL_OK) Error_Handler();

    uint32_t last_led = HAL_GetTick();
    for (;;) {
        CAN_Test_Poll();
        uint32_t now = HAL_GetTick();
        if ((uint32_t)(now - last_led) >= 2000U) {
            last_led = now;
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        }
        HAL_Delay(1);
    }
}
