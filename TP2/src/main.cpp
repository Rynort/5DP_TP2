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
double minValue = 60;
double maxValue = 0;
unsigned long startTime;
unsigned long twoMinuteMark = 2 * 60 * 1000;  // 2 minutes in milliseconds
unsigned long fiveMinuteMark = 5 * 60 * 1000; // 5 minutes in milliseconds

String GetContentType(String filename)
{
  struct Mime
  {
    String ext, type;
  } mimeType[] = {
      {".html", "text/html"},
      {".js", "application/javascript"},
      {".css", "text/css"},
      {".wasm", "application/wasm"}};

  for (int i = 0; i < sizeof(mimeType) / sizeof(Mime); i++)
  {
    if (filename.endsWith(mimeType[i].ext))
    {
      return mimeType[i].type;
    }
  }

  return "application/octect-stream";
}

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
}

void handleRoot()
{
  double currentTemp = readTemperature();
  
  String reponse = "<html>";
  reponse += "<head><meta http-equiv=\"refresh\" content=\"30\"></head>";
  reponse += "<body>";
  reponse += "<a href=\"/led?action=on\">ON</a>";
  reponse += "<a href=\"/led?action=off\">OFF</a>";
  reponse += "<h1>Température actuelle : " + String(currentTemp, 2) + "</h1>";
  reponse += "";
  reponse += "</body></html>";

  httpd.send(200, "text/html", reponse.c_str());
}

void setup()
{
  Serial.begin(115200);
  startTime = millis();
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

    // Affichage sur le port série
    if (Input < minValue)
      minValue = Input;
    if (Input > maxValue)
      maxValue = Input;

    if (millis() - startTime >= twoMinuteMark)
    {

      minValue = 60;
      maxValue = 0;
      startTime = millis();
    }
    if (millis() - startTime >= fiveMinuteMark)
    {

      minValue = 60;
      maxValue = 0;
      startTime = millis();
    }

    Serial.print("Température: ");
    Serial.print(Input, 2);
    Serial.print("\t\t");
    Serial.print("Chauffage: ");
    Serial.println(Output);

    httpd.send(200, "text/html", "<html><body><h1>Température actuelle : " + String(Input, 2) + "</h1></body></html>");
  }
}
