#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "lib/ssd1306.h"
#include "pico/bootrom.h"
#include "lib/ws2812.pio.h"

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

// Configurações da matriz de LEDs WS2812
#define LED_PIN 7    // Pino de controle dos LEDs WS2812
#define LED_COUNT 25 // 5x5 = 25 LEDs

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

// Função para definir a cor de todos os LEDs da matriz com 50% de intensidade
void set_led_matrix_color(uint32_t color)
{
    // Aplica 50% de intensidade a cada canal de cor
    uint8_t red = (color >> 16) & 0xFF;  // Extrai o canal vermelho
    uint8_t green = (color >> 8) & 0xFF; // Extrai o canal verde

    red = red / 2;     // 50% de intensidade para o vermelho
    green = green / 2; // 50% de intensidade para o verde

    // Converte a cor para o formato GRB
    uint32_t grb_color = (green << 16) | (red << 8);

    // Envia a cor para todos os LEDs
    for (int i = 0; i < LED_COUNT; i++)
    {
        pio_sm_put_blocking(pio0, 0, grb_color << 8u);
    }
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

// Função para verificar a senha
bool verificar_senha(const char *senha)
{
    return strcmp(senha, SENHA_CORRETA) == 0;
}

// Função para bloquear o sistema após tentativas falhas
void bloquear_sistema()
{
    exibir_mensagem("BLOQUEADO!");
    gpio_put(LED_RED, 1);           // Acende o LED vermelho
    set_led_matrix_color(0xFF0000); // Exibe vermelho na matriz de LEDs
    sleep_ms(10000);                // Bloqueia por 10 segundos
    gpio_put(LED_RED, 0);           // Apaga o LED vermelho
    set_led_matrix_color(0x000000); // Desliga a matriz de LEDs
    tentativas = 0;                 // Reseta o contador de tentativas
}

// Função para tocar uma nota musical no buzzer
void tocar_nota(uint32_t frequencia, uint32_t duracao_ms)
{
    // Configura o pino do buzzer para PWM
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint channel_num = pwm_gpio_to_channel(BUZZER_PIN);

    // Configura a frequência do PWM
    uint32_t clock = 125000000; // Clock do Raspberry Pi Pico (125 MHz)
    uint32_t divisor = 16;      // Divisor para reduzir a frequência
    uint32_t wrap = clock / (divisor * frequencia) - 1;

    pwm_set_clkdiv(slice_num, divisor);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, channel_num, wrap / 2); // 50% de duty cycle
    pwm_set_enabled(slice_num, true);

    // Mantém a nota tocando pelo tempo especificado
    sleep_ms(duracao_ms);

    // Desliga o PWM
    pwm_set_enabled(slice_num, false);
}

// Função para tocar uma melodia de sucesso (senha correta)
void tocar_melodia_sucesso()
{
    tocar_nota(NOTA_C5, 200); // Dó5
    sleep_ms(50);
    tocar_nota(NOTA_E5, 200); // Mi5
    sleep_ms(50);
    tocar_nota(NOTA_G5, 200); // Sol5
    sleep_ms(50);
    tocar_nota(NOTA_A5, 400); // Lá5
}

// Função para tocar um som de erro (senha incorreta)
void tocar_som_erro()
{
    tocar_nota(NOTA_C5, 200); // Dó5
    sleep_ms(50);
    tocar_nota(NOTA_C5, 200); // Dó5
}

// Função para debounce do botão
bool debounce(uint pin)
{
    if (!gpio_get(pin)) // Verifica se o botão está pressionado
    {
        sleep_ms(50);       // Aguarda 50ms para estabilizar o sinal
        if (!gpio_get(pin)) // Verifica novamente
        {
            return true; // Botão pressionado
        }
    }
    return false; // Botão não pressionado
}

int main()
{
    stdio_init_all();

    // Inicialização do I2C e display OLED
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init(&ssd, WIDTH, HEIGHT, false, DISPLAY_ADDR, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Inicialização do ADC para o joystick
    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);
    adc_gpio_init(JOYSTICK_Y_PIN);

    // Inicialização dos GPIOs
    gpio_init(JOYSTICK_PB);
    gpio_set_dir(JOYSTICK_PB, GPIO_IN);
    gpio_pull_up(JOYSTICK_PB);

    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(BUTTON_B); // Inicializa o botão B
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);

    // Inicialização da matriz de LEDs WS2812
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, 0, offset, LED_PIN, 800000, false);

    // Exibe o teclado no display
    desenhar_teclado();

    // Loop principal
    uint8_t indice_senha = 0;
    while (true)
    {
        // Lê o joystick com debounce
        ler_joystick_com_debounce();

        // Move o cursor com o joystick
        mover_cursor();

        // Verifica se o botão A foi pressionado (com debounce)
        if (debounce(BUTTON_A))
        {
            // Adiciona o número selecionado à senha
            senha_digitada[indice_senha] = teclado[cursor_y][cursor_x];
            indice_senha++;

            // Exibe a senha digitada no display
            char mensagem[20];
            snprintf(mensagem, sizeof(mensagem), "Senha: %s", senha_digitada);
            exibir_mensagem(mensagem);

            // Aguarda o botão ser solto
            while (!gpio_get(BUTTON_A))
            {
                sleep_ms(10);
            }

            // Verifica se a senha foi completamente digitada
            if (indice_senha == 4)
            {
                // Quando a senha estiver correta
                if (verificar_senha(senha_digitada))
                {
                    exibir_mensagem("ACESSO LIBERADO!");
                    gpio_put(LED_GREEN, 1);         // Acende o LED verde
                    set_led_matrix_color(0x00FF00); // Exibe verde na matriz de LEDs (corrigido)
                    tocar_melodia_sucesso();        // Toca a melodia de sucesso
                    cofre_aberto = true;
                    sleep_ms(5000);                 // Mantém o cofre aberto por 5 segundos
                    gpio_put(LED_GREEN, 0);         // Apaga o LED verde
                    set_led_matrix_color(0x000000); // Desliga a matriz de LEDs
                    cofre_aberto = false;
                }
                else
                {
                    tentativas++;
                    exibir_mensagem("SENHA INCORRETA!");
                    gpio_put(LED_RED, 1);           // Acende o LED vermelho
                    set_led_matrix_color(0xFF0000); // Exibe vermelho na matriz de LEDs (corrigido)
                    tocar_som_erro();               // Toca o som de erro
                    sleep_ms(2000);                 // Mantém o LED vermelho aceso por 2 segundos
                    gpio_put(LED_RED, 0);           // Apaga o LED vermelho
                    set_led_matrix_color(0x000000); // Desliga a matriz de LEDs

                    if (tentativas >= TENTATIVAS_MAX)
                    {
                        bloquear_sistema();
                    }
                }

                // Reseta a senha digitada
                indice_senha = 0;
                memset(senha_digitada, 0, sizeof(senha_digitada));
                desenhar_teclado();
            }
        }

        // Verifica se o botão B foi pressionado (com debounce)
        if (debounce(BUTTON_B))
        {
            exibir_mensagem("Entrando no modo USB...");
            sleep_ms(1000);       // Aguarda um pouco para exibir a mensagem
            reset_usb_boot(0, 0); // Entra no modo de bootloader USB
        }

        sleep_ms(10); // Pequeno atraso para evitar leituras rápidas demais
    }
}