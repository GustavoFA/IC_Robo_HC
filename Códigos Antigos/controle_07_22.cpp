// Código Controle - versão 07.22

/*

        Para essa versão temos que o controle possui os 4 botões para selecionar a direção/sentido do movimento, junto com
    uma chave para seleção da velocidade (no caso haverá 2 velocidades que só serão definidas no robô), uma chave de trava dos
    comandos do controle e 4 LEDs indicativos para algumas situações (descrito ao longo do código).

        O controle apenas envia os comandos inseridos pelo usuário sem haver grandes interpretações do mesmo. Um comandos é enviado
    a cada 50 ms a cada comando pressionado. Caso não haja nada pressionado não será enviado nenhum dado.

*/

// ----------- Includes ---------

#include <esp_now.h>
#include <WiFi.h>
#include <driver/gpio.h>  // Para controle das GPIOs do ESP
#include <driver/adc.h>   // Para controle dos ADCs

// ----------------------------


// ====== Defines =========

#define bot_f GPIO_NUM_34        // movimento frontal
#define bot_t GPIO_NUM_35        // movimento traseiro
#define bot_a GPIO_NUM_32        // movimento anti-horário
#define bot_h GPIO_NUM_33        // movimento horário
#define chave_trava GPIO_NUM_26  // Chave de ponto morto
#define chave_velo GPIO_NUM_27   // Chave para mudar velocidade

#define led_obst GPIO_NUM_19     // LED indicativo para obstáculos
#define led_msg  GPIO_NUM_18     // LED indicativo para quando o envio do pacote ocorrer com sucesso
#define led_esp  GPIO_NUM_5      // LED indicativo para quando o ESP32 estiver funcionando
#define led_cor  GPIO_NUM_2      // LED indicativo para quando a corrente ultrapassar seu limiar
// =====================


// ====== Variáveis ========


// ----- ESP NOW  -------
uint8_t broadcastAddress[] = {0xa4, 0xe5, 0x7c, 0x47, 0x8c, 0xf0}; // MAC address atualizado do ESP32 utilizado no Lab. 

// Estrutura utilizada para o envio dos dados (deve ser semelhante a struct que recebe)
typedef struct mensagem_enviada{
  char direcao;
  int velocidade;
  bool chave;
}mensagem_enviada;
mensagem_enviada msg_enviada;

// Estrutura utilizada para receber o pacote de dados do robô
typedef struct mensagem_recebida{
  char direcao;
  int velocidade;
  bool chave;
  bool sensor_frente;
  bool sensor_tras;
  bool sensor_corrente;
}mensagem_recebida;
mensagem_recebida msg_recebida;
// -----------------------------

// Variáveis auxiliares para armazenarem temporiariamente o status dos comandos
char dir_aux = 'p';             // Variável para armazenar o movimento atual acionado no controle
char dir_aux_comp = 'p';        // Variável comparativa que serve para não ficar enviando o mesmo comando de forma desnecessária
bool chave_de_controle = false; // Variável para armazenar a chave de controle
int val_pot = 3000;             // Variável para indicar valor da velocidade do robô (2000 --> 60% e 3000 --> 80%)
bool chave_de_controle_aux = 0;

// variável para permitir que o sistema não fique enviando comandos todo momento
bool pode_enviar = 0;

// Variável para salvar o último tempo registrado
unsigned long lastTime = 0;

// Tempo entre cada pacote enviado
const int intervalo = 50;

// Variável auxiliar para caso o controle entre em um loop de erros nas mensagens ele possa reiniciar o micro
uint8_t contador = 0;

// Contador para temporização para apagar o LED de envio correto de pacote
// Loop reinicia a cada 125 ns (aproximadamente, 80 MHz)
unsigned long cont_time_led = 0;

// ===============================


// ======== Funções ================

// Função para verificar status do envio do pacote
void verifica_envio(const uint8_t *mac_addr, esp_now_send_status_t status){

    // Não tem motivo de ter essa função callback

}

// Função de recebimento
void verifica_receb(const uint8_t * mac, const uint8_t *incomingData, int len){
  //Serial.println("recv");
  memcpy(&msg_recebida, incomingData, sizeof(msg_recebida));

  // Quando receber o eco do robô confirmo que a comunicação está OK
  gpio_set_level(led_msg, 1);

  if((msg_recebida.velocidade == msg_enviada.velocidade) && (msg_recebida.direcao == msg_enviada.direcao) && (msg_recebida.chave == msg_enviada.chave)){
    contador = 0;
  }else{
    // Depois de ocorrer uma quantidade de vezes seguidas de erro na mensagem reinicio o ESP32
    if(contador >= 10){ // troquei para de == para >=
      ESP.restart();  
    }else{
      contador++;
    }
    msg_enviada.chave = true; // Caso os dados estejam errados atualizo o dado da chave para 1 para que assim o robô fique parado 
    // e não execute o pacote de comandos errados
    esp_now_send(broadcastAddress, (uint8_t *) &msg_enviada, sizeof(msg_enviada));
  }

  // verifico sensor de corrente (caso ocorra uma vez deixo evidente ao controlador a ocorrência)
  if(msg_recebida.sensor_corrente){
    gpio_set_level(led_cor, 1);
  }

  // verifico se os sensores foram acionados
  if(msg_recebida.sensor_frente || msg_recebida.sensor_tras){
    gpio_set_level(led_obst, 1);
  }else{
    gpio_set_level(led_obst, 0);
  }
  
}

