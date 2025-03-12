// ROBÔ VISITA : )

// Controle + WiFi(ESP NOW) v1.2
// ESP8266 - E12
// ESP NOW -> https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html

// IMPORTANTE:
  // Precisa confirmar que tipo de retorno tem a função esp_now_add_peer()

/*
 * Comandos de direção e sentido, 4 Push button (OK)
 * Comando de velocidade, Potenciômetro (OK)
 * Interruptor PANI do controle, Chave gangorra (OK)
 * LED para sinalizar interruptor de comandos, LED vermelho (OK)
 * Conexão ESP NOW (falta testar)
 * Chave de acesso para paridade ESP NOW, Key (OK)
 * LED para sinalizar conexão entre os pares, (falta testar)
 */

#include <ESP8266WiFi.h>
#include <espnow.h>

#define VEL       A0          // velocidade
#define cima      8           // sentido 1
#define baixo     7           // sentido 2
#define esquerda  6           // sentido 3
#define direita   9           // sentido 4
#define chave     10          // Chave de trava dos comandos do controle(não interrompe WiFi)
#define led_chave 5           // LED para sinalizar Trava dos comandos
#define led_conec 4           // LED para sinalizar conexão ESPNOW OK
#define tempo_sinal_cont  800

uint8_t MAC_adress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Endereço MAC do ESP utilizado no Robô

// Chave para conexão ESPNOW
uint8_t key[16] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

byte sentido = 0;             // Parado --> sentido = 0
byte vel = 1;                 // Velocidade mínima = 1 / Velocidade máxima = 5

// Struct para envio dos dados por WiFi
typedef struct struct_send{
  uint8_t send_Sentido;   // sentido
  uint8_t send_Velocidade;// velocidade
} struct_send;

struct_send Mensagem;     // variável tipo struct

// Função para debug do envio de dados pelo ESP-NOW 
void Status_send(uint8_t *mac_addr, uint8_t sendStatus){
  Serial.print("Status do envio: ");
  if(!sendStatus){
    Serial.println("Envio com sucesso");
  }else{
    Serial.println("Envio com falha");
  }
}

// Função de velocidade --> Poteniômetro
uint16_t velocidade(){
   
  uint16_t atual = analogRead(VEL); // Leitura de Potenciômetro
  uint8_t registro;                 // Saída de valor de estado do Pot
  //static uint8_t ant_reg;           

  if(atual < 200){
    registro = 1;
  }else if(atual >= 200 && atual < 400){
    registro = 2;
  }else if(atual >= 400 && atual < 600){
    registro = 3;
  }else if(atual >= 600 && atual < 800){
    registro = 4;
  }else{
    registro = 5;
  }

  return registro;
  
}

// Funções dos Push Buttons:
//    > Todas as funções dos botões seguem um padrão

// Botão CIMA
uint8_t F_cima(){

   bool estadoBotao_cima;       // Estado do botão atual
   static uint32_t timer_c = 0; // Tempo para verificar se foi segurado
   bool estadoBotao_c;          // Estado do botão que será enviado para checagem final(loop)

   estadoBotao_cima = digitalRead(cima);  // Guardo valor do botão

   if(estadoBotao_cima){  // Faço essa condicional para verificar por quanto tempo o botão está pressionado
    timer_c = millis();
   }

   if((millis() - timer_c) >= tempo_sinal_cont){  // Caso fique o tempo mínimo de 800 ms
    estadoBotao_c = 1;
   }else{
    estadoBotao_c = 0;
   }

   return estadoBotao_c;  // Retorno a resposta do botão para os comandos
  
}

//Função BAIXO
uint8_t F_baixo(){

   bool estadoBotao_baixo;
   static uint32_t timer_b = 0;
   bool estadoBotao_b;

   estadoBotao_baixo = digitalRead(baixo);

   if(estadoBotao_baixo){
    timer_b = millis();
   }

   if((millis() - timer_b) >= tempo_sinal_cont){
    estadoBotao_b = 1;
   }else{
    estadoBotao_b = 0;
   }

   return estadoBotao_b;
  
}

// Função DIREITA
uint8_t F_direita(){

  bool estadoBotao_direita;
  static unsigned long timer_d = 0;
  bool estadoBotao_dir;
  
  estadoBotao_direita = digitalRead(direita);

  if(estadoBotao_direita){
    timer_d = millis();
  }

  if((millis() - timer_d) >= tempo_sinal_cont){
    estadoBotao_dir = 1;
  }else{
    estadoBotao_dir = 0;
  }
 
  return estadoBotao_dir;
  
}

