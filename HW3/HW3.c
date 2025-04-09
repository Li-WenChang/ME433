#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

#define LED_button_PIN 12
#define LED_control_PIN 16

volatile bool ready = false; // this variable tells if the button is pressed

void led_control_callback(){
 
    // if detect a falling edge and the digital input is low after 10ms, meaning that it's a real press
    sleep_ms(10);
    if (gpio_get(LED_button_PIN) == false){

        gpio_put(LED_control_PIN, true); // LED off
        ready = true;
    }
}

void read_voltage(int n){
    for(int i = 0; i < n; i++){
        uint16_t result = adc_read();
        float voltage = 3.3/4095*result;
        printf("Voltage: %.2fV\r\n", voltage);
        sleep_ms(10); // maintain 100hz, but this is not technically 100 hz
    }
    printf("\r\n");
}


int main()
{
    stdio_init_all();

    // wait until the USB port is opened
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }

    // initialize the button pin ans set it as input pin
    gpio_init(LED_button_PIN);
    gpio_set_dir(LED_button_PIN, GPIO_IN);

    // initialize the LED pin ans set it as output pin
    gpio_init(LED_control_PIN);
    gpio_set_dir(LED_control_PIN, GPIO_OUT);
    gpio_put(LED_control_PIN, false); // LED on

    adc_init(); // init the adc module
    adc_gpio_init(26); // set ADC0 pin to be adc input instead of GPIO
    adc_select_input(0); // select to read from ADC0

    
 
    // if the button, the led_control_callback function will be called
    // and turn off the LED
    gpio_set_irq_enabled_with_callback(LED_button_PIN, GPIO_IRQ_EDGE_FALL, true, &led_control_callback);
 
    
    printf("Start!\n");


    // wait until the button is pressed and LED is turned off
    while(!ready){}
    

    while (1) {
        int sample_number = 0;
        // char message[100];
        printf("Enter a number, between 1 and 100: \r\n");
        scanf("%d", &sample_number);
        printf("Number of samples: %d\r\n", sample_number);
        if(sample_number> 100 || sample_number < 1){
            printf("Your number %d is not valid, try a different number.\r\n", sample_number);
        }
        else{
            read_voltage(sample_number);
        }

    }
}
