// Código do Robô

// ESP-NOW Espressif : https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html


// ----- Includes ----

#include <WiFi.h>
#include <esp_now.h>

// -------------------



// ----- Variáveis e defines ------

#define led_f 5 
#define led_t 18
#define led_a 19
#define led_h 21
#define led_p 22

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

uint8_t broadcastAddress[] = {0x40, 0xf5, 0x20, 0x86, 0x5c, 0xfc};

// Variáveis temporais para desligar led de identificação de envio de mensagens
unsigned int lastTime;
const int intervalo = 55;

// variáveis de armazenamento
char dir_t = 'p';
int vel_t = 0;
bool chave_t = false;
bool sens_f = 0;
bool sens_t = 0;
bool sens_c = 0;

// variável auxiliar para troca de status do robô
char status_troca = 0;

// ------------------


// Função de envio
void verifica_envio(const uint8_t *mac_addr, esp_now_send_status_t status){
  
  if(status == ESP_NOW_SEND_SUCCESS){
    digitalWrite(2,1);  // Led continua ligado caso não receba mensagem não identificando que não está mais recebendo dados, logo, não está mais enviando também
  }else{
    digitalWrite(2,0);
  }
  
}

void func_recebimento(const uint8_t * mac, const uint8_t *incomingData, int len){

  memcpy(&msg_recebida, incomingData, sizeof(msg_recebida));

  dir_t = msg_recebida.direcao;
  vel_t = msg_recebida.velocidade;
  chave_t = msg_recebida.chave;   

  Serial.println(dir_t);
  Serial.println(vel_t);
  Serial.println(chave_t);

  // Variável auxiliar para permitir trocar a configuração do robô
  status_troca = 1;

  // Armazeno os dados recebidos com os dados analisados no robô para serem enviados de retorno ao controle
  msg_enviada.velocidade = msg_recebida.velocidade;
  msg_enviada.direcao = msg_recebida.direcao;
  msg_enviada.chave = msg_recebida.chave;
  msg_enviada.sensor_frente = sens_f;
  msg_enviada.sensor_tras = sens_t;
  msg_enviada.sensor_corrente = sens_c;
  // Envio dos dados
  esp_now_send(broadcastAddress, (uint8_t *) &msg_enviada, sizeof(msg_enviada));
  
}


void setup() {

  Serial.begin(115200);

  // Configuração dos pinos
  pinMode(led_f, OUTPUT);
  pinMode(led_t, OUTPUT);
  pinMode(led_a, OUTPUT);
  pinMode(led_h, OUTPUT);
  pinMode(led_p, OUTPUT);
  pinMode(2, OUTPUT);

  
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

  lastTime = millis();

}

void loop() {

  if(status_troca){

   digitalWrite(led_f, 0);
   digitalWrite(led_t, 0);
   digitalWrite(led_a, 0);
   digitalWrite(led_h, 0);
   digitalWrite(led_p, 0);

    if(!chave_t){
    if(dir_t == 'f'){
      digitalWrite(led_f, 1);
    }else if(dir_t == 't'){
      digitalWrite(led_t, 1);
    }else if(dir_t == 'a'){
      digitalWrite(led_a, 1);
    }else if(dir_t == 'h'){
      digitalWrite(led_h, 1);
    }else{
      digitalWrite(led_p, 1);
    }
    }else{
      digitalWrite(led_p, 1);
    }

    status_troca = 0;
    
  }

  if(millis() - lastTime >= intervalo){
    dir_t = 'p';
    digitalWrite(2, 0);
    lastTime = millis();
  }

}
