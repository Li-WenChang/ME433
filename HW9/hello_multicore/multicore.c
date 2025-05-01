/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/adc.h"

#define FLAG_VALUE 123
#define CMD_READ_ADC  0
#define CMD_LED_ON    1
#define CMD_LED_OFF   2
#define CMD_ADC_DONE  100

volatile float adc_result = 0;  // Shared global variable


void core1_entry() {
    // Initialize GPIO and ADC
    const uint LED_PIN = 25;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    adc_init();
    adc_gpio_init(26); // A0 = GPIO26
    adc_select_input(0); // ADC0 is GPIO26

    while (1) {
        uint32_t cmd = multicore_fifo_pop_blocking();

        if (cmd == CMD_READ_ADC) {
            adc_result = adc_read()/4095.0f * 3.3;  // Read raw ADC value
            multicore_fifo_push_blocking(CMD_ADC_DONE);  // Signal done
        } else if (cmd == CMD_LED_ON) {
            gpio_put(LED_PIN, 1);
        } else if (cmd == CMD_LED_OFF) {
            gpio_put(LED_PIN, 0);
        }
    }
}
int main() {
    stdio_init_all();

    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }

    printf("Core communication ready. Enter 0 (ADC), 1 (LED ON), or 2 (LED OFF):\n");

    multicore_launch_core1(core1_entry);

    while (1) {
        int c ;  // Non-blocking input
        scanf("%d", &c);
        if (c == 0) {
            multicore_fifo_push_blocking(CMD_READ_ADC);
            uint32_t response = multicore_fifo_pop_blocking();
            if (response == CMD_ADC_DONE) {
                printf("ADC Reading: %.2f V\n", adc_result);
            }
        } else if (c == 1) {
            multicore_fifo_push_blocking(CMD_LED_ON);
            printf("LED ON\n");
        } else if (c == 2) {
            multicore_fifo_push_blocking(CMD_LED_OFF);
            printf("LED OFF\n");
        }
        sleep_ms(50);  // Small delay to avoid spamming the FIFO
    }
}