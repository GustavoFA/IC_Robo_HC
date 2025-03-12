// Código Robô - versão 04.22


/* Módulos: 
 * Comunicação ESPNOW    
 * Sensores de Obstáculo
 * Sensor de Corrente
 * Comandos para movimentação base do motor
 * Controle de desaceleração (faltando)
*/


// ===== Includes =======

#include <WiFi.h>         // WiFi
#include <esp_now.h>      // esp now
#include <driver/timer.h> // Temporizador
#include <driver/gpio.h>  // GPIO
#include <driver/adc.h>   // ADC
#include <driver/mcpwm.h> // PWM
// ====================


// ===== Defines =========

// Led indicativo
#define LED GPIO_NUM_2

// Controle do motor 1
#define EN_1 GPIO_NUM_5   
#define pwm_1 GPIO_NUM_15
#define A_1 GPIO_NUM_17
#define B_1 GPIO_NUM_16
#define CS_1 GPIO_NUM_4 

// Controle do motor 2
#define EN_2 GPIO_NUM_14
#define pwm_2 GPIO_NUM_25
#define A_2 GPIO_NUM_26
#define B_2 GPIO_NUM_27
#define CS_2 GPIO_NUM_12

#define SENS_obs_d GPIO_NUM_32 // sensor dianteiro
#define SENS_obs_t GPIO_NUM_33 // sensor traseiro

// ========================



// ====== Variáveis =======

// ------ ESPNOW -----
// Endereço MAC
uint8_t broadcastAddress[] = {0x40, 0xf5, 0x20, 0x86, 0x5c, 0xfc};

// Estrutura para enviar um pacote de dados
typedef struct mensagem_enviada {
    char direcao;
    int velocidade;
    bool chave;
    bool sensor_frente;
    bool sensor_tras;
    bool sensor_corrente;
} mensagem_enviada;
mensagem_enviada msg_enviada;

// Estrutura exemplo para receber os dados (deve ser semelhante à struct do sender)
typedef struct mensagem_recebida {
    char direcao;
    int velocidade;
    bool chave;
} mensagem_recebida;
mensagem_recebida msg_recebida;

// -----------------------

// Variável obrigatória para função da interrupção do temporizador
char *tag_temp = "temp";

// variável de sentido/direção do robô (p/f/t/h/a)
char dir_t = 'p';    

// variável auxiliar para guardar último estado do movimento (estado anterior)
char dir_t_ant = 'p';

// variável inicial para obter o valor registrado pelo potênciometro do controle
int vel_t = 1;            

// variável de controle da trava do robô (false = desativada)
bool chave_t = false; 

// variável auxiliar para troca de status do robô
char status_troca = 0;

// Variáveis para os sensores de obstáculo - Valor lógico 1 simboliza que o sensor foi ativado (tem obstáculo)
bool sens_obs_d = 0;
bool sens_obs_t = 0;

// Variável para o sensoriamento das correntes --> Utilizo apenas uma variável, pois, qualquer um dos CS que passar do valor limite travará o robô
bool sens_cor = 0;  // valor lógico 1 simboliza que a corrente ultrapassou o valor limite

// Variável auxiliar para utilizar o temporizador para o sensor de corrente
char cont_cor = 0;

// variáveis auxiliares para armazenar o valor digital do sensor de corrente
int leitor_1 = 0;
int leitor_2 = 0;

// vetor para alocar os valores medidos do sensor de corrente
int aux_med_cor_1[10];
int aux_med_cor_2[10];

// Variáveis auxiliares para salvar as medições instantâneas do ADC
unsigned int sen_med_1 = 0;
unsigned int sen_med_2 = 0;

// Variável para identificar elemento do vetor de medição do sensor de corrente
byte vet_cont = 0;

// Indicador para controle de desaceleração
bool ctrl_acel = 0;

// COntador para desaceleração
byte cont_acel = 0;

// contador para desativar o movimento
byte cont_p = 0;
// =============================



// ========= Funções ============

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

//  send callback ESPNOW
void verifica_envio(const uint8_t *mac_addr, esp_now_send_status_t status){
    //Serial.print("ENV:\t");
    if(status == ESP_NOW_SEND_SUCCESS){
         //Serial.println("env_OK");
   }else{
       //Serial.println("env_n_OK");
   }

}

