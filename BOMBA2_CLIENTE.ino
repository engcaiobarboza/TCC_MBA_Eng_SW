/*  PROGRAMA PARA ESP32 BOMBA2 
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

// Controle de reconexão MQTT
unsigned long ultimaTentativaMQTT = 0;
const unsigned long intervaloTentativaMQTT = 5000;

// Sensor ACS712
const int pinoSensorCorrente = 33;
const float offset = 2.54;
const float sensibilidade = 0.185; // V/A para ACS712 5A
const float limiarCorrente = 1.0;
const int tempoLigada = 3000;       // tempo mínimo de corrente alta
const int tempoEsperaEnvio = 120000; // espera de 2 minutos antes de enviar

WiFiUDP udp;
NTPClient relogio(udp, "pool.ntp.org", -10800); // UTC-3

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

Preferences SalvarHora;

void setup() 
{
  Serial.begin(115200);
  pinMode(pinoSensorCorrente, INPUT);
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

float lerCorrenteRMS() {
  const int numAmostras = 1000;
  float somaQuadrados = 0;

  for (int i = 0; i < numAmostras; i++) 
  {
    int leituraADC = analogRead(pinoSensorCorrente);
    float tensao = (leituraADC / 4095.0) * 3.3;
    float delta = tensao - offset;
    somaQuadrados += delta * delta;
  }

  float tensaoRMS = sqrt(somaQuadrados / numAmostras);
  return tensaoRMS / sensibilidade;
}

void loop() 
{
  float correnteRMS = lerCorrenteRMS();
  Serial.print("Corrente RMS: ");
  Serial.print(correnteRMS, 3);
  Serial.println(" A");

  //bool correnteAlta = correnteRMS >= limiarCorrente;

  if (correnteRMS >= limiarCorrente)
     {
      relogio.update();
      String horaLigada = relogio.getFormattedTime();
      SalvarHora.begin("HorarioBOMBA2", false);  // false = leitura/escrita
      Serial.println("Entrou 4");
      SalvarHora.putString("LIGADA", horaLigada);
      SalvarHora.end();
      Serial.println(" Bomba2 ligada às " + horaLigada);
      enviaHTTP("BOMBA2_LIGADA");
      unsigned long espera = millis();
      Serial.println("Entrou 5");
      while (millis() - espera < 120000) //voltar para 120000
      {
        Serial.println("Entrou 6");
        mqtt.loop();
        delay(10);
      }
    if (!mqtt.connected())
      {
        Serial.println("Entrou 7");

      reconectarMQTT();
      }
    String topicoLIG = "Bomba2/horaLigada";
      Serial.println("Entrou 8");

    mqtt.publish(topicoLIG.c_str(),horaLigada.c_str());
    while (correnteRMS >= limiarCorrente)
      {
        Serial.println("Entrou 9");
        correnteRMS = lerCorrenteRMS();
        Serial.print("Corrente RMS dentro do loop: ");
        Serial.println(correnteRMS, 3);
        delay(100);
        mqtt.loop();
      
       }
    if (correnteRMS <= limiarCorrente)
      {
          Serial.println("Entrou 10");

      relogio.update();
      String horaDesligada = relogio.getFormattedTime();
      SalvarHora.begin("HorarioBOMBA2", false);  // false = leitura/escrita
      SalvarHora.putString("DESLIGADA", horaDesligada);
      SalvarHora.end();
      Serial.println(" Bomba2 desligada às " + horaDesligada);
      if (!mqtt.connected())
      {
      reconectarMQTT();
      }
        Serial.println("Entrou 11");

      String topicoDESL = "Bomba2/horaDesligada";
      mqtt.publish(topicoDESL.c_str(),horaDesligada.c_str());  
    }
  }
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

    if (mqtt.connect("ESP32_BOMBA2"))
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
