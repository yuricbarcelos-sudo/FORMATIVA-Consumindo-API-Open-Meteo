#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>


// Insira as credenciais da sua rede Wi-Fi
const char* ssid = "iPhone";
const char* password = "12345678";


// Definição dos pinos
const int TRIG_PIN = 5;
const int ECHO_PIN = 18;
const int RELAY_PIN = 23;


// Limiar de distância para acionamento do relé (em centímetros)
const float DISTANCE_THRESHOLD = 20.0;


// Definição da lógica de acionamento do relé (ajuste se seu relé for ativado em HIGH)
#define RELAY_ON LOW
#define RELAY_OFF HIGH


// Velocidade do som em cm/us
#define SOUND_SPEED 0.034


WebServer server(80);


// Função para medir a distância usando o sensor HC-SR04
float readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);


  // Lê o tempo de duração da resposta do pino ECHO (timeout de 30ms)
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) {
    return -1.0; // Retorna -1 em caso de falha/fora de alcance
  }
  return (duration * SOUND_SPEED) / 2.0;
}


// Rota raiz ("/"): entrega a página HTML com JavaScript de auto-atualização
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Painel de Controle - ESP32</title>
  <style>
    body { font-family: Arial, sans-serif; background-color: #f0f2f5; margin: 0; padding: 40px 20px; text-align: center; }
    .container { max-width: 450px; margin: 0 auto; background: white; padding: 30px; border-radius: 12px; box-shadow: 0 4px 15px rgba(0,0,0,0.1); }
    h1 { color: #1a73e8; margin-bottom: 25px; font-size: 1.8rem; }
    .card { background: #f8f9fa; border: 1px solid #e9ecef; border-radius: 8px; padding: 15px; margin-bottom: 15px; text-align: left; }
    .card-title { font-size: 0.9rem; color: #6c757d; text-transform: uppercase; letter-spacing: 0.5px; }
    .card-value { font-size: 1.6rem; font-weight: bold; color: #212529; margin-top: 5px; }
    .status-on { color: #2e7d32; font-weight: bold; }
    .status-off { color: #c62828; font-weight: bold; }
  </style>
</head>
<body>
  <div class="container">
    <h1>ESP32 Web Server</h1>
    <div class="card">
      <div class="card-title">Distância Atual</div>
      <div class="card-value"><span id="distancia">--</span> cm</div>
    </div>
    <div class="card">
      <div class="card-title">Estado da Lâmpada</div>
      <div class="card-value"><span id="estado">--</span></div>
    </div>
  </div>


  <script>
    function updateData() {
      fetch('/data')
        .then(response => response.json())
        .then(data => {
          if (data.distancia < 0) {
            document.getElementById('distancia').innerText = "Fora de alcance";
          } else {
            document.getElementById('distancia').innerText = data.distancia.toFixed(1);
          }


          const estadoElement = document.getElementById('estado');
          estadoElement.innerText = data.estado;
          if (data.estado === "Ligada") {
            estadoElement.className = "status-on";
          } else {
            estadoElement.className = "status-off";
          }
        })
        .catch(err => console.error("Erro ao buscar dados:", err));
    }


    // Atualiza automaticamente os dados a cada 1000ms (1 segundo)
    setInterval(updateData, 1000);
    updateData();
  </script>
</body>
</html>
)rawliteral";


  server.send(200, "text/html", html);
}


// Rota de API ("/data"): retorna a leitura atualizada e o estado do relé em formato JSON
void handleData() {
  float distance = readDistanceCM();
  bool isRelayOn = false;


  // Lógica de controle do relé baseada no limite de distância
  if (distance > 0 && distance <= DISTANCE_THRESHOLD) {
    digitalWrite(RELAY_PIN, RELAY_ON);
    isRelayOn = true;
  } else {
    digitalWrite(RELAY_PIN, RELAY_OFF);
    isRelayOn = false;
  }


  String json = "{";
  json += "\"distancia\":" + String(distance, 1) + ",";
  json += "\"estado\":\"" + String(isRelayOn ? "Ligada" : "Desligada") + "\"";
  json += "}";


  server.send(200, "application/json", json);
}


void setup() {
  Serial.begin(115200);


  // Configuração dos pinos
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);


  // Inicializa o relé desligado
  digitalWrite(RELAY_PIN, RELAY_OFF);


  // Conexão Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }


  Serial.println("\nConectado com sucesso!");
  Serial.print("Acesse a página web no endereço IP: http://");
  Serial.println(WiFi.localIP());


  // Definição das rotas do servidor web
  server.on("/", handleRoot);
  server.on("/data", handleData);


  server.begin();
  Serial.println("Servidor Web iniciado!");
}


void loop() {
  server.handleClient();
}