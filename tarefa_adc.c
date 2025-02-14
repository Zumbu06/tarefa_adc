#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"

#define BOTAO_A 5
#define BOTAO_JOY 22
#define JOY_Y 26
#define JOY_X 27
#define LED_R 13
#define LED_G 11
#define LED_B 12

static uint slice = 0;
static volatile bool flag = true;
static int16_t v_y = 2048;
static int16_t v_x = 2048;

void callback(uint gpio, uint32_t events)
{
    static uint32_t tempo_antes = 0;
    uint32_t tempo_agora = to_ms_since_boot(get_absolute_time());

    if (tempo_agora - tempo_antes > 200)
    {   
        if(gpio == BOTAO_A)
        {
            flag = !flag;
            pwm_set_enabled(slice, flag);
        } 
        else if (gpio == BOTAO_JOY)
        {
            gpio_put(LED_G, !gpio_get(LED_G));
        }
        tempo_antes = tempo_agora;
    }
    gpio_acknowledge_irq(gpio, events);
}

void gpio_configuracao()
{
    gpio_init(BOTAO_A);
    gpio_init(BOTAO_JOY);
    gpio_init(LED_R);
    gpio_init(LED_G);
    gpio_init(LED_B);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_set_dir(BOTAO_JOY, GPIO_IN);
    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_set_dir(LED_B, GPIO_OUT);
    gpio_pull_up(BOTAO_A);
    gpio_pull_up(BOTAO_JOY);
}

void pwm_configuracao()
{
    gpio_set_function(LED_R, GPIO_FUNC_PWM);
    gpio_set_function(LED_B, GPIO_FUNC_PWM);

    slice = pwm_gpio_to_slice_num(LED_R); 

    pwm_set_clkdiv(slice, 125.0);
    pwm_set_wrap(slice, 8196);
    pwm_set_gpio_level(LED_R, 0);
    pwm_set_gpio_level(LED_B, 0);
    pwm_set_enabled(slice, true);  
}

void adc_configuracao()
{
    adc_init();
    adc_gpio_init(JOY_Y);
    adc_gpio_init(JOY_X);
    adc_set_round_robin(0x03);
}

int main()
{   
    gpio_configuracao();
    pwm_configuracao();
    adc_configuracao();

    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, callback);
    gpio_set_irq_enabled_with_callback(BOTAO_JOY, GPIO_IRQ_EDGE_FALL, true, callback);

    int16_t y_led;
    int16_t x_led;

    while(true)
    {   
        adc_select_input(0);
        v_y = adc_read();
        adc_select_input(1);
        v_x = adc_read();

        if(flag)
        {
            y_led = abs(v_y - 2048) * 4;
            x_led = abs(v_x - 2048) * 4;
            pwm_set_gpio_level(LED_B, y_led);
            pwm_set_gpio_level(LED_R, x_led);
        }
        sleep_ms(100);
    }
}
