#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "ssd1306.h" 
#include "font.h" 

#define BOTAO_A 5
#define BOTAO_JOY 22
#define JOY_Y 26
#define JOY_X 27
#define LED_R 13
#define LED_G 11
#define LED_B 12
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define WIDTH 128
#define HEIGHT 64

static uint slice = 0;
static volatile bool flag = true;
static int16_t v_y = 2048;
static int16_t v_x = 2048;
static ssd1306_t ssd;
static uint8_t borda_estilo = 0; // Variável para alternar entre estilos de borda

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
            gpio_put(LED_G, !gpio_get(LED_G)); // Alterna o LED verde
            borda_estilo = (borda_estilo + 1) % 3; // Alterna entre 3 estilos de borda
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

void i2c_configuracao()
{
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

void desenha_borda()
{
    // Desenha a borda com base no estilo selecionado
    ssd1306_fill(&ssd, false);

    if (borda_estilo == 0) { // Borda simples
        ssd1306_rect(&ssd, 0, 0, WIDTH, HEIGHT, true, false); // Borda sólida
    }
    else if (borda_estilo == 1) { // Borda dupla
        // Ajuste do espaçamento entre a primeira e a terceira borda
        ssd1306_rect(&ssd, 2, 2, WIDTH - 4, HEIGHT - 4, true, false); // Borda interna
        ssd1306_rect(&ssd, 0, 0, WIDTH, HEIGHT, true, false); // Borda externa
    }
    else if (borda_estilo == 2) { // Borda com espaços
        for (int i = 0; i < 3; i++) {
            ssd1306_rect(&ssd, i * 2, i * 2, WIDTH - (i * 2 * 2), HEIGHT - (i * 2 * 2), true, false);
        }
    }

    ssd1306_send_data(&ssd);
}

void quadrado(int16_t x_quad, int16_t y_quad)
{   
    // Atualiza a posição do quadrado
    ssd1306_fill(&ssd, false);

    // Restrições para o quadrado não sair da área da tela
    if (x_quad < 176) {
        x_quad = 176;
    }
    if (x_quad > 3920) {
        x_quad = 3920;
    }
    if (y_quad < 1218) {
        y_quad = 1218;
    }
    if (y_quad > 2878) {
        y_quad = 2878;
    }

    // Ajusta a posição do quadrado para as coordenadas da tela
    x_quad = x_quad - 176;
    x_quad = (int)(x_quad / 32.5) + 3;
    y_quad = y_quad - 1218;
    y_quad = (int)(y_quad / 33.0);
    y_quad = abs(y_quad - 50) + 3;

    // Calcula a posição da borda, limitando o quadrado à borda atual
    int limite_x = (borda_estilo == 0) ? WIDTH : (borda_estilo == 1) ? WIDTH - 4 : WIDTH - 6;
    int limite_y = (borda_estilo == 0) ? HEIGHT : (borda_estilo == 1) ? HEIGHT - 4 : HEIGHT - 6;

    // Impede o quadrado de ultrapassar as bordas
    if (x_quad + 8 > limite_x) {
        x_quad = limite_x - 8;
    }
    if (y_quad + 8 > limite_y) {
        y_quad = limite_y - 8;
    }

    // Desenha o quadrado
    ssd1306_rect(&ssd, y_quad, x_quad, 8, 8, true, true);
    ssd1306_send_data(&ssd);
}

int main()
{   
    gpio_configuracao();
    pwm_configuracao();
    adc_configuracao();
    i2c_configuracao();

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

        desenha_borda(); // Desenha a borda no display
        quadrado(v_x, v_y); // Atualiza a posição do quadrado no display
        sleep_ms(10);
    }
}