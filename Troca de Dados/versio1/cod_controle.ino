// Código do Controle

// ESP-NOW Espressif : https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html
// Ref:https://randomnerdtutorials.com/esp-now-esp32-arduino-ide/ | https://randomnerdtutorials.com/esp-now-two-way-communication-esp32/


// Protocolo de comunicação ESP-NOW (Espressif)
// Fazemos uma comunicação entre dois dispositivos sem a conexão fixa de WiFi entre eles
// Utilizamos o código MAC para fazer essa ponte de envio de dados
/* Para proteger os dados enviados é utilizado o método CCMP, sendo que o WiFi possui PMK (Primary Master
Key) e um LMK (Local Master Key), sendo que ambas possuem tamanho de 16 bytes.
*/

// Farme format
//  ------------------------------------------------------------------------------------------------------------
//  | MAC Header | Category Code | Organization Identifier | Random Values | Vendor Specific Content |   FCS   |
//  ------------------------------------------------------------------------------------------------------------
//    24 bytes         1 byte              3 bytes               4 bytes             7~255 bytes        4 bytes

// Vendor Specific Content
//  -------------------------------------------------------------------------------
//  | Element ID | Length | Organization Identifier | Type | Version |    Body    |
//  -------------------------------------------------------------------------------
//      1 byte     1 byte            3 bytes         1 byte   1 byte   0~250 bytes

// O Body do Vendor Specific Content contém os dados que pretendemos enviar

// Teste com envio a cada 10ms apresentou muitos erros no envio dos pacotes e o microcontrolador esquentou muito. Nesse teste utilizei muitos prints na serial,
// logo, pretendo tirar alguns deles para verificar se melhora o desempenho. Para 15 a 20 ms apresentou uma quantidade de erros menores, mas ainda eram mais frequentes
// e para o tempo de 25ms a frequência de erros foi muito baixa demonstrando ser uma latência mais eficiente.

// Tive que tirar todos os prints, pois, com alguns poucos deles já impossibilitava o envio dos pacotes sem ocorrer, com alta frequência, erros.



// ----------- Includes ---------

#include <esp_now.h>
#include <WiFi.h>
#include <esp_err.h>

// -----------------------



// ---------- Variáveis e defines ------

// GPIOS utilizadas para testes
#define botao 4 // Botão simbolizando um dos comandos de direção
#define pote 34 // Potenciômetro utilizado para escolha de velocidade
#define ponto_morto 23  // Chave para manter o robô fixo (parado)

// Não uso
bool validade_dos_dados = true;

// Endereço MAC (do ESP32 do Robô) utilizado para fazer o envio do pacote de dados
uint8_t broadcastAddress[] = {0xb4, 0xe6, 0x2d, 0xc7, 0x5d, 0xbd}; 

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
const int intervalo = 15;

// Variável auxiliar para caso o controle entre em um loop de erros nas mensagens ele possa reiniciar o micro
uint8_t contador = 0;

// -------------------------------



 
void setup() {
  
  // Serial para verificão do funcionamento
  // Não estou utilizando, pois, atrapalha no funcionamento do sistema em geral
  Serial.begin(115200);

  // Configuro as GPIOs
  pinMode(ponto_morto, INPUT_PULLUP);  // INPUT para chave
  pinMode(botao, INPUT);        // INPUT para botão de comando
  pinMode(2,OUTPUT);            // OUTPUT para led indicador de status da mensagem recebida (se aceso a mensagem executada pelo robô está errada)
  digitalWrite(2,0);            
 
  // Configuro o WiFi para o modo Station
  WiFi.mode(WIFI_STA);

  // ==== Configuração para o ESP NOW =====

  // Inicio o ESP NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro na inicializacao do ESP-NOW");
    Serial.println("Reiniciando");
    ESP.restart();
  }
  
  // Informações do par a ser enviado os dados
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  // Adiciono o par especificado
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Erro em adicionar o par");
    return;
  }

  // Adiciono uma função para quando ocorrer o envio dos dados (serve para verificar se o par recebeu o pacote)
  // Não estou utilizando essa função
  esp_now_register_send_cb(verifica_envio);

  // Adiciono uma função para receber os dados enviados de retorno do par
  // A ideia aqui é que os dados enviadas retornem ao emissor (controle) para poder checar se o que ele, no caso o robô, recebeu está correto
  esp_now_register_recv_cb(func_recebimento);

  // ==========================================

  // Começo a marcar o tempo para futuramente enviar os dados a cada 1 segundo
  lastTime = millis();
  
}




