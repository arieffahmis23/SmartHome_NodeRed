#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define DHTPIN  13
#define ldrPin 36
#define lamp1 12
#define relay 4
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Update these with values suitable for your network.
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "public.mqtthq.com"; 
const char* username = "";
const char* pass = "";

const float GAMMA = 0.7;
const float RL10 = 50;

char str_hum_data[10];
char str_temp_data[10];
char str_lux[10];

float temp_data;
int autoswitch;
int manualswitch;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

#define sub1 "status"

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String messageLamp;
  String messagekontrol;
  
  if(strstr(topic, "rumah_arief/kendali")){
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
      messagekontrol += (char)payload[i];
    }
    if(messagekontrol == "auto"){
      autoswitch = 1;
    }
    else if (messagekontrol == "manual"){
      autoswitch = 0;
    }
  }
  if (strstr(topic, "rumah_arief/kamar_1/lamp")) {
        for (int i = 0; i < length; i++) {
          Serial.print((char)payload[i]);
          messageLamp += (char)payload[i];
        }
        if (messageLamp == "On") {
          digitalWrite(lamp1, HIGH);
        }
        else if (messageLamp == "Off") {
          digitalWrite(lamp1, LOW);
        }
  }
  Serial.println();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), username, pass)) {
      Serial.println("connected");
      client.subscribe("rumah_arief/kamar_1/lamp");
      client.subscribe("rumah_arief/kendali");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Hello, Muhammad Arief Fahmi Surbakti");
  dht.begin();
  lcd.init();
  lcd.backlight();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  pinMode(lamp1, OUTPUT);
  pinMode(relay, OUTPUT);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();

  if (now - lastMsg > 2000) {
    int intensitas = analogRead(ldrPin);
    float voltage = intensitas / 4095.0 * 5.0;
    float resistance = 2000 * voltage / (1 - voltage / 5.0);
    float lux = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1 / GAMMA));

    float hum_data = dht.readHumidity();
    dtostrf(hum_data, 4, 2, str_hum_data);
    temp_data = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit
    dtostrf(temp_data, 4, 2, str_temp_data);
    dtostrf(lux, 4, 2, str_lux);
    
    if(autoswitch == 1){
      if (temp_data > 40){
        digitalWrite(relay, HIGH);  
        client.publish("rumah_arief/kamar_1/ac", "On");
      }
      else{
        digitalWrite(relay, LOW);
        client.publish("rumah_arief/kamar_1/ac", "Off");
      }

      if(lux > 500){
        digitalWrite(lamp1, HIGH);
        client.publish("rumah_arief/kamar_1/lamp", "On");
      }
      else{
        digitalWrite(lamp1, LOW);
        client.publish("rumah_arief/kamar_1/lamp", "Off");
      }
    }
    

    lcd.setCursor(0,0);
    lcd.print("Temp = ");
    lcd.print(str_temp_data);
    lcd.setCursor(0,1);
    lcd.print("Hum = ");
    lcd.print(str_hum_data);
    
    lastMsg = now;
    Serial.println("Publish message: ");
    Serial.print("Temperature:  "); Serial.println(str_temp_data); 
    client.publish("rumah_arief/kamar_1/suhu", str_temp_data);
    Serial.print("Humidity:  "); Serial.println(str_hum_data); 
    client.publish("rumah_arief/kamar_1/humidity", str_hum_data);
    Serial.print("Cahaya Luar:  "); Serial.println(str_lux); 
    client.publish("rumah_arief/kamar_1/cahaya", str_lux);
  }
}
