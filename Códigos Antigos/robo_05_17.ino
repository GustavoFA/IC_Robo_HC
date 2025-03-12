// Código Robô - versão 05.17


/* Módulos: 
 * Comunicação ESPNOW    
 * Sensores de Obstáculo --> Usar ADC 1, pois, o ADC 2 não pode ser usados quando utilizamos o WiFi
 * Sensor de Corrente
 * Comandos para movimentação base do motor
 * Controle de desaceleração (faltando)
 * Envio de sensores de corrente e obstáculo sem espera de comando 
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
#define CS_1 GPIO_NUM_36  // channel 0 (VP)

// Controle do motor 2
#define EN_2 GPIO_NUM_14
#define pwm_2 GPIO_NUM_25
#define A_2 GPIO_NUM_26
#define B_2 GPIO_NUM_27
#define CS_2 GPIO_NUM_39  // channel 3 (VN)

#define SENS_obs_d GPIO_NUM_33 // sensor dianteiro
#define SENS_obs_t GPIO_NUM_32 // sensor traseiro

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

// variável inicial para obter o valor registrado pelo potênciometro do controle
int vel_t = 1;            

// variável de controle da trava do robô (false = desativada)
bool chave_t = false; 

// variável auxiliar para troca de status do robô
char status_troca = 0;

// Variáveis para os sensores de obstáculo - Valor lógico 1 simboliza que o sensor foi ativado (tem obstáculo)
bool sens_obs_d = 0;
bool sens_obs_t = 0;

// Variáveis auxiliares para status anterior dos sensores de obstáculo
bool sens_obs_d_ant = 0;
bool sens_obs_t_ant = 0;

// Variável para o sensoriamento das correntes --> Utilizo apenas uma variável, pois, qualquer um dos CS que passar do valor limite travará o robô
bool sens_cor = 0;  // valor lógico 1 simboliza que a corrente ultrapassou o valor limite

// Variável auxiliar para o sensor de corrente
bool sens_cor_ant = 0;

// Variável auxiliar para utilizar o temporizador para o sensor de corrente
char cont_cor = 0;

// variáveis auxiliares para armazenar o valor digital do sensor de corrente
int leitor_1 = 0;
int leitor_2 = 0;

// vetor para alocar os valores medidos do sensor de corrente
int aux_med_cor_1[40];
int aux_med_cor_2[40];

// Variáveis auxiliares para salvar as medições instantâneas do ADC
unsigned long sen_med_1 = 0;
unsigned long sen_med_2 = 0;

// Variável para identificar elemento do vetor de medição do sensor de corrente
byte vet_cont = 0;

// contador para desativar o movimento
byte cont_p = 0;
// =============================



// ========= Funções ============

// Funções de movimentação

void mov_frente(){
  
    gpio_set_level(A_1, 0);
    gpio_set_level(B_1, 1);

    gpio_set_level(A_2, 0);
    gpio_set_level(B_2, 1);

}
  
void mov_tras(){
    
    gpio_set_level(A_1, 1);
    gpio_set_level(B_1, 0);

    gpio_set_level(A_2, 1);
    gpio_set_level(B_2, 0);

}
  
void mov_ant_hora(){
    
    gpio_set_level(A_1, 0);
    gpio_set_level(B_1, 1);
    
    gpio_set_level(A_2, 1);
    gpio_set_level(B_2, 0);
    
}
  
void mov_hora(){
    
    gpio_set_level(A_1, 1);
    gpio_set_level(B_1, 0);
    
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
    // Essa função está sem fazer nada 
    if(status == ESP_NOW_SEND_SUCCESS){
      // Não tem o que fazer
   }

}

// recv callback ESPNOW
void verifica_receb(const uint8_t * mac, const uint8_t *incomingData, int len){

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

    // Antes de enviar os valores dos sensores iremos salvá-los para saber o último valor mandado por cada um deles
    sens_obs_d_ant = sens_obs_d;
    sens_obs_t_ant = sens_obs_t;
    sens_cor_ant = sens_cor;
    
    // Envio dos dados
    esp_now_send(broadcastAddress, (uint8_t *) &msg_enviada, sizeof(msg_enviada));
}

// RSI - Temporizador de 6 ms
// Rotina de serviço de interrupção do temporizador
void IRAM_ATTR isr_temp_callback(void *args) {
  
  // Contagem para sinalizar quando obter um valor da medição do ADC
  cont_cor++;

  cont_p++;
  // Verifico se está recebendo algum comando
  if((!status_troca) && cont_p>10 ){
    // Coloco o movimento do robô como parado depois de 60 ms sem receber nenhum comando novo
    dir_t = 'p';
    mov_parado();
    // Deixo o PWM zerado para evitar ruídos no CS
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 0);
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, 0);

    // Sinalizo que não há comando para ser executado
    gpio_set_level(LED, 0);
  }
  
  
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
    // Não sei se if seria a melhor forma de iniciarmos o esp now
    if (esp_now_init() != 0) {
        return;
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
  timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 6000); // Aqui adiciono o valor máximo do contador para um tempo final de 6 ms
  timer_enable_intr(TIMER_GROUP_0, TIMER_0);  // Ativo uma RSI
  timer_isr_register(TIMER_GROUP_0, TIMER_0, &isr_temp_callback, &tag_temp, 2, NULL); // Relaciono com a respectiva RSI e coloco Prioridade 2
  
  // ------------------------
  
  // --------- Configuração do sensor de Obstáculo dianteiro -----

    gpio_pad_select_gpio(SENS_obs_d);
    // Configuro como entrada a GPIO
    gpio_set_direction(SENS_obs_d, GPIO_MODE_INPUT);
    

    // Deixo as GPIOs dos sensores em alta impedância
    gpio_pullup_dis(SENS_obs_d);
    gpio_pulldown_dis(SENS_obs_d);
  // -----------------------------------------

  // ---------- Configuração do sensor de Obstáculo traseiro ---------

    // As funções abaixo possuem a mesma lógica para as do sensor dianteiro
    gpio_pad_select_gpio(SENS_obs_t);
    gpio_set_direction(SENS_obs_t, GPIO_MODE_INPUT);
    
    
    // Deixo as GPIOs dos sensores em alta impedância
    gpio_pullup_dis(SENS_obs_t);
    gpio_pulldown_dis(SENS_obs_t);
  // -----------------------------------------------

  // ------- Configuração do ADC -----------
  // Usaremos atenuação de 0dB que está relacionado a faixa de medição de 100mV a 950mV
  adc1_config_width(ADC_WIDTH_BIT_12); 
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_0);
  adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_0);
  // ----------------------------------------

  // =============== Configuração dos motores ===================

    // ------- Enable dos drivers -------
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

        pwm_config_1.frequency = 15000;                   // Frequência --> limitada em 15 kHz pois o driver tem limite de 20 kHz
        pwm_config_1.cmpr_a = 100;                        // Duty Cycle
        pwm_config_1.cmpr_b = 100;                        // Duty Cycle
        pwm_config_1.duty_mode = MCPWM_DUTY_MODE_0;       // Tempo do duty cycle é de nível alto
        pwm_config_1.counter_mode = MCPWM_UP_COUNTER;     // Contagem crescente
        mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config_1); // Aplico a configuração no módulo com o temporizador 0

        // Motor 2
        mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, pwm_2);  // Seleciono a unidade de PWM, sua saída e a GPIO
        mcpwm_config_t pwm_config_2;                      // Struct para configuração do PWM

        pwm_config_2.frequency = 15000;                   // Frequência
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
    
    // ----- Leitura do Sensor de corrente -------
    // Ocorre a cada 6 ms e após 240 ms temos um valor médio da medição
        if(cont_cor > 0){

            cont_cor = 0;
            // Pega 40 medições do CS
            if(vet_cont > 39){ 
                vet_cont = 0;
                for(int i = 0; i < 40; i++){
                    sen_med_1 += aux_med_cor_1[i];
                    sen_med_2 += aux_med_cor_2[i];
                }

                /*
                  Fórmula utilizada:
                    Tensão = 0,239 * (BIT) + 40,18 [mV]
                */
               
                // Print para teste da tensão 
                
                Serial.print("M1: ");
                Serial.println((sen_med_1)/40);
                Serial.print("M2: ");
                Serial.println((sen_med_2)/40);
                

                // Valor limite de tensão será de 390mV (3A) --> ~ 1463 digital
                // Se em algum momento for identificado uma corrente superior ao limiar o robô para (recebe comandos, mas não executa nenhum)
                // Só poderá se movimentar novamente caso reinicie o robô
                // Utilizamos um valor digital de 2000 e chegamos próximo de 600 mV
                if((sen_med_1/40 > 3000) || (sen_med_2/40 > 3000)){
                    sens_cor = 1;
                }
                // Zero as variáveis de soma dos valores digitais
                sen_med_1 = sen_med_2 = 0;

            }else{
                // Leitura dos sensores
                aux_med_cor_1[vet_cont] = adc1_get_raw(ADC1_CHANNEL_0);
                aux_med_cor_2[vet_cont] = adc1_get_raw(ADC1_CHANNEL_3);
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

              
            f_dir(dir_t);               // Atualizo movimento

            // Seleciona velocidade (pwm) do movimento
            // Aplicação da velocidade especificada 
            if(dir_t == 'p'){
              // Deixo o PWM zerado para evitar ruídos no CS (parece que não entra aqui, pois, status_troca estará zerada por não ter mais comandos para executar)
                mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 0);
                mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, 0);
            }else{
                mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 20*f_vel_atual(vel_t));
                mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, 20*f_vel_atual(vel_t));
            }
            
            

            }else{
            // Caso a chave de ponto morto esteja acionada deixamos as portas de ENABLE do shield em nível baixo (não permitindo movimento)
                gpio_set_level(EN_1, 0);
                gpio_set_level(EN_2, 0);

                // Deixo o PWM zerado para evitar ruídos no CS
                mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 0);
                mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, 0);
            
            }
            
            status_troca = 0;     // Zeramos a variável para identificar que o comando recebido já foi executado pelo robô

        }else{

          // Enviar status do sensor de obstáculo e do sensor de corrente
          // Enviar isso apenas uma vez caso o sensor mude de status
          if(sens_obs_d_ant != sens_obs_d || sens_obs_t_ant != sens_obs_t || sens_cor){
            // Terá que enviar de novo os valores e atualzar o ultimo valor mandado
            sens_obs_d_ant = sens_obs_d;
            sens_obs_t_ant = sens_obs_t;

            msg_enviada.sensor_frente = sens_obs_d;
            msg_enviada.sensor_tras = sens_obs_t;
            msg_enviada.sensor_corrente = sens_cor;

            // Envio dos dados
            esp_now_send(broadcastAddress, (uint8_t *) &msg_enviada, sizeof(msg_enviada));
          }
        }
        
    // ----------------------------------------



}

// Obter valor da velocidade
int f_vel_atual(int valor){
    // Aplica a velocidade especificada (Está ruim a divisão de valores para cada valor de vel_atual)
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

}
