#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <PID_v1.h>

// Remplacez par vos propres SSID et mot de passe
const char *ssid = "Votre_SSID";
const char *password = "Votre_mot_de_passe";

ESP8266WebServer server(80);

// Paramètres PID
double Setpoint, Input, Output;
double Kp = 2, Ki = 5, Kd = 1;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

double TempConversion(int valeur)
{
    double voltage = valeur * 5.0 / 1023.0;
    double resistance = 10000.0 * voltage / (5.0 - voltage);
    double tempK = 1.0 / (1.0 / 298.15 + log(resistance / 10000.0) / 3977); // B (beta) - 3977
    double tempCelcius = tempK - 273.15;
    return tempCelcius;
}
// Remplacez par votre propre code pour lire la température
double readTemperature()
{
    return TempConversion(analogRead(A0));
}

void controlHeater(double output)
{
    int relayState = output > 0 ? HIGH : LOW;
    digitalWrite(D1, relayState); // D1 est le pin du relais
    delay(output);                // Durée d'activation du relais
    digitalWrite(D1, LOW);
    delay(1000 - output); // Durée de désactivation du relais
}

void handleRoot()
{
    double currentTemp = readTemperature();
    server.send(200, "text/html", "<html><body><h1>Température actuelle : " + String(currentTemp, 2) + "</h1></body></html>");
}

void setup()
{
    Serial.begin(115200);

    WiFi.softAP(ssid, password);
    server.on("/", handleRoot);
    server.begin();

    Setpoint = 43;
    myPID.SetMode(AUTOMATIC);

    pinMode(A0, OUTPUT);
}

void loop()
{
    Input = readTemperature();
    myPID.Compute();
    controlHeater(Output);

    // Gestion des requêtes Web
    server.handleClient();

    // Affichage sur le port série
    Serial.print("Température: ");
    Serial.print(Input, 2);
    Serial.print("\t\t");
    Serial.print("Chauffage: ");
    Serial.println(Output);

    delay(1000); // Pause d'une seconde
}
