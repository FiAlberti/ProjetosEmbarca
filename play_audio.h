//extern int main_audio();
extern void setup_audio();
extern void setup_gpio();
//extern void read_buttons();

extern const uint BUZZER_A; // Pino para o buzzer A
extern const uint BUZZER_B ; // Pino para o buzzer B
extern const uint BUTTON_A ;  // Pino para o botão A
extern const uint BUTTON_B ;  // Pino para o botão B
extern const uint BUTTON_C;  // Pino para o botão B
extern bool vocalize, display_set;
extern uint8_t dir;

extern uint base_notes[12];
extern char * base_symbols[36];

extern uint32_t bpm_us_counter, bpm_in_us, note;

extern uint8_t current_pattern, pattern_index, rhythm_index;

#define UP   2
#define STAY 1
#define DOWN 0