#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "registers.h"
#include "ssd1306.h"
#include <stdlib.h>

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 12
#define I2C_SCL 13

#define MPU6050Address 0b1101000

void draw_accel_line(int x_acc_length, int y_acc_length) {
    int x0 = 64;
    int y0 = 16;
    int x1 = 64 - x_acc_length;
    int y1 = 16 + y_acc_length;

    int dx = abs(x1 - x0);
    int dy = -abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy; // error value e_xy

    while (1) {
        ssd1306_drawPixel(x0, y0, 1);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }

    ssd1306_update();
}

int main()
{
    stdio_init_all();

    // initialize the LED a.k.a heart beat
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    int led_status = -1;

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    uint8_t init_arr[2];
    init_arr[0] = PWR_MGMT_1; 
    init_arr[1] = 0b00000000; 
    i2c_write_blocking(I2C_PORT, MPU6050Address, init_arr, 2, false);

    init_arr[0] = GYRO_CONFIG; 
    init_arr[1] = 0b00011000; 
    i2c_write_blocking(I2C_PORT, MPU6050Address, init_arr, 2, false);

    init_arr[0] = ACCEL_CONFIG; 
    init_arr[1] = 0b00000000; 
    i2c_write_blocking(I2C_PORT, MPU6050Address, init_arr, 2, false);

    // set up the lcd screen
    ssd1306_setup();
    

    absolute_time_t last_time = get_absolute_time();
    while (true) {
        
        uint8_t who_reg = WHO_AM_I;
        uint8_t whoami_test;
        i2c_write_blocking(i2c_default, MPU6050Address, &who_reg, 1, true);
        i2c_read_blocking(i2c_default, MPU6050Address, &whoami_test, 1, false);

        uint8_t sensor_data[14];
        uint8_t data_start_reg = ACCEL_XOUT_H;
        // Request data starting from register 0x3B
        i2c_write_blocking(I2C_PORT, MPU6050Address, &data_start_reg, 1, true);
        i2c_read_blocking(I2C_PORT, MPU6050Address, sensor_data, 14, false);

        int16_t accel_x = (sensor_data[0] << 8) | sensor_data[1];
        int16_t accel_y = (sensor_data[2] << 8) | sensor_data[3];
        int16_t accel_z = (sensor_data[4] << 8) | sensor_data[5];

        int16_t temp_raw = (sensor_data[6] << 8) | sensor_data[7];

        int16_t gyro_x = (sensor_data[8]  << 8) | sensor_data[9];
        int16_t gyro_y = (sensor_data[10] << 8) | sensor_data[11];
        int16_t gyro_z = (sensor_data[12] << 8) | sensor_data[13];

        // Convert raw values to physical units
        float ax = accel_x * 0.000061f; // g
        float ay = accel_y * 0.000061f;
        float az = accel_z * 0.000061f;

        float temperature = (temp_raw / 340.0f) + 36.53f; // °C

        float gx = gyro_x * 0.00763f;  // deg/s
        float gy = gyro_y * 0.00763f;
        float gz = gyro_z * 0.00763f;
        // Print the results
        printf("Accel (g):    X=%.3f Y=%.3f Z=%.3f\n", ax, ay, az);
        printf("Gyro (°/s):   X=%.2f Y=%.2f Z=%.2f\n", gx, gy, gz);
        printf("Temperature:  %.2f °C\n", temperature);
        

        // 1g = 15 pixels
        int x_acc_length = (int)(ax * 15);
        int y_acc_length = (int)(ay * 15);


        
        ssd1306_clear();
        draw_accel_line(x_acc_length, y_acc_length);
        // ssd1306_update();
            









        // heart beat
        absolute_time_t this_time = get_absolute_time();
        int64_t elapsed_us = absolute_time_diff_us(last_time, this_time);
        // every 500000us = 0.5sec, led goes on and off
        if(elapsed_us >= 250000){
            printf("here\n");
            printf("Who am I? %d \n", whoami_test); // 0x68 = 104 in decimal
            
            if(led_status == 1){gpio_put(PICO_DEFAULT_LED_PIN, true);}
            else{gpio_put(PICO_DEFAULT_LED_PIN, false);}
            led_status *= -1;
            last_time = this_time;
        }
    }
}
