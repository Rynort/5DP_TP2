#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <PID_v1.h>

// Remplacez par vos propres SSID et mot de passe
const char* ssid = "Votre_SSID";
const char* password = "Votre_mot_de_passe";

ESP8266WebServer server(80);

// Paramètres PID
double Setpoint, Input, Output;
double Kp = 2, Ki = 5, Kd = 1;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

// Remplacez par votre propre code pour lire la température
double readTemperature() {
    // Code pour lire la température
    return 0.0; // Valeur fictive
}

void controlHeater(double output) {
    int relayState = output > 0 ? HIGH : LOW;
    digitalWrite(D1, relayState); // D1 est le pin du relais
    delay(output); // Durée d'activation du relais
    digitalWrite(D1, LOW);
    delay(1000 - output); // Durée de désactivation du relais
}

void handleRoot() {
    double currentTemp = readTemperature();
    server.send(200, "text/html", "<html><body><h1>Température actuelle : " + String(currentTemp, 2) + "</h1></body></html>");
}

void setup() {
  Serial.begin(9600);

  WiFi.softAP(ssid, password);
  server.on("/", handleRoot);
  server.begin();

  Setpoint = 43;
  myPID.SetMode(AUTOMATIC);

  pinMode(D1, OUTPUT); // Configure
  //À complété
}
