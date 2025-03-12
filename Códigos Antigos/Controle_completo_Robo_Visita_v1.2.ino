// Controle Robô - Versão 1.2

// Neste código temos apenas o necessário do funcionamento do controle
// sem contar a conexão WiFi 

/*  --> Funcionalidades
 * Comandos de Direção e Sentido (OK)
 * Potênciometro de Velocidade (OK)
 * Chave de trava de comandos (OK)
 * Led de sinalização de Chave (OK)
 */

#define VEL       A0          // velocidade
#define cima      8           // sentido 1
#define baixo     7           // sentido 2
#define esquerda  6           // sentido 3
#define direita   9           // sentido 4
#define chave     10          // Chave de trava dos comandos do controle(não interrompe WiFi)
#define led_chave 5           // LED para sinalizar Trava dos comandos
#define tempo_sinal_cont  800

uint8_t sentido = 0;  // parado
uint8_t vel = 1;      // Velocidade mínima = 1 / Velocidade máxima = 5


void setup() {

  Serial.begin(115200);
  
  pinMode(VEL, INPUT);        // Potenciômetro para setagem de velocidade

  // Push buttons para controle de movimento
  
  pinMode(cima, INPUT_PULLUP);
  pinMode(baixo, INPUT_PULLUP);
  pinMode(esquerda, INPUT_PULLUP);
  pinMode(direita, INPUT_PULLUP);
  
  pinMode(chave, INPUT_PULLUP); // Chave PANI: Bloqueia/Desbloqueia os botões de comandos do controle
  
  pinMode(led_chave, OUTPUT);   // LED indicativo para modo Bloqueio da Chave

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

void loop() {

  bool K = digitalRead(chave);  // Estado da chave de Bloqueio

  // Caso esteja em baixo(não setada), executo os comandos dos demais botões
  if(K){

    digitalWrite(led_chave, LOW);
    
    vel = velocidade(); // Início configurando a velocidade do Robô
    // Coloco com maior prioridade para ter uma resposta mais cedo(rápida)
  
    // Verifico cada função dos botões
    uint8_t B_cima = F_cima();
    uint8_t B_baixo = F_baixo();
    uint8_t B_esquerda = F_esquerda();
    uint8_t B_direita = F_direita();
  
    // Condicionais para cada botão, sendo que somente uma pode ser acionada por vez
    if(B_cima){
      Serial.println("Direcao y > Sentido +");
      Serial.print("Velocidade: ");
      Serial.println(vel);
      while(!digitalRead(cima)){  // Loop para manter o botão com reposta para o sistema
        sentido = 1;
      }
    }else if(B_baixo){
       Serial.println("Direcao y > Sentido -");
       Serial.print("Velocidade: ");
       Serial.println(vel);
       while(!digitalRead(baixo)){
        sentido = 2;
       }
    }else if(B_esquerda){
      Serial.println("Direcao x > Sentido -");
      Serial.print("Velocidade: ");
      Serial.println(vel);
      while(!digitalRead(esquerda)){
        sentido = 3;
      }
    }else if(B_direita){
      Serial.println("Direcao x > Sentido +");
      Serial.print("Velocidade: ");
      Serial.println(vel);
      while(!digitalRead(direita)){
        sentido = 4;
      }
    }else{
      sentido = 0;
    }
  } // Caso chave esteja setada não executo nenhuma leitura dos comandos dos botões e deixo o robô parado
  else{
    sentido = 0;
    digitalWrite(led_chave, HIGH);  // Ativo led de indicação de Estado da chave
  }

}
