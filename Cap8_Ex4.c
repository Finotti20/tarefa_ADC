#include <stdio.h>            
#include "pico/stdlib.h"      
#include "hardware/adc.h"     
#include "hardware/pwm.h"     

#define VRX_PIN 26  
#define VRY_PIN 27  
#define LED_PIN 13  
#define LED2_PIN 12
#define LED3_PIN 11
#define SW_PIN 22
#define BUTTON_A 5
#define BUTTON_B 6
#define DEBOUNCE_TIME 200000  

static volatile uint32_t last_time = 0;
static volatile uint32_t last_time_red = 0;
static volatile uint32_t last_time_blue = 0;
static volatile bool led_state = false;
static volatile bool led_state_red = false;
static volatile bool led_state_blue = false;

static void gpio_irq_handler(uint gpio, uint32_t events);

uint pwm_init_gpio(uint gpio, uint wrap) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_enabled(slice_num, true);  
    return slice_num;  
}

int main() {
    stdio_init_all();
    sleep_ms(2000);

    adc_init(); 
    adc_gpio_init(VRX_PIN); 
    adc_gpio_init(VRY_PIN); 

    uint pwm_wrap = 4096;  
    uint slice_led_red = pwm_init_gpio(LED_PIN, pwm_wrap);
    uint slice_led_blue = pwm_init_gpio(LED2_PIN, pwm_wrap);

    // Configuração dos botões
    gpio_init(SW_PIN);
    gpio_set_dir(SW_PIN, GPIO_IN);
    gpio_pull_up(SW_PIN);

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    // Configuração dos LEDs
    gpio_init(LED3_PIN);
    gpio_set_dir(LED3_PIN, GPIO_OUT);
    gpio_put(LED3_PIN, led_state);

    gpio_set_function(LED2_PIN, GPIO_FUNC_PWM);
    gpio_set_function(LED_PIN, GPIO_FUNC_PWM);

    // Configuração das interrupções
    gpio_set_irq_enabled_with_callback(SW_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled(BUTTON_A, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BUTTON_B, GPIO_IRQ_EDGE_FALL, true);

    while (true) {
        adc_select_input(0);  
        uint16_t vrx_value = adc_read(); 

        adc_select_input(1);  
        uint16_t vry_value = adc_read();

        // 🔴 Se o LED vermelho estiver ligado, aplica PWM, exceto quando vrx_value estiver entre 2020 e 2024
        if (led_state_red && !(vrx_value >= 2018 && vrx_value <= 2048)) {
            gpio_set_function(LED_PIN, GPIO_FUNC_PWM); // Garante que está em PWM
            pwm_set_gpio_level(LED_PIN, vrx_value); 
            printf("O valor de vrx_value e: %d\n", vrx_value);
        } else {
            gpio_set_function(LED_PIN, GPIO_FUNC_SIO);
            gpio_set_dir(LED_PIN, GPIO_OUT);
            gpio_put(LED_PIN, 0);
        }

        // 🔵 Se o LED azul estiver ligado, aplica PWM
        if (led_state_blue && !(vrx_value >= 2018 && vrx_value <= 2048)) {
            gpio_set_function(LED2_PIN, GPIO_FUNC_PWM);
            pwm_set_gpio_level(LED2_PIN, vry_value);
        } else {
            gpio_set_function(LED2_PIN, GPIO_FUNC_SIO);
            gpio_set_dir(LED2_PIN, GPIO_OUT);
            gpio_put(LED2_PIN, 0);
        }

        sleep_ms(100);  
    }

    return 0;  
}

void gpio_irq_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    if (gpio == SW_PIN) {
        if (current_time - last_time > DEBOUNCE_TIME) {
            last_time = current_time;
            led_state = !led_state;
            gpio_put(LED3_PIN, led_state);
        }
    }
    
    if (gpio == BUTTON_A) {
        if (current_time - last_time_red > DEBOUNCE_TIME) {
            last_time_red = current_time;
            led_state_red = !led_state_red;
        }
    }

    if (gpio == BUTTON_B) {
        if (current_time - last_time_blue > DEBOUNCE_TIME) {
            last_time_blue = current_time;
            led_state_blue = !led_state_blue;
        }
    }

    gpio_acknowledge_irq(gpio, events);
}



