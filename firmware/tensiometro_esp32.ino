#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <HX710AB.h>

// --- Configurações da Rede Wi-Fi (ajuste para sua rede local)---
const char* ssid = "NOME_DA_REDE_WIFI"; // substitua pelo nome da sua rede Wi-Fi
const char* password = "SENHA_DA_REDE_WIFI"; // substitua pela senha da sua rede Wi-Fi

// --- Configuração da API Python (FastAPI) ---
// Se a API estiver rodando no mesmo computador, use o IP local da sua máquina na rede (ex: http://<IP_LOCAL_DA_SUA_MAQUINA>:8000/api/leituras)
// Quando hospedar a API na nuvem, substitua pela URL pública (ex: https://sua-api.onrender.com/api/leituras)
const char* serverUrl = "http://192.168.X.X:8000/api/leituras";

// --- Configuração dos Pinos do HX710B ---
const uint8_t dataPin = 21;   // OUT / DAT
const uint8_t clockPin = 22;  // SCK / CLK

HX710B sensor(dataPin, clockPin);

// Constantes de Calibração do Tensiômetro
const int32_t VALOR_ZERO = 2218811;  // Leitura bruta em pressão atmosférica (ajuste com seu teste)
const float FATOR_ESCALA = 10000.0;  // Fator de conversão para kPa

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Inicializa o Sensor
  sensor.begin();
  Serial.println("Sensor HX710B Inicializado.");

  // Conexão Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Conectado com sucesso!");
  Serial.print("Endereço IP do ESP32: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (sensor.is_ready()) {
      int32_t leituraBruta = sensor.read();
      
      // Cálculo de Pressão em kPa
      float pressaoKpa = (float)(leituraBruta - VALOR_ZERO) / FATOR_ESCALA;
      if (pressaoKpa < 0) pressaoKpa = 0.0;

      Serial.println("\n--------------------------------");
      Serial.print("Leitura Bruta: "); Serial.println(leituraBruta);
      Serial.print("Pressao (kPa): "); Serial.println(pressaoKpa);

      // JSON
      StaticJsonDocument<200> doc;
      doc["dispositivo_id"] = "tensiometro_01";
      doc["pressao_kpa"] = pressaoKpa;
      doc["leitura_bruta"] = leituraBruta;

      String jsonPayload;
      serializeJson(doc, jsonPayload);

      // Envio HTTP POST para a API
      HTTPClient http;
      http.begin(serverUrl);
      http.addHeader("Content-Type", "application/json");

      int httpResponseCode = http.POST(jsonPayload);

      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.print("Resposta da API (Code ");
        Serial.print(httpResponseCode);
        Serial.print("): ");
        Serial.println(response);
      } else {
        Serial.print("Erro ao enviar POST: ");
        Serial.println(httpResponseCode);
      }

      http.end();
    } else {
      Serial.println("Sensor HX710B ocupado ou desconectado.");
    }
  } else {
    Serial.println("Wi-Fi Desconectado. Tentando reconectar...");
    WiFi.reconnect();
  }

  // Intervalo entre leituras
  delay(60000);
}