// recv callback ESPNOW
void verifica_receb(const uint8_t * mac, const uint8_t *incomingData, int len){
    //Serial.print("REC:\t");

    // Indico pelo led o recebimento
    gpio_set_level(LED, 1);

    // Variável auxiliar para permitir trocar a configuração do robô
    status_troca = 1;
    cont_p = 0;

    memcpy(&msg_recebida, incomingData, sizeof(msg_recebida));
    
    // Passo os dados recebidos para variáveis 
    dir_t = msg_recebida.direcao;
    vel_t = msg_recebida.velocidade;
    chave_t = msg_recebida.chave; 

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

// RSI - Temporizador de 60 ms
// Rotina de serviço de interrupção do temporizador
void IRAM_ATTR isr_temp_callback(void *args) {
  
  // Contagem para sinalizar quando obter um valor da medição do ADC
  cont_cor++;

  cont_p++;
  // Verifico se está recebendo algum comando
  if((!status_troca) && cont_p>1 ){
    // Coloco o movimento do robô como parado
    dir_t = 'p';
    Serial.println('4');
  }

  if(ctrl_acel){
      cont_acel++;  // incremento a cada 60ms
      if(cont_acel > 3) ctrl_acel = 0, cont_acel = 0;   // após 180 ms desabilito o controle de desaceleração
  }

  // Desativo o led depois de um tempo(60ms) de recebido os dados 
  gpio_set_level(LED, 0);
  
  // Finaliza a interrupção
  TIMERG0.int_clr_timers.t0 = 1;  
  TIMERG0.hw_timer[0].update=1;
  TIMERG0.hw_timer[0].config.alarm_en = 1;

}


// ===========================





void setup(){
 
    // Serial UART para debug
    Serial.begin(115200);

    // ---- LED indicativo ----
    gpio_pad_select_gpio(LED);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    gpio_set_level(LED, 0);
    // ------------------------

    // ----- ESPNOW Config -----

    WiFi.mode(WIFI_STA);

    // Inicialização do ESP NOW
    if (esp_now_init() != 0) {
        //Serial.println("ERRO ini espnow");
        return;
        //ESP.restart();
    }

    // Função para verificar o envio dos dados
    esp_now_register_send_cb(verifica_envio);

    // Registro do par
    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    // Adiciono o par especificado 
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
        //Serial.println("ERRO pareacao");
        return;
    }

    // Função para recebimento dos dados
    esp_now_register_recv_cb(verifica_receb);

    // -------------------------

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
  
  // --------- Configuração do sensor de Obstáculo dianteiro -----

    gpio_pad_select_gpio(SENS_obs_d);
    // Configuro como entrada a GPIO
    gpio_set_direction(SENS_obs_d, GPIO_MODE_INPUT);
    // Configuro o resistor da GPIO como PULL UP 
    gpio_set_pull_mode(SENS_obs_d, GPIO_PULLUP_ONLY);

  // -----------------------------------------

  // ---------- Configuração do sensor de Obstáculo traseiro ---------

    // As funções abaixo possuem a mesma lógica para as do sensor dianteiro
    gpio_pad_select_gpio(SENS_obs_t);
    gpio_set_direction(SENS_obs_t, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SENS_obs_t, GPIO_PULLUP_ONLY);
  // -----------------------------------------------

  // ------- Configuração do ADC -----------
  adc2_config_channel_atten(ADC2_CHANNEL_0, ADC_ATTEN_DB_11); // GPIO 4
  adc2_config_channel_atten(ADC2_CHANNEL_5, ADC_ATTEN_DB_11); // GPIO 12
  // ----------------------------------------

  // =============== Configuração dos motores ===================

    // ------- ENable dos drivers -------
        gpio_pad_select_gpio(EN_1);
        gpio_set_direction(EN_1, GPIO_MODE_OUTPUT);
        gpio_set_level(EN_1, 0);

        gpio_pad_select_gpio(EN_2);
        gpio_set_direction(EN_2, GPIO_MODE_OUTPUT);
        gpio_set_level(EN_2, 0);  
    // ----------------------------------

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

  // ===========================================================

  // Inicio o temporizador
  timer_start(TIMER_GROUP_0, TIMER_0);

}




