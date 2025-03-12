// Código do Controle 


// ----------- Includes ---------

#include <esp_now.h>
#include <WiFi.h>
#include <driver/gpio.h>  // Para controle das GPIOs do ESP
#include <driver/adc.h>   // Para controle dos ADCs

// ----------------------------


// ---------- Variáveis e defines ------

// GPIOS utilizadas para testes
#define bot_f GPIO_NUM_34        // movimento frontal
#define bot_t GPIO_NUM_35        // movimento traseiro
#define bot_a GPIO_NUM_32        // movimento anti-horário
#define bot_h GPIO_NUM_33        // movimento horário
#define chave_trava GPIO_NUM_26  // Chave de ponto morto
// Utilizo GPIO 36 para ADC

// Endereço MAC (do ESP32 do Robô) utilizado para fazer o envio do pacote de dados
uint8_t broadcastAddress[] = {0xa4, 0xe5, 0x7c, 0x6d, 0xfb, 0xd8}; 
  
// Estrutura utilizada para o envio dos dados (deve ser semelhante a struct que recebe)
typedef struct mensagem_enviada{
  char direcao;
  int velocidade;
  bool chave;
}mensagem_enviada;

// Criação da struct
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

// Criação da struct
mensagem_recebida msg_recebida;

// Variáveis para marcação de tempo, e assim, permitir que o envio seja feito a cada determinado intervalo
unsigned int lastTime;
// Latência dos dados
const int intervalo = 50;

// Variável auxiliar para caso o controle entre em um loop de erros nas mensagens ele possa reiniciar o micro
uint8_t contador = 0;

// Variáveis auxiliares para armazenarem temporiariamente o status dos comandos
uint8_t dir_aux = 'p';          // Variável para armazenar o movimento atual acionado no controle
uint8_t dir_aux_comp = 'p';     // Variável comparativa que serve para não ficar enviando o mesmo comando de forma desnecessária
bool chave_de_controle = false; // Variável para armazenar a chave de controle
bool chave_de_controle_aux = 0;
int val_pot = 0;
bool pode_enviar = 0;

// -------------------------------

// Função para configuração rápida dos botões
void conf_gpio(gpio_num_t input_gpio){
  gpio_pad_select_gpio(input_gpio);
  gpio_set_direction(input_gpio, GPIO_MODE_INPUT);
  gpio_set_pull_mode(input_gpio, GPIO_PULLUP_ONLY);
}

// Função para verificar status do envio do pacote
void verifica_envio(const uint8_t *mac_addr, esp_now_send_status_t status){

  if(status == ESP_NOW_SEND_SUCCESS){
    gpio_set_level(GPIO_NUM_2, 0);
  }else{
    gpio_set_level(GPIO_NUM_2, 1);
  }
  
}


// Função de recebimento
void func_recebimento(const uint8_t * mac, const uint8_t *incomingData, int len){
  // Passo o pacote recebido para a struct de recebimento
  memcpy(&msg_recebida, incomingData, sizeof(msg_recebida));

  if((msg_recebida.velocidade == msg_enviada.velocidade) && (msg_recebida.direcao == msg_enviada.direcao) && (msg_recebida.chave == msg_enviada.chave)){
    contador = 0;
  }else{
    // Depois de ocorrer uma quantidade de vezes seguidas de erro na mensagem reinicio o ESP32
    if(contador == 5){
      ESP.restart();  
    }else{
      contador++;
    }
    msg_enviada.chave = true; // Caso os dados estejam errados atualizo o dado da chave para 1 para que assim o robô fique parado 
    // e não execute o pacote de comandos errados
    esp_now_send(broadcastAddress, (uint8_t *) &msg_enviada, sizeof(msg_enviada));
  }
  
}




void setup() {

   Serial.begin(115200);
  
  // --- Configuração das GPIOs ------

  // Configurando os botões de comandos de direção
  conf_gpio(bot_f);
  conf_gpio(bot_t);
  conf_gpio(bot_a);
  conf_gpio(bot_h);

  // Configurando a chave de ponto morto
  gpio_pad_select_gpio(chave_trava);
  gpio_set_direction(chave_trava, GPIO_MODE_INPUT);
  
  // ------------------------------

  // Configuro o WiFi para o modo Station
  WiFi.mode(WIFI_STA);

  // ==== Configuração para o ESP NOW =====

  // Inicio o ESP NOW
  if (esp_now_init() != ESP_OK) {
    ESP.restart();
  }
  
  // Informações do par a ser enviado os dados
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Adiciono o par especificado e valido tal conexão
  
  esp_now_add_peer(&peerInfo);
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    return;
  }

  // =========================================

  // Adiciono uma função para quando ocorrer o envio dos dados (serve para verificar se o par recebeu o pacote)
  // Não estou utilizando essa função
  esp_now_register_send_cb(verifica_envio);

  // Adiciono uma função para receber os dados enviados de retorno do par
  // A ideia aqui é que os dados enviadas retornem ao emissor (controle) para poder checar se o que ele, no caso o robô, recebeu está correto
  esp_now_register_recv_cb(func_recebimento);

  // ==========================================

  // Configurar o ADC com relação a sua atenuação -> 150 ~ 2450 mV 
  // Configurar o ADC do módulo 1 canal 0 (GPIO 36 = sensor VP)
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_11);

  // Começo a marcar o tempo para futuramente enviar os dados a cada 50 ms
  lastTime = millis();

}




void loop() {

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
    
    // Potenciômetro: obtêm o valor convertido pelo ADC e armazena na variável val_pot, com uma resolução de 12 bits
    val_pot = adc1_get_raw(ADC1_CHANNEL_0);
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
  if(millis() - lastTime >= intervalo && (pode_enviar | chave_de_controle)){  // não seria ||?
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
