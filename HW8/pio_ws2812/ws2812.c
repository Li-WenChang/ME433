/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"
#include "hardware/pwm.h"

/**
 * NOTE:
 *  Take into consideration if your WS2812 is a RGB or RGBW variant.
 *
 *  If it is RGBW, you need to set IS_RGBW to true and provide 4 bytes per 
 *  pixel (Red, Green, Blue, White) and use urgbw_u32().
 *
 *  If it is RGB, set IS_RGBW to false and provide 3 bytes per pixel (Red,
 *  Green, Blue) and use urgb_u32().
 *
 *  When RGBW is used with urgb_u32(), the White channel will be ignored (off).
 *
 */
#define IS_RGBW false
#define NUM_PIXELS 4

#ifdef PICO_DEFAULT_WS2812_PIN
#define WS2812_PIN PICO_DEFAULT_WS2812_PIN
#else
// default to pin 2 if the board doesn't have a default WS2812 pin defined
#define WS2812_PIN 10
#endif

// Check the pin is compatible with the platform
#if WS2812_PIN >= NUM_BANK0_GPIOS
#error Attempting to use a pin>=32 on a platform that does not support it
#endif

#define MotorPin 16 // the built in LED on the Pico


// link three 8bit colors together
typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} wsColor; 

static inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb) {
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return
            ((uint32_t) (g) << 8) |
            ((uint32_t) (r) << 16) |
            (uint32_t) (b);
}

// adapted from https://forum.arduino.cc/index.php?topic=8498.0
// hue is a number from 0 to 360 that describes a color on the color wheel
// sat is the saturation level, from 0 to 1, where 1 is full color and 0 is gray
// brightness sets the maximum brightness, from 0 to 1
wsColor HSBtoRGB(float hue, float sat, float brightness) {
    float red = 0.0;
    float green = 0.0;
    float blue = 0.0;

    if (sat == 0.0) {
        red = brightness;
        green = brightness;
        blue = brightness;
    } else {
        if (hue == 360.0) {
            hue = 0;
        }

        int slice = hue / 60.0;
        float hue_frac = (hue / 60.0) - slice;

        float aa = brightness * (1.0 - sat);
        float bb = brightness * (1.0 - sat * hue_frac);
        float cc = brightness * (1.0 - sat * (1.0 - hue_frac));

        switch (slice) {
            case 0:
                red = brightness;
                green = cc;
                blue = aa;
                break;
            case 1:
                red = bb;
                green = brightness;
                blue = aa;
                break;
            case 2:
                red = aa;
                green = brightness;
                blue = cc;
                break;
            case 3:
                red = aa;
                green = bb;
                blue = brightness;
                break;
            case 4:
                red = cc;
                green = aa;
                blue = brightness;
                break;
            case 5:
                red = brightness;
                green = aa;
                blue = bb;
                break;
            default:
                red = 0.0;
                green = 0.0;
                blue = 0.0;
                break;
        }
    }

    unsigned char ired = red * 255.0;
    unsigned char igreen = green * 255.0;
    unsigned char iblue = blue * 255.0;

    wsColor c;
    c.r = ired;
    c.g = igreen;
    c.b = iblue;
    return c;
}

int main() {
    //set_sys_clock_48();
    stdio_init_all();
    printf("WS2812 Smoke Test, using pin %d\n", WS2812_PIN);

    // PWM setting-------------------------------------
    gpio_set_function(MotorPin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(MotorPin); // Get PWM slice number
    float div = 50; // must be between 1-255
    pwm_set_clkdiv(slice_num, div); // divider
    uint16_t wrap = 60000; // f = 150000000/50/60000
    pwm_set_wrap(slice_num, wrap);

    pwm_set_enabled(slice_num, true); // turn on the PWM
    

    // todo get free sm
    PIO pio;
    uint sm;
    uint offset;

    // This will find a free pio and state machine for our program and load it for us
    // We use pio_claim_free_sm_and_add_program_for_gpio_range (for_gpio_range variant)
    // so we will get a PIO instance suitable for addressing gpios >= 32 if needed and supported by the hardware
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &pio, &sm, &offset, WS2812_PIN, 1, true);
    hard_assert(success);

    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);



    int num_LEDsteps = 1000;
    int LEDstep = 0;
    unsigned int t1 = to_us_since_boot(get_absolute_time());
    while (1) {

        unsigned int t2 = to_us_since_boot(get_absolute_time());

        // update LED's color every 0.005 sec
        if((t2-t1) > 5000000/num_LEDsteps){

            // LED control---------------------------------------------------------------------

            float offset = 360.0f/num_LEDsteps*LEDstep;

            for(int i=0;i<NUM_PIXELS;i++){

                float hue = fmodf((360.0f - 90.0f * i) + offset, 360.0f);
                if (hue < 0) hue += 360.0f;  // fix negative wrap
                wsColor current_led = HSBtoRGB(hue, 1.0, 0.05);
                put_pixel(pio, sm, urgb_u32(current_led.r, current_led.g, current_led.b));
            }

            // remove the sleep line because the motor control also takes some time
            //sleep_ms(1); // wait at least the reset time


            // Motor control-------------------------------------------------------------------
            // motor goes from 0 to 180 and then back to 0 in 5 sec, which is als 360 degree in 5 sec
            // therefore we can combine the updates and use the offset as control

            float motor_angle;
            if (offset > 180){motor_angle = 360-offset;}
            else{motor_angle = offset;}

            // 0.5ms = 2.5%, 2.5ms = 12.5%
            float duty = 2.5 + 10 * (motor_angle/180);

            // command the motor
            pwm_set_gpio_level(MotorPin, wrap*duty/100);







            LEDstep++;
            if(LEDstep == num_LEDsteps){LEDstep = 0;}
            // update the time
            t1 = t2;

        }

        
    }

    // This will free resources and unload our program
    pio_remove_program_and_unclaim_sm(&ws2812_program, pio, sm, offset);
}