void loop(){
    
    // ---- Varredura dos sensores de obstáculo ---
        // dianteiro
        if(!gpio_get_level(SENS_obs_d)){
            sens_obs_d = 1;
            //Serial.println("Sen_D");
        }else{
            sens_obs_d = 0;
        }
        // traseiro
        if(!gpio_get_level(SENS_obs_t)){
            sens_obs_t = 1;
            //Serial.println("Sen_T");
        }else{
            sens_obs_t = 0;
        }
    // -------------------------------------------
    
    // ----- Leitura do Sensor de corrente -------
    // Ocorre a cada 120 ms e após 1.2s temos um valor médio da medição
        if(cont_cor > 1){
            adc2_get_raw(ADC2_CHANNEL_0, ADC_WIDTH_BIT_12, &leitor_1);
            adc2_get_raw(ADC2_CHANNEL_5, ADC_WIDTH_BIT_12, &leitor_2);
            cont_cor = 0;
            if(vet_cont > 9){
                vet_cont = 0;
                for(int i = 0; i < 10; i++){
                    sen_med_1 += aux_med_cor_1[i];
                    sen_med_2 += aux_med_cor_2[i];
                }
                if((sen_med_1/10 > 5000) || (sen_med_2/10 > 5000)){
                    //Serial.println("Cor_n_ok");
                    sens_cor = 1;
                }else{
                    //Serial.println("Cor_ok");
                    sens_cor = 0;
                }
            }else{
                aux_med_cor_1[vet_cont] = leitor_1;
                aux_med_cor_2[vet_cont] = leitor_2;
                vet_cont++;
            }
            
        }
    // --------------------------------------------

     // ----- Resposta aos comando ------
        if(status_troca){               // Verifica se pode trocar (essa variável é atualizada sempre que receber um pacote)

            // Verifica se a chave de ponto morto está acionada e se os sensores estão em alto
            if(!chave_t && !sens_cor){                 

            gpio_set_level(EN_1, 1);    // Caso não estejam permite que os motores sejam acionados
            gpio_set_level(EN_2, 1);

            if(dir_t == 'p' && dir_t_ant != 'p'){
                ctrl_acel = 1;
            }

            if(ctrl_acel){
                // seleciona direção do movimento
                if(dir_t != 'p'){
                    f_dir(dir_t);
                    Serial.println('1');
                }else{
                    f_dir(dir_t_ant);
                    Serial.println('2');
                }
                // seleciona velocidade (pwm) do movimento
                // Aplicação da velocidade especificada
                mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, ((3 - cont_acel)/3)*20*f_vel_atual(vel_t));
                mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, ((3 - cont_acel)/3)*20*f_vel_atual(vel_t));
            }else{
                // seleciona direção do movimento
                f_dir(dir_t);
                Serial.println('3');
                // seleciona velocidade (pwm) do movimento
                // Aplicação da velocidade especificada
                mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 20*f_vel_atual(vel_t));
                mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, 20*f_vel_atual(vel_t));
            }

            }else{
            // Caso a chave de ponto morto esteja acionada deixamos as portas de ENABLE do shield em nível baixo (não permitindo movimento)
            gpio_set_level(EN_1, 0);
            gpio_set_level(EN_2, 0);
            
            }
            
            status_troca = 0;     // Zeramos a variável para identificar que o comando recebido já foi executado pelo robô

        }else{
            if(dir_t != 'p'){
                f_dir(dir_t);
                Serial.println('1');
            }else{
                f_dir(dir_t_ant);
                Serial.println('2');
            }
            // seleciona velocidade (pwm) do movimento
            // Aplicação da velocidade especificada
            mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, ((3 - cont_acel)/3)*20*f_vel_atual(vel_t));
            mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, ((3 - cont_acel)/3)*20*f_vel_atual(vel_t));
        }    

    // ----------------------------------------

}

// Obter valor da velocidade
int f_vel_atual(int valor){
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

      return vel_atual;
}

// Direcionar movimento do robô
void f_dir(char dado){
    // Verifica qual direção/sentido foi enviado como comando
      if(dado == 'f' && !sens_obs_d){           
        mov_frente();
      }else if(dado == 't' && !sens_obs_t){
        mov_tras();
      }else if(dado == 'a'){
        mov_ant_hora();
      }else if(dado == 'h'){
        mov_hora();
      }else{
        mov_parado();
      }

    // Salvo valor anterior da direção
    dir_t_ant = dir_t;
}
