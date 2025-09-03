/*  PROGRAMA PARA COMUNICAÇÃO VIA MQQT DA BOMBA 1
    AUTOR: CAIO CAVALARI BARBOZA
    SINALIZADOR LED SERÁ INTERFACEADO VIA ZMPT101B E SERÁ LIGADO NAS ENTRADAS GPIO:
    ENTRADA 2 = SINALIZADOR BOMBA LIGADO
    ENTRADA 4 = SINALIZADOR BOMBA SOBRECARGA
*/

#include <WiFi.h>
#include <PubSubClient.h>

// Wi-Fi
//const char* ssid = "SEU_SSID";
//const char* password = "SUA_SENHA";
const char* ssid = "POTTER";
const char* password = "*****";

// MQTT
const char* mqtt_server = "192.168.15.17";   //ENDEREÇO DO BROKER MQQT AEDES INTERNO DO NODE-RED
const int mqtt_port = 1883;                  //PORTA DO BROKER MQQT AEDES
const char* mqtt_topic_bomba1 = "esp32/bomba1_ligada";
const char* mqtt_topic_sobrecarga1 = "esp32/bomba1_sobrecarga";

WiFiClient esp32Client;                     //CRIA UM OBJETO DE CLIENTE WIFI CHAMADO ESP32CLIENT
PubSubClient MQTTclient(esp32Client);       //CRIA UM CLIENTE MQTT CHAMADO MQTTclient

// GPIO
const int bomba1Pin = 2;                     //ENTRADA BOMBA LIGADA = GPIO 2
const int sobrecargaPin = 4;                 //ENTRADA SOBRECARGA DA BOMBA = GPIO 4

void setup_wifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {    //FICA TENTANDO CONECTAR AO WIFI
    delay(500);
  }
}

void reconnect() {
  while (!MQTTclient.connected()) {
    if (MQTTclient.connect("BOMBA1_CLIENTE")) {
      // Conectado
    } else {
      delay(1000);
    }
  }
}

void setup() {
  pinMode(bomba1Pin, INPUT);                        //DEFINE A ENTRADA GPIO 2 COMO ENTRADA
  pinMode(sobrecargaPin, INPUT);                   //DEFINE A ENTRADA GPIO 4 COMO ENTRADA

  setup_wifi();
  MQTTclient.setServer(mqtt_server, mqtt_port);    //DEFINE O IP E A PORTA DO BROKER MQTT
}

void loop() {
  if (!MQTTclient.connected()) {                    //FICA TENTANDO A RECONEÃO CASO NÃO ESTEJA CONECTADO
    reconnect();
  }
  MQTTclient.loop();                                 //FICA SEMPRE OUVINDO OS TÓPICOS

  int bomba1Status = digitalRead(bomba1Pin);
  int sobrecargaStatus = digitalRead(sobrecargaPin);

  MQTTclient.publish(mqtt_topic_bomba1, bomba1Status == HIGH ? "ON" : "OFF");    
  //SE O STATUS DA BOMBA1 FOR "1" PUBLICA NO TÓPICO "esp32/bomba1_ligada" -->"ON"
  //SE O STATUS DA BOMBA1 FOR "0" PUBLICA NO TÓPICO "esp32/bomba1_ligada" -->"OFF"
  MQTTclient.publish(mqtt_topic_sobrecarga1, sobrecargaStatus == HIGH ? "SOBRECARGA" : "NORMAL");
  //SE BOMBA1 ESTIVER EM SOBRECARGA PUBLICA NO TÓPICO "esp32/bomba1_sobrecarga" -->"SOBRECARGA"
  //SE BOMBA1 NÃO ESTIVER EM SOBRECARGA PUBLICA NO TÓPICO "esp32/bomba1_sobrecarga" -->"NORMAL"
  delay(1000); // REPETE A CADA SEGUNDO A LEITURA DAS ENTRADAS GPIO 2 E 4
}