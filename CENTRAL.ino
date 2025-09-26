/*  PROGRAMA PARA ESP32 CENTRAL 
    AUTOR: CAIO CAVALARI BARBOZA
    NA SAÍDA GPIO4 ESTÁ LIGADA A ALIMENTAÇÃO DO ORANGE PI ZER 3 POR 01h30m
*/

#include <WiFi.h>
#include <WebServer.h>

#define GPIO_SAIDA 4  // GPIO que será ativada por 60 minutos

const char* ssid = "SSID";
const char* password = "***";

WebServer server(80);

unsigned long tempoLigado = 0;
bool gpioAtivo = false;

void setup() {
  Serial.begin(115200);
  pinMode(GPIO_SAIDA, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(GPIO_SAIDA, LOW);

  WiFi.begin(ssid, password);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n Wi-Fi conectado");
  digitalWrite(2, HIGH);
  Serial.println("MAC: " + WiFi.macAddress());
  Serial.println("IP: " + WiFi.localIP());

  server.on("/bomba", HTTP_POST, handlePost);
  server.begin();
  Serial.println("Servidor HTTP iniciado");
}

void handlePost() {
  String evento = server.arg("plain");  // Lê o corpo como texto simples
  Serial.println(" Recebido: " + evento);

  if (evento == "BOMBA1_LIGADA" || evento == "BOMBA2_LIGADA") {
    digitalWrite(GPIO_SAIDA, HIGH);
    tempoLigado = millis();
    gpioAtivo = true;
    }

  server.send(200, "text/plain", "OK");
}

void loop() 
{
  server.handleClient();
  if (gpioAtivo && millis() - tempoLigado >= 90 * 60 * 1000) 
  {
    digitalWrite(GPIO_SAIDA, LOW);
    gpioAtivo = false;
    Serial.println("GPIO04 desligado após 90 minutos");
  }
}
