#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <math.h>

#define sample_rate 100
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

const float* sine_wave() {
    static float sine_arr[sample_rate];
    for(int i = 0; i < sample_rate; i++){
        sine_arr[i] = 3.3f / 2 + 3.3f / 2 * sin((float)i / sample_rate * 2.0f * 3.1416f);
    }
    return sine_arr;
}

const float* triangle_wave() {
    static float triangle_arr[sample_rate];
    for(int i = 0; i < sample_rate; i++){
        if (i < sample_rate/2){
            triangle_arr[i] = 3.3 * (float)i/(sample_rate/2);
        }
        else{
            triangle_arr[i] = 6.6 - 6.6* (float)i/(sample_rate);
        }
    }
    return triangle_arr;
}

int main()
{
    stdio_init_all();

    // Enable SPI 0 at 1 MHz and connect to GPIOs
    spi_init(spi_default, 1000 ); // the baud, or bits per second
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);

    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
    gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    
    const float* t_arr = triangle_wave();  // cached triangle wave
    const float* s_arr = sine_wave();      // cached sine wave
    int wave = 0;
    int count = 0;

    
    if(wave == 0){
        while(true){
            
            uint digital = (uint)((t_arr[count] / 3.3f) * 1023);
            printf("Voltage: %.2fV, D: %d\r\n", t_arr[count], digital);

            // Construct 16-bit command
            uint16_t command = 0;
            command |= (0b0111 << 12);       // Control bits: 0111 (write to channel A, buffered, 1x gain, output on)
            command |= (digital << 2);       // 10-bit value shifted into position (bits 11-2)

            // Split into two bytes for SPI
            uint8_t data[2];
            data[0] = (command >> 8) & 0xFF; // High byte
            data[1] = command & 0xFF;        // Low byte

            int len = 2;
            // write triangle_wave()[count] to DAC
            cs_select(PICO_DEFAULT_SPI_CSN_PIN);
            spi_write_blocking(spi_default, data, len); // where data is a uint8_t array with length len
            cs_deselect(PICO_DEFAULT_SPI_CSN_PIN);



            // wait for 1000/1/sample_rate ms, f = 1hz
            sleep_ms(1000/1/sample_rate);
            count++;
            if(count == sample_rate){count = 0;}

        }
    }
    if(wave == 1){
        while(true){
            uint digital = (uint)((s_arr[count] / 3.3f) * 1023);

            // Construct 16-bit command
            uint16_t command = 0;
            command |= (0b0111 << 12);       // Control bits: 0111 (write to channel A, buffered, 1x gain, output on)
            command |= (digital << 2);       // 10-bit value shifted into position (bits 11-2)

            // Split into two bytes for SPI
            uint8_t data[2];
            data[0] = (command >> 8) & 0xFF; // High byte
            data[1] = command & 0xFF;        // Low byte

            int len = 2;

            // write sine_wave()[count] to DAC
            cs_select(PICO_DEFAULT_SPI_CSN_PIN);
            spi_write_blocking(spi_default, data, len); // where data is a uint8_t array with length len
            cs_deselect(PICO_DEFAULT_SPI_CSN_PIN);


            // wait for 1000/2/sample_rate ms, f = 2hz
            sleep_ms(1000/2/sample_rate);
            
            count++;
            if(count == sample_rate){count = 0;}

        }
    }
}