// Função para verificar se ocorreu o envio do pacote
// Função correta para checagem de recebimento dos dados
void verifica_envio(const uint8_t *mac_addr, esp_now_send_status_t status){
  //Serial.println("\n------");
  //Serial.print("Verificando o envio:\t");
  //Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Envio com sucesso" : "Falha no envio");
  //Serial.println("--------");
}






// Função para receber o pacote de dados
void func_recebimento(const uint8_t * mac, const uint8_t *incomingData, int len){
  // Copia as variáveis da struct enviada para a struct criada nesse código
  memcpy(&msg_recebida, incomingData, sizeof(msg_recebida));
  //Serial.print("\n --------- \n Pacote recebido: \n");
  // Prints para verificar tal funcionamento
  //Serial.println(msg_recebida.velocidade);
  //Serial.println(msg_recebida.direcao);
  //Serial.println(msg_recebida.chave);
  //Serial.println(" ------- ");

  // Comparando valores enviados com os recebidos

  if((msg_recebida.velocidade == msg_enviada.velocidade) && (msg_recebida.direcao == msg_enviada.direcao) && (msg_recebida.chave == msg_enviada.chave)){
    //Serial.println("Dados executados pelo robo estao corretos!");
    //validade_dos_dados = true;
    digitalWrite(2,0);
    contador = 0;
  }else{
    //Serial.println("Dados executados pelo robo estao incorretos!");
    //validade_dos_dados = false;
    // Depois de ocorrer uma quantidade de vezes seguidas de erro na mensagem reinicio o ESP32
    if(contador == 5){
      Serial.println("\n\n\n\nREINICIANDO");
      ESP.restart();  
    }else{
      contador++;
    }
    digitalWrite(2,1);
    msg_enviada.chave = true; // Caso os dados estejam errados atualizo o dado da chave para 1 para que assim o robô fique parado 
    // e não execute o pacote de comandos errados
    esp_now_send(broadcastAddress, (uint8_t *) &msg_enviada, sizeof(msg_enviada));
  }


  /*
  // Caso tenha alguma invalidade nos dados os valores serão reinviados
  // Caso o LED do micro do controle acender teremos que a mensagem que executada no robô está
  if(!validade_dos_dados){
    msg_enviada.chave = true; // Caso os dados estejam errados atualizo o dado da chave para 1 para que assim o robô fique parado 
    // e não execute o pacote de comandos errados
    esp_now_send(broadcastAddress, (uint8_t *) &msg_enviada, sizeof(msg_enviada));
    digitalWrite(2,1);
  }else{
    digitalWrite(2,0);
  }
  */

  /*
  if(msg_recebida.velocidade == msg_enviada.velocidade){
    Serial.println("Velocidade OK");
  }else{
    Serial.println("Velocidade diferente");
  }

  if(msg_recebida.direcao == msg_enviada.direcao){
    Serial.println("Direcao OK");
  }else{
    Serial.println("Direcao diferente");
  }

  if(msg_recebida.chave == msg_enviada.chave){
    Serial.println("Chave OK");
  }else{
    Serial.println("Chave diferente");
  }
  */
  
}





 
void loop() {

  // Acionadores (botao, potenciômetro e chave)

  char dir_aux;
  int pote_vel;
  bool chave_de_controle;

  // Botão
  if(digitalRead(botao)){
    dir_aux = 'f';
  }else{
    dir_aux = 'p';
  }

  // Potenciometro
  pote_vel = analogRead(pote);

  // Chave
  if(digitalRead(ponto_morto)){
    chave_de_controle = true;
  }else{
    chave_de_controle = false;
  }

  // ---- Comandos simples para teste
  /*
  char dir_aux = 'p';
  int pote_vel = 12;
  bool chave_de_controle = true;
  */
  
  // Envio das variáveis a cada 10ms
  if(millis() - lastTime >= intervalo){

    // Atualizo as variáveis (isso poderia ser feito em outra função, por exemplo, por um controle ou sensor)
    // Apenas teste
    // A mensagem(pacote) enviado pode ser um número real, um inteiro, um conjunto de caracteres ou uma struct podendo conter os 3 tipos
    msg_enviada.velocidade = pote_vel;
    msg_enviada.direcao = dir_aux;
    msg_enviada.chave = chave_de_controle;

    // Envio da mensagem
    esp_err_t retorno = esp_now_send(broadcastAddress, (uint8_t *) &msg_enviada, sizeof(msg_enviada));

    /*
    Serial.print("Checando envio:   ");
    
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
    
  // Armazeno o último tempo depois de concluir o envio
    lastTime = millis(); 
  }

}
