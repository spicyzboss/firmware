/*
 * (c) Copyright 2018 by Coinkite Inc. This file is covered by license found in COPYING-CC.
 *
 * gpio.c -- setup and control GPIO pins (and one button)
 *
 */
#include "basics.h"
#include "gpio.h"
#include "stm32l4xx_hal.h"

// PA0 - onewire bus for 508a
// PA2 - onewire bus for 508a - second SE
#define ONEWIRE_PIN      GPIO_PIN_0
#define ONEWIRE2_PIN     GPIO_PIN_2
#define ONEWIRE_PORT     GPIOA

// gpio_setup()
//
// set directions, lock critical ones, etc.
//
    void
gpio_setup(void)
{
    // NOTES:
    // - try not to limit PCB changes for future revs; leave unused unchanged.
    // - oled_setup() uses pins on PA4 thru PA8

    // enable clock to GPIO's ... we will be using them all at some point
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    {   // Onewire bus pins used for ATECC608 comms
        GPIO_InitTypeDef setup = {
            .Pin = ONEWIRE_PIN,
            .Mode = GPIO_MODE_AF_OD,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_MEDIUM,
            .Alternate = GPIO_AF8_UART4,
        };
        HAL_GPIO_Init(ONEWIRE_PORT, &setup);
    }

    // Bugfix: re-init of console port pins seems to wreck
    // the mpy uart code, so avoid after first time.
    if(USART1->BRR == 0) {
        // debug console: USART1 = PA9=Tx & PA10=Rx
        GPIO_InitTypeDef setup = {
            .Pin = GPIO_PIN_9,
            .Mode = GPIO_MODE_AF_PP,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_MEDIUM,
            .Alternate = GPIO_AF7_USART1,
        };
        HAL_GPIO_Init(GPIOA, &setup);

        setup.Pin = GPIO_PIN_10;
        setup.Mode = GPIO_MODE_INPUT;
        setup.Pull = GPIO_PULLUP;
        HAL_GPIO_Init(GPIOA, &setup);
    }

    {   // Port B - mostly unused, but want TEAR input
        // TEAR from LCD: PB11
        GPIO_InitTypeDef setup = {
            .Pin = GPIO_PIN_11,
            .Mode = GPIO_MODE_INPUT,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,       // 60Hz
            .Alternate = 0,
        };

        HAL_GPIO_Init(GPIOB, &setup);
    }

    // Port C - Outputs
    // SD1 active LED: PC7
    // USB active LED: PC6
    // TURN OFF: PC0
    // SD mux: PC13
    {   GPIO_InitTypeDef setup = {
            .Pin = GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_0, GPIO_PIN_13,
            .Mode = GPIO_MODE_OUTPUT_PP,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
        };
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, 0);               // keep power on!
        HAL_GPIO_Init(GPIOC, &setup);

        // turn LEDs off, SD mux to A
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7|GPIO_PIN_6|GPIO_PIN_13, 0);
    }

    // Port C - Inputs
    // SD card detect switch: PC1 battery/not
    {   GPIO_InitTypeDef setup = {
            .Pin = GPIO_PIN_13 | GPIO_PIN_1,
            .Mode = GPIO_MODE_INPUT,
            .Pull = GPIO_PULLUP,
            .Speed = GPIO_SPEED_FREQ_LOW,
        };
        HAL_GPIO_Init(GPIOC, &setup);
    }

    // Port D - outputs
    // SD2 active LED: PD0
    {   GPIO_InitTypeDef setup = {
            .Pin = GPIO_PIN_0,
            .Mode = GPIO_MODE_OUTPUT_PP,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
        };
        HAL_GPIO_Init(GPIOD, &setup);

        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, 0);    // turn off
    }

    // Port D - Inputs
    // SD slots detects: PD3/4
    {   GPIO_InitTypeDef setup = {
            .Pin = GPIO_PIN_3 | GPIO_PIN_4,
            .Mode = GPIO_MODE_INPUT,
            .Pull = GPIO_PULLUP,        // required
            .Speed = GPIO_SPEED_FREQ_LOW,
        };
        HAL_GPIO_Init(GPIOD, &setup);
    }

    // Port E - Q1 things
    // QR_RESET/TRIG - ignore for now
    // BL_ENABLE: PE3
    // NFC_ACTIVE: PE4 (led)
    // GPU control: 2,5,6 outputs
    {   GPIO_InitTypeDef setup = {
            .Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_2 | GPIO_PIN_5 | GPIO_PIN_6,
            .Mode = GPIO_MODE_OUTPUT_PP,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
        };
        HAL_GPIO_Init(GPIOE, &setup);

        // turn off NFC LED, reset GPU, keep in reset
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4|GPIO_PIN_5, 0);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, 1);    // turn on Backlight,
    }


#if 0
    // TEST CODE -- keep
    // enable MCO=PA8 for clock watching. Conflicts w/ OLED normal use.
    GPIO_InitTypeDef mco_setup = {
        .Pin = GPIO_PIN_8,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .Alternate = GPIO_AF0_MCO,
    };
    HAL_GPIO_Init(GPIOA, &mco_setup);

    // select a signal to view here.
    // RCC_MCO1SOURCE_SYSCLK => 120Mhz (correct)
    // RCC_MCO1SOURCE_PLLCLK  (PLL R output) => (same os SYSCLK)
    // RCC_MCO1SOURCE_HSI48  => 48Mhz
    // RCC_MCO1SOURCE_HSE => 8Mhz (correct)
    __HAL_RCC_MCO1_CONFIG(RCC_MCO1SOURCE_SYSCLK, RCC_MCODIV_1);
#endif
}

// turn_power_off()
//
// kill system power; instant
//
    void
turn_power_off(void)
{
    gpio_setup();

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, 1);

    LOCKUP_FOREVER();
}

// EOF
