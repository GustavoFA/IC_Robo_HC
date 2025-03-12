// Código para controle dos motores

// Configurando as pontes H para movimentação dos motores
/* Estaremos configurando nossas saídas PWM e saídas digitais para controle da movimentação
do robô*/

// Temporizador
/* Utilizarei um temporizador para verificar a cada 50ms o comando enviado pelo controle, sendo
que este estará armazenado em uma variável, e quando ocorrer a interrupção do temporizador
verificarei tal variável e por meio dela será definido o movimento a ser executado.
*/

// Variável para ponto morto
/* Deve incluir uma variável de parada obrigatória (ponto morto), que fará com que o robô
fique parado (não muda as variáveis de movimento e velocidade, apenas criamos uma condição
para que essas variáveis não sejam lidas e utilizadas para movimentar o robô)

  Caso a comunicação falhe as variáveis poderão assumir o caracter "_", que deve ser interpretado
na movimentação como "parado", pois, o envio da informação falhou.
*/



#include <driver/mcpwm.h>
#include <driver/timer.h>
#include <driver/gpio.h>


#define mot_pwm_1 GPIO_NUM_13
#define mot_pwm_2 GPIO_NUM_27

// Motores 1 e 2, sendo o 1 da direita e 2 da esquerda

#define mot_A1 GPIO_NUM_16
#define mot_B1 GPIO_NUM_17
#define mot_A2 GPIO_NUM_18
#define mot_B2 GPIO_NUM_19

// Variável que armazena o comando de movimento (vinda da comunicação Server)
// Os comandos são: p (parado), f (frente), t (tras), a (anti-horário), h (horário)
String dir_mov = "p";
// Variável que armazena o comando de velocidade do movimento (vinda da comunicação Server)
// Os valores possíveis são: 1,2,3,4 e 5; sendo 5 a maior velocidade
String vel_mov = "1";

// Variável de ponto morto
String ponto_morto = "N";

char *tag;

// Funções de configuração do movimento
// Inserir como parâmetro a velocidade enviada pelo controle, assim, cada
// função de movimento já configura a velocidade desejada

void mov_frente(){
  
  gpio_set_level(mot_A1, 1);
  gpio_set_level(mot_B1, 0);
  
  gpio_set_level(mot_A2, 0);
  gpio_set_level(mot_B2, 1);

}

void mov_tras(){
  
  gpio_set_level(mot_A1, 0);
  gpio_set_level(mot_B1, 1);
  
  gpio_set_level(mot_A2, 1);
  gpio_set_level(mot_B2, 0);
  
}

void mov_ant_hora(){
  
  gpio_set_level(mot_A1, 1);
  gpio_set_level(mot_B1, 0);
  
  gpio_set_level(mot_A2, 1);
  gpio_set_level(mot_B2, 0);
  
}

void mov_hora(){
  
  gpio_set_level(mot_A1, 0);
  gpio_set_level(mot_B1, 1);
  
  gpio_set_level(mot_A2, 0);
  gpio_set_level(mot_B2, 1);

}

void mov_parado(){
  
  gpio_set_level(mot_A1, 0);
  gpio_set_level(mot_B1, 0);
  
  gpio_set_level(mot_A2, 0);
  gpio_set_level(mot_B2, 0);
  
}


// ----------------------------------------------------


// --------- RSI do temporizador dos comandos ----------

void IRAM_ATTR ISR_execucao_comandos(void *args) {

  uint8_t vel_atual = 1;

  if(dir_mov == "t"){
      mov_tras();
  }else if(dir_mov == "f"){
      mov_frente();
  }else if(dir_mov == "a"){
      mov_ant_hora();
  }else if(dir_mov == "h"){
      mov_hora();
  }else{
      mov_parado();
  }

  if(vel_mov == "1"){
    vel_atual = 1;
  }else if(vel_mov == "2"){
    vel_atual = 2;
  }else if(vel_mov == "3"){
    vel_atual = 3;
  }else if(vel_mov == "4"){
    vel_atual = 4;
  }else{
    vel_atual = 5;
  }

  mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 20*vel_atual);
  mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 20*vel_atual);

  // Finaliza a interrupção para o Temporizador do grupo 0 e timer 1
  TIMERG0.int_clr_timers.t0 = 1;  
  TIMERG0.hw_timer[1].update=1;
  TIMERG0.hw_timer[1].config.alarm_en = 1;

}

// ------------------------------------------------------


void setup() {


  // ------------------- PWM -----------------------
  
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, mot_pwm_1);
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, mot_pwm_2);

  // Struct para configuração do PWM
  mcpwm_config_t pwm_config;

  // Frequência
  pwm_config.frequency = 25000;
  // Duty Cycle do A e B
  pwm_config.cmpr_a = 100;
  pwm_config.cmpr_b = 100;
  // Duty Cycle proporcional ao modo alto (tempo do duty é de nível alto)
  pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
  // Contar de forma crescente
  pwm_config.counter_mode = MCPWM_UP_COUNTER;

  mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);

  // ------------------------------------------------

  // ------------------ A / B ------------------------

  // A1
  gpio_pad_select_gpio(mot_A1);
  gpio_set_direction(mot_A1, GPIO_MODE_OUTPUT);
  gpio_set_level(mot_A1, 0);
  // B1
  gpio_pad_select_gpio(mot_B1);
  gpio_set_direction(mot_B1, GPIO_MODE_OUTPUT);
  gpio_set_level(mot_B1, 0);
  // A2
  gpio_pad_select_gpio(mot_A2);
  gpio_set_direction(mot_A2, GPIO_MODE_OUTPUT);
  gpio_set_level(mot_A2, 0);
  // B2
  gpio_pad_select_gpio(mot_B2);
  gpio_set_direction(mot_B2, GPIO_MODE_OUTPUT);
  gpio_set_level(mot_B2, 0);

  // --------------------------------------------------

  // ----------------- Temporizador -----------------
  
  // Configuração semelhante a usada na entrada do CS. Pode ser reutilizada dela
  // usando apenas uma configuração de temporizador
  timer_config_t timer_comand = {};
  timer_comand.divider = 80;
  timer_comand.counter_dir = TIMER_COUNT_UP;
  timer_comand.counter_en = TIMER_PAUSE;
  timer_comand.auto_reload = TIMER_AUTORELOAD_EN;

  // Configuro o timer 1 do grupo 0 com as configurações feitas anteriormente  
  timer_init(TIMER_GROUP_0, TIMER_1, &timer_comand); 
  // Configuro o contador do meu temporizador para iniciar com o valor 0
  timer_set_counter_value(TIMER_GROUP_0, TIMER_1, 0);
  // Configuro o meu temporizador para gerar um alarme quando o contador bater 50000
  // Para gerar um tempo de 50 ms
  timer_set_alarm_value(TIMER_GROUP_0, TIMER_1, 50000); 
  // Habilito uma interrupção para esse alarme
  timer_enable_intr(TIMER_GROUP_0, TIMER_1);
  // Registro uma RSI para essa interrupção
  timer_isr_register(TIMER_GROUP_0, TIMER_1, &ISR_execucao_comandos, &tag, 2, NULL);

  // --------------------------------------------------

  // Inicio meu temporizador
  timer_start(TIMER_GROUP_0, TIMER_1); 
  
}

void loop() {
}
