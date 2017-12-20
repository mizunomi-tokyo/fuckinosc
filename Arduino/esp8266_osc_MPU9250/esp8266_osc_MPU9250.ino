//
// ESP-OSC with MPU9250
// broadcasting Acceleration, gyro, magnetic, yaw, pitch and roll on OSC
//
// Written by Kazme Egawa 2017/11/17
//

#define WAIT 0.5

//Libraries for ESP8266
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>

//Libraries for MPU9250
#include <quaternionFilters.h>
#include <MPU9250.h>

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
const unsigned int outPort = 7001;          // remote port to receive OSC
const unsigned int localPort = 8888;        // local port to listen for OSC packets (actually not used for sending)

MPU9250 myIMU;

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
    setup_MPU9250();
    MODE_Server = false;
  }
}

void loop() {
  if (MODE_Server) {
    server.handleClient();
  } else {
    func_MPU9250();
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
void setup_MPU9250() {
  myIMU.MPU9250SelfTest(myIMU.SelfTest);
  myIMU.calibrateMPU9250(myIMU.gyroBias, myIMU.accelBias);
  myIMU.initMPU9250();
  myIMU.initAK8963(myIMU.magCalibration);
}

/**
   function for BME280
*/
void func_MPU9250() {
  if (myIMU.readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01)
  {
    myIMU.readAccelData(myIMU.accelCount);
    myIMU.getAres();

    myIMU.ax = (float)myIMU.accelCount[0] * myIMU.aRes;
    myIMU.ay = (float)myIMU.accelCount[1] * myIMU.aRes;
    myIMU.az = (float)myIMU.accelCount[2] * myIMU.aRes;

    myIMU.readGyroData(myIMU.gyroCount);

    myIMU.getGres();

    myIMU.gx = (float)myIMU.gyroCount[0] * myIMU.gRes;
    myIMU.gy = (float)myIMU.gyroCount[1] * myIMU.gRes;
    myIMU.gz = (float)myIMU.gyroCount[2] * myIMU.gRes;

    myIMU.readMagData(myIMU.magCount);
    myIMU.getMres();

    myIMU.magbias[0] = +8.19;
    myIMU.magbias[1] = +17.136;
    myIMU.magbias[2] = +15.28;

    myIMU.mx = (float)myIMU.magCount[0] * myIMU.mRes * myIMU.magCalibration[0] -
               myIMU.magbias[0];
    myIMU.my = (float)myIMU.magCount[1] * myIMU.mRes * myIMU.magCalibration[1] -
               myIMU.magbias[1];
    myIMU.mz = (float)myIMU.magCount[2] * myIMU.mRes * myIMU.magCalibration[2] -
               myIMU.magbias[2];
  } // if (readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01)

  myIMU.updateTime();

  MahonyQuaternionUpdate(myIMU.ax, myIMU.ay, myIMU.az, myIMU.gx * DEG_TO_RAD,
                         myIMU.gy * DEG_TO_RAD, myIMU.gz * DEG_TO_RAD, myIMU.my,
                         myIMU.mx, myIMU.mz, myIMU.deltat);

  myIMU.yaw   = atan2(2.0f * (*(getQ() + 1) * *(getQ() + 2) + *getQ() *
                              *(getQ() + 3)), *getQ() * *getQ() + * (getQ() + 1) * *(getQ() + 1)
                      - * (getQ() + 2) * *(getQ() + 2) - * (getQ() + 3) * *(getQ() + 3));
  myIMU.pitch = -asin(2.0f * (*(getQ() + 1) * *(getQ() + 3) - *getQ() *
                              *(getQ() + 2)));
  myIMU.roll  = atan2(2.0f * (*getQ() * *(getQ() + 1) + * (getQ() + 2) *
                              *(getQ() + 3)), *getQ() * *getQ() - * (getQ() + 1) * *(getQ() + 1)
                      - * (getQ() + 2) * *(getQ() + 2) + * (getQ() + 3) * *(getQ() + 3));
                      
  myIMU.pitch *= RAD_TO_DEG;
  myIMU.yaw   *= RAD_TO_DEG;

  myIMU.yaw   -= 7.78;
  myIMU.roll  *= RAD_TO_DEG;

  Serial.print(1000 * myIMU.ax); Serial.print("\t");
  Serial.print(1000 * myIMU.ay); Serial.print("\t");
  Serial.print(1000 * myIMU.az); Serial.print("\t");

  // Print gyro values in degree/sec
  Serial.print(myIMU.gx, 3); Serial.print("\t");
  Serial.print(myIMU.gy, 3); Serial.print("\t");
  Serial.print(myIMU.gz, 3); Serial.print("\t");

  // Print mag values in degree/sec
  Serial.print(myIMU.mx); Serial.print("\t");
  Serial.print(myIMU.my); Serial.print("\t");
  Serial.println(myIMU.mz);

  Serial.print(myIMU.yaw, 2);
  Serial.print("\t");
  Serial.print(myIMU.pitch, 2);
  Serial.print("\t");
  Serial.print(myIMU.roll, 2);
  Serial.print("\t");
  Serial.println((float)myIMU.sumCount / myIMU.sum, 2);

  // make OSC message
  OSCMessage msg1("/a");
  msg1.add(1000 * myIMU.ax);
  msg1.add(1000 * myIMU.ay);
  msg1.add(1000 * myIMU.az);
  Udp.beginPacket(outIp, outPort);
  msg1.send(Udp);
  Udp.endPacket();
  msg1.empty();

  // make OSC message
  OSCMessage msg2("/g");
  msg2.add(myIMU.gx);
  msg2.add(myIMU.gy);
  msg2.add(myIMU.gz);
  Udp.beginPacket(outIp, outPort);
  msg2.send(Udp);
  Udp.endPacket();
  msg2.empty();

  // make OSC message
  OSCMessage msg3("/m");
  msg3.add(myIMU.mx);
  msg3.add(myIMU.my);
  msg3.add(myIMU.mz);
  Udp.beginPacket(outIp, outPort);
  msg3.send(Udp);
  Udp.endPacket();
  msg3.empty();

  // make OSC message
  OSCMessage msg4("/ahrs");
  msg4.add(myIMU.yaw);
  msg4.add(myIMU.pitch);
  msg4.add(myIMU.roll);
  Udp.beginPacket(outIp, outPort);
  msg4.send(Udp);
  Udp.endPacket();
  msg4.empty();

  delay(WAIT * 1000);
}

