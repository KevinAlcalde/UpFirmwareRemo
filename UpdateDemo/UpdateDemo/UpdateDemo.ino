#if defined(ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <Update.h>
#include <ArduinoOTA.h>

WebServer server(80);

#elif defined(ESP8266)
#include <ESP8266WiFi.h> 
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>

ESP8266WebServer server(80);

#endif

#include "data.h"  // Asegúrate de que este archivo incluya `ssid`, `password`, y `nombre`.

// Configuración de la IP fija
IPAddress local_IP(192, 168, 1, 47); // IP fija para el dispositivo
IPAddress gateway(192, 168, 1, 1);   // Dirección IP del router
IPAddress subnet(255, 255, 255, 0);  // Máscara de subred
IPAddress primaryDNS(8, 8, 8, 8);    // DNS primario
IPAddress secondaryDNS(8, 8, 4, 4);  // DNS secundario

// Contraseña correcta para la autenticación en la web
const char* correctPassword = "puertas";

// Página HTML de login para autenticación
const char* loginPage = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Autenticación</title>
</head>
<body>
  <h2>Ingrese la contraseña para proceder con la actualización del firmware</h2>
  <form action="/login" method="POST">
    <input type="password" name="password" placeholder="Contraseña">
    <input type="submit" value="Ingresar">
  </form>
</body>
</html>
)rawliteral";

void ActualizarPaso1() {
  server.sendHeader("Connection", "close");
  server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
  delay(1000);  // Añadir un pequeño retraso antes del reinicio
  ESP.restart();
}

void ActualizarPaso2() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.setDebugOutput(true);
#if defined(ESP8266)
    WiFiUDP::stopAll();
#endif
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
      Serial.printf("Actualización exitosa: %u\nReiniciando...\n", upload.totalSize);
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

  // Configura la IP fija
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Configuración de IP estática fallida");
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);  // Usa las credenciales definidas en data.h

  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    Serial.print("Conectado a WiFi. IP: ");
    Serial.println(WiFi.localIP());

    // Configuración de mDNS
    if (MDNS.begin(nombre)) {  // Usa el nombre del dispositivo definido en data.h
      Serial.println("mDNS iniciado");
    }

    // Configuración OTA para Arduino IDE
    ArduinoOTA.setHostname(nombre);  // Usar el nombre definido en data.h
    ArduinoOTA.begin();

    // Configuración de los eventos de OTA
    ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else { // U_SPIFFS
        type = "filesystem";
      }
      Serial.println("Inicio OTA: Actualizando " + type);
    });

    ArduinoOTA.onEnd([]() {
      Serial.println("\nFin OTA");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progreso: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Error de Autenticación");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Error al Comenzar");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Error de Conexión");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Error de Recepción");
      } else if (error == OTA_END_ERROR) {
        Serial.println("Error al Finalizar");
      }
    });

    Serial.println("OTA listo");

    server.on("/", HTTP_GET, []() {
      server.send(200, "text/html", loginPage);  // Muestra la página de login
    });

    server.on("/login", HTTP_POST, []() {
      if (server.arg("password") == correctPassword) {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", Pagina);  // Muestra la página de actualización desde data.h
        server.on("/actualizar", HTTP_POST, ActualizarPaso1, ActualizarPaso2);  // Habilita la actualización vía web
      } else {
        server.send(401, "text/html", "<h1>Acceso denegado</h1><p>Contraseña incorrecta.</p>");
      }
    });

    server.begin();
    MDNS.addService("http", "tcp", 80);
    Serial.printf("Listo!\nAbre http://%s.local en navegador\n", nombre);
    Serial.print("o en la IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Error en WiFi");
  }
}

void loop() {
  server.handleClient();  // Manejar solicitudes HTTP
  ArduinoOTA.handle();    // Procesar las actualizaciones OTA
#if defined(ESP8266)
  MDNS.update();
#endif
  delay(2);
}
