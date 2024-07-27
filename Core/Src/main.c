#include <stdint.h>
#include "stm32f1xx.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_tim.h"
#include "stm32f1xx_hal_rcc.h"
#include "sample.h"
#include "music.h"

uint16_t sBuff[256];
volatile uint8_t sbPtr = 0;

void TIM2_IRQHandler(void) {
    if (TIM2->SR & TIM_SR_UIF) { // If update interrupt occurred
        TIM2->SR = ~TIM_SR_UIF; // Clear interrupt flag
        TIM2->CCR1 = sBuff[sbPtr++];
    }
}

// Define the music arrays
extern const uint16_t music[];
extern const uint16_t music2[];
extern const uint16_t music3[];
//extern const uint16_t music4[]; // Example of an additional music array

// Dynamically define the array of music arrays and their sizes
const uint16_t *musicArray[] = {music, music2, music3}; // Add or remove arrays as needed
const size_t musicSizes[] = {sizeof(music)/2, sizeof(music2)/2, sizeof(music3)/2};
const size_t musicCount = sizeof(musicArray) / sizeof(musicArray[0]); // Automatically calculate the number of music arrays

int main(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // Initialize external high-speed oscillator
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        // Initialization failed
        while (1);
    }

    // Initialize clock signals
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        // Initialization failed
        while (1);
    }

    // Enable clock signals for GPIO and alternative functions
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    // Disable JTAG and enable SWD
    __HAL_AFIO_REMAP_SWJ_NOJTAG();

    // Configure pins
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    //================

    __HAL_RCC_TIM2_CLK_ENABLE();  // enable TIM2 clock
    TIM_HandleTypeDef htim2;
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 0;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 1151;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
        // Initialization Error
        while (1);
    }

    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK) {
        // Initialization Error
        while (1);
    }

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 128;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        // Initialization Error
        while (1);
    }

    if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1) != HAL_OK) {
        // PWM Generation Error
        while (1);
    }

    HAL_TIM_Base_Start_IT(&htim2);
    HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);

    uint16_t cnt1 = 0;
    uint16_t data;
    uint32_t time = 0;
    uint16_t tone[8] = {0,0,0,0,0,0,0,0};
    uint16_t tCnt[8] = {0,0,0,0,0,0,0,0};
    uint8_t ltCnt[8] = {0,0,0,0,0,0,0,0};
    int8_t sData[8] = {0,0,0,0,0,0,0,0};
    uint16_t sPtr[8] = {0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff};
    uint8_t i, j;
    uint16_t max;
    uint8_t wbPtr = 0;
    size_t musicIndex = 0;

    while (1) {
        const uint16_t *currentMusic = musicArray[musicIndex];
        size_t currentMusicSize = musicSizes[musicIndex];

        while (1) {
            data = currentMusic[cnt1];
            if (++cnt1 >= currentMusicSize) {
                cnt1 = 0;
                musicIndex = (musicIndex + 1) % musicCount;
                break;
            }

            if (data >= 0x8000) {
                time = data - 0x8000;
                time *= 16;
                break;
            } else {
                j = 0;
                max = 0;
                for (i = 0; i < 8; i++) {
                    if (max < sPtr[i]) {
                        max = sPtr[i];
                        j = i;
                    }
                }

                tone[j] = data;
                tCnt[j] = 0;
                ltCnt[j] = 0xff;
                sPtr[j] = 0;
            }
        }

        while (time) {
            while (wbPtr == sbPtr) {}

            for (i = 0; i < 8; i++) {
                if (sPtr[i] < sizeof(sample)) {
                    if (ltCnt[i] != (tCnt[i] + tone[i]) >> 12) {
                        sData[i] = (int8_t)sample[sPtr[i]] / 2;
                        sPtr[i]++;
                    }
                    tCnt[i] += tone[i];
                    ltCnt[i] = tCnt[i] >> 12;
                }
            }

            sBuff[wbPtr++] = 512 + sData[0] + sData[1] + sData[2] + sData[3] + sData[4] + sData[5] + sData[6] + sData[7];
            time--;
        }
    }
}
