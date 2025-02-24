/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "ssd1306.h"
#include "hardware/i2c.h"
#include "play_audio.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "neoLed.h"

// Biblioteca gerada pelo arquivo .pio durante compilação.
#include "ws2818b.pio.h"

#define GET_SLICE(x) (x >> 1) & 0x7

uint8_t y_1 = 0;
uint8_t y2 = 24, mode, note_index, oct_index;

#define PADRAO 0
#define NOTA   1
#define DIREC  2


const uint I2C_SDA_PIN = 14;
const uint I2C_SCL_PIN = 15;

uint8_t led_i = 0;
uint64_t us_current_val[2] = {0, 0}, us_last_value[2] = {0, 0};

// Estados para o gerenciamento dos botões
typedef enum
{
  IDLE,         // Estado inativo
  DEBOUNCING_A, // Debounce para o botão A
  RELEASE_A,    // Liberação do botão A
  DEBOUNCING_B, // Debounce para o botão B
  RELEASE_B,    // Liberação do botão B
  ACTION_A,     // Ação após o botão A ser pressionado
  ACTION_B      // Ação após o botão B ser pressionado
} state_button;


// Função para ler o estado dos botões com debounce
void read_buttons()
{
  static state_button s = IDLE;    // Estado atual do botão
  static uint cnt = 0;             // Contador para debounce
  const uint DEBOUNCE_CYCLES = 10; // Ciclos de debounce
  uint8_t slice_num;
  uint ch;

  // Máquina de estados para leitura dos botões
  switch (s)
  {
  case IDLE:                     // Estado inativo, monitora os botões
  
    if ( gpio_get(BUTTON_A) == 0) // Se o botão A foi pressionado
      s = DEBOUNCING_A;          // Vai para o estado de debounce do botão A

    if ( (bool)(sio_hw->gpio_in & (1ul << (uint)BUTTON_B)) == 0) // Se o botão B foi pressionado
      s = DEBOUNCING_B;          // Vai para o estado de debounce do botão B
    cnt = 0;                     // Reinicia o contador
    break;


  case DEBOUNCING_A: // Estado de debounce do botão A
    if ( gpio_get(BUTTON_A) == 0)
    {        // Se o botão A continua pressionado
      cnt++; // Incrementa o contador
      if (cnt > DEBOUNCE_CYCLES)
      {                // Se passou o tempo de debounce
        cnt = 0;       // Reinicia o contador
        s = RELEASE_A; // Vai para o estado de liberação do botão A
      }
    }
    else
    {
      s = IDLE; // Se o botão foi liberado, volta ao estado inativo
    }
    break;



  case DEBOUNCING_B: // Estado de debounce do botão B
    if ( (bool)(sio_hw->gpio_in & (1ul << (uint)BUTTON_B)) == 0)
    {        // Se o botão B continua pressionado
      cnt++; // Incrementa o contador
      if (cnt > DEBOUNCE_CYCLES)
      {                // Se passou o tempo de debounce
        cnt = 0;       // Reinicia o contador
        s = RELEASE_B; // Vai para o estado de liberação do botão B
      }
    }
    else
    {
      s = IDLE; // Se o botão foi liberado, volta ao estado inativo
    }
    break;



  case RELEASE_A:                // Estado de liberação do botão A
    if ( gpio_get(BUTTON_A) == 1) // Se o botão A foi solto
      s = ACTION_A;              // Vai para a ação associada ao botão A
    break;



  case RELEASE_B: 
  
  // Estado de liberação do botão B
    if ( (bool)(sio_hw->gpio_in & (1ul << (uint)BUTTON_B)) == 1) // Se o botão B foi solto
      s = ACTION_B;              // Vai para a ação associada ao botão B
    break;



  case ACTION_A:            // Ação do botão A (aumenta brilho do LED e divisor do buzzer)
    s = IDLE;               // Volta para o estado inativo

    mode = (mode +1) % 3;

    if(mode == PADRAO){


        y_1 = 0; y2 = 24;

        sio_hw->gpio_clr = 1 << 11;
        sio_hw->gpio_clr = 1 << 12;
        sio_hw->gpio_set = 1 << 13;

    }else if(mode == NOTA){


        y_1 = 40; y2 = 70;

        sio_hw->gpio_clr = 1 << 11;
        sio_hw->gpio_set = 1 << 12;
        sio_hw->gpio_clr = 1 << 13;

    }else if(mode == DIREC){

        y_1 = 88; y2 = 110;
        sio_hw->gpio_set = 1 << 11;
        sio_hw->gpio_clr = 1 << 12;
        sio_hw->gpio_clr = 1 << 13;


    }

 
    
   
    display_set = false;
  
    
   
    break;



  case ACTION_B:              // Ação do botão B (diminui brilho do LED e divisor do buzzer)
    s = IDLE;                 // Volta para o estado inativo
    if(mode == PADRAO){


        current_pattern = (current_pattern + 1) % 8;


    }else if(mode == NOTA){

        note_index++;
        if(note_index == 36)note_index = 0;
        uint8_t index = note_index % 12;
        if(note_index < 12){
            
            oct_index = 1;

        }else if(note_index < 24){

            oct_index = 2;
        }else if(note_index < 36){
            oct_index = 4;

        }

        note = base_notes[(index)]/oct_index;


    }else if(mode == DIREC){

        dir = (dir + 1);
        if(dir == 3) dir = 0;

    }


    display_set = false;

    

    break;

  default: // Estado padrão (se houver erro, volta ao inativo)
    s = IDLE;
    cnt = 0;
  }
}



