// Gustavo Freitas Alves
// Código Motor C-19

// Bibliotecas
#include <ESP8266WiFi.h>
#include <espnow.h>

// Defines 
#define IN1 5
#define IN2 7
#define IN3 6
#define IN4 8
#define ENA 1
#define ENB 2

uint8_t MAC_adress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Endereço MAC do ESP utilizado no Controle

// Chave para conexão ESPNOW
uint8_t key[16] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

byte sentido = 0;             // Parado --> sentido = 0
unsigned int DutyCycle = 1;   // Velocidade mínima = 1(204) / Velocidade máxima = 5(1020)

// Struct para recebimento dos dados por WiFi
typedef struct struct_receive{
  uint8_t receive_Sentido;   // sentido
  uint8_t receive_Velocidade;// velocidade
} struct_receive;

struct_receive Mensagem;     // variável tipo struct

// Função para verificar se recebeu os dados do controle(DEBUG)
void Verifica_recebimento(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&Mensagem, incomingData, sizeof(Mensagem));
  Serial.print("Tamanho(bytes): ");
  Serial.println(len);
  Serial.print("Sentido: ");
  Serial.println(Mensagem.receive_Sentido);
  Serial.print("Velocidade: ");
  Serial.println(Mensagem.receive_Velocidade);
  Serial.println();
}

void setup() {

  Serial.begin(115200); // DEBUG

  WiFi.mode(WIFI_STA); // WiFi modo Station para ESP NOW

  if(esp_now_init() != 0){
    Serial.println("Erro na inicializacao do ESP NOW"); 
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);  // classificação do microcontrolador -> SLAVE (só recebe do MASTER[controle])

  esp_now_register_recv_cb(Verifica_recebimento); // Função para DEBUG do ESPNOW

  
  // Configuração dos GPIOS
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

}

void loop() {


  sentido = Mensagem.receive_Sentido;

  // CONTROLE DE SENTIDO

  switch(sentido){

    case 1: // Frente
      Serial.println("Frente");
      digitalWrite(IN1, HIGH);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN2, LOW);
      digitalWrite(IN4, LOW);
      break;

    case 2: // Trás
      Serial.println("Tras");
      digitalWrite(IN2, HIGH);
      digitalWrite(IN4, HIGH);
      digitalWrite(IN1, LOW);
      digitalWrite(IN3, LOW);
      break;

    case 3: // Anti-horário
      Serial.println("Anti-horario");
      digitalWrite(IN2, HIGH);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN1, LOW);
      digitalWrite(IN4, LOW);
      break;

    case 4: // Horário
      Serial.println("Horario");
      digitalWrite(IN1, HIGH);
      digitalWrite(IN4, HIGH);
      digitalWrite(IN3, LOW);
      digitalWrite(IN2, LOW);
      break;

    default:  // Parado
      Serial.println("Parado");
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, LOW);
      break;
  }

  // CONTROLE DE VELOCIDADE

  DutyCycle = Mensagem.receive_Sentido;

  analogWrite(ENA, DutyCycle*204);  // Aumento por pacotes de 204( 204 - 1020 )
  analogWrite(ENB, DutyCycle*204);
  
  
}
