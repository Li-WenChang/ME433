#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9

#define MCP23008_ADDR 0b0100010



int main()
{
    stdio_init_all();

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);

    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);


    
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c


    // initialize the LED a.k.a heart beat
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    int led_status = -1;



    


    uint8_t iodir_reg = 0x00;
    uint8_t gpio_reg = 0x09;
    uint8_t olat_reg = 0x0A;

    // initialize the expander--------------------------------------------------------------
    uint8_t init_arr[2];  // Write to IODIR: set GP0 as input, others output
    init_arr[0] = iodir_reg; // IODIR register
    init_arr[1] = 0b01111111; // set GP0 as input and GP7 as output and others to be input pins
    i2c_write_blocking(I2C_PORT, MCP23008_ADDR, init_arr, 2, false);



    absolute_time_t last_time = get_absolute_time();

    while (true) {



        // read from GP0
        uint8_t reading;
        i2c_write_blocking(i2c_default, MCP23008_ADDR, &gpio_reg, 1, true);  // true to keep master control of bus
        i2c_read_blocking(i2c_default, MCP23008_ADDR, &reading, 1, false);  // false - finished with bus

        bool gp0 = (reading & 0x01);

        // write to GP7
        uint8_t cmd[2];
        cmd[0] = olat_reg;

        if(gp0){cmd[1] = 0b10000000;}
        else{cmd[1] = 0b00000000;}
        i2c_write_blocking(i2c_default, MCP23008_ADDR, cmd, 2, false);


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
    }
}