int main()
{
    stdio_init_all();
    setup_gpio();

    // Inicializa matriz de LEDs NeoPixel.
    npInit(LED_PIN);
    npClear();

    // Aqui, você desenha nos LEDs.
    for (uint i = 0; i < LED_COUNT; i++)
    {

        npSetLED(i, 0, 1, 2);
    }

    npWrite(); // Escreve os dados nos LEDs.



    // I2C is "open drain", pull ups to keep signal high when no data is being
    // sent
    i2c_init(i2c1, SSD1306_I2C_CLK * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    // run through the complete initialization process
    SSD1306_init();

    // Initialize render area for entire frame (SSD1306_WIDTH pixels by SSD1306_NUM_PAGES pages)
    struct render_area frame_area = {
        start_col : 0,
        end_col : SSD1306_WIDTH - 1,
        start_page : 0,
        end_page : SSD1306_NUM_PAGES - 1
    };

    calc_render_area_buflen(&frame_area);

    // zero the entire display
    uint8_t buf[SSD1306_BUF_LEN];
    memset(buf, 0, SSD1306_BUF_LEN);
    render(buf, &frame_area);

restart:

    SSD1306_scroll(true);
    sleep_ms(1000);
    SSD1306_scroll(false);

    char *text[] = {
        " VOCALEASY 2025 ",
        " ",
        "",
        "  APERTE START  "
    };
        

    int y = 0;
    int x = 0;
    for (uint i = 0; i < count_of(text); i++)
    {
        WriteString(buf, x, y, text[i]);
        y += 8;
    }
    render(buf, &frame_area);

    while(gpio_get(BUTTON_C));


    memset(buf, 0, SSD1306_BUF_LEN);
    render(buf, &frame_area);


    char *text2[] = {
        "PAD  NOTA  DIR",
        " ",
        " ",
        " ",
        " ",
        " ",
        " ",
        " ",
    };
     

    y = 0;
    x = 0;
    for (uint i = 0; i < count_of(text2); i++)
    {
        WriteString(buf, x, y, text2[i]);
        y += 8;
    }
    render(buf, &frame_area);

    DrawLine(buf, y_1, 20, y2, 20, true);
    render(buf, &frame_area);

    sio_hw->gpio_set = 1 << 13;

    setup_audio();

    sleep_ms(500);
    char direcao[5];// = {};

    display_set = false;

    // loop principal
    while (true)
    {

      //  main_audio();
        read_buttons(); 

        if(vocalize){

            
            if(gpio_get(BUTTON_C)){

                ///read_buttons();       // Lê o estado dos botões
        
            }else{
        
                irq_set_enabled(PWM_IRQ_WRAP, false);
                uint slice_num = GET_SLICE(BUZZER_A);
                pwm_hw->slice[slice_num].csr &= ~(uint)0x1;
                slice_num = GET_SLICE(BUZZER_B);
                pwm_hw->slice[slice_num].csr &= ~(uint)0x1;
                vocalize = false;
                display_set = false;

                DrawLine(buf, 0, 20, 127, 20, false);
                render(buf, &frame_area);

                DrawLine(buf, y_1, 20, y2, 20, true);
                render(buf, &frame_area);

                sleep_ms(200);
            }


            if(!display_set){
    

                char *text2[] = {              
                    "  APERTE START  ",
                    "  PARA SAIR     ",
                };

                y = 48;
                x = 0;
                for (uint i = 0; i < count_of(text2); i++)
                {
                    WriteString(buf, x, y, text2[i]);
                    y += 8;
                }

           

                if(dir == UP)  strcpy(direcao, "UP  ");
                if(dir == DOWN) strcpy(direcao, "DOWN");
                if(dir == STAY) strcpy(direcao, "STAY");

  
                x = 87;
                for (uint i = 0; i < 4; i++)
                {
                    WriteChar(buf, x, 8, direcao[i]);
                    x += 8;
                }

                x = 44;
                WriteString(buf, x, 8, base_symbols[note_index]);

                x = 9;
                WriteChar(buf, x, 8, (char)(current_pattern + 48));
               
                DrawLine(buf, 0, 20, 127, 20, false);
               
                render(buf, &frame_area);
                display_set = true;

                DrawLine(buf, y_1, 20, y2, 20, true);
               // render(buf, &frame_area);


                render(buf, &frame_area);
                display_set = true;
                sleep_ms(500);

            }


        


        }else{


            if(gpio_get(BUTTON_C)){




            }else{
            
       
                pattern_index = 0;
                rhythm_index = 0;
                irq_set_enabled(PWM_IRQ_WRAP, true);
                vocalize = true;
                display_set = false;
                sleep_ms(200);
        
            }

            if(!display_set){
        
                char *text2[] = {
                    "  APERTE START  ",
                    "  PARA INICIAR  ",
                };

                y = 48;
                x = 0;
                for (uint i = 0; i < count_of(text2); i++)
                {
                    WriteString(buf, x, y, text2[i]);
                    y += 8;
                }

                if(dir == UP)  strcpy(direcao, "UP  ");
                if(dir == DOWN) strcpy(direcao, "DOWN");
                if(dir == STAY) strcpy(direcao, "STAY");
  
                x = 87;
                for (uint i = 0; i < 4; i++)
                {
                    WriteChar(buf, x, 8, direcao[i]);
                    x += 8;
                }
                
                x = 44;
                WriteString(buf, x, 8, base_symbols[note_index]);

                x = 9;
                WriteChar(buf, x, 8, (char)(current_pattern + 48));


             //   render(buf, &frame_area);

                DrawLine(buf, 0, 20, 127, 20, false);
               
                render(buf, &frame_area);
                display_set = true;

                DrawLine(buf, y_1, 20, y2, 20, true);
                render(buf, &frame_area);
                sleep_ms(500);

            }    


        }


    }

    return 0;
}
