/*
Programa para microcontrolador ESP32
Autor: Caio Cavalari Barboza
Este programa checa o estado de duas entradas (GPIO2 e GPIO4) do microcontrolador ESP 32 e mostra estes estados em uma página web armazenada
dentro de um servidor web hospedado no próprio ESP32.
Além de mostrar estas mensagens na página web, há o armazenamento das datas e horas (histórico), em 10x posições de memória não volátil (NVS),
essas 10x posições são sobreescritas e armazenam somente o dado de bomba ligada

TODO COMANDO "Serial.println("MENSAGEM")" SERVE APENAS PARA MONITORAMENTO DAS VARIÁVEIS E FUNÇÕES, IMPRIMINDO A SAÍDA NO CONSOLE "SERIAL MONITOR"
DO ARDUÍNO IDE.

*/

#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <time.h>
#include <Preferences.h>

// Informação para a configuração da conexão WiFi
const char* ssid = "POTTER";
const char* password = "*****";
const int maxRegistro = 10;
WebServer server(80); // Configura a porta 80 do servidor web
const int LED_VERDE = 2;  // LED VERDE DO PAINEL = BOMBA LIGADA, ENTRADA GERAL GPIO2 DO ESP32
const int LED_VERM = 4; // LED VERMELHO DO PAINEL = SOBRECORRENTE DA BOMBA, ENTRADA GERAL GPIO4  DO ESP32
bool BOMBA_STATUS = false; // STATUS DA BOMBA
bool BOMBA_OC = false;     //STATUS DO RELÉ TÉRMICO
bool estadoVerdeanterior = 0;

Preferences prefs;  //criação de um namespace (porção de memória) chamado prefs na memória NVS

// Aqui é o conteúdo HTML para a página web
String HTMLPage()
{
  String html = "<!DOCTYPE html><html>";
  html += "<head><meta charset='UTF-8'><title>Monitoramento da Bomba Aurélio Marengo</title></head><ul>";
  html += "<body style='text-align: left; font-family: Arial;'>";
  html += "<h1>Status Bomba 1</h1>";
  html += BOMBA_STATUS ? "<p>BOMBA ESTÁ <strong>LIGADA</strong></p>" : "<p>BOMBA ESTÁ <strong>DESLIGADA</strong></p>";
  html += BOMBA_OC ? "<p>BOMBA ESTÁ COM <strong>SOBRECORRENTE</strong></p>": "<p>BOMBA ESTÁ <strong>OK</strong></p>";
  html += "<h1>Histórico:</h1>";
    prefs.begin("Registros", true);
  for (int i = 0; i < maxRegistro; i++)
  {
      String chave = "registro" + String(i);
      String valor = prefs.getString(chave.c_str(), "");
      if (valor !="")
      {
      html += "<li>" + valor + "</li>";
      }
  }
  prefs.end();
  html += "</html></body></ul>";
  return html;
}

// Função para a página raiz (root) do servidor web
void handleRoot()
{
  server.send(200, "text/html", HTMLPage());
}

// Função para salvar em 10x posições da NVS.
// Será gravado a data no formato DiadaSemana, Mês Dia Ano Hora: Minutos. Ex.: Wednesday, June 11 2025 22:43
void salvarRegistro(String novoValor)
{
  prefs.begin("Registros", false);
  for (int i = maxRegistro -1; i > 0; i--)
    {
      String chaveAtual = "registro"+String(i);
      String chaveAnterior = "registro"+String(i-1);
      String valorAnterior = prefs.getString(chaveAnterior.c_str(), "");
      prefs.putString(chaveAtual.c_str(), valorAnterior.c_str());
      //Serial.println(chaveAtual.c_str());     
    }
  prefs.putString("registro0", novoValor);
  prefs.end();
}

//Função que busca data/hora do servidor NTP
String printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo))
  {
    Serial.println("Falha ao conectar servidor do tempo");
    //return();
  }
    char buffer[30];
    strftime(buffer, sizeof(buffer), "%A, %B %d %Y %H:%M:%S", &timeinfo);
    Serial.println("entrou na funçao pega tempo");
    Serial.println(buffer);
    return String(buffer);  
}

// parte do programa de configuração das diversas funções
void setup() {
    
  //Serial.println(String(buffer));
  Serial.begin(115200);
  configTime(-10800, 0, "pool.ntp.org");

  //DEFINIR AS PORTAS DO ESP32
  pinMode(LED_VERDE, INPUT);
  pinMode(LED_VERM, INPUT);
  
  // Conectar ao Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) 
    {
    delay(500);
    Serial.print("*");
    }
  Serial.println("\nConectado a rede Wi-Fi");
  Serial.println(WiFi.localIP());

  // Define as rotas do servidor web
  server.on("/", handleRoot);
 
  // Inicia o servidor web
  server.begin();
  Serial.println("Server started");
} 

// parte principal onde o programa fica em loop
void loop() {

    int estadoVermelho = digitalRead(LED_VERM);
    int estadoVerde = digitalRead(LED_VERDE);
  
    if (estadoVermelho == HIGH) 
    {
        BOMBA_OC = true;
        BOMBA_STATUS = false;
       // Serial.println("botao RED  HIGH");
    } 
    else 
    {
        BOMBA_OC = false;
        BOMBA_STATUS = true;
    }
    //criado este if somente para ser executado 1x, de forma a gravar somente 1 data/hora quando LED_VERDE = alto
    if (estadoVerde == HIGH && estadoVerdeanterior == LOW)
    {
       salvarRegistro(printLocalTime());
       Serial.println("botao GREEN && HIGH");
       estadoVerdeanterior = estadoVerde;
    }
    else 
     {
      BOMBA_STATUS = false;
     }
            
    if (estadoVerde == HIGH)
     {
       BOMBA_STATUS = true;
       BOMBA_OC = false;
       Serial.println("botao GREEN SÒ HIGH");
     }

     else 
     {
      estadoVerdeanterior = LOW;
      BOMBA_STATUS = false;  
     }

    server.handleClient(); // Lida com requisições do servidor, se aplicável
    delay(100); // Pequeno atraso para estabilidade
}