// Função ESQUERDA
uint8_t F_esquerda(){

  bool estadoBotao_esquerda;
  static unsigned long timer_e = 0;
  bool estadoBotao_esq;


  estadoBotao_esquerda = digitalRead(esquerda);
  
  if(estadoBotao_esquerda){
    timer_e = millis();
  }

  if((millis() - timer_e) >= tempo_sinal_cont){
    estadoBotao_esq = 1;
  }else{
    estadoBotao_esq = 0;
  }
  
  return estadoBotao_esq;
  
}

void setup() {
  
  Serial.begin(115200); // Serial para debug

  pinMode(VEL, INPUT);        // Potenciômetro para setagem de velocidade

  // Push buttons para controle de movimento
  
  pinMode(cima, INPUT_PULLUP);
  pinMode(baixo, INPUT_PULLUP);
  pinMode(esquerda, INPUT_PULLUP);
  pinMode(direita, INPUT_PULLUP);
  
  pinMode(chave, INPUT_PULLUP); // Chave PANI: Bloqueia/Desbloqueia os botões de comandos do controle
  
  pinMode(led_chave, OUTPUT);   // LED indicativo para modo Bloqueio da Chave

  pinMode(led_conec, OUTPUT);   // LED incidicativo de conexão entre Slave e Master (ESPNOW)

  WiFi.mode(WIFI_STA); // Wifi Station mode --> Modo utilizado no ESP NOW

  //esp_now_init();
  // Iniciando o ESP NOW
  if(!esp_now_init()){
    Serial.println("Erro ao inciar o ESP-NOW");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER); // Classificação do nosso microcontrolador 
  /*  Tipos:
   *   ESP_NOW_ROLE_IDLE
   *   ESP_NOW_ROLE_CONTROLLER --> Master ( controle )
   *   ESP_NOW_ROLE_SLAVE --> Slave ( robô )
   *   ESP_NOW_ROLE_COMBO
   *   ESP_NOW_ROLE_MAX
   */

  esp_now_register_send_cb(Status_send); // callback para apresentar status de envio de mensagem ( DEBUG )

  if(esp_now_add_peer(MAC_adress, ESP_NOW_ROLE_SLAVE, 1, key, 16)){ // Função para fazer paridade
    digitalWrite(led_conec, HIGH);  // Precisa confirmar o tipo de resposta que a função anterior retorna
  }else{
    digitalWrite(led_conec, LOW);
  }
  /* 
    esp_now_add_peer(
      uint8_t mac_addr,
      uint8_t role,
      uint8_t channel,
      uint8_t key,
      uint8_t key_lenght
      );
   */

}

void enviar_ESP_NOW(uint8_t V, uint8_t S){

  // Atualizo variável Struct
  Mensagem.send_Sentido = S;
  Mensagem.send_Velocidade = V;

  esp_now_send(MAC_adress, (uint8_t *) &Mensagem, sizeof(Mensagem));  // envio os dados para o ROBÔ( SLAVE )
  
}

void loop() {

  bool K = digitalRead(chave);  // Estado da chave de Bloqueio

  // Caso esteja em baixo(não setada), executo os comandos dos demais botões
  if(K){

    digitalWrite(led_chave, LOW);
    
    vel = velocidade(); // Início configurando a velocidade do Robô
    // Coloco com maior prioridade para ter uma resposta mais cedo(rápida)
  
    uint8_t B_cima = F_cima();
    uint8_t B_baixo = F_baixo();
    uint8_t B_esquerda = F_esquerda();
    uint8_t B_direita = F_direita();
  
    if(B_cima){
      Serial.println("Direcao y > Sentido +");
      sentido = 1;
    }else if(B_baixo){
      Serial.println("Direcao y > Sentido -");
      sentido = 2;
    }else if(B_esquerda){
      Serial.println("Direcao x > Sentido -");
      sentido = 3;
    }else if(B_direita){
      Serial.println("Direcao x > Sentido +");
      sentido = 4;
    }else{
      sentido = 0;
    }
    
  } // Caso chave esteja setada não executo nenhuma leitura dos comandos dos botões e deixo o robô parado
  else{
    sentido = 0;
    digitalWrite(led_chave, HIGH);  // Ativo led de indicação de Estado da chave
  }

  enviar_ESP_NOW(vel, sentido); // Comunicação com o ESP ROBÔ (SLAVE)

}
