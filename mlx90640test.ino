#include <Adafruit_MLX90640.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "arduino_secrets.h"

Adafruit_MLX90640 mlx;
#define ROWS 24
#define COLUMNS 32
float frame[ROWS * COLUMNS];

#define UPDATE_INTERVAL 500

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

const char broker[] = "192.168.68.121";
int port = 1883;
const char* topic = "helloworld";
const char* clientname = "arduino";

void setup() {
  Wire.begin();
  Wire.setClock(800000);
  Serial.begin(9600);
  delay(3000);    // wait for serial monitor

  Serial.print("-- Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("");
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("-- You're connected to the network");

  Serial.println();
  Serial.print("--- Attempting to connect to the MQTT broker: ");
  Serial.println(broker);
  mqttClient.setServer(broker, port);
  mqttClient.setBufferSize(100000);

  if (!mqttClient.connect(clientname)) {
    Serial.print("MQTT connection failed!  State: ");
    Serial.println(mqttClient.state());
    while (1);
  }
  Serial.println("--- You're connected to the MQTT broker!");
  Serial.println();

  Serial.println("---- Starting thermal camera MLX90640");
  if (! mlx.begin(MLX90640_I2CADDR_DEFAULT, &Wire)) {
    Serial.println("MLX90640 not found!");
    while (1) delay(10);
  }
  Serial.print("Found MLX90640! Serial number: ");  
  Serial.print(mlx.serialNumber[0], HEX);
  Serial.print(mlx.serialNumber[1], HEX);
  Serial.println(mlx.serialNumber[2], HEX);
  mlx.setMode(MLX90640_CHESS);
  Serial.print("Current mode: ");
  if (mlx.getMode() == MLX90640_CHESS) {
    Serial.println("Chess");
  } else {
    Serial.println("Interleave");    
  }
  mlx.setResolution(MLX90640_ADC_18BIT);
  Serial.print("Current resolution: ");
  mlx90640_resolution_t res = mlx.getResolution();
  switch (res) {
    case MLX90640_ADC_16BIT: Serial.println("16 bit"); break;
    case MLX90640_ADC_17BIT: Serial.println("17 bit"); break;
    case MLX90640_ADC_18BIT: Serial.println("18 bit"); break;
    case MLX90640_ADC_19BIT: Serial.println("19 bit"); break;
  }
  mlx.setRefreshRate(MLX90640_16_HZ);
  Serial.print("Current frame rate: ");
  mlx90640_refreshrate_t rate = mlx.getRefreshRate();
  switch (rate) {
    case MLX90640_0_5_HZ: Serial.println("0.5 Hz"); break;
    case MLX90640_1_HZ: Serial.println("1 Hz"); break; 
    case MLX90640_2_HZ: Serial.println("2 Hz"); break;
    case MLX90640_4_HZ: Serial.println("4 Hz"); break;
    case MLX90640_8_HZ: Serial.println("8 Hz"); break;
    case MLX90640_16_HZ: Serial.println("16 Hz"); break;
    case MLX90640_32_HZ: Serial.println("32 Hz"); break;
    case MLX90640_64_HZ: Serial.println("64 Hz"); break;
  }

  Serial.println("---- Finished thermal camera MLX90640");
}

void loop() {
  String data = generateReadingsJSON(millis());
  int str_len = data.length() + 1;
  char char_array[str_len];
  data.toCharArray(char_array, str_len);
  boolean blah = mqttClient.publish(topic, char_array);
  delay(UPDATE_INTERVAL);
}

String generateReadingsJSON(long timestamp) {
  char data[4000];
  String values = generateThermalCameraCellJSON();  
  snprintf(data, sizeof(data), "{\"t\": \"%lu\", \"s\": [{\"n\": \"thermal camera\", \"t\": \"tmatrix\", \"r\": \"%s\", \"c\": \"%s\", \"v\": \"", timestamp, String(ROWS), String(COLUMNS));
  String string = String(data);
  string += values;
  string += "\"}]}";
  return string;
}
String generateThermalCameraCellJSON() {
  if (mlx.getFrame(frame) != 0) {
    Serial.println("Failed to read from thermal camera");
    return "";
  }
  String json = "";
  for(uint8_t row = 0; row < ROWS; row++) {
    for(uint8_t column = 0; column < COLUMNS; column++) {
      float temperature = frame[row * COLUMNS + column];
      char data[10];
      snprintf(data, sizeof(data), "%.2f", temperature);      
      json += String(data);
      if(!((row == ROWS - 1) && (column == COLUMNS -1))) {
        json += ",";
      }
    }
  }
  return json;
}