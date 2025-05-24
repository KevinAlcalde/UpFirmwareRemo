// Creado por ChepeCarlos de ALSW
// Tutorial Completo en https://nocheprogramacion.com
// Canal Youtube https://youtube.com/alswnet?sub_confirmation=1

#if defined(ESP32)
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
WebServer server(80);

#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
ESP8266WebServer server(80);
#endif

IPAddress ip_local(192, 168, 1, 69); // IP fija para el módulo
IPAddress gateway(192, 168, 1, 44);   // Puerta de enlace de tu red
IPAddress subnet(255, 255, 255, 0);   // Máscara de subred

#include "data.h"  // Asumiendo que el archivo data.h contiene ssid y password

// Función para manejar la página principal
void PaginaSimple() {
  server.sendHeader("Connection", "close");
  server.send(200, "text/html", Pagina);  // 'Pagina' debería estar definida en data.h
}

// Función que maneja el proceso de actualización
void ActualizarPaso1() {
  server.sendHeader("Connection", "close");
  server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
  ESP.restart();  // Reinicia el dispositivo después de la actualización
}

// Función que maneja la subida del firmware
void ActualizarPaso2() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.setDebugOutput(true);
    Serial.printf("Actualizando: %s\n", upload.filename.c_str());
    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin(maxSketchSpace)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("Actualización Exitosa: %u bytes\nReiniciando...\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
    Serial.setDebugOutput(false);
  } else {
    Serial.printf("Problema con la Actualización (Tal vez problema con la conexión); Estado=%d\n", upload.status);
  }
  yield();
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Iniciando Servidor...");

  // Configuración de la IP fija
  if (!WiFi.config(ip_local, gateway, subnet)) {
    Serial.println("Error en la configuración de IP");
  }

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);  // Las credenciales de WiFi están en data.h

  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    // Configuración del servidor web
    server.on("/", HTTP_GET, PaginaSimple);
    server.on("/actualizar", HTTP_POST, ActualizarPaso1, ActualizarPaso2);
    server.begin();
    Serial.print("Conectado a la red WiFi. IP local: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Error en la conexión WiFi");
  }
}

void loop() {
  server.handleClient();  // Maneja las solicitudes del cliente web
  delay(2);  // Pequeño retraso para evitar saturación de la CPU
}
