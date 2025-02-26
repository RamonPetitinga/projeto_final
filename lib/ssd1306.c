#include "ssd1306.h"
#include "font.h"
#include <string.h>

// Inicializa o display OLED
void ssd1306_init(ssd1306_t *ssd, uint8_t width, uint8_t height, bool external_vcc, uint8_t address, i2c_inst_t *i2c)
{
  ssd->width = width;
  ssd->height = height;
  ssd->pages = height / 8U;
  ssd->address = address;
  ssd->i2c_port = i2c;
  ssd->external_vcc = external_vcc;
  ssd->bufsize = ssd->pages * ssd->width + 1;
  ssd->ram_buffer = calloc(ssd->bufsize, sizeof(uint8_t));
  ssd->ram_buffer[0] = 0x40;  // Co = 0, D/C = 1 (dados)
  ssd->port_buffer[0] = 0x80; // Co = 1, D/C = 0 (comando)
}

// Envia um comando para o display
void ssd1306_command(ssd1306_t *ssd, uint8_t command)
{
  ssd->port_buffer[1] = command;
  i2c_write_blocking(ssd->i2c_port, ssd->address, ssd->port_buffer, 2, false);
}

// Envia os dados do buffer para o display
void ssd1306_send_data(ssd1306_t *ssd)
{
  ssd1306_command(ssd, SET_COL_ADDR);
  ssd1306_command(ssd, 0);
  ssd1306_command(ssd, ssd->width - 1);
  ssd1306_command(ssd, SET_PAGE_ADDR);
  ssd1306_command(ssd, 0);
  ssd1306_command(ssd, ssd->pages - 1);
  i2c_write_blocking(ssd->i2c_port, ssd->address, ssd->ram_buffer, ssd->bufsize, false);
}

// Configura o display OLED
void ssd1306_config(ssd1306_t *ssd)
{
  ssd1306_command(ssd, SET_DISP | 0x00);                 // Desliga o display
  ssd1306_command(ssd, SET_MEM_ADDR);                    // Configura o modo de endereçamento de memória
  ssd1306_command(ssd, 0x00);                            // Modo horizontal
  ssd1306_command(ssd, SET_DISP_START_LINE | 0x00);      // Define a linha inicial do display
  ssd1306_command(ssd, SET_SEG_REMAP | 0x01);            // Mapeamento de segmentos (inverte colunas)
  ssd1306_command(ssd, SET_MUX_RATIO);                   // Configura a proporção do multiplexador
  ssd1306_command(ssd, HEIGHT - 1);                      // Altura do display - 1
  ssd1306_command(ssd, SET_COM_OUT_DIR | 0x08);          // Define a direção dos pinos COM
  ssd1306_command(ssd, SET_DISP_OFFSET);                 // Define o deslocamento do display
  ssd1306_command(ssd, 0x00);                            // Sem deslocamento
  ssd1306_command(ssd, SET_COM_PIN_CFG);                 // Configura os pinos COM
  ssd1306_command(ssd, 0x12);                            // Configuração padrão para OLED 128x64
  ssd1306_command(ssd, SET_DISP_CLK_DIV);                // Configura o divisor de clock
  ssd1306_command(ssd, 0x80);                            // Frequência padrão
  ssd1306_command(ssd, SET_PRECHARGE);                   // Configura o tempo de pré-carga
  ssd1306_command(ssd, ssd->external_vcc ? 0x22 : 0xF1); // Configuração de pré-carga
  ssd1306_command(ssd, SET_VCOM_DESEL);                  // Configura o nível VCOMH
  ssd1306_command(ssd, 0x30);                            // Configuração padrão
  ssd1306_command(ssd, SET_CONTRAST);                    // Configura o contraste
  ssd1306_command(ssd, 0xFF);                            // Contraste máximo
  ssd1306_command(ssd, SET_ENTIRE_ON);                   // Habilita o display inteiro
  ssd1306_command(ssd, SET_NORM_INV);                    // Display normal (não invertido)
  ssd1306_command(ssd, SET_CHARGE_PUMP);                 // Configura a bomba de carga
  ssd1306_command(ssd, ssd->external_vcc ? 0x10 : 0x14); // Habilita a bomba de carga
  ssd1306_command(ssd, SET_DISP | 0x01);                 // Liga o display
}

// Desenha um pixel na posição (x, y)
void ssd1306_pixel(ssd1306_t *ssd, uint8_t x, uint8_t y, bool value)
{
  if (x >= ssd->width || y >= ssd->height)
    return; // Verifica limites
  uint16_t index = 1 + (y / 8) * ssd->width + x;
  uint8_t bit = y % 8;
  if (value)
    ssd->ram_buffer[index] |= (1 << bit);
  else
    ssd->ram_buffer[index] &= ~(1 << bit);
}

// Preenche o display com um valor (true = ligado, false = desligado)
void ssd1306_fill(ssd1306_t *ssd, bool value)
{
  uint8_t byte = value ? 0xFF : 0x00;
  for (uint16_t i = 1; i < ssd->bufsize; i++)
  {
    ssd->ram_buffer[i] = byte;
  }
}

// Desenha um retângulo
void ssd1306_rect(ssd1306_t *ssd, uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool value, bool fill)
{
  for (uint8_t i = x; i < x + w; i++)
  {
    for (uint8_t j = y; j < y + h; j++)
    {
      if (fill || i == x || i == x + w - 1 || j == y || j == y + h - 1)
      {
        ssd1306_pixel(ssd, i, j, value);
      }
    }
  }
}

// Desenha uma linha
void ssd1306_line(ssd1306_t *ssd, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool value)
{
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx - dy;

  while (true)
  {
    ssd1306_pixel(ssd, x0, y0, value);
    if (x0 == x1 && y0 == y1)
      break;
    int e2 = 2 * err;
    if (e2 > -dy)
    {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx)
    {
      err += dx;
      y0 += sy;
    }
  }
}

// Desenha um caractere na posição (x, y)
void ssd1306_draw_char(ssd1306_t *ssd, char c, uint8_t x, uint8_t y)
{
  uint16_t index = 0;
  if (c >= 'A' && c <= 'Z')
  {
    index = (c - 'A' + 11) * 8; // Letras maiúsculas (A-Z).
  }
  else if (c >= 'a' && c <= 'z')
  {
    index = (c - 'a' + 37) * 8; // Letras minúsculas (a-z)
  }
  else if (c >= '0' && c <= '9')
  {
    index = (c - '0' + 1) * 8; // Números (0-9)
  }
  else
  {
    return; // Caractere não suportado
  }

  for (uint8_t i = 0; i < 8; i++)
  {
    uint8_t line = font[index + i];
    for (uint8_t j = 0; j < 8; j++)
    {
      ssd1306_pixel(ssd, x + i, y + j, line & (1 << j));
    }
  }
}

// Desenha uma string na posição (x, y)
void ssd1306_draw_string(ssd1306_t *ssd, const char *str, uint8_t x, uint8_t y)
{
  while (*str)
  {
    ssd1306_draw_char(ssd, *str++, x, y);
    x += 8;
    if (x + 8 >= ssd->width)
    {
      x = 0;
      y += 8;
    }
    if (y + 8 >= ssd->height)
    {
      break;
    }
  }
}