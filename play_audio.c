#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include "pico/time.h"
#include "notes.h"


// Definição dos pinos utilizados
const uint BUZZER_A = 21; // Pino para o buzzer A
const uint BUZZER_B = 10; // Pino para o buzzer B
const uint BUTTON_A = 5;  // Pino para o botão A
const uint BUTTON_B = 6;  // Pino para o botão B
const uint BUTTON_C = 22;  // Pino para o botão B

const uint LED = 12;      // Pino para o LED

// Constantes para configurações PWM e LED
const float DIVISOR_CLK_PWM = 16.0;      // Divisor de clock para o PWM
const uint16_t PERIOD = 4095;            // Período do PWM para o LED
const uint16_t LED_STEP = 200;           // Passo para controle de brilho do LED


#define GET_SLICE(x) (x >> 1) & 0x7


uint32_t bpm_us_counter, bpm_in_us = 500000u, note;

uint8_t end = 0;

uint8_t current_pattern = 0, pattern_index = 0, rhythm_index = 0;
bool vocalize = false;

#define UP   2
#define STAY 1
#define DOWN 0
uint8_t dir = UP;

bool display_set = false;









void on_pwm_wrap() {
  
  // Clear the interrupt flag that brought us here
  pwm_clear_irq(0);
  bpm_us_counter++;

  if(bpm_us_counter >= bpm_in_us){
    
      if(rhythm_index == 0){
        uint ch;
        uint slice_num = GET_SLICE(BUZZER_B);

        pwm_hw->slice[slice_num].csr |= 0x1;

        pwm_hw->slice[slice_num].top = note/six_notes_patterns[current_pattern][pattern_index];
        ch = BUZZER_B & 1u;
        pwm_hw->slice[slice_num].cc = 
                ((uint32_t)(pwm_hw->slice[slice_num].top >> 1) << (ch ? PWM_CH0_CC_B_LSB : PWM_CH0_CC_A_LSB)) 
                  |  (pwm_hw->slice[slice_num].cc & (ch ? PWM_CH0_CC_A_BITS : PWM_CH0_CC_B_BITS));//chan write
      
                
        slice_num = GET_SLICE(BUZZER_A);

        pwm_hw->slice[slice_num].csr |= 0x1;

        pwm_hw->slice[slice_num].top = note/six_notes_patterns[current_pattern][pattern_index];
        ch = BUZZER_A & 1u;
        pwm_hw->slice[slice_num].cc = 
                ((uint32_t)(pwm_hw->slice[slice_num].top >> 1) << (ch ? PWM_CH0_CC_B_LSB : PWM_CH0_CC_A_LSB)) 
                  |  (pwm_hw->slice[slice_num].cc & (ch ? PWM_CH0_CC_A_BITS : PWM_CH0_CC_B_BITS));//chan write
                          

      }else{

        uint slice_num = GET_SLICE(BUZZER_A);
        pwm_hw->slice[slice_num].csr &= ~(uint)0x1;
        slice_num = GET_SLICE(BUZZER_B);
        pwm_hw->slice[slice_num].csr &= ~(uint)0x1;


      }

      rhythm_index = (rhythm_index + 1) % pattern_rhythm[pattern_index];

      if(rhythm_index == 0){
        pattern_index = (pattern_index + 1) % pattern_size;

        if((pattern_index == 0) && (dir == UP)) note /= m2;
        else if((pattern_index == 0) && (dir == DOWN)) note *= m2;

        //printf("%d", dir);

      }
    
      bpm_us_counter = 0;
    
      
  }

 
  //  if(wave_index2 < 2048) sio_hw->gpio_set = 1 << 11;
   // else sio_hw->gpio_clr = 1 << 11;



}

// Função de configuração inicial
void setup_gpio()
{
  uint slice, ch;
  stdio_init_all(); // Inicializa a comunicação padrão (UART)

  
  iobank0_hw->io[BUTTON_A].ctrl = GPIO_FUNC_SIO << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
  iobank0_hw->io[BUTTON_B].ctrl = GPIO_FUNC_SIO << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
  iobank0_hw->io[BUTTON_C].ctrl = GPIO_FUNC_SIO << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
  iobank0_hw->io[11].ctrl = GPIO_FUNC_SIO << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
  iobank0_hw->io[12].ctrl = GPIO_FUNC_SIO << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
  iobank0_hw->io[13].ctrl = GPIO_FUNC_SIO << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
 
  

  //gpio_set_dir(BUTTON_A, GPIO_IN); // Define o botão A como entrada
  sio_hw->gpio_oe_clr = 1 << BUTTON_A;
  sio_hw->gpio_oe_clr = 1 << BUTTON_B;
  sio_hw->gpio_oe_clr = 1 << BUTTON_C;
  sio_hw->gpio_oe_set = 1 << 11;
  sio_hw->gpio_oe_set = 1 << 12;
  sio_hw->gpio_oe_set = 1 << 13;
 


  padsbank0_hw->io[BUTTON_A] &= ~(1 << PADS_BANK0_GPIO0_PDE_LSB);
  padsbank0_hw->io[BUTTON_A] |= 1 << PADS_BANK0_GPIO0_PUE_LSB; 

  padsbank0_hw->io[BUTTON_B] &= ~(1 << PADS_BANK0_GPIO0_PDE_LSB);
  padsbank0_hw->io[BUTTON_B] |= 1 << PADS_BANK0_GPIO0_PUE_LSB;

  padsbank0_hw->io[BUTTON_C] &= ~(1 << PADS_BANK0_GPIO0_PDE_LSB);
  padsbank0_hw->io[BUTTON_C] |= 1 << PADS_BANK0_GPIO0_PUE_LSB;

  padsbank0_hw->io[11] &= ~(1 << PADS_BANK0_GPIO0_IE_LSB);
  padsbank0_hw->io[12] &= ~(1 << PADS_BANK0_GPIO0_IE_LSB);
  padsbank0_hw->io[13] &= ~(1 << PADS_BANK0_GPIO0_IE_LSB);
 


}

void setup_audio(){

  uint slice;
  iobank0_hw->io[BUZZER_A].ctrl = GPIO_FUNC_PWM << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
  slice = GET_SLICE(BUZZER_A);    // Obtém o slice PWM para o buzzer A
  pwm_hw->slice[slice].div = 0x00000100;     // Define o divisor de clock para o PWM do buzzer A
  pwm_hw->slice[slice].csr |= 0x1;


  iobank0_hw->io[BUZZER_B].ctrl = GPIO_FUNC_PWM << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;
  slice = GET_SLICE(BUZZER_B);     // Obtém o slice PWM para o buzzer A
  pwm_hw->slice[slice].div = 0x00000100;     // Define o divisor de clock para o PWM do buzzer A
  pwm_hw->slice[slice].csr |= 0x1;


  slice = 0;  
  pwm_hw->slice[slice].div = 0x00000010; //DIV 1.0 
  pwm_hw->slice[slice].top = 124; //1MhZ
  pwm_hw->slice[slice].csr |= 0x1;//ENABLE

  pwm_clear_irq(slice);
  pwm_set_irq_enabled(slice, true);
  irq_set_exclusive_handler(PWM_IRQ_WRAP, on_pwm_wrap);






}


