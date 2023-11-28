#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <PID_v1.h>
#include <LittleFS.h>
#include <algorithm>
#include <iterator>

// Pour requetes http:
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFiMulti.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

ESP8266WiFiMulti wifiMulti;
WiFiClient espClient;
HTTPClient http;
PubSubClient client(espClient);

// Remplacez par vos propres SSID et mot de passe
const char *ssid = "Make Québec Great Again";
const char *password = "Speak White";

ESP8266WebServer httpd(80);

// Paramètres PID
double Setpoint{43}, Input, Output;
double Kp = 5, Ki = 10, Kd = 1;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

bool isOn{false};
float timer{0};
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
  if (Input > 43.5f || !isOn)
  {
    digitalWrite(D1, LOW);
    Output = 0;
  }
  else
  {
    int relayState = Output > 0 ? HIGH : LOW;
    digitalWrite(D1, relayState);
    delay(1000 * (Output / 255));
    digitalWrite(D1, LOW);
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

  if (currentTemp < 41 || currentTemp > 45)
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
  reponse += "<h1>Temp de stabilité: " + String((millis() - stableTime) / 1000) + "</h1>";
  reponse += "</body></html>";

  httpd.send(200, "text/html", reponse.c_str());
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 3 seconds");
      // Wait 3 seconds before retrying
      delay(3000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  // if ((char)payload[0] == '1') {
  //   digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
  //   // but actually the LED is on; this is because
  //   // it is active low on the ESP-01)
  // } else {
  //   digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  // }
}

void setup()
{
  Serial.begin(115200);

  // Host web page
  WiFi.softAP(ssid, password);
  httpd.on("/", handleRoot);
  httpd.begin();
  myPID.SetMode(AUTOMATIC);

  // Connect to depti network
  wifiMulti.addAP("DEPTI_2.4", "2021depTI"); // access point
  Serial.println("Connection....");
  while (wifiMulti.run() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(250);
  }
  Serial.println();
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID()); // le nom du wifi connecté.
  Serial.print("IP: ");
  Serial.println(WiFi.localIP()); // Adresse ip

  // Connect to client
  String url = "mqtt://172.16.0.209";
  client.setServer(url.c_str(), 1883);
  client.setCallback(callback);

  pinMode(D1, OUTPUT);
}

void loop()
{
  if (millis() - timer > 1000)
  {
    timer = millis();

    if (!client.connected())
    {
      reconnect();
    }
    client.loop();

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
    Serial.print((Output / 255) * 100);
    Serial.println("%");
  }
}
