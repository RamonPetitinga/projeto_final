#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "lib/ssd1306.h"
#include "pico/bootrom.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define DISPLAY_ADDR 0x3C

#define JOYSTICK_X_PIN 27
#define JOYSTICK_Y_PIN 26
#define JOYSTICK_PB 22

#define BUTTON_A 5
#define BUTTON_B 6
#define LED_GREEN 11
#define LED_RED 13
#define BUZZER_PIN 10

#define SENHA_CORRETA "1234"
#define TENTATIVAS_MAX 3

// Variáveis globais
ssd1306_t ssd;
char senha_digitada[5] = "";
uint8_t tentativas = 0;
bool cofre_aberto = false;
int border_size = 2;

// Posição do cursor no teclado numérico
uint8_t cursor_x = 0;
uint8_t cursor_y = 0;

// Matriz do teclado numérico (4x3) DISPLAY
const char teclado[4][3] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};

// Frequências das notas musicais (em Hz)
#define NOTA_C5 523 // Dó5
#define NOTA_E5 659 // Mi5
#define NOTA_G5 784 // Sol5
#define NOTA_A5 880 // Lá5

// Variáveis para debounce
uint32_t ultima_leitura_joystick = 0;
uint16_t joystick_x_debounced = 0;
uint16_t joystick_y_debounced = 0;
uint32_t ultima_movimentacao_cursor = 0;
const uint32_t intervalo_debounce_joystick = 50;
const uint32_t intervalo_movimentacao_cursor = 200;

// Função para exibir mensagem no display OLED
void exibir_mensagem(const char *mensagem)
{
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, mensagem, 0, 0);
    ssd1306_send_data(&ssd);
}

// Função para desenhar o teclado numérico no display
void desenhar_teclado()
{
    ssd1306_fill(&ssd, false); // Limpa a tela

    // Desenha a borda da tela
    for (int i = 0; i < border_size; i++)
    {
        ssd1306_rect(&ssd, i, i, WIDTH - (2 * i), HEIGHT - (2 * i), true, false);
    }

    // Desenha os números do teclado
    for (uint8_t i = 0; i < 4; i++)
    {
        for (uint8_t j = 0; j < 3; j++)
        {
            uint8_t x = j * 42; // Posição X do número
            uint8_t y = i * 16; // Posição Y do número

            // Desenha o número
            ssd1306_draw_char(&ssd, teclado[i][j], x + 10, y + 5);

            // Desenha um retângulo ao redor do número selecionado
            if (i == cursor_y && j == cursor_x)
            {
                ssd1306_rect(&ssd, x, y, 40, 15, true, false);
            }
        }
    }

    ssd1306_send_data(&ssd); // Envia os dados para o display
}

// Função para ler o joystick com debounce
void ler_joystick_com_debounce()
{
    uint32_t tempo_atual = to_ms_since_boot(get_absolute_time());

    // Verifica se o intervalo de debounce já passou
    if (tempo_atual - ultima_leitura_joystick >= intervalo_debounce_joystick)
    {
        // Lê os valores do joystick
        adc_select_input(1); // Seleciona o canal do eixo Y (agora X)
        uint16_t adc_x = adc_read();
        adc_select_input(0); // Seleciona o canal do eixo X (agora Y)
        uint16_t adc_y = adc_read();

        // Aplica uma zona morta para evitar movimentos indesejados
        if (adc_x < 1500 || adc_x > 2500) // Zona morta no eixo X
        {
            joystick_x_debounced = adc_x;
        }
        else
        {
            joystick_x_debounced = 2048; // Valor central (nenhum movimento)
        }

        if (adc_y < 1500 || adc_y > 2500) // Zona morta no eixo Y
        {
            joystick_y_debounced = adc_y;
        }
        else
        {
            joystick_y_debounced = 2048; // Valor central (nenhum movimento)
        }

        // Atualiza o tempo da última leitura
        ultima_leitura_joystick = tempo_atual;
    }
}

// Função para mover o cursor com base no joystick
void mover_cursor()
{
    uint32_t tempo_atual = to_ms_since_boot(get_absolute_time());

    // Verifica se o intervalo mínimo entre movimentos já passou
    if (tempo_atual - ultima_movimentacao_cursor >= intervalo_movimentacao_cursor)
    {
        // Usa os valores do joystick com debounce
        uint16_t adc_x = joystick_x_debounced;
        uint16_t adc_y = joystick_y_debounced;

        // Movimento para a esquerda (eixo X)
        if (adc_x < 1000 && cursor_x > 0)
        {
            cursor_x--;
            ultima_movimentacao_cursor = tempo_atual; // Atualiza o tempo da última movimentação
        }
        // Movimento para a direita (eixo X)
        else if (adc_x > 3000 && cursor_x < 2)
        {
            cursor_x++;
            ultima_movimentacao_cursor = tempo_atual; // Atualiza o tempo da última movimentação
        }

        // Movimento para cima (eixo Y) - Agora invertido
        if (adc_y > 3000 && cursor_y > 0)
        {
            cursor_y--;
            ultima_movimentacao_cursor = tempo_atual; // Atualiza o tempo da última movimentação
        }
        // Movimento para baixo (eixo Y) - Agora invertido
        else if (adc_y < 1000 && cursor_y < 3)
        {
            cursor_y++;
            ultima_movimentacao_cursor = tempo_atual; // Atualiza o tempo da última movimentação
        }

        // Atualiza o teclado no display
        desenhar_teclado();
    }
}
