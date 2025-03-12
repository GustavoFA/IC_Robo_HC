// Código Módulo VNH2SP30 para ESP32


// ------- Includes -----------
#include <driver/mcpwm.h> // PWM
#include <driver/timer.h> // Temporizador
#include <driver/gpio.h>  // GPIO
#include <driver/adc.h>   // ADC
// -----------------------------

// -------- Defines ---------------

// Controle do motor 1
#define EN_1 GPIO_NUM_5
#define pwm_1 GPIO_NUM_15
#define A_1 GPIO_NUM_16
#define B_1 GPIO_NUM_17
#define CS_1 GPIO_NUM_4 

// Controle do motor 2
#define EN_2 GPIO_NUM_14
#define pwm_2 GPIO_NUM_25
#define A_2 GPIO_NUM_26
#define B_2 GPIO_NUM_27
#define CS_2 GPIO_NUM_12

// ---------------------------------

// -------- Variáveis --------------
char *tag;            // Variável necessária para o temporizador 
char dir_t = 't';     // variável de sentido/direção do robô (p/f/t/h/a)
int vel_t = 1;        // variável de velocidade do robô (1/2/3/4/5)
bool chave_t = false; // variável de controle da trava do robô (false = desativada)
// --------------------------------


// -------- Movimentos ---------

  void mov_frente(){
  
  gpio_set_level(A_1, 0);
  gpio_set_level(B_1, 1);
  
  gpio_set_level(A_2, 1);
  gpio_set_level(B_2, 0);

  }
  
  void mov_tras(){
    
    gpio_set_level(A_1, 1);
    gpio_set_level(B_1, 0);
    
    gpio_set_level(A_2, 0);
    gpio_set_level(B_2, 1);
    
  }
  
  void mov_ant_hora(){
    
    gpio_set_level(A_1, 1);
    gpio_set_level(B_1, 0);
    
    gpio_set_level(A_2, 1);
    gpio_set_level(B_2, 0);
 
  }
  
  void mov_hora(){
    
    gpio_set_level(A_1, 0);
    gpio_set_level(B_1, 1);
    
    gpio_set_level(A_2, 0);
    gpio_set_level(B_2, 1);
  
  }
  
  void mov_parado(){
    
    gpio_set_level(A_1, 0);
    gpio_set_level(B_1, 0);
    
    gpio_set_level(A_2, 0);
    gpio_set_level(B_2, 0);
    
  }

// -----------------------------


void setup() {

  // --- Serial --- para debug e avaliações
  Serial.begin(115200);
  // ----------------

  // ------- Configuração do ADC -----------
  adc2_config_channel_atten(ADC2_CHANNEL_0, ADC_ATTEN_DB_11); // GPIO 4
  adc2_config_channel_atten(ADC2_CHANNEL_5, ADC_ATTEN_DB_11); // GPIO 12

  // ------- EN -------
  gpio_pad_select_gpio(EN_1);
  gpio_set_direction(EN_1, GPIO_MODE_OUTPUT);
  gpio_set_level(EN_1, 0);

  gpio_pad_select_gpio(EN_2);
  gpio_set_direction(EN_2, GPIO_MODE_OUTPUT);
  gpio_set_level(EN_2, 0);  
  // ------------------

  // ------- PWM -------
  // Motor 1
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, pwm_1);  // Seleciono a unidade de PWM, sua saída e a GPIO
  mcpwm_config_t pwm_config_1;                      // Struct para configuração do PWM

  pwm_config_1.frequency = 25000;                   // Frequência
  pwm_config_1.cmpr_a = 100;                        // Duty Cycle
  pwm_config_1.cmpr_b = 100;                        // Duty Cycle
  pwm_config_1.duty_mode = MCPWM_DUTY_MODE_0;       // Tempo do duty cycle é de nível alto
  pwm_config_1.counter_mode = MCPWM_UP_COUNTER;     // Contagem crescente
  mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config_1); // Aplico a configuração no módulo com o temporizador 0

  // Motor 2
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, pwm_2);  // Seleciono a unidade de PWM, sua saída e a GPIO
  mcpwm_config_t pwm_config_2;                      // Struct para configuração do PWM

  pwm_config_2.frequency = 25000;                   // Frequência
  pwm_config_2.cmpr_a = 100;                        // Duty Cycle
  pwm_config_2.cmpr_b = 100;                        // Duty Cycle
  pwm_config_2.duty_mode = MCPWM_DUTY_MODE_0;       // Tempo do duty cycle é de nível alto
  pwm_config_2.counter_mode = MCPWM_UP_COUNTER;     // Contagem crescente
  mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config_2); // Aplico a configuração no módulo com o temporizador 0
  
  // ---------------------

  // ----- Saídas de ativação do movimento ----

  // A1
  gpio_pad_select_gpio(A_1);
  gpio_set_direction(A_1, GPIO_MODE_OUTPUT);
  gpio_set_level(A_1, 0);
  // B1
  gpio_pad_select_gpio(B_1);
  gpio_set_direction(B_1, GPIO_MODE_OUTPUT);
  gpio_set_level(B_1, 0);

  // A2
  gpio_pad_select_gpio(A_2);
  gpio_set_direction(A_2, GPIO_MODE_OUTPUT);
  gpio_set_level(A_2, 0);
  // B2
  gpio_pad_select_gpio(B_2);
  gpio_set_direction(B_2, GPIO_MODE_OUTPUT);
  gpio_set_level(B_2, 0);
  
  // -------------------------------------------
  
  
}

void loop() {

  // ---- Para teste situacional do motor ------
  
  if(!chave_t){
    gpio_set_level(EN_1, 1);
    gpio_set_level(EN_2, 1);
  }else{
    gpio_set_level(EN_1, 0);
    gpio_set_level(EN_2, 0);
  }
  
  if(dir_t == 't'){
      mov_tras();
  }else if(dir_t == 'f'){
      mov_frente();
  }else if(dir_t == 'a'){
      mov_ant_hora();
  }else if(dir_t == 'h'){
      mov_hora();
  }else{
      mov_parado();
  }

  mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 20*vel_t);
  mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, 20*vel_t);
  // ------------------------------

  // -------- Sensor de corrente -----
  // Para ser bem feito precisa de um temporizador e verificar a cada 100 ms
  
  int leitor_1 = 0;
  int leitor_2 = 0;
  adc2_get_raw(ADC2_CHANNEL_0, ADC_WIDTH_BIT_12, &leitor_1);
  adc2_get_raw(ADC2_CHANNEL_5, ADC_WIDTH_BIT_12, &leitor_2);
  Serial.print("Leitor 1: ");
  Serial.println(leitor_1);
  Serial.print("Leitor 2: ");
  Serial.println(leitor_2);
  
}
