#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "ssd1306.h"
#include "font.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 12
#define I2C_SCL 13

void draw_Char(int x, int y, char c);
void draw_Message(int x, int y, char* mes);

int main()
{
    stdio_init_all();
    // while (!stdio_usb_connected()) {
    //     sleep_ms(100);
    // }
    // char c = ':' - 0x20;
    // printf("%d %d %d %d %d \n", ASCII[c][0], ASCII[c][1], ASCII[c][2], ASCII[c][3], ASCII[c][4]);

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c

    adc_init(); // init the adc module
    adc_gpio_init(26); // set ADC0 pin to be adc input instead of GPIO
    adc_select_input(0); // select to read from ADC0

    // set up the lcd screen
    ssd1306_setup();
    
    
    

    
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    int led_status = -1;
    absolute_time_t last_time = get_absolute_time();
    unsigned int t1 = to_us_since_boot(get_absolute_time());
    while (true) {
        // heart beat
        absolute_time_t this_time = get_absolute_time();
        int64_t elapsed_us = absolute_time_diff_us(last_time, this_time);
        // every 500000us = 0.5sec, led goes on and off
        if(elapsed_us >= 250000){
            printf("here\n");
            if(led_status == 1){gpio_put(PICO_DEFAULT_LED_PIN, true);}
            else{gpio_put(PICO_DEFAULT_LED_PIN, false);}
            led_status *= -1;
            last_time = this_time;
        }

        uint16_t result = adc_read();
        float voltage = 3.3/4095*result;
        unsigned int t2 = to_us_since_boot(get_absolute_time());
        char mes[50];
        char fps_mes[50];
        sprintf(mes, "Voltage is: %.2f", voltage);
        sprintf(fps_mes, "FPS: %.2f", 1000000.0f/(t2-t1));
        ssd1306_clear();
        draw_Message(0, 0, mes);
        draw_Message(0, 24, fps_mes);
        ssd1306_update();
        t1 = t2;
        
        
    }
}



void draw_Char(int x, int y, char c){
    char real = c - 0x20;

    for(int col = 0; col < 5; col++){
        for(int row = 0; row < 8; row ++){
            bool result = (ASCII[real][col] >> row) & 1;
            if(result){
                ssd1306_drawPixel(x + col, y + row, 1);
            }
        }
    }
}

void draw_Message(int x, int y, char* mes){
    int i = 0;
    int char_width = 6;
    int width_of_screen = (128-char_width)/char_width;
    while(mes[i] != '\0'){
        int col = i%width_of_screen;
        int row = i / width_of_screen;

        int new_x = (x+i*char_width)%(128/char_width*char_width-char_width);
        int new_y = y+ (x+i*char_width)/(128/char_width*char_width-char_width)*8;
        draw_Char(new_x, new_y, mes[i]);
        if(i==0){printf("%d %d", new_x, new_y);}
        i++;
    }

}
