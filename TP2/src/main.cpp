#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <PID_v1.h>
#include <LittleFS.h>

// Remplacez par vos propres SSID et mot de passe
const char *ssid = "Make Québec Great Again";
const char *password = "Speak White";

ESP8266WebServer httpd(80);

// Paramètres PID
double Setpoint{43}, Input, Output;
double Kp = 2, Ki = 5, Kd = 1;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

float currentTime{0};

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

void controlHeater()
{
    int relayState = Output > 0 ? HIGH : LOW;
    digitalWrite(D1, relayState); // D1 est le pin du relais
    digitalWrite(D1, LOW);
}

void handleRoot()
{
    double currentTemp = readTemperature();
    httpd.send(200, "text/html", "<html> <head><meta http-equiv=\"refresh\" content=\"1\"></head> <body><h1>Température actuelle : " + String(currentTemp, 2) + "</h1></body></html>");
}

void setup()
{
    Serial.begin(115200);

    WiFi.softAP(ssid, password);
    httpd.on("/", handleRoot);
    httpd.begin();

    myPID.SetMode(AUTOMATIC);

    pinMode(D1, OUTPUT);
}

void loop()
{
    if (millis() - currentTime > 1000)
    {
        currentTime = millis();

        Input = readTemperature();
        myPID.Compute();
        controlHeater();
        digitalWrite(D1, Output);

        // Gestion des requêtes Web
        httpd.handleClient();

        // Affichage sur le port série
        Serial.print("Température: ");
        Serial.print(Input, 2);
        Serial.print("\t\t");
        Serial.print("Chauffage: ");
        Serial.println(Output);

        httpd.send(200, "text/html", "<html><body><h1>Température actuelle : " + String(Input, 2) + "</h1></body></html>");
        
    }
}
