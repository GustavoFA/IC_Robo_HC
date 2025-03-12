// Código para criar server no ESP32 do robô

/*
  Lógica da comunicação entre o robô e o controle: O usuário do controle pressiona os comandos que deseja que o robô faça e 
  o robô, a cada determinado tempo, faz uma requisição dos comandos do controle, por exemplo, movimento, velocidade e chave PAN
  (ponto morto) e o controle os envia. Esse determinado tempo deve ser curto (na faixa dos 10 milissegundos).
*/ 

// ------------- ATENÇÃO -------------------------
/*
Devemos ter uma flag para identificar se a comunicação foi perdida e uma lógica para recuperar tal comunnicação
*/

// Referência: https://randomnerdtutorials.com/esp32-client-server-wi-fi/


// Deixaremos este ESP32 com o WiFi em modo AP (access point)
// Endereço de IP --> 192.168.4.1

// O que é esperado do controle:
/* O controle possui 4 push buttons para comandos da direção, sendo que terá apenas uma variável para direção do 
movimento. Além disso, possui um potenciômetro para regular a velocidade do movimento, sendo que haverá uma variável
para essa informação. E por fim, temos uma chave para selecionar o modo "ponto morto" do robô, sendo que haverá uma variável
para este comando também. 

Logo, temos 3 variáveis enviadas pelo controle, sendo elas: direção do movimento (dir_mov_contr), velocidade do movimento
(vel_mov_contr) e ponto morto (pto_mor_contr).

Estas 3 variáveis serão sempre atualizadas no controle e a um determinado tempo serão requisitadas pelo robô
*/

// Utilizaremos um temporizador para gerar uma interrupção a cada 40 ms, e assim, fazermos a requisição das variáveis

#include <WiFi.h>
#include <driver/gpio.h>
#include <driver/timer.h>
#include "HTTPClient.h" // https://github.com/me-no-dev/ESPAsyncWebServer

char *tag;
String dir_mov = "p";
String vel_mov = "1";

const char* SSID = "Access_point_Server";
const char* PASS = "HC123Unicamp";

const char* dir_mov_contr = "http://192.168.4.1/Direcao";
const char* vel_mov_contr = "http://192.168.4.1/Velocidade";
const char* pon_mor_contr = "http://192.168.4.1/Ponto";

String http_recebe_comunicacao(const char* server_name){

  WiFiClient client;
  HTTPClient http;

  http.begin(client, server_name);

  int httpResponseCode = http.GET();

  String caixa = "_"; // Caracter padrão, que caso apareça em uma das variáveis de interesse
  // representa que a comunicação não foi bem sucedida

  if(httpResponseCode>0){
    caixa = http.getString();
  }

  http.end();

  return caixa;

}

void IRAM_ATTR ISR_deco_mensagem(void *args) {

  dir_mov = http_recebe_comunicacao(dir_mov_contr);
  vel_mov = http_recebe_comunicacao(vel_mov_contr);


  // Finaliza a interrupção para o Temporizador do grupo 1 e timer 0
  TIMERG1.int_clr_timers.t0 = 1;  
  TIMERG1.hw_timer[0].update=1;
  TIMERG1.hw_timer[0].config.alarm_en = 1;

}



void setup(){

  // ----------------- Temporizador -----------------
  
  // Configuração semelhante a usada na entrada do CS. Pode ser reutilizada dela
  // usando apenas uma configuração de temporizador
  timer_config_t timer_comand = {};
  timer_comand.divider = 80;
  timer_comand.counter_dir = TIMER_COUNT_UP;
  timer_comand.counter_en = TIMER_PAUSE;
  timer_comand.auto_reload = TIMER_AUTORELOAD_EN;

  // Configuro o timer 1 do grupo 0 com as configurações feitas anteriormente  
  timer_init(TIMER_GROUP_1, TIMER_0, &timer_comand); 
  // Configuro o contador do meu temporizador para iniciar com o valor 0
  timer_set_counter_value(TIMER_GROUP_1, TIMER_0, 0);
  // Configuro o meu temporizador para gerar um alarme quando o contador bater 40000
  // Para gerar um tempo de 40 ms
  timer_set_alarm_value(TIMER_GROUP_1, TIMER_0, 40000); 
  // Habilito uma interrupção para esse alarme
  timer_enable_intr(TIMER_GROUP_1, TIMER_0);
  // Registro uma RSI para essa interrupção
  timer_isr_register(TIMER_GROUP_1, TIMER_0, &ISR_deco_mensagem, &tag, 3, NULL);

  // --------------------------------------------------

  WiFi.begin(SSID, PASS);

  // Inicio meu temporizador
  timer_start(TIMER_GROUP_1, TIMER_0); 

}

void loop(){



}
