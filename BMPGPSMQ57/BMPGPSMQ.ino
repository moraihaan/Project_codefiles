#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <TinyGPS++.h>

const char* ssid = "S20plus";     // Your WiFi SSID
const char* password = "12345678"; // Your WiFi password

WebServer server(80); // Create a WebServer object that listens on port 80

#define BMP_SCK  (13)
#define BMP_MISO (12)
#define BMP_MOSI (11)
#define BMP_CS   (10)

#define MQ5_PIN 36 // GPIO pin for MQ5 sensor
#define MQ7_PIN 39 // GPIO pin for MQ7 sensor

Adafruit_BMP280 bmp; // BMP280 object

#define RX_PIN 16
#define TX_PIN 17
HardwareSerial gpsSerial(1); // Using hardware serial port 1
TinyGPSPlus gps;

unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 1000; // Update interval in milliseconds (adjust as needed)

float temperatureValue;
float pressureValue;
float altitudeValue;
String mq5MessageValue;
String mq7MessageValue;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); // Print the IP address

  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData); // New endpoint to handle AJAX requests for sensor data
  server.begin();

  Serial.println("HTTP server started");

  if (!bmp.begin(0x76)) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring or try a different address!");
    while (1);
  }

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL, Adafruit_BMP280::SAMPLING_X2, Adafruit_BMP280::SAMPLING_X16, Adafruit_BMP280::FILTER_X16, Adafruit_BMP280::STANDBY_MS_500);

  gpsSerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
}

void loop() {
  server.handleClient();

  // Read GPS data
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      if (gps.location.isValid()) {
        Serial.print("Latitude: ");
        Serial.println(gps.location.lat(), 6);
        Serial.print("Longitude: ");
        Serial.println(gps.location.lng(), 6);
      }
    }
  }

  // Update sensor data when the update interval has passed
  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdateTime >= updateInterval) {
    lastUpdateTime = currentMillis;

    float temperature = bmp.readTemperature();
    float pressure = bmp.readPressure();
    float altitude = bmp.readAltitude(1017); // Adjusted to local forecast

    int mq5Value = analogRead(MQ5_PIN); // Read analog value from MQ5 sensor
    int mq7Value = analogRead(MQ7_PIN); // Read analog value from MQ7 sensor


    Serial.print("MQ5 Sensor Value: ");
    Serial.println(mq5Value);
    Serial.print("MQ7 Sensor Value: ");
    Serial.println(mq7Value);


    String mq5Message;
    if (mq5Value > 400) {
      mq5Message = "LPG Gas Is Present";
    } else {
      mq5Message = "LPG Gas Is Absent";
    }

    String mq7Message;
    if (mq7Value > 500) {
      mq7Message = "CO Gas Level Is Dangerous";
    } else {
      mq7Message = "CO Gas Level Is Safe";
    }

    // Update global variables for later retrieval
    temperatureValue = temperature;
    pressureValue = pressure;
    altitudeValue = altitude;
    mq5MessageValue = mq5Message;
    mq7MessageValue = mq7Message;
  }
}

void handleRoot() {
  float latitude = gps.location.lat();
  float longitude = gps.location.lng();

 String webpage = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"initial-scale=1.0, user-scalable=no\"><meta charset=\"utf-8\"><style>body { margin: 0; padding: 0; position: relative; } #title { text-align: center; padding-top: 20px; font-size: 50px; } #map { width: 700px; height: 700px; position: absolute; top: 450px; left: 50%; transform: translate(-50%, -50%); } #dataContainer { position: absolute; top: 820px; left: 50%; transform: translateX(-50%); text-align: left; padding: 20px; background-color: #F0F0F0; border: 4px solid black; } .temperature { color: red; } .pressure { color: blue; } .altitude { color: green; } .mq5.present { color: red; } .mq5.absent { color: darkgreen; } .mq7.dangerous { color: red; } .mq7.safe { color: darkgreen; }</style></head><body><div id=\"title\">Current Location of Rover</div><div id=\"map\"></div><div id=\"dataContainer\"><h2>BMP280 Data</h2><p class=\"temperature\" id=\"temperature\">Temperature: - °C</p><p class=\"pressure\" id=\"pressure\">Pressure: - Pa</p><p class=\"altitude\" id=\"altitude\">Approximate Altitude: - m</p><h2>MQ5 Sensor Data</h2><p class=\"mq5\" id=\"mq5Message\">Loading...</p><h2>MQ7 Sensor Data</h2><p class=\"mq7\" id=\"mq7Message\">Loading...</p><h2>GPS Data</h2><p class=\"gps\" id=\"gpsData\">Latitude: " + String(latitude, 6) + ", Longitude: " + String(longitude, 6) + "</p></div><script src=\"https://unpkg.com/leaflet@1.7.1/dist/leaflet.js\"></script><link rel=\"stylesheet\" href=\"https://unpkg.com/leaflet@1.7.1/dist/leaflet.css\" /><script>function fetchData() { var xhttp = new XMLHttpRequest(); xhttp.onreadystatechange = function() { if (this.readyState == 4 && this.status == 200) { var data = JSON.parse(this.responseText); document.getElementById(\"temperature\").innerHTML = \"Temperature: \" + data.temperature + \" °C\"; document.getElementById(\"pressure\").innerHTML = \"Pressure: \" + data.pressure + \" Pa\"; document.getElementById(\"altitude\").innerHTML = \"Approximate Altitude: \" + data.altitude + \" m\"; document.getElementById(\"mq5Message\").innerHTML = data.mq5Message; document.getElementById(\"mq7Message\").innerHTML = data.mq7Message; document.getElementById(\"mq5Message\").className = data.mq5Message === \"LPG Gas Is Present\" ? \"mq5 present\" : \"mq5 absent\"; document.getElementById(\"mq7Message\").className = data.mq7Message === \"CO Gas Level Is Dangerous\" ? \"mq7 dangerous\" : \"mq7 safe\"; } }; xhttp.open(\"GET\", \"/data\", true); xhttp.send(); } setInterval(fetchData, 1000); var map = L.map('map').setView([" + String(latitude, 6) + ", " + String(longitude, 6) + "], 13);L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {maxZoom: 19,}).addTo(map);L.marker([" + String(latitude, 6) + ", " + String(longitude, 6) + "]).addTo(map).bindPopup('I am here!').openPopup();</script></body></html>";

  server.send(200, "text/html", webpage);
}

void handleData() {
  String jsonData = "{\"temperature\":" + String(temperatureValue) + ",\"pressure\":" + String(pressureValue) + ",\"altitude\":" + String(altitudeValue) + ",\"mq5Message\":\"" + mq5MessageValue + "\",\"mq7Message\":\"" + mq7MessageValue + "\",\"latitude\":" + String(gps.location.lat(), 6) + ",\"longitude\":" + String(gps.location.lng(), 6) + "}";

  server.send(200, "application/json", jsonData);
}
