// Código para client no controle do robô

#include <WiFi.h>
#include <HTTPClient.h>


const char* SSID = "Access_point_Server";
const char* PASS = "HC123Unicamp";

const char* server_Dir = "http://192.168.4.1/Direcao";

unsigned long lastTime = 0;
unsigned long timerDelay = 1000;

void setup(){

  Serial.begin(115200);

  WiFi.begin(SSID,PASS);

  lastTime = millis();

  while(WL_IDLE_STATUS != WL_CONNECTED){
      if (millis() - lastTime >= 500){
        Serial.println("Sem conexão!");
        lastTime = millis();
      }
  }

  Serial.println("Conectado");

}

String pegar_comando(const char* serverName){
    WiFiClient client;
    HTTPClient http;


    http.begin(client, serverName);

    int httpResponseCode = http.GET();

    String caixa = "_";

    if (httpResponseCode > 0){
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        caixa = http.getString();
    }else{
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
    }

    http.end();

    return caixa;
}


void loop(){

  if ((millis() - lastTime) > timerDelay) {

    if(WiFi.status()== WL_CONNECTED){

      String message = pegar_comando(server_Dir);
      Serial.print("Mensagem recebida: ");
      Serial.print(message + "\n");
    
    }
    else {
      Serial.println("Falha na comunicacao");
      WiFi.disconnect();
      WiFi.reconnect();
    }

    lastTime = millis();

  }


}