// Função para configuração rápida dos botões (INPUTs)
void conf_gpio_in(gpio_num_t input_gpio){
  gpio_pad_select_gpio(input_gpio);
  gpio_set_direction(input_gpio, GPIO_MODE_INPUT);
  gpio_set_pull_mode(input_gpio, GPIO_PULLUP_ONLY);
}

// Função para configuração rápida dos leds (OUTPUTs)
void conf_gpio_out(gpio_num_t output_gpio){
  gpio_pad_select_gpio(output_gpio);
  gpio_set_direction(output_gpio, GPIO_MODE_OUTPUT);
  gpio_set_level(output_gpio, 0);
}

// ================================

void setup() {

  Serial.begin(115200);

  // ----- ESPNOW Config -----

    WiFi.mode(WIFI_STA);

    // Inicialização do ESP NOW
    if (esp_now_init() != 0) {
        Serial.println("ERRO ini espnow");
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
        Serial.println("ERRO pareacao");
        return;
    }

    // Função para recebimento dos dados
    esp_now_register_recv_cb(verifica_receb);

  // -------------------------

  // --- Configuração das GPIOs ------
  
    // Configurando os botões de comandos de direção
    conf_gpio_in(bot_f);
    conf_gpio_in(bot_t);
    conf_gpio_in(bot_a);
    conf_gpio_in(bot_h);

    
    conf_gpio_out(led_cor);
    conf_gpio_out(led_msg);
    conf_gpio_out(led_obst);
    conf_gpio_out(led_esp);

    // Habilito led para sinalizar que o ESP32 está funcionando
    gpio_set_level(led_esp, 1);
  
    // Configurando a chave de ponto morto
    gpio_pad_select_gpio(chave_trava);
    gpio_set_direction(chave_trava, GPIO_MODE_INPUT);
    gpio_set_pull_mode(chave_trava, GPIO_PULLDOWN_ONLY);

    // Configurando a chave de velocidade
    gpio_pad_select_gpio(chave_velo);
    gpio_set_direction(chave_velo, GPIO_MODE_INPUT); 
    pio_set_pull_mode(chave_velo, GPIO_PULLDOWN_ONLY);
    
  // ------------------------------
  
  lastTime = millis();
}

void loop() {

   // Temporização de 42 ms para desligar led de pacote correto
   if(cont_time_led >= 1000){
      gpio_set_level(led_msg, 0);
      cont_time_led = 0;
   }else{
      cont_time_led++;
   }

   envia_comandos();
  
}


void envia_comandos(){

  // Condicional para obter os valores dos botões, chave e potenciômetro
  // Primeiramente, verifico se a chave de ponto morto está ativa 
  if(gpio_get_level(chave_trava)){
    chave_de_controle = true;
    if(chave_de_controle_aux){  // Esse variável serve para que quando a chave seja acionada o envio do pacote só ocorra uma vez
      // fazendo com que só retorne quando desativar a chave.
      lastTime = millis();
    }
  }else{ // Caso a chave não esteja acionada, verifico qual botão está ativado e o valor do potenciômetro
    chave_de_controle = false;
    if(gpio_get_level(bot_f)){
      dir_aux = 'f';
    }else if(gpio_get_level(bot_t)){
      dir_aux = 't';
    }else if (gpio_get_level(bot_a)){
      dir_aux = 'a';
    }else if(gpio_get_level(bot_h)){
      dir_aux = 'h';
    }else{
      dir_aux = 'p';
    }
    
    // Checagem da velocidade desejada
    if(gpio_get_level(chave_velo)){
        // 80% PWM
        val_pot = 4000;
    }else{
        // 60% PWM
        val_pot = 3000;
    }

  }
  
  // Com essa verificação consiguimos fazer apenas os comandos úteis serem enviados
  /* A ideia é verificar se o último comando enviado foi de parado e se caso for
     o próximo comando deverá ser algo diferente de parado para ser enviado.
   */
  if(dir_aux_comp == 'p'){
    if(dir_aux == 'p'){
      pode_enviar = 0;
    }else{
      pode_enviar = 1;
    }
  }else{
    pode_enviar = 1;
  }

  // Envio das variáveis 
  if(millis() - lastTime >= intervalo && (pode_enviar || chave_de_controle)){  

    // Atualizo as variáveis (isso poderia ser feito em outra função, por exemplo, por um controle ou sensor)
    // A mensagem(pacote) enviado pode ser um número real, um inteiro, um conjunto de caracteres ou uma struct podendo conter os 3 tipos
    
    msg_enviada.velocidade = val_pot;
    msg_enviada.direcao = dir_aux;  
    msg_enviada.chave = chave_de_controle;

    // Envio da mensagem
    esp_err_t retorno = esp_now_send(broadcastAddress, (uint8_t *) &msg_enviada, sizeof(msg_enviada));

    // Armazeno a última direção enviada, para depois utilizar em uma comparação
    dir_aux_comp = dir_aux;

    // Essa checagem serve para que apenas uma vez seja enviado para o robô
    // que o robô entrará em ponto morto
    if(chave_de_controle){
      chave_de_controle_aux = 1;
    }else{
      chave_de_controle_aux = 0;
    }
    
  // Armazeno o último tempo depois de concluir o envio
    lastTime = millis(); 
  }

}
