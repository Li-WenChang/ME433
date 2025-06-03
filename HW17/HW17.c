#include <stdio.h>
#include "pico/stdlib.h"
#include "cam.h"
#include "hardware/i2c.h"

#define PWM_PIN1 16   // GPIO2, PWM3_A
#define PWM_PIN2 17   // GPIO3, PWM3_B


uint pwm_init_dual(uint gpio_pin1, uint gpio_pin2, float freq_khz);
void generate_pwm_arrays(int arr1[80], int arr2[80], int high, int low);


int main()
{
    stdio_init_all();

    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }

    // // PWM init
    // // Set both pins to PWM function
    // gpio_set_function(PWM_PIN1, GPIO_FUNC_PWM);
    // gpio_set_function(PWM_PIN2, GPIO_FUNC_PWM);

    // // Get slice number from either pin (same slice for both)
    // uint slice = pwm_gpio_to_slice_num(PWM_PIN1);

    // // Set frequency: 150MHz / 1 / 10000 = 15kHz
    // pwm_set_clkdiv(slice, 1.0f);         // clk divider
    // pwm_set_wrap(slice, 10000);          // wrap value
    uint slice = pwm_init_dual(PWM_PIN1, PWM_PIN2, 15);

    // // Set different duty cycles for A and B channels
    // int duty1 = 15; // 25% duty for PWM_PIN1
    // int duty2 = 90; // 60% duty for PWM_PIN2

    
    int pwm_array1[80], pwm_array2[80];
    generate_pwm_arrays(pwm_array1, pwm_array2, 100, 0);  // use default high = 75, low = 30

    printf("Hello, camera!\n");

    init_camera_pins();
 
    while (true) {
        // uncomment these and printImage() when testing with python 
        // char m[10];
        // scanf("%s",m);
        

        setSaveImage(1);
        while(getSaveImage()==1){}
        convertImage();
        


        int com = findLine(IMAGESIZEY/2); // calculate the position of the center of the ine
        setPixel(IMAGESIZEY/2,com,0,255,0); // draw the center so you can see it in python
        // printImage();


        int left_pwm, right_pwm;
        left_pwm = pwm_array2[com];
        right_pwm = pwm_array1[com];
        printf("Center at: %d\r\n",com); // comment this when testing with python
        printf("Left PWM: %d, Right PWM: %d \r\n", left_pwm, right_pwm);


        pwm_set_gpio_level(PWM_PIN1, 10000 * left_pwm / 100);
        pwm_set_gpio_level(PWM_PIN2, 10000 * right_pwm / 100);
        pwm_set_enabled(slice, true);
    }
}















uint pwm_init_dual(uint gpio_pin1, uint gpio_pin2, float freq_khz) {
    // Set GPIOs to PWM function
    gpio_set_function(gpio_pin1, GPIO_FUNC_PWM);
    gpio_set_function(gpio_pin2, GPIO_FUNC_PWM);

    // Get the PWM slice (must be same for both pins)
    uint slice = pwm_gpio_to_slice_num(gpio_pin1);

    // Calculate wrap value for desired frequency
    float clk_div = 1.0f;  // you can parameterize this too if needed
    uint32_t sys_clk = 150000000; // 150 MHz system clock
    uint32_t wrap = (uint32_t)(sys_clk / (clk_div * freq_khz * 1000));

    // Set frequency
    pwm_set_clkdiv(slice, clk_div);
    pwm_set_wrap(slice, wrap);

    // Enable PWM slice
    pwm_set_enabled(slice, true);

    return slice;
}

void generate_pwm_arrays(int arr1[80], int arr2[80], int high, int low) {
    // Default values if not overridden
    if (high == -1) high = 75;
    if (low == -1)  low = 30;

    // Array 1
    for (int i = 0; i < 80; i++) {
        if (i < 40) {
            arr1[i] = high;
        } else if (i < 70) {
            // Linear decay from high to low over 30 steps (40–69)
            float t = (i - 40) / 30.0f;
            arr1[i] = (int)((1 - t) * high + t * low + 0.5f);  // +0.5 for rounding
        } else {
            arr1[i] = low;
        }
    }

    // Array 2
    for (int i = 0; i < 80; i++) {
        if (i < 10) {
            arr2[i] = low;
        } else if (i < 40) {
            // Linear rise from low to high over 30 steps (10–39)
            float t = (i - 10) / 30.0f;
            arr2[i] = (int)((1 - t) * low + t * high + 0.5f);
        } else {
            arr2[i] = high;
        }
    }
}