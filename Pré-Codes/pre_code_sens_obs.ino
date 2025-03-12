// Código para leitura dos sensores de obstáculo


// Leitura dos sensores de Obstáculo
/* Essa leitura será feita por interrupção externa, dependendo do nível lógico será
acionado uma interrupção. Sendo que os sensores KY-032 trabalham com o nível lógico 0 para
detecção de obstáculo. Utilizo geração de interrupção em borda de subida e descida, assim,
podendo travar e liberar a movimentação do robô quando as situações forem favoráveis.
*/

/* Quando tem um obstávulo o sensor gera uma interrupção (mudança de nível de 1 -> 0, borda 
de descida) e na RSI verificamos que o nível é 0, logo, há um obstáculo e a variável de trava
é trocada para 1 impedindo que o robô se movimente para aquele sentido. Quando não houver mais
obstáculo ocorre outra interrupção (mudança de nível de 0 -> 1, borda de subida) e na RSI
verificamos que o nível é 1, logo, não há mais obstáculo e a variável de trava é trocada para
0 possibilitando que o robô se movimente naquele sentido.
*/

#include <driver/gpio.h>

#define sens_obs_1 GPIO_NUM_32  // sensor da frente
#define sens_obs_2 GPIO_NUM_33  // sensor de tras

bool nao_mov_frente = 0;
bool nao_mov_tras = 0;

char *tag = "teste";

// Rotina de serviço de interrupção do sensor de obstáculo 1

void sens_isr_1(void *arg){

  /* Aqui aplicamos a lógica que a interrupção ocorre tanto para borda de subida
  quanto para descida, então, na RSI devemos apenas verificar o nível de tensão do
  sensor.
  */

  // Lembrando: quando há obstáculo a saída do sensor vai para nível 0
  
  if(gpio_get_level(sens_obs_1)){
    nao_mov_frente = 0;
  }else{
    nao_mov_frente = 1;
  }
  
  uint32_t gpio_intr_status = READ_PERI_REG(GPIO_STATUS_REG); 
  SET_PERI_REG_MASK(GPIO_STATUS_W1TC_REG, gpio_intr_status); 
}

// Rotina de serviço de interrupção do sensor de obstáculo 2

void sens_isr_2(void *arg){

  // Lógica semelhante aplicada no sensor 1
  if(gpio_get_level(sens_obs_2)){
    nao_mov_tras = 0;
  }else{
    nao_mov_tras = 1;
  }
  
  uint32_t gpio_intr_status = READ_PERI_REG(GPIO_STATUS_REG); 
  SET_PERI_REG_MASK(GPIO_STATUS_W1TC_REG, gpio_intr_status); 
}

void setup() {


  // --------- INT externa do sensor 1 -----

  gpio_pad_select_gpio(sens_obs_1);
  // Configuro como entrada a GPIO
  gpio_set_direction(sens_obs_1, GPIO_MODE_INPUT);
  // Configuro o resistor da GPIO como PULL UP 
  gpio_set_pull_mode(sens_obs_1, GPIO_PULLUP_ONLY);
  // Configuro o disparador de interrupção tanto borda de subida quanto borda de descida
  // Faço isso, pois assim, podemos controlar melhor a "trava" de movimentação do robô
  gpio_set_intr_type(sens_obs_1, GPIO_INTR_ANYEDGE);
  // Habilito a interrupção na GPIO
  gpio_intr_enable(sens_obs_1);
  // Associo uma Rotina de serviço à interrupção
  gpio_isr_register(sens_isr_1, tag, ESP_INTR_FLAG_LEVEL3, NULL);

  // -----------------------------------------

  // ---------- INT externa do sensor 2 ---------

  // As funções abaixo possuem a mesma lógica para as do sensor 1
  gpio_pad_select_gpio(sens_obs_2);
  gpio_set_direction(sens_obs_2, GPIO_MODE_INPUT);
  gpio_set_pull_mode(sens_obs_2, GPIO_PULLUP_ONLY);
  gpio_set_intr_type(sens_obs_2, GPIO_INTR_ANYEDGE);
  gpio_intr_enable(sens_obs_2);
  gpio_isr_register(sens_isr_2, tag, ESP_INTR_FLAG_LEVEL3, NULL);
  // Utilizo a mesma prioridade para ambas as interrupções (imagino que seja difícil ocorrer
  // as duas simultaneamente)
  // -----------------------------------------------

}

void loop() {

}
