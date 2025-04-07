/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

 #include <stdio.h>
 #include "pico/stdlib.h"
 #include "hardware/gpio.h"
 
 #define GPIO_WATCH_PIN 12
 
 volatile int led_status = -1;
 volatile int button_times = 0;
 
 void pico_led_init(void) {
     #ifdef PICO_DEFAULT_LED_PIN
         // A device like Pico that uses a GPIO for the LED will define PICO_DEFAULT_LED_PIN
         // so we can use normal GPIO functionality to turn the led on and off
         gpio_init(PICO_DEFAULT_LED_PIN);
         gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
     #endif
     }
     
     // Turn the LED on or off
     void pico_set_led(bool led_on) {
     #if defined(PICO_DEFAULT_LED_PIN)
         // Just set the GPIO on or off
         gpio_put(PICO_DEFAULT_LED_PIN, led_on);
     #endif
     }
 
 void gpio_callback(uint gpio, uint32_t events){
 
     // if detect a falling edge and the digital input is low after 10ms, meaning that it's a real press
     sleep_ms(10);
     if (gpio_get(GPIO_WATCH_PIN) == false){
 
         // toggle the led status
         led_status *= -1;
         button_times += 1;
 
         if (led_status == 1){pico_set_led(true);}
         else{pico_set_led(false);}
         printf("The button has been pressed %d times\n", button_times);
     }
 
 }
 
 int main() {
     stdio_init_all();
     pico_led_init();
 
     printf("Hello GPIO IRQ\n");
     gpio_init(GPIO_WATCH_PIN);
 
     gpio_set_dir(GPIO_WATCH_PIN, GPIO_IN);
 
     gpio_set_irq_enabled_with_callback(GPIO_WATCH_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
 
     // Wait forever
     while (1){}
 }
 
 
