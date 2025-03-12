// Código Robô


/* Anotações
 *  Ajustar os valores do sensor de corrente
 */

// ----- Includes ----

#include <WiFi.h>         // WiFi
#include <esp_now.h>      // esp now
#include <driver/timer.h> // Temporizador
#include <driver/gpio.h>  // GPIO
#include <driver/adc.h>   // ADC
#include <driver/mcpwm.h> // PWM
// -------------------

// ---- Defines -------

// Led indicativo
#define LED GPIO_NUM_2

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

#define SENS_obs_d GPIO_NUM_32 // sensor dianteiro
#define SENS_obs_t GPIO_NUM_33 // sensor traseiro

// -------------------


// ----- Variáveis -----

// Estrutura exemplo para receber os dados (deve ser semelhante à struct do sender)
typedef struct mensagem_recebida {
    char direcao;
    int velocidade;
    bool chave;
} mensagem_recebida;

// Criação da struct
mensagem_recebida msg_recebida;

// Estrutura para enviar um pacote de dados
typedef struct mensagem_enviada {
    char direcao;
    int velocidade;
    bool chave;
    bool sensor_frente;
    bool sensor_tras;
    bool sensor_corrente;
} mensagem_enviada;

// Criação da struct
mensagem_enviada msg_enviada;

// Mac Address do ESP32 que receberá os dados
uint8_t broadcastAddress[] = {0x40, 0xf5, 0x20, 0x86, 0x5c, 0xfc};

char *tag;            // Variável necessária para o temporizador 
char dir_t = 'p';     // variável de sentido/direção do robô (p/f/t/h/a)
int vel_t;            // variável inicial para obter o valor registrado pelo potênciometro do controle
bool chave_t = false; // variável de controle da trava do robô (false = desativada)

// variável auxiliar para troca de status do robô
char status_troca = 0;

// Variável obrigatória para função da interrupção do temporizador
char *tag_temp = "temp";

// Variáveis para os sensores de obstáculo - Valor lógico 1 simboliza que o sensor foi ativado (tem obstáculo)
bool sens_obs_d = 0;
bool sens_obs_t = 0;

// Variável obrigatória para função da interrupção externa
char *tag_ext = "ext";

// Variável para o sensoriamento das correntes --> Utilizo apenas uma variável, pois, qualquer um dos CS que passar do valor limite travará o robô
bool sens_cor = 0;  // valor lógico 1 simboliza que a corrente ultrapassou o valor limite

// Variável auxiliar para utilizar o temporizador para o sensor de corrente
char cont_cor = 0;

// ----------------------


// ------ Funções -------

// Função de envio
void verifica_envio(const uint8_t *mac_addr, esp_now_send_status_t status){

  if(status == ESP_NOW_SEND_SUCCESS){
    // Não vejo algo útil a ser feito nessa função  
   }
}

// Função de recebimento dos dados
void func_recebimento(const uint8_t * mac, const uint8_t *incomingData, int len){

  // Indico pelo led o recebimento
  gpio_set_level(LED, 1);

  memcpy(&msg_recebida, incomingData, sizeof(msg_recebida));
  
  // Passo os dados recebidos para variáveis 
  dir_t = msg_recebida.direcao;
  vel_t = msg_recebida.velocidade;
  chave_t = msg_recebida.chave;   

  // Variável auxiliar para permitir trocar a configuração do robô
  status_troca = 1;

  // Armazeno os dados recebidos com os dados analisados no robô para serem enviados de retorno ao controle
  msg_enviada.velocidade = msg_recebida.velocidade;
  msg_enviada.direcao = msg_recebida.direcao;
  msg_enviada.chave = msg_recebida.chave;
  msg_enviada.sensor_frente = sens_obs_d;
  msg_enviada.sensor_tras = sens_obs_t;
  msg_enviada.sensor_corrente = sens_cor;
  
  // Envio dos dados
  esp_now_send(broadcastAddress, (uint8_t *) &msg_enviada, sizeof(msg_enviada));
  
}


// Rotina de serviço de interrupção do temporizador
void IRAM_ATTR isr_temp_callback(void *args) {
  // Verifico se está recebendo algum comando
  if(!status_troca){
    // Coloco o movimento do robô como parado
    dir_t = 'p';
  }

 
  // Condição para verificação do Sensor de corrente (a cada 120 ms)
  if(cont_cor >= 1){
    cont_cor = 0;
    // variáveis auxiliares para armazenar o valor digital do sensor de corrente
    int leitor_1 = 0;
    int leitor_2 = 0;
    // Armazenamento
    adc2_get_raw(ADC2_CHANNEL_0, ADC_WIDTH_BIT_12, &leitor_1);
    adc2_get_raw(ADC2_CHANNEL_5, ADC_WIDTH_BIT_12, &leitor_2);
    // Verificando se ultrapassa o valor limite 
    if(leitor_1 >= 5000 || leitor_2 >= 5000){ // AJUSTAR AQUI
      sens_cor = 1;
    }else{
      sens_cor = 0;
    }
  }else{
    cont_cor++;
  }

  // Desativo o led depois de um tempo(60ms) de recebido os dados 
  gpio_set_level(LED, 0);
  
  // Finaliza a interrupção
  TIMERG0.int_clr_timers.t0 = 1;  
  TIMERG0.hw_timer[0].update=1;
  TIMERG0.hw_timer[0].config.alarm_en = 1;

}


// Funções de movimentação

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

// -------------------------------

