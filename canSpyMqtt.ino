#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <mcp_can.h>
#include <SPI.h>
#include <PubSubClient.h>
#include <WiFi.h>

// TFT wiring
#define TFT_MOSI 11
#define TFT_SCLK 13
#define TFT_CS 10
#define TFT_RST 8
#define TFT_DC 9
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

#define MAX_LINES 14
#define TEXT_HEIGHT 8


// MCP2515 wiring
#define SCK 18
#define MOSI 19
#define MISO 16
#define INT 20
MCP_CAN CAN(17);

unsigned long rxId;
unsigned char len;
unsigned char rxBuf[8];

unsigned long lastId = 0;
byte lastData[8] = {0, 0, 0, 0, 0, 0, 0, 0};
byte lastLen = 0;


String lines[MAX_LINES];
int currentLine = 0;

#define UPDATE_INTERVAL 2000
unsigned long lastUpdate = 0;

const char* ssid = "RPI-SD";
const char* password = "seadrive";

WiFiClient espClient;
PubSubClient client(espClient);
const char* mqtt_server = "192.168.4.1";

String clientId;


void setup() {
  Serial.begin(115200);
  delay(50);
  SPI.setSCK(SCK);
  SPI.setTX(MOSI);
  SPI.setRX(MISO);
  if (CAN.begin(MCP_STDEXT, CAN_500KBPS, MCP_8MHZ) == CAN_OK)
    Serial.println("MCP2515 Initialized Successfully!");
  else
    Serial.println("MCP2515 not connected......");

  pinMode(INT, INPUT);

  // CAN Filter
  //CAN.init_Mask(0, 0, 0x1FFFFFFF); // Mask for Filter 
  //CAN.init_Mask(1, 0, 0x1FFFFFFF);  // Mask for filter 
  
  
  CAN.init_Mask(0, 0, 0x00000000); // Mask for Filter 0
  CAN.init_Mask(1, 0, 0x00000000);  // Mask for filter 1-5
  CAN.init_Filt(0, 0, 0x00000000); // 
  CAN.init_Filt(1, 1, 0x00000000); // Filter 1: Disabled 
  CAN.init_Filt(2, 1, 0x00000000); // Filter 2: Disabled
  CAN.init_Filt(3, 1, 0x00000000); // Filter 3: Disabled
  CAN.init_Filt(4, 1, 0x00000000); // Filter 4: Disabled
  CAN.init_Filt(5, 1, 0x00000000); // Filter 5: Disabled

  CAN.setMode(MCP_NORMAL);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);

  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.print("Pico W is connected to WiFi network ");
  Serial.println(WiFi.SSID());

  Serial.print("Assigned IP Address: ");
  Serial.println(WiFi.localIP());



  clientId = "pico_123";
  // clientId += String(random(0xffff), HEX); // Add a random number to the client ID

  client.setServer(mqtt_server, 1883);
  while (!client.connected()) {
    if (client.connect(clientId.c_str())) {
      Serial.println("Connected to MQTT broker!");
    } else {
      Serial.print("Failed to connect, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}



void loop() {
  if (!client.connected()) {
    while (!client.connected()) {
      if (client.connect(clientId.c_str())) {
        Serial.println("Connected to MQTT broker!");
      } else {
        Serial.print("Failed to connect, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        delay(5000);
      }
    }
  }
  client.loop();


  // Read the CAN bus
  if (!digitalRead(INT)) {
    CAN.readMsgBuf(&rxId, &len, (byte *)rxBuf);

    // Check if this message is the same as the last one
    if (rxId != lastId || len != lastLen || memcmp(rxBuf, lastData, len) != 0) {
      // Save this message
      lastId = rxId;
      memcpy(lastData, rxBuf, len);
      lastLen = len;

      // Add this message to the buffer
      String idLine = "ID: " + String(rxId, HEX);
      String dataLine = "MSG: ";
      for (int i = 0; i < len; i++) {
        if (rxBuf[i] < 0x10) {
          // Print a leading zero for single-digit hex values
          dataLine += "0";
        }
        dataLine += String(rxBuf[i], HEX);
      }

      // Scroll the lines up if the buffer is full
      if (currentLine >= MAX_LINES) {
        for (int i = 0; i < MAX_LINES - 2; i++) {
          lines[i] = lines[i + 2];
        }
        currentLine = MAX_LINES - 2;
      }

      // Add the new lines to the buffer
      lines[currentLine] = idLine;
      currentLine++;
      lines[currentLine] = dataLine;
      currentLine++;
    }
  }

  // Update the screen
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    lastUpdate = millis();

    // Clear the screen
    tft.fillScreen(ST77XX_BLACK);

    // Draw the lines from the buffer
    for (int i = 0; i < currentLine; i++) {
      tft.setCursor(0, i * TEXT_HEIGHT);
      tft.println(lines[i]);
    }
  }

  lastId = rxId;
  memcpy(lastData, rxBuf, len);
  lastLen = len;

  // Convert the data to strings
  String idStr = String(rxId, HEX);
  String dataStr;
  for (int i = 0; i < len; i++) {
    if (rxBuf[i] < 0x10) {
      // Add a leading zero for single-digit hex values
      dataStr += "0";
    }
    dataStr += String(rxBuf[i], HEX);
  }

  // Publish the data to the MQTT broker
  client.publish(("canbus/" + idStr).c_str(), dataStr.c_str());


}

