#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"
#include "notas.h"
#include "musica_struct.h"
#include "asa_branca.h"
#include "got.h"
#include "lullaby.h"

// BUZZER
#define buzzer_1 PIOC
#define buzzer_ID_1	ID_PIOC
#define buzzer_IDX_1	13
#define buzzer_IDX_MASK_1	(1u<<buzzer_IDX_1)

// BUTTONS
#define BUT1_PIO PIOD
#define BUT1_PIO_ID ID_PIOD
#define BUT1_PIO_IDX 28
#define BUT1_PIO_IDX_MASK (1u << BUT1_PIO_IDX)

#define BUT2_PIO PIOC
#define BUT2_PIO_ID ID_PIOC
#define BUT2_PIO_IDX 31
#define BUT2_PIO_IDX_MASK (1u << BUT2_PIO_IDX)

// LED
#define LED_PIO      PIOC
#define LED_PIO_ID   ID_PIOC
#define LED_IDX      8
#define LED_IDX_MASK (1u << LED_IDX)


volatile int but1_flag = 0;
volatile int but2_flag = 0;


void but1_callback(void) {
	but1_flag = !but1_flag;
}


void but2_callback(void) {
	but2_flag = 1;
}


void init_but() {
	// BUT1
	pmc_enable_periph_clk(BUT1_PIO_ID);
	pio_configure(BUT1_PIO, PIO_INPUT, BUT1_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_handler_set(
		BUT1_PIO,
		BUT1_PIO_ID,
		BUT1_PIO_IDX_MASK,
		PIO_IT_RISE_EDGE,
		but1_callback
	);
	pio_enable_interrupt(BUT1_PIO, BUT1_PIO_IDX_MASK);
	pio_get_interrupt_status(BUT1_PIO);
	NVIC_EnableIRQ(BUT1_PIO_ID);
	NVIC_SetPriority(BUT1_PIO_ID, 4); // Prioridade 4
	pio_set_debounce_filter(BUT1_PIO, BUT1_PIO_IDX_MASK, 80);
	// BUT2
	pmc_enable_periph_clk(BUT2_PIO_ID);
	pio_configure(BUT2_PIO, PIO_INPUT, BUT2_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_handler_set(
		BUT1_PIO,
		BUT2_PIO_ID,
		BUT2_PIO_IDX_MASK,
		PIO_IT_RISE_EDGE,
		but2_callback
	);
	pio_enable_interrupt(BUT2_PIO, BUT2_PIO_IDX_MASK);
	pio_get_interrupt_status(BUT2_PIO);
	NVIC_EnableIRQ(BUT2_PIO_ID);
	NVIC_SetPriority(BUT2_PIO_ID, 4); // Prioridade 4
	pio_set_debounce_filter(BUT2_PIO, BUT2_PIO_IDX_MASK, 80);
}


void init(void){
	sysclk_init();
	board_init();
	delay_init();
	WDT ->WDT_MR = WDT_MR_WDDIS;
	// Init BUTTONS
	init_but();
	// Init OLED
	gfx_mono_ssd1306_init();
	// Ativa o BUZZER
	pmc_enable_periph_clk(buzzer_1);
	pio_set_output(buzzer_1,buzzer_IDX_MASK_1,0,0,0);
	// Configura led
	pmc_enable_periph_clk(LED_PIO_ID);
	pio_configure(LED_PIO, PIO_OUTPUT_0, LED_IDX_MASK, PIO_DEFAULT);
}


void tone(int freq, int time){
	double t = 1000000/freq;
	if(freq == 0){
		pio_clear(buzzer_1, buzzer_IDX_MASK_1);
		delay_ms(100);
	}
	else{
		for(int i = 0; i<= time*1000/t; i++){
			pio_set(buzzer_1, buzzer_IDX_MASK_1);
			delay_us(t/2);
			pio_clear(buzzer_1, buzzer_IDX_MASK_1);
			delay_us(t/2);
			if (but1_flag) {
				gfx_mono_draw_string("<>", 0, 0, &sysfont);
				while (but1_flag) {
					delay_ms(10);
				}
				gfx_mono_draw_string("||", 0, 0, &sysfont);
			}
		}
	}
}


void play_music(musica m) {
	int tempo = m.tempo;
	int wholenote = (60000*4)/tempo;
	int divider = 0;
	int noteDuration = 0;
	int size = m.size * 2;
	float progresso;
	int buzzer ={buzzer_1};
	int mask = {buzzer_IDX_MASK_1};
		
	gfx_mono_draw_string("||", 0, 0, &sysfont);
	gfx_mono_draw_string(m.nome, 25, 0, &sysfont);
	
	for (int thisNote = 0; thisNote <= size; thisNote+=2) {
		divider = m.notas[thisNote + 1];
		if (divider > 0) {
			noteDuration = (wholenote) / divider;
			} 
		else if (divider < 0) {
			
			noteDuration = (wholenote) / abs(divider);
			noteDuration *= 1.5; 
		}
	//	m.notas[thisNote]
		tone(m.notas[thisNote], noteDuration);
		//controller(buzzer,440, mask);
		pio_set(LED_PIO, LED_IDX_MASK);
		delay_ms(noteDuration);
		pio_clear(LED_PIO, LED_IDX_MASK);
	
	
		// pular musica
		if (but2_flag) {
			but2_flag = 0;
			break;
		}
		// atuliza barra de progresso
		progresso = (float)thisNote/(float)size;
		for (int i = 0;i < 30 * progresso;i++){
			gfx_mono_draw_rect(i*4 + 3, 20, 2, 10, GFX_PIXEL_SET);
		}
	}
	// limpa barra de progresso
	for (int i = 0;i < 30;i++){
		gfx_mono_draw_rect(i*4 + 3, 20, 2, 10, GFX_PIXEL_CLR);
	}
}


int main (void)
{
	init();
	musica playlist[3] = {asa_branca, got, lullaby};
	int t = 0;
	while(1){
		// for musica in playlist
		play_music(playlist[t % 3]);
		t++;
	}
}
