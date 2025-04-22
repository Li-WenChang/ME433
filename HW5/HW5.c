#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <math.h>

#define DAC_CS_PIN 17
#define RAM_CS_PIN 14
#define sample_rate 1000

static inline void cs_select(uint cs_pin) {
    asm volatile("nop \n nop \n nop"); // FIXME
    gpio_put(cs_pin, 0);
    asm volatile("nop \n nop \n nop"); // FIXME
}

static inline void cs_deselect(uint cs_pin) {
    asm volatile("nop \n nop \n nop"); // FIXME
    gpio_put(cs_pin, 1);
    asm volatile("nop \n nop \n nop"); // FIXME
}


union FloatInt {
    float f;
    uint32_t i;
};

const float* sine_wave() {
    
    static float sine_arr[sample_rate];
    for(int i = 0; i < sample_rate; i++){
        sine_arr[i] = 3.3f / 2 + 3.3f / 2 * sin((float)i / sample_rate * 2.0f * 3.1416f);
    }
    return sine_arr;
}

int main()
{
    stdio_init_all();
    
    // Enable DEFAULT SPI at 1 MHz and connect to GPIOs
    spi_init(spi_default, 1000*1000 ); // the baud, or bits per second
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);

    // set up CS pin for DAC, it's high because, it's low when active
    gpio_init(DAC_CS_PIN);
    gpio_set_dir(DAC_CS_PIN, GPIO_OUT);
    gpio_put(DAC_CS_PIN, 1);

    // set up CS pin for external RAM, it's high because, it's low when active
    gpio_init(RAM_CS_PIN);
    gpio_set_dir(RAM_CS_PIN, GPIO_OUT);
    gpio_put(RAM_CS_PIN, 1);
    

    //set the external RAM to sequential operation mode-----------------------------------------------
    uint8_t data[2];
    data[0] = 0b00000001;
    data[1] = 0b01000000;

    cs_select(RAM_CS_PIN);
    spi_write_blocking(spi_default, data, 2); // where data is a uint8_t array with length len
    cs_deselect(RAM_CS_PIN);
    // writing the whole sine wave to RAM ------------------------------------------------------------

    const float* s_arr = sine_wave();
    uint8_t write_cmd[3];
    write_cmd[0] = 0b00000010;           // WRITE command
    write_cmd[1] = 0x00; // Address high byte (start at 0x0000)
    write_cmd[2] = 0x00; // Address low byte

    uint8_t write_data[sample_rate * 4]; // 1000 floats need 4000 bytes
    for(int i = 0; i < sample_rate; i++){
        union FloatInt my_num;
        my_num.f = s_arr[i];
        write_data[i*4 + 0] = (my_num.i >> 24) & 0xFF;
        write_data[i*4 + 1] = (my_num.i >> 16) & 0xFF;
        write_data[i*4 + 2] = (my_num.i >> 8) & 0xFF;
        write_data[i*4 + 3] = (my_num.i) & 0xFF;
    }

    cs_select(RAM_CS_PIN);
    spi_write_blocking(spi_default, write_cmd, 3);
    spi_write_blocking(spi_default, write_data, sample_rate * 4);
    cs_deselect(RAM_CS_PIN);

    
    // reading one voltage at a time from RAM and then command the DAC to output it--------------------
    while(true){
        for(int i = 0; i < sample_rate; i++){
            absolute_time_t start = get_absolute_time();

            uint16_t address = i*4;  // address
            uint8_t high_byte = (address >> 8) & 0xFF; // High byte (upper 8 bits)
            uint8_t low_byte = address & 0xFF; // Low byte (lower 8 bits)
            uint8_t read_cmd[3];
            read_cmd[0] = 0b00000011;      // READ command
            read_cmd[1] = high_byte;      // Start address high byte
            read_cmd[2] = low_byte;      // Start address low byte

            uint8_t read_data[4];
            cs_select(RAM_CS_PIN);

            // Send the READ command + address
            spi_write_blocking(spi_default, read_cmd, 3);

            // Read 8 bytes (2 floats)
            spi_read_blocking(spi_default, 0, read_data, 4);

            cs_deselect(RAM_CS_PIN);

            union FloatInt read_num;

            read_num.i = (read_data[0] << 24) | (read_data[1] << 16) |
                    (read_data[2] << 8)  | (read_data[3]);


            uint digital_DAC = (uint)((read_num.f / 3.3f) * 1023);

            // Construct 16-bit command
            uint16_t command_DAC = 0;
            command_DAC |= (0b0111 << 12);       // Control bits: 0111 (write to channel A, buffered, 1x gain, output on)
            command_DAC |= (digital_DAC << 2);   // 10-bit value shifted into position (bits 11-2)

            // Split into two bytes for SPI
            uint8_t DAC_data[2];
            DAC_data[0] = (command_DAC >> 8) & 0xFF; // High byte
            DAC_data[1] = command_DAC & 0xFF;        // Low byte

            // write voltage to DAC
            cs_select(DAC_CS_PIN);
            spi_write_blocking(spi_default, DAC_data, 2); // where data is a uint8_t array with length len
            cs_deselect(DAC_CS_PIN);
            while(true){
                absolute_time_t end = get_absolute_time();
                int64_t elapsed_us = absolute_time_diff_us(start, end);
                if(elapsed_us >= 1000){break;}
                sleep_us(5);
            }
        }
    }


    // // wait until the USB port is opened
    // while (!stdio_usb_connected()) {
    //     sleep_ms(100);
    // }

    // volatile float f1, f2;
    // printf("Enter two floats to use:");
    // scanf("%f %f", &f1, &f2);
    // printf("\n");
    // volatile float f_add, f_sub, f_mult, f_div;
    
    // absolute_time_t t0 = get_absolute_time();
    // uint64_t t_start = to_us_since_boot(t0);

    // for(int i = 0; i < 1000; i++){f_add = f1+f2;}
    // absolute_time_t t1 = get_absolute_time();
    // uint64_t t_stop1 = to_us_since_boot(t1);
    // float a_d = (t_stop1 - t_start)*1000/1000/6.667;

    // for(int i = 0; i < 1000; i++){f_sub = f1-f2;}
    // absolute_time_t t2 = get_absolute_time();
    // uint64_t t_stop2 = to_us_since_boot(t2);
    // float s_d = (t_stop2 - t_stop1)*1000/1000/6.667;

    // for(int i = 0; i < 1000; i++){f_mult = f1*f2;}
    // absolute_time_t t3 = get_absolute_time();
    // uint64_t t_stop3 = to_us_since_boot(t3);
    // float m_d = (t_stop3 - t_stop2)*1000/1000/6.667;

    // for(int i = 0; i < 1000; i++){f_div = f1/f2;}
    // absolute_time_t t4 = get_absolute_time();
    // uint64_t t_stop4 = to_us_since_boot(t4);
    // float d_d = (t_stop4 - t_stop3)*1000/1000/6.667;

    // printf("addition duration = %.2f cycles\n", a_d);
    // printf("subtraction duration = %.2f cycles\n", s_d);
    // printf("multiplication duration = %.2f cycles\n", m_d);
    // printf("divition duration = %.2f cycles\n", d_d);

    // printf("\nResults: \n%f+%f=%f \n%f-%f=%f \n%f*%f=%f \n%f/%f=%f\n",
    //         f1,f2,f_add, f1,f2,f_sub, f1,f2,f_mult, f1,f2,f_div);

}
