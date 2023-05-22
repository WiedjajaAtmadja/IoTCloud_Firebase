#include <DHTesp.h>
#include <BH1750.h>
#include <Firebase_ESP_Client.h>
#define FIREBASE_HOST "https://ce-binus-iot-course-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "Du6OLGVeZMdhMce3NFlnh4V7JYFTXM6S1el4U5pH"
#define WIFI_SSID "iot2.4g"
#define WIFI_PASSWORD "iot2.4g123"

#define LED_GREEN  4
#define LED_YELLOW 5
#define LED_RED    18
#define DHT_PIN 19

FirebaseData fbdo;
FirebaseConfig fbConfig;
FirebaseData fbdoStream;
DHTesp dht;
BH1750 lightMeter;

void WifiConnect();
void Firebase_Init(const String& streamPath);
void onFirebaseStream(FirebaseStream data);

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);
  dht.setup(DHT_PIN, DHTesp::DHT11);
  Wire.begin();
  lightMeter.begin(); 
  Serial.println("Booting...");
  WifiConnect();
  Serial.println("Connecting to Firebase...");
  Firebase_Init("cmd");
  Serial.println("System ready.");
  digitalWrite(LED_BUILTIN, 0);
}

void loop() {
  if (millis() % 10000 ==0)
  {
    digitalWrite(LED_BUILTIN, 1);
    float fHumidity = dht.getHumidity();
    float fTemperature = dht.getTemperature();
    float lux = lightMeter.readLightLevel();
    Serial.printf("Humidity: %.2f, Temperature: %.2f, Light: %.2f \n",
       fHumidity, fTemperature, lux);
    Firebase.RTDB.setInt(&fbdo, "/data/current/milis", millis());
    Firebase.RTDB.setFloat(&fbdo, "/data/current/temperature", fTemperature);
    Firebase.RTDB.setFloat(&fbdo, "/data/current/humidity", fHumidity);
    Firebase.RTDB.setFloat(&fbdo, "/data/current/lux", lux);

    Firebase.RTDB.pushFloat(&fbdo, "/data/history/temperature", fTemperature);
    Firebase.RTDB.pushFloat(&fbdo, "/data/history/humidity", fHumidity);
    Firebase.RTDB.pushFloat(&fbdo, "/data/history/lux", lux);

    digitalWrite(LED_BUILTIN, 0);
  }
}

void onFirebaseStream(FirebaseStream data)
{
  //onFirebaseStream: /cmd /ledGreen int 0
  Serial.printf("onFirebaseStream: %s %s %s %s\n", data.streamPath().c_str(),
                data.dataPath().c_str(), data.dataType().c_str(),
                data.stringData().c_str());
  if (data.dataType() == "int")
  {
    int value = data.intData();
    if (data.dataPath() == "/ledGreen")
      digitalWrite(LED_GREEN, value);
    else if (data.dataPath() == "/ledYellow")
      digitalWrite(LED_YELLOW, value);
    else if (data.dataPath() == "/ledRed")
      digitalWrite(LED_RED, value);
  }
}

void Firebase_Init(const String& streamPath)
{
  FirebaseAuth fbAuth;
  fbConfig.host = FIREBASE_HOST;
  fbConfig.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&fbConfig, &fbAuth);
  Firebase.reconnectWiFi(true);

  fbdo.setResponseSize(2048);
  Firebase.RTDB.setwriteSizeLimit(&fbdo, "medium");
  while (!Firebase.ready())
  {
    Serial.println("Connecting to firebase...");
    delay(1000);
  }
  String path = streamPath;
  if (Firebase.RTDB.beginStream(&fbdoStream, path.c_str()))
  {
    Serial.println("Firebase stream on "+ path);
    Firebase.RTDB.setStreamCallback(&fbdoStream, onFirebaseStream, 0);
  }
  else
    Serial.println("Firebase stream failed: "+fbdoStream.errorReason());
}

void WifiConnect()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }  
  Serial.print("System connected with IP address: ");
  Serial.println(WiFi.localIP());
  Serial.printf("RSSI: %d\n", WiFi.RSSI());
}
