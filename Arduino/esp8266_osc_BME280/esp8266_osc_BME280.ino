//
// ESP-OSC with BME280
// broadcasting Temperature, Humidity and Pressure on OSC
//
// Written by Kazme Egawa 2017/11/16
//

#define WAIT 0.5

//Libraries for ESP8266
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>

//Libraries for BME280
#include <BME280_MOD-1022.h>
#include <Wire.h>

//---------------------------------------

// モードフラグ
bool MODE_Server = false;

// モード切り替えピン
const int MODE_PIN = 0; // GPIO0

// Wi-Fi設定保存ファイル
const char* settings = "/wifi_settings.txt";

// サーバモードでのパスワード
const String pass = "egapyonpyon";


ESP8266WebServer server(80);

WiFiUDP Udp;                                // A UDP instance to let us send and receive packets over UDP
const IPAddress outIp(192, 168, 1, 255);     // remote IP of your computer
const unsigned int outPort = 7000;          // remote port to receive OSC
const unsigned int localPort = 8888;        // local port to listen for OSC packets (actually not used for sending)

//---------------------------------------


void setup() {
  Wire.begin();
  Serial.begin(115200);

  // 1秒以内にMODEを切り替える
  //  0 : Server
  //  1 : Client
  delay(1000);

  // ファイルシステム初期化
  SPIFFS.begin();

  pinMode(MODE_PIN, INPUT);
  if (digitalRead(MODE_PIN) == 0) {
    // サーバモード初期化
    Serial.println("Server Mode selected");
    setup_server();
    MODE_Server = true;
  } else {
    // クライアントモード初期化
    Serial.println("Client Mode Selected");
    setup_client();
    setup_BME280();
    MODE_Server = false;
  }
}

void loop() {
  if (MODE_Server) {
    server.handleClient();
  } else {
    func_BME280();
  }
}

/**
   WiFi設定
*/
void handleRootGet() {
  String html = "";
  html += "<h1>WiFi Settings</h1>";
  html += "<form method='post'>";
  html += "  <input type='text' name='ssid' placeholder='ssid'><br>";
  html += "  <input type='text' name='pass' placeholder='pass'><br>";
  html += "  <input type='submit'><br>";
  html += "</form>";
  server.send(200, "text/html", html);
}

void handleRootPost() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  File f = SPIFFS.open(settings, "w");
  f.println(ssid);
  f.println(pass);
  f.close();

  String html = "";
  html += "<h1>WiFi Settings</h1>";
  html += ssid + "<br>";
  html += pass + "<br>";
  server.send(200, "text/html", html);
}

/**
   初期化(クライアントモード)
*/
void setup_client() {
  File f = SPIFFS.open(settings, "r");
  String ssid = f.readStringUntil('\n');
  String pass = f.readStringUntil('\n');
  f.close();

  ssid.trim();
  pass.trim();

  Serial.println("SSID: " + ssid);
  Serial.println("PASS: " + pass);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid.c_str());
  WiFi.begin(ssid.c_str(), pass.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
}

/**
   初期化(サーバモード)
*/
void setup_server() {
  byte mac[6];
  WiFi.macAddress(mac);
  String ssid = "";
  for (int i = 0; i < 6; i++) {
    ssid += String(mac[i], HEX);
  }
  Serial.println("SSID: " + ssid);
  Serial.println("PASS: " + pass);

  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ssid.c_str(), pass.c_str());

  server.on("/", HTTP_GET, handleRootGet);
  server.on("/", HTTP_POST, handleRootPost);
  server.begin();
  Serial.println("HTTP server started.");
}

/**
   初期化（BME280）
*/
void setup_BME280() {
  // need to read the NVM compensation parameters
  BME280.readCompensationParams();

  // Need to turn on 1x oversampling, default is os_skipped, which means it doesn't measure anything
  BME280.writeOversamplingPressure(os1x);  // 1x over sampling (ie, just one sample)
  BME280.writeOversamplingTemperature(os1x);
  BME280.writeOversamplingHumidity(os1x);
}

/**
   function for BME280
*/
void func_BME280() {
  float temp, humidity, pressure;

  // forced sample.  After taking the measurement the chip goes back to sleep
  BME280.writeMode(smForced);
  while (BME280.isMeasuring()) {
    delay(1);
  }

  // read out the data - must do this before calling the getxxxxx routines
  BME280.readMeasurements();
  temp = BME280.getTemperature();  // must get temp first
  humidity = BME280.getHumidity();
  pressure = BME280.getPressure();

  Serial.print(temp);
  Serial.print("\t");
  Serial.print(humidity);
  Serial.print("\t");
  Serial.println(pressure);

  // make OSC message
  OSCMessage msg1("/temp");
  msg1.add(temp);
  Udp.beginPacket(outIp, outPort);
  msg1.send(Udp);
  Udp.endPacket();
  msg1.empty();

  // make OSC message
  OSCMessage msg2("/humidity");
  msg2.add(humidity);
  Udp.beginPacket(outIp, outPort);
  msg2.send(Udp);
  Udp.endPacket();
  msg2.empty();

  // make OSC message
  OSCMessage msg3("/pressure");
  msg3.add(pressure);
  Udp.beginPacket(outIp, outPort);
  msg3.send(Udp);
  Udp.endPacket();
  msg3.empty();

  delay(WAIT * 1000);
}

