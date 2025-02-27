# Sistema de Controle de Acesso com Joystick e Display SSD1306

Link vídeo:https://youtu.be/oRs8hdRdmHI

# Descrição

Este projeto implementa um sistema de controle de acesso baseado em senha, utilizando um joystick analógico para navegação e seleção de dígitos, um display OLED SSD1306 para exibição do teclado e feedback ao usuário, e uma matriz de LEDs WS2812 para indicação visual do status do sistema. O sistema também conta com um buzzer para fornecer feedback sonoro em caso de sucesso ou erro.

# Funcionalidades

Entrada de senha usando um joystick analógico.

Exibição do teclado numérico e mensagens no display OLED SSD1306.

Indicação visual por matriz de LEDs WS2812.

Feedback sonoro através de um buzzer.

Bloqueio do sistema após três tentativas falhas.

Modo de boot USB acionado por um botão dedicado.

# Componentes Utilizados

Microcontrolador: Raspberry Pi Pico W

Display: OLED SSD1306 via I2C

Joystick: Entrada analógica via ADC

Matriz de LEDs: WS2812 (5x5)

Buzzer: Gerador de som via PWM

LEDs Indicadores: LED verde e LED vermelho

Botões: Botão A para confirmação, Botão B para entrar no modo USB

# Configuração de Hardware

Os seguintes pinos do Raspberry Pi Pico W são utilizados:

Componente

Pino GPIO
I2C SDA 14
I2C SCL 15
Joystick X 27
Joystick Y 26
Joystick PB 22
Botão A 5
Botão B 6
LED Verde 11
LED Vermelho 13
Buzzer 10
Matriz WS2812 7

# Dependências

Este projeto depende das seguintes bibliotecas:

hardware/adc.h - Para leitura do joystick

hardware/i2c.h - Para comunicação com o display ssd1306

hardware/pwm.h - Para controle do buzzer

hardware/pio.h - Para controle da matriz de LEDs WS2812

hardware/clocks.h - Para manipulação de temporização

lib/ssd1306.h - Biblioteca para o display ssd1306

lib/ws2812.pio.h - Biblioteca para controle dos LEDs WS2812

# Como Funciona

Inicialização: Configuração dos periféricos, incluindo ADC, I2C e GPIOs.

Exibição do teclado: O teclado numérico é desenhado no display ssd1306.

Entrada de senha: O usuário move o cursor com o joystick e pressiona o botão A para selecionar números.

Validação: Quando a senha completa é digitada, o sistema verifica se é correta.

Se correta: O cofre é aberto, o LED verde acende e uma melodia de sucesso é tocada.

Se incorreta: O LED vermelho acende, um som de erro é tocado e o número de tentativas aumenta.

Se três tentativas falharem: O sistema é bloqueado por 10 segundos.

Modo USB: Se o botão B for pressionado, o sistema entra no modo boot USB para atualização do firmware.

# Como Usar

Ligue o sistema e observe o teclado numérico no display ssd1306.

Use o joystick para mover o cursor sobre os números.

Pressione o botão A para inserir os números da senha - SENHA PADRÃO "1234"

Se a senha estiver correta, o sistema liberará o acesso.

Se errar três vezes, o sistema se bloqueará temporariamente.

Para entrar no modo USB, pressione o botão B.

# Como Compilar e Executar

1️⃣ Clonar o repositório

git clone https://github.com/RamonPetitinga/projeto_final

2️⃣ Configurar o ambiente

Certifique-se de que o Pico SDK está configurado corretamente e que você tem o CMake instalado.

3️⃣ Compilar e Executar
Compile no VS Code e implemente o codigo na sua BitDogLab.
