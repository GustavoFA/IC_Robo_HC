// Estudo de controle de PWM ESP32

#include "driver/mcpwm.h"

// Configuração : Início

/*  Selecionar 2 GPIOs para saída --> mcpwm_gpio_init() 
 * mcpwm_gpio_init(mcpwm_unit_t mcpwm_num, mcpwm_io_signals_t io_signal, int gpio_num)
 * mcpwm_num = MCPWM_UNIT_x --> x = 0 | x = 1            / Selecionar uma das duas unidades de MCPWM
 * io_signal = MCPWMxy --> x = 0 | x = 1 , y = A | y = B / Como é um par de saídas
 * gpio_num = Pin da GPIO que será usada para saída
 */

/*  Configurar a struct MCPWM
 * mcpwm_config_t <nome>
 * uint32_tfrequency = Frequência em Hz
 * float cmpr_x = duty cycle de operação em x = A | x = B
 * mcpwm_duty_type_t duty_mode -> Seleciona o modo de operação do duty cycle (MCPWM_DUTY_MODE_0, MCPWM_DUTY_MODE_1, ...)
 * mcpwm_counter_type_t counter_mode -> Seleciona o modo de contador, 'counter' (MCPWM_FREEZE_COUNTER, MCPWM_UP_COUNTER, MCPWM_DOWN_COUNTER, ...)
 * (opicional) mcpwm_group_set_resolution(mcpwm_unit_t mcpwm_num, unsigned long int resolution) --> Registra a resolução da frequência do grupo -> mcpwm_num = MCPWM_UNIT_x, x = 0 | 1    -> resolution = resolução esperada da frequência
 * (opicional) mcpwm_timer_set_resolution(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num, unsigned long int resolution) --> Registra a resolução de cada timer
 */

// Inicializar o MCPWM com os parâmetros configurados anteriormente: mcpwm_init(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num, const mcpwm_config_t *mcpwm_conf)

// Configuração : Fim


#define gpio_pwm0a 12
#define gpio_pwm0b 13

//int *arg_ = 1;

void setup() {

  Serial.begin(115200);

  // INICIO::PWM

  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, gpio_pwm0a);  // Identifica a GPIO x para o MCPWM0A
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, gpio_pwm0b);  // Identifica a GPIO y para o MCPWM0B
  
  // Estrutura para configuração do PWM
  
  mcpwm_config_t pwm_config;

  pwm_config.frequency = 10;
  pwm_config.cmpr_a = 30;
  pwm_config.cmpr_b = 30;
  pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
  pwm_config.counter_mode = MCPWM_UP_COUNTER;

  mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);

  // FIM::PWM


void loop() {}