void setup() {

  Serial.begin(115200); // Para verificações e ajustes

  // ---- LED indicativo ----
  gpio_pad_select_gpio(LED);
  gpio_set_direction(LED, GPIO_MODE_OUTPUT);
  gpio_set_level(LED, 0);
  
  // ---- Temporizador ------

  timer_config_t timer_config = {}; // crio a variável de configuração

  timer_config.divider = 80; // Como o clock está em 80MHz aqui teremos um temporizador de 1MHz
  timer_config.counter_dir = TIMER_COUNT_UP; // Contagem ascendente
  timer_config.counter_en = TIMER_PAUSE; // timer começa pausado
  timer_config.alarm_en = TIMER_ALARM_EN; // Ativa o alarme, que será necessário para a interrupção
  timer_config.auto_reload = TIMER_AUTORELOAD_EN; // O temporizador se auto reiniciará ao atingir o valor máximo de contagem
  timer_init(TIMER_GROUP_0, TIMER_0, &timer_config);  // Configuro qual grupo e temporizador irei utilizar
  timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0); // Aqui certifico que o contador sempre começará em 0
  timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 60000); // Aqui adiciono o valor máximo do contador para um tempo final de 60 ms
  timer_enable_intr(TIMER_GROUP_0, TIMER_0);  // Ativo uma RSI
  timer_isr_register(TIMER_GROUP_0, TIMER_0, &isr_temp_callback, &tag_temp, 2, NULL); // Relaciono com a respectiva RSI e coloco Prioridade 2
  
  // ------------------------

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


    // --------- Configuração do sensor de Obstáculo dianteiro -----

  gpio_pad_select_gpio(SENS_obs_d);
  // Configuro como entrada a GPIO
  gpio_set_direction(SENS_obs_d, GPIO_MODE_INPUT);
  // Configuro o resistor da GPIO como PULL UP 
  gpio_set_pull_mode(SENS_obs_d, GPIO_PULLUP_ONLY);

  // -----------------------------------------

  // ---------- Configuração do sensor de Obstáculo dianteiro ---------

  // As funções abaixo possuem a mesma lógica para as do sensor dianteiro
  gpio_pad_select_gpio(SENS_obs_t);
  gpio_set_direction(SENS_obs_t, GPIO_MODE_INPUT);
  gpio_set_pull_mode(SENS_obs_t, GPIO_PULLUP_ONLY);
  // -----------------------------------------------
  

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

  // --- WiFi --------------

   WiFi.mode(WIFI_STA);

  // ======= Configuração do ESP NOW =======

  // Inicialização do ESP NOW
  if (esp_now_init() != 0) {
    ESP.restart();
  }

  // Informações do par a ser enviado os dados
  
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  // ---
  
  // Adiciono o par especificado e valido tal conexão
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    return;
  }

  // Função para recebimento dos dados
  esp_now_register_recv_cb(func_recebimento);

  // Função para verificar o envio dos dados
  esp_now_register_send_cb(verifica_envio);

  // =================================

  // Inicio o temporizador
  timer_start(TIMER_GROUP_0, TIMER_0);

}



void loop() {

  // ---- Varredura dos sensores de obstáculo ---
  // dianteiro
  if(!gpio_get_level(SENS_obs_d)){
    sens_obs_d = 1;
  }else{
    sens_obs_d = 0;
  }
  // traseiro
  if(!gpio_get_level(SENS_obs_t)){
    sens_obs_t = 1;
  }else{
    sens_obs_t = 0;
  }
  // -------------------------------------------

  // ----- Resposta aos comando ------
  if(status_troca){               // Verifica se pode trocar (essa variável é atualizada sempre que receber um pacote)

    // Verifica se a chave de ponto morto está acionada e se os sensores estão em alto
    if(!chave_t && !sens_cor){                 

      gpio_set_level(EN_1, 1);    // Caso não estejam permite que os motores sejam acionados
      gpio_set_level(EN_2, 1);

      // Verifica qual direção/sentido foi enviado como comando
      if(dir_t == 'f' && !sens_obs_d){           
        mov_frente();
      }else if(dir_t == 't' && !sens_obs_t){
        mov_tras();
      }else if(dir_t == 'a'){
        mov_ant_hora();
      }else if(dir_t == 'h'){
        mov_hora();
      }else{
        mov_parado();
      }
      
      // Aplica a velocidade especificada
      int vel_atual;    // variável auxiliar para aplicarmos a velocidade adequada
      // Conversão do valor registrado pelo potênciometro no controle para o valor adequado utilizado no robô
      if(3276 <= vel_t && vel_t <= 4095){
        vel_atual = 5;
      }else if(2457<= vel_t && vel_t < 3276){
        vel_atual = 4;
      }else if(1638 <= vel_t && vel_t < 2457){
        vel_atual = 3;
      }else if(819 <= vel_t && vel_t < 1638){
        vel_atual = 2;
      }else if(0 <= vel_t && vel_t < 819){
        vel_atual = 1;
      }else{
        vel_atual = 0;
      }
      // Aplicação da velocidade especificada
      mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 20*vel_atual);
      mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, 20*vel_atual);

    }else{
      // Caso a chave de ponto morto esteja acionada deixamos as portas de ENABLE do shield em nível baixo (não permitindo movimento)
      gpio_set_level(EN_1, 0);
      gpio_set_level(EN_2, 0);
      
    }
    
    status_troca = 0;     // Zeramos a variável para identificar que o comando recebido já foi executado pelo robô
  }
    

}
