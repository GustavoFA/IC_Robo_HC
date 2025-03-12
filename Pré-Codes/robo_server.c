// Código para criar server no ESP32 do robô

#include <WiFi.h>
#include "ESPAsyncWebServer.h" // https://github.com/me-no-dev/ESPAsyncWebServer

const char* server_Dir = "http://192.168.4.1/Direcao";

const char* SSID = "Access_point_Server";
const char* PASS = "HC123Unicamp";

unsigned int lastTime;

AsyncWebServer server(80);

void setup(){

  Serial.begin(115200);

  WiFi.softAP(SSID, PASS);

  lastTime = millis();

  while(WL_IDLE_STATUS != WL_CONNECTED){
      if (millis() - lastTime >= 500){
        Serial.println("Sem conexão!");
        lastTime = millis();
      }
  }

  Serial.println("Conectado");

  server.on("/Direcao", HTTP_GET,[](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", "OLA");
  });

  server.begin();

}

void loop(){



}
