#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <UrlEncode.h>
#include "MAX30100_PulseOximeter.h"

// Configurações do WiFi e API
const char* ssid = "SATC IOT";
const char* password = "IOT2024@#";
String phoneNumber = "+554896618376";
String apiKey = "3019484";

// Definição dos pinos I2C e intervalos
#define SDA_PIN 21
#define SCL_PIN 22
#define MESSAGE_INTERVAL 30000
#define READING_PERIOD 1000
#define HEART_RATE_HIGH 100
#define HEART_RATE_LOW 60

PulseOximeter pox;
unsigned long lastReading = 0;
unsigned long lastMessage = 0;

void setup() {
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCL_PIN);
    
    // Conecta ao WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi conectado!");
    
    // Inicializa o sensor
    if (!pox.begin()) {
        Serial.println("Falha ao inicializar sensor. Reiniciando...");
        ESP.restart();
    }
    
    pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
    Serial.println("Sistema inicializado!");
}

void sendMessage(String message) {
    HTTPClient http;
    String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber + 
                 "&apikey=" + apiKey + "&text=" + urlEncode(message);
    
    http.begin(url);
    int httpResponseCode = http.POST(url);
    Serial.printf("Código de resposta HTTP: %d\n", httpResponseCode);
    http.end();
}

void loop() {
    pox.update();
    
    unsigned long currentMillis = millis();
    
    if (currentMillis - lastReading >= READING_PERIOD) {
        float heartRate = pox.getHeartRate();
        float spo2 = pox.getSpO2();
        
        if (heartRate > 0 && spo2 > 0 && heartRate < 255 && spo2 <= 100) {
            Serial.printf("BPM: %.1f, SpO2: %.1f%%\n", heartRate, spo2);
            
            if (currentMillis - lastMessage >= MESSAGE_INTERVAL) {
                String status = (heartRate > HEART_RATE_HIGH) ? "ALTA" : 
                                (heartRate < HEART_RATE_LOW) ? "BAIXA" : "NORMAL";
                
                String message = "FC " + status + ": " + String(heartRate, 1) + 
                                 "BPM/SpO2:" + String(spo2, 1) + "%";
                
                sendMessage(message);
                lastMessage = currentMillis;
            }
        } else {
            Serial.println("Leitura inválida");
        }
        
        lastReading = currentMillis;
    }
    
    delay(200);
}