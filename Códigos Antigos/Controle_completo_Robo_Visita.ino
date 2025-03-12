// Gustavo Freitas Alves
// Robo Visita Controle v1

#define cima      8           // sentido 1
#define baixo     7           // sentido 2
#define esquerda  6           // sentido 3
#define direita   9           // sentido 4
#define debounce  50
#define tempo_sinal_cont  800

byte sentido = 0;             // parado
byte vel = 1;                 // Velocidade mínima = 1 / Velocidade máxima = 5
  
void setup() {
  
  Serial.begin(9600);
  pinMode(cima, INPUT_PULLUP);
  pinMode(baixo, INPUT_PULLUP);
  pinMode(esquerda, INPUT_PULLUP);
  pinMode(direita, INPUT_PULLUP);

}

byte F_cima(){

   static bool estadoBotao_Ant_cima;  // Estado anterior do botão
   static unsigned long delayBotao_cima = 0;  // Tempo para usar no Debounce
   static unsigned long botaoApertado_cima;   // Tempo de apertado do botão
   static byte fase_cima = 0;   // Fase do botão --> 1 = Mudança de estado registrado / 0 = Nada ocorreu

   bool estadoBotao_cima;       // Estado atual do botão
   byte estadoRet_cima;         // Estado de retorno 

   estadoRet_cima = 0; 
    
   if ( (millis() - delayBotao_cima) > debounce ) { 
    
       estadoBotao_cima = digitalRead(cima);  
        // Condicional para Mudança de estado( Apertado ) do botão
       if ( !estadoBotao_cima && (estadoBotao_cima != estadoBotao_Ant_cima) ) {
          // Se verificarmos que o botão teve alguma mudança atualizamos os tempos para:
          delayBotao_cima = millis();          // -> Tempo para Debounce
          botaoApertado_cima = millis();        // -> Tempo de botão pressionado
          fase_cima = 1;        // Registrar que algo ocorreu no botão
          
       } 
        // Condicional para Segurado
       if ( (fase_cima) && ((millis() - botaoApertado_cima) >= tempo_sinal_cont) ) {
          
          fase_cima = 0;
          estadoRet_cima = 2; // Valor para Status de botão pressinado por um tempo
          
       }
        // Condicional para Botão pressionado
       if ( estadoBotao_cima && (estadoBotao_cima != estadoBotao_Ant_cima) ) {
        
          delayBotao_cima = millis();

          if ( fase_cima ) {
             estadoRet_cima = 1;
          } 
          fase_cima = 0;
       } 
       
       estadoBotao_Ant_cima = estadoBotao_cima;
   }

   return estadoRet_cima;
  
}

byte F_baixo(){

   static bool estadoBotao_Ant_baixo; 
   static unsigned long delayBotao_baixo = 0;
   static unsigned long botaoApertado_baixo;
   static byte fase_baixo = 0;

   bool estadoBotao_baixo;
   byte estadoRet_baixo;

   estadoRet_baixo = 0;  
   if ( (millis() - delayBotao_baixo) > debounce ) {
       estadoBotao_baixo = digitalRead(baixo);
       if ( !estadoBotao_baixo && (estadoBotao_baixo != estadoBotao_Ant_baixo) ) {
          delayBotao_baixo = millis();          
          botaoApertado_baixo = millis();
          fase_baixo = 1;
       } 

       if ( (fase_baixo == 1) && ((millis() - botaoApertado_baixo) >= tempo_sinal_cont) ) {
          fase_baixo = 0;
          estadoRet_baixo = 2;
       }
       
       if ( estadoBotao_baixo && (estadoBotao_baixo != estadoBotao_Ant_baixo) ) {
          delayBotao_baixo = millis();

          if ( fase_baixo == 1 ) {
             estadoRet_baixo = 1;
          } 
          fase_baixo = 0;
       } 
       
       estadoBotao_Ant_baixo = estadoBotao_baixo;
   }

   return estadoRet_baixo;
  
}

byte F_esquerda(){

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

byte F_direita(){

  bool estadoBotao_direita = digitalRead(direita);
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

void loop() {

  byte B_cima = F_cima();
  byte B_baixo = F_baixo();
  byte B_esquerda = F_esquerda();
  byte B_direita = F_direita();

  if(B_cima){
    if(B_cima == 1){
      if(vel <5){
        vel++;
        Serial.println("Velocidade aumentada");
        Serial.println("Vel = " + String(vel));
      }else{
        vel = 5;
        Serial.println("Velocidade no limite maximo");
      }
    }else if(B_cima == 2){
      Serial.println("Direcao y > Sentido +");
      while(!digitalRead(cima)){
        sentido = 1;
      }
    }
  }else if(B_baixo){
    if(B_baixo == 1){
      if(vel > 1){
        vel--;
        Serial.println("Velocidade diminuida");
        Serial.println("Vel = " + String(vel));
      }else{
        vel = 1;
        Serial.println("Velocidade no limite minimo");
      }
    }else if(B_baixo == 2){
      Serial.println("Direcao y > Sentido -");
      while(!digitalRead(baixo)){
        sentido = 2;
      }
    }
  }else if(B_esquerda){
    Serial.println("Direcao x > Sentido -");
    while(!digitalRead(esquerda)){
      sentido = 3;
    }
  }else if(B_direita){
    Serial.println("Direcao x > Sentido +");
    while(!digitalRead(direita)){
      sentido = 4;
    }
  }else{
    sentido = 0;
  }
  
}
