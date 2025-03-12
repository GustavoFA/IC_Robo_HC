// Código prévio para leitura do CS 

// Leitura do saída do sensor de corrente (CS - current sense) da ponte H VNH2SP30
/* Para isso estaremos utilizando um temporizador e uma interrupção que ocorrerá a cada 100 ms
para verificar a tensão gerada no CS. Como o sinal é analógico, logo, utilizaremos o ADC 
para converter em um sinal digital e podermos trabalhar com ele. 
*/

#include <driver/timer.h> // Para Temporizadores
#include <driver/gpio.h> // controle das GPIO
#include <driver/adc.h> // para ADC

#define sens_obs_1 GPIO_NUM_34
#define sens_obs_2 GPIO_NUM_35

// GPIOs utilizadas como entrada do sinal do CS
 // ADC2 CH5 - GPIO12
 // ADC2 CH6 - GPIO14

// Variável usada apenas por uma função da API do esp pedir
char *tag;


// Rotina de serviço de interrupção para checagem do CS
void IRAM_ATTR ISR_leitura_CS(void *args) {

  // Verifica o valor do CS de cada ponto H
  int leitor_1 = 0; 
  int leitor_2 = 0;
  adc2_get_raw(ADC2_CHANNEL_5, ADC_WIDTH_BIT_12, &leitor_1);
  adc2_get_raw(ADC2_CHANNEL_6, ADC_WIDTH_BIT_12, &leitor_2);

  if( leitor_1 >= 1000 || leitor_2 >= 1000){
    // Força todos os motores a ficarem parados
  }

  // Finaliza a interrupção para o Temporizador 0
  TIMERG0.int_clr_timers.t0 = 1;  
  TIMERG0.hw_timer[0].update=1;
  TIMERG0.hw_timer[0].config.alarm_en = 1;

}


void setup() {

  // -------------- Configução do ADC ---------------------
  
  adc2_config_channel_atten(ADC2_CHANNEL_5,ADC_ATTEN_DB_11);
  adc2_config_channel_atten(ADC2_CHANNEL_6,ADC_ATTEN_DB_11);

  // -------------- Temporizador ----------------------------

  // Configuração do temporizador
  
  timer_config_t timer_config = {}; // Criando struct para configuração
  // Preparando configuração do temporizador
  timer_config.divider = 80;  // Prescaler de 80, com clock de 80 MHz, logo, teremos um temporizador de 1MHz
  timer_config.counter_dir = TIMER_COUNT_UP;  // contagem ascendente no contador
  timer_config.counter_en = TIMER_PAUSE; // o temporizador começará pausado até ser chamado para iniciar
  timer_config.alarm_en = TIMER_ALARM_EN; // temporizador com alarme
  timer_config.auto_reload = TIMER_AUTORELOAD_EN; // o temporizador se auto reiniciará

  // Inicializo o temporizador e passo sua configuração
  timer_init(TIMER_GROUP_0, TIMER_0, &timer_config);
  // Configura o valor que o temporizador irá inicializar
  timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0); 
  // Configura o valor para o alarme
  timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, TIMER_BASE_CLK/800); // Gerar um alarme a cada 100ms 
  // Ativar uma interrupção 
  timer_enable_intr(TIMER_GROUP_0, TIMER_0);
  // Configura uma ISR
  timer_isr_register(TIMER_GROUP_0, TIMER_0, &ISR_leitura_CS, &tag, 2, NULL); // 4° parâmetro se trata da prioridade (level) da nossa interrupção
  
  // --------------------------------------------------------

  // 

  
  
  // -----------------------------------------

  // Iniciar a contagem do temporizador
  timer_start(TIMER_GROUP_0, TIMER_0);

}

void loop() {

}
