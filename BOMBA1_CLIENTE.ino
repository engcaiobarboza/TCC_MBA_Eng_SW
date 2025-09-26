/*  PROGRAMA PARA ESP32 BOMBA1 
    AUTOR: CAIO CAVALARI BARBOZA
    MANTIDO VÁRIOS CÓDIGOS "SERIAL.PRINTLN" PARA PRINT DEBUGGING
*/

#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <Preferences.h>

// Wi-Fi
const char* ssid = "SSID";
const char* senha = "***";

// HTTP
const char* urlServidor = "http://192.168.15.43/bomba";

// MQTT
const char* broker = "192.168.15.17";
const int portaMQTT = 1883;

// GPIOs
const int pinoBOMBA1_LIG = 33;
const int pinoBOMBA1_SOB = 32;

// Controle de reconexão MQTT
unsigned long ultimaTentativaMQTT = 0;
const unsigned long intervaloTentativaMQTT = 5000;

WiFiUDP udp;
NTPClient relogio(udp, "pool.ntp.org", -10800); // UTC-3

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

Preferences SalvarHora;

void setup() 
{
  Serial.begin(115200);
  pinMode(pinoBOMBA1_LIG, INPUT_PULLDOWN);
  pinMode(pinoBOMBA1_SOB, INPUT_PULLDOWN);
  pinMode(2, OUTPUT);

  WiFi.begin(ssid, senha);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n Wi-Fi conectado!");
  digitalWrite(2, HIGH);
  Serial.println("IP: " + WiFi.localIP().toString());

  relogio.begin();
  relogio.update();

  mqtt.setServer(broker, portaMQTT);
  
}

void loop() 
{
  Serial.println("Entrou 1");
  int estado = digitalRead(pinoBOMBA1_LIG);
  Serial.println(estado == HIGH ? " Bomba ligada" : " Bomba desligada");
  delay(500);

    Serial.println("Entrou 2");

  // Detecta bomba ligada
  if (digitalRead(pinoBOMBA1_LIG) == HIGH)
  {
      Serial.println("Entrou3");

    relogio.update();
    String horaLigada = relogio.getFormattedTime();
    SalvarHora.begin("HorarioBOMBA1", false);  // false = leitura/escrita
      Serial.println("Entrou 4");

    SalvarHora.putString("LIGADA", horaLigada);
    SalvarHora.end();
    Serial.println(" Bomba ligada às " + horaLigada);

    enviaHTTP("BOMBA1_LIGADA");
    unsigned long espera = millis();
      Serial.println("Entrou 5");

    while (millis() - espera < 120000)
      {
        Serial.println("Entrou 6");

      mqtt.loop();
      //server.handleClient();
      delay(10);
      }
    if (!mqtt.connected())
      {
        Serial.println("Entrou 7");

      reconectarMQTT();
      }
    String topicoLIG = "Bomba1/horaLigada";
      Serial.println("Entrou 8");

    mqtt.publish(topicoLIG.c_str(),horaLigada.c_str());
    while (digitalRead(pinoBOMBA1_LIG) == HIGH)
      {
        Serial.println("Entrou 9");

      delay(100);
      mqtt.loop();
      //server.handleClient();
      }
    if (digitalRead(pinoBOMBA1_LIG) == LOW)
      {
          Serial.println("Entrou 10");

      relogio.update();
      String horaDesligada = relogio.getFormattedTime();
      SalvarHora.begin("HorarioBOMBA1", false);  // false = leitura/escrita
      SalvarHora.putString("DESLIGADA", horaDesligada);
      SalvarHora.end();
      Serial.println(" Bomba desligada às " + horaDesligada);
      if (!mqtt.connected())
      {
      reconectarMQTT();
      }
        Serial.println("Entrou 11");

      String topicoDESL = "Bomba1/horaDesligada";
      mqtt.publish(topicoDESL.c_str(),horaDesligada.c_str());  
    }
  }
 
  // Detecta bomba sobrecarga
  if (digitalRead(pinoBOMBA1_SOB) == HIGH)
  {
      Serial.println("Entrou 12");

    relogio.update();
    String horaSob = relogio.getFormattedTime();
    SalvarHora.begin("HorarioBOMBA1", false);  // false = leitura/escrita
    SalvarHora.putString("SOBRECARGA", horaSob);
    SalvarHora.end();
    Serial.println(" Bomba com sobecarga às " + horaSob);
    if (!mqtt.connected())
    {
    reconectarMQTT();
    }
    String topicoSOB = "Bomba1/horaSobrecarga";
    mqtt.publish(topicoSOB.c_str(),horaSob.c_str());
    while (digitalRead(pinoBOMBA1_SOB) == HIGH)
      {
      delay(100);
      mqtt.loop();
      
      }
   }
  delay(200);
} 

void enviaHTTP(String hora) 
{
   Serial.println("Entrou 13");

 bool enviado = false;
 while (!enviado)
  {
  if (WiFi.status() == WL_CONNECTED) 
    {
      Serial.println("Entrou 14");

    HTTPClient http;
    http.begin(urlServidor);
    http.addHeader("Content-Type", "text/plain");
    int resposta = http.POST(hora);
    Serial.println(resposta);
    if (resposta==200)
    {
    Serial.println("POST enviado com sucesso");
    enviado = true;
    }
    else{
    Serial.println("Falha no POST, tentando again");
    delay(1000);
    }
    http.end();
    }
  else
  {
    Serial.println ("wI-FI DESCONECTADO");
    WiFi.begin(ssid,senha);
    delay (2000);
  }
}
}

void reconectarMQTT() 
{
    Serial.println("Entrou 15");

  if (millis() - ultimaTentativaMQTT > intervaloTentativaMQTT) 
  {
    ultimaTentativaMQTT = millis();
    Serial.println(" Tentando reconectar ao MQTT...");

    if (mqtt.connect("ESP32_BOMBA1"))
    {
      Serial.println(" Conectado ao MQTT!");
      //mqtt.subscribe(topicoStatus);
    } 
    else 
    {
      Serial.print(" Falha MQTT, código: ");
      Serial.println(mqtt.state());
    }
  }
}

