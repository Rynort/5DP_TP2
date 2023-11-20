#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <PID_v1.h>
#include <LittleFS.h>
#include <algorithm>
#include <iterator>

// Remplacez par vos propres SSID et mot de passe
const char *ssid = "Make Québec Great Again";
const char *password = "Speak White";

ESP8266WebServer httpd(80);

// Paramètres PID
double Setpoint{43}, Input, Output;
double Kp = 2, Ki = 5, Kd = 1;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

bool isOn{false};
float currentTime{0};
float stableTime;
double twoMin[120];
double fiveMin[300];
unsigned long twoMinuteMark = 2 * 60 * 1000;  // 2 minutes in milliseconds
unsigned long fiveMinuteMark = 5 * 60 * 1000; // 5 minutes in milliseconds

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
  if (Input > 44 || !isOn)
  {
    digitalWrite(D1, LOW);
    Output = 0;
  }
  else
  {
    int relayState = Output > 0 ? HIGH : LOW;
    digitalWrite(D1, relayState);
  }
}

void handleRoot()
{
  if (httpd.hasArg("action"))
  {
    String action = httpd.arg("action");
    if (action == "on")
      isOn = true;
    else if (action == "off")
      isOn = false;
  }

  double currentTemp = readTemperature();

  if (currentTemp >= 41 && currentTemp <= 45)
  {
    stableTime = millis();
  }

  String reponse = "<html>";
  reponse += "<head><meta http-equiv=\"refresh\" content=\"30\">";
  reponse += "<meta charset=\"utf-8\"></head>";
  reponse += "<body>";
  reponse += "<a href=\"/?action=on\">ON</a>";
  reponse += "&emsp;";
  reponse += "<a href=\"/?action=off\">OFF</a>";
  reponse += "<h1>Température actuelle: " + String(currentTemp, 2) + "</h1>";
  reponse += "<h1>Intensité de l'élément chauffant: " + String((Output / 255) * 100) + "%</h1>";
  reponse += "<h1>Température minimale 2 minutes: " + String(*std::min_element(std::begin(twoMin), std::end(twoMin))) + "</h1>";
  reponse += "<h1>Température maximale 2 minutes: " + String(*std::max_element(std::begin(twoMin), std::end(twoMin))) + "</h1>";
  reponse += "<h1>Température minimale 5 minutes: " + String(*std::min_element(std::begin(fiveMin), std::end(fiveMin))) + "</h1>";
  reponse += "<h1>Température maximale 5 minutes: " + String(*std::max_element(std::begin(fiveMin), std::end(fiveMin))) + "</h1>";
  reponse += "<h1>Temp de stabilité: " + String((millis() - stableTime)/1000) + "</h1>";
  reponse += "</body></html>";

  httpd.send(200, "text/html", reponse.c_str());
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

    // Gestion des requêtes Web
    httpd.handleClient();

    //  Update temp array.
    twoMin[(millis() / 1000) % 120] = Input;
    fiveMin[(millis() / 1000) % 300] = Input;

    handleRoot();

    // Affichage sur le port série
    Serial.print("Température: ");
    Serial.print(Input, 2);
    Serial.print("\t\t");
    Serial.print("Chauffage: ");
    Serial.println((Output/255)*100);
    Serial.print("%");

    // httpd.send(200, "text/html", "<html><body><h1>Température actuelle : " + String(Input, 2) + "</h1></body></html>");
  }
}
