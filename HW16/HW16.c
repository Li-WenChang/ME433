#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define DIRECTION_PIN 17
#define PWM_PIN 16
int main()
{
    stdio_init_all();
    // wait until the USB port is opened
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }


    // I want the motro control's PWM frequency to be 15khz, 150Mhz/15k = 10000
    gpio_set_function(PWM_PIN, GPIO_FUNC_PWM); // Set the LED Pin to be PWM
    uint slice_num = pwm_gpio_to_slice_num(PWM_PIN); // Get PWM slice number
    float div = 1; // must be between 1-255
    pwm_set_clkdiv(slice_num, div); // divider
    uint16_t wrap = 10000; // when to rollover, must be less than 65535
    pwm_set_wrap(slice_num, wrap);


    int duty_cycle = 25;
    pwm_set_gpio_level(PWM_PIN, wrap*duty_cycle/100);
    pwm_set_enabled(slice_num, true); // turn on the PWM

    gpio_init(DIRECTION_PIN);            // Initialize the pin
    gpio_set_dir(DIRECTION_PIN, GPIO_OUT); // Set the pin as output

    
    while (true) {
        char command;
        printf("-------------------------------------------------------------------\n");
        printf("Press 'h' to increase duty cycle and 'g' for decreasing duty cycle\n");
        scanf(" %c", &command);

        
        if (command == 'h') {
            if (duty_cycle < 100) duty_cycle += 1;
            printf("You just increased the duty cycle, now the duty cycle is %d %%\n\n", duty_cycle);
        } else if (command == 'g') {
            if (duty_cycle > -100) duty_cycle -= 1;
            printf("You just decreased the duty cycle, now the duty cycle is %d %%\n\n", duty_cycle);
        }

        if (duty_cycle > 0) {gpio_put(DIRECTION_PIN, 0);} // set direction to forward
        else {gpio_put(DIRECTION_PIN, 1);} // set direction to backward
        pwm_set_gpio_level(PWM_PIN, wrap*abs(duty_cycle)/100);
    }
}
