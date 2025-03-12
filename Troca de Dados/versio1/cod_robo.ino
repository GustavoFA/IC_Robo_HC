// Código do Robô

// ESP-NOW Espressif : https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html

// Protocolo de comunicação ESP-NOW (Espressif)
// Fazemos uma comunicação entre dois dispositivos sem a conexão fixa de WiFi entre eles
// Utilizamos o código MAC para fazer essa ponte de envio de dados

// ----- Includes ----

#include <WiFi.h>
#include <esp_now.h>

// -------------------



// ----- Variáveis e defines ------

#define mov 4
#define trava 5

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

uint8_t broadcastAddress[] = {0xb4, 0xe6, 0x2d, 0x86, 0x26, 0x95};

// Variáveis temporais para desligar led de identificação de envio de mensagens
unsigned int lastTime;
const int intervalo = 10;

// ------------------



 
void setup() {

  // Configuração das GPIOs
  pinMode(mov, OUTPUT);
  pinMode(trava, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(2,0);
  digitalWrite(trava,0);
  digitalWrite(mov,0);
  
  // Não está sendo muito utilizado, devido aos problemas que causa no teste com a comunicação em baixa latência
  Serial.begin(115200);
  
  // WiFi Station podendo se conectar a qualquer fonte Access point
  // No caso, utilizando ESP-NOW não precisamo deixar nosso WiFi conectado a de outro dispositivo

  // Em código exemplo é utilizado o modo WIFI_AP, porém, quando uso aqui o modo Access Point com o outro dispositivo estando em modo Station o envio de dados não ocorre
  WiFi.mode(WIFI_STA);

  // ======= Configuração do ESP NOW =======

  // Inicialização do ESP NOW
  if (esp_now_init() == 0) {
    Serial.println("Inicializacao ocorreu com sucesso");
  }else{
    Serial.println("Erro na inicializacao");
    // Recomenda-se fazer um contador para passar um tempo e verificar de novo, e caso ocorra erro, reiniciar o ESP
    Serial.println("Reiniciando");
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
    Serial.println("Erro em adicionar o par");
    return;
  }

  // Função para recebimento dos dados
  esp_now_register_recv_cb(func_recebimento);

  // Função para verificar o envio dos dados
  esp_now_register_send_cb(verifica_envio);

  // =================================
}





// Função de callback que é executada quando recebe algum dado
// No Docs da Espressif não é recomendado que está função faça operações longas
void func_recebimento(const uint8_t * mac, const uint8_t *incomingData, int len){
  // Copia as variáveis da struct enviada para a struct criada nesse código
  memcpy(&msg_recebida, incomingData, sizeof(msg_recebida));
  // Prints para verificar tal funcionamento
  //Serial.println(" ---------- ");
  //Serial.println("Pacote recebido: ");
  //Serial.println(msg_recebida.velocidade);
  //Serial.println(msg_recebida.direcao);
  //Serial.println(msg_recebida.chave);

  msg_enviada.velocidade = msg_recebida.velocidade;
  msg_enviada.direcao = msg_recebida.direcao;
  msg_enviada.chave = msg_recebida.chave;

  if(msg_recebida.direcao == 'f'){
    digitalWrite(mov, 1);
  }else{
    digitalWrite(mov, 0);
  }

  if(msg_recebida.chave == false){
    digitalWrite(trava, 1);
  }else{
    digitalWrite(trava, 0);
  }
  
  msg_enviada.sensor_frente = 0;
  msg_enviada.sensor_tras = 0;
  msg_enviada.sensor_corrente = 0;
  
  //Serial.println("\nEnviando comandos recebidos de volta para o emissor");
  esp_err_t retorno = esp_now_send(broadcastAddress, (uint8_t *) &msg_enviada, sizeof(msg_enviada));
  /*
  Serial.print("Verificando se o envio ocorreu: ");
  // Verifico retorno
    if(retorno == ESP_OK){
      Serial.println("ESP_OK");
    }else if(retorno == ESP_ERR_ESPNOW_NOT_INIT){
      Serial.println("ESP_ERR_ESPNOW_NOT_INIT");
    }else if(retorno == ESP_ERR_ESPNOW_ARG){
      Serial.println("ESP_ERR_ESPNOW_ARG");
    }else if(retorno == ESP_ERR_ESPNOW_INTERNAL ){
      Serial.println("ESP_ERR_ESPNOW_INTERNAL ");
    }else if(retorno == ESP_ERR_ESPNOW_NO_MEM ){
      Serial.println("ESP_ERR_ESPNOW_NO_MEM ");
    }else if(retorno == ESP_ERR_ESPNOW_NOT_FOUND ){
      Serial.println("ESP_ERR_ESPNOW_NOT_FOUND ");
    }else if(retorno == ESP_ERR_ESPNOW_IF ){
      Serial.println("ESP_ERR_ESPNOW_IF ");
    }else{
      Serial.println(retorno);
    }
  */
}



void verifica_envio(const uint8_t *mac_addr, esp_now_send_status_t status){
  //Serial.println();
  //Serial.print("Verificando o envio:\t");
  // Checagem visual se a mensagem foi recebida
  if(status == ESP_NOW_SEND_SUCCESS){
    digitalWrite(2,1);  // Led continua ligado caso não receba mensagem não identificando que não está mais recebendo dados, logo, não está mais enviando também
  }else{
    digitalWrite(2,0);
  }
  
  //Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Envio com sucesso" : "Falha no envio");
  //Serial.println();
  //Serial.println(" --------- ");
}

void loop() {

  if(millis() - lastTime >= intervalo){

    digitalWrite(2,0);
    lastTime = millis();
    
  }
  

}
