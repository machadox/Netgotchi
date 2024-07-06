// Netgotchi - lives to protect your network!
// Created by MXZZ https://github.com/MXZZ
// GNU General Public License v3.0

#include <ESP8266WiFi.h>
#include <ESPping.h>
#include <Wire.h>
#include <SSD1306Wire.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>  // Include the WiFiManager library
#include <ESP8266FtpServer.h>
#include <ESP8266mDNS.h>

FtpServer ftpSrv;  // Create an instance of the FTP server

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Inicializa o display OLED com os pinos SDA e SCL definidos
SSD1306Wire display(0x3C, 14, 12);  // Endereço I2C 0x3C, SDA (D6), SCL (D5)

const int NUM_STARS = 100;
float stars[NUM_STARS][3];
float ufoX = SCREEN_WIDTH / 2;
float ufoY = SCREEN_HEIGHT / 2;
float ufoZ = 0;

String status = "Idle";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

unsigned long previousMillis = 0;
unsigned long previousMillisScan = 0;
unsigned long previousMillisPing = 0;

const long interval = 20000;
int i = 0;
int ipnum = 0;   // display counter
int iprows = 0;  // ip rows
int currentScreen = 0;
int max_ip = 255;
bool startScan = false;
const long intervalScan = 60000 * 4;
const long intervalPing = 60000 * 5;
int seconds = 0;

int ips[255] = {};

bool pingScanDetected = false;
unsigned long lastPingTime = 0;
bool honeypotTriggered = false;

String externalNetworkStatus = "";
String networkStatus = "";
bool scanOnce = true;
String stats = "not available";
String pwnagotchiFace = "(-v_v)";
String pwnagotchiFace2 = "(v_v-)";
String pwnagotchiFaceBlink = "( .__.)";
String pwnagotchiFaceSleep = "(-__- )";
String pwnagotchiFaceSurprised = "(o__o)";
String pwnagotchiFaceHappy = "(^=^)";
String pwnagotchiFaceSad = "(T_T)";
String pwnagotchiFaceSad2 = "(T__T)";
String pwnagotchiFaceSuspicious = "(>_>)";
String pwnagotchiFaceSuspicious2 = "(<_<)";
String pwnagotchiFaceHit = "(x_x)";
String pwnagotchiFaceHit2 = "(X__X)";
String pwnagotchiFaceStarryEyed = "(*_*)";
String pwnagotchiScreenMessage = "Saving planets!";
int animState = 0;
int animation = 0;
long old_seconds = 0;
int moveX = 0;
String currentIP = "";

//**
//Type of Subnet supported
//192.168.0.0/24 = type 0
//192.168.1.0/24 = type 1
//192.168.88.0/24 = type 2
//192.168.100.0/24  = type 3
// or add your own subnet in the pingNetwork Function
int subnet = 0;

//Use wifi manager or use the SSID/PASSWORD credential below
bool useWifiManager = false;
//ssid and password are used only when useWifiManager == false
const char* ssid = "xxxxxxxxx";
const char* password = "xxxxxx";

void setup() {
  Serial.begin(115200);

  WiFiManager wifiManager;

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0, "Connecting to WiFi");
  display.display();

  if (useWifiManager) {
    if (wifiManager.autoConnect("AutoConnectAP")) {
      display.drawString(0, 10, "Connection Successful");
      display.display();
    } else {
      display.drawString(0, 10, "Select Wifi AutoConnectAP");
      display.drawString(0, 20, "to run Wifi Setup");
      display.display();
    }
  } else {
    WiFi.begin(ssid, password);
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.drawString(0, 20, ".");
    display.display();
  }
  currentIP = WiFi.localIP().toString().c_str();
  Serial.println(currentIP);
  timeClient.begin();
  initStars();
  ftpSrv.begin("admin", "admin");  // Set FTP username and password
}

void loop() {
  unsigned long currentMillis = millis();
  seconds = currentMillis / 1000;

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    if (currentScreen == 0) {};
    if (currentScreen == 1) displayIPS();
    if (currentScreen == 2) NetworkStats();
    if (currentScreen > 2) {
      currentScreen = 0;
      animation++;
    }
    currentScreen++;
  }

  if (currentMillis - previousMillisScan >= intervalScan) {
    previousMillisScan = currentMillis;
    startScan = !startScan;
  }

  if (currentMillis - previousMillisPing >= intervalPing) {
    previousMillisPing = currentMillis;
    scanOnce = true;
  }

  if (startScan) {
    if (i < 255) {
      pingNetwork(i);
      i++;
    } else {
      i = 0;
      ipnum = 0;
    }
  }

  ftpHoneypotScan();

  if (animation == 0) drawSpace();
  if (animation == 1) pwnagotchi_face();
  if (animation > 1) animation = 0;

  delay(5);
}

void NetworkStats() {
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  networkStatus = WiFi.status() == WL_CONNECTED ? "connected" : "disconnected";
  display.drawString(0, 0, "Network: " + networkStatus);

  if (scanOnce) {
    IPAddress ip(1, 1, 1, 1);  // ping Google Cloudflare
    Serial.println("pinging cloudflare");

    if (Ping.ping(ip, 2)) {
      externalNetworkStatus = "Reachable";
      scanOnce = false;
      stats = "\n min: " + String(Ping.minTime()) + "ms \n avg: " + String(Ping.averageTime()) + "ms \n max: " + String(Ping.maxTime()) + "ms";
      delay(500);
      Serial.println("ping sent");
      Serial.println(stats);
    } else externalNetworkStatus = "Unreachable";
  }
  display.drawString(0, 16, "Network Speed: " + stats);
  display.drawString(0, 24, "Internet: " + externalNetworkStatus);
  display.display();
  delay(5000);
}

void ftpHoneypotScan() {
  ftpSrv.handleFTP();
  // Check for FTP connections
  if (ftpSrv.returnHoneypotStatus()) {
    honeypotTriggered = true;
  }
}

void drawSpace() {
  display.clear();
  updateAndDrawStars();
  drawUFO();
  displayTimeAndDate();
  display.display();
  delay(10);
}

void initStars() {
  for (int i = 0; i < NUM_STARS; i++) {
    stars[i][0] = random(-1000, 1000);
    stars[i][1] = random(-1000, 1000);
    stars[i][2] = random(1, 1000);
  }
}

void updateAndDrawStars() {
  for (int i = 0; i < NUM_STARS; i++) {
    stars[i][2] -= 5;
    if (stars[i][2] <= 0) stars[i][2] = 1000;

    int x = (stars[i][0] / stars[i][2]) * 64 + SCREEN_WIDTH / 2;
    int y = (stars[i][1] / stars[i][2]) * 32 + SCREEN_HEIGHT / 2;

    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
      display.setPixel(x, y);
    }
  }
}

void drawUFO() {
  int ufoSize = 8;
  display.drawLine(ufoX - ufoSize, ufoY, ufoX + ufoSize, ufoY);
  display.drawLine(ufoX, ufoY - ufoSize / 2, ufoX, ufoY + ufoSize / 2);
  display.drawCircle(ufoX, ufoY, ufoSize / 2);

  ufoX = SCREEN_WIDTH / 2 + sin(millis() / 1000.0) * 20;
  ufoY = SCREEN_HEIGHT / 2 + cos(millis() / 1500.0) * 10;
}

void displayTimeAndDate() {
  timeClient.update();
  String formattedTime = timeClient.getFormattedTime();
  time_t epochTime = timeClient.getEpochTime();
  struct tm* ptm = gmtime((time_t*)&epochTime);

  int currentDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon + 1;
  int currentYear = ptm->tm_year + 1900;

  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(5, 0, formattedTime);
  display.drawString(0, 8, String(currentDay) + "/" + String(currentMonth) + "/" + String(currentYear));
  display.drawString(0, 55, "Host found:" + String(ipnum));
  display.drawString(75, 0, "Honeypot");

  if (honeypotTriggered) {
    if (((seconds % 2) == 0)) {
      display.drawString(80, 8, "Breached");
    }
  } else {
    display.drawString(80, 8, "OK");
  }

  display.drawString(90, 55, startScan ? "Scan" : "Idle");
}

char hostString[16] = { 0 };
void serviceDiscover() {
  if (!MDNS.begin(hostString)) {
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("Sending mDNS query");
    int n = MDNS.queryService("https", "tcp");
    Serial.println("mDNS query done");
    if (n == 0) {
      Serial.println("no services found");
    } else {
      Serial.print(n);
      Serial.println(" service(s) found");
      for (int i = 0; i < n; ++i) {
        Serial.print(MDNS.hostname(i));
        Serial.print(MDNS.IP(i));
      }
    }
  }
}

void displayIPS() {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Found Hosts:" + String(ipnum));
  display.drawString(0, 10, "This:" + currentIP);

  String ipprefix = "";
  if (subnet == 0) ipprefix = "192.168.0.";
  if (subnet == 1) ipprefix = "192.168.1.";
  if (subnet == 2) ipprefix = "192.168.88.";
  if (subnet == 3) ipprefix = "192.168.100.";

  for (int j = 0; j < max_ip; j++) {
    if (ips[j] == 1 || ips[j] == -1) {
      if (iprows >= 4) {
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.drawString(5, 0, "Hosts:" + String(ipnum));
        iprows = 0;
      }
      display.drawString(0, 20 + (iprows)*10, ipprefix + String(j) + (ips[j] == 1 ? " alive" : " disconnected"));
      iprows++;
      delay(1500);
      display.display();
    }
  }
  if (ipnum > 0) delay(5000);
}

void pingNetwork(int i) {
  status = "Scanning";
  IPAddress ip;
  if (subnet == 0) ip = IPAddress(192, 168, 0, i);
  if (subnet == 1) ip = IPAddress(192, 168, 1, i);
  if (subnet == 2) ip = IPAddress(192, 168, 88, i);
  if (subnet == 3) ip = IPAddress(192, 168, 100, i);
  if (Ping.ping(ip, 1)) {
    iprows++;
    ipnum++;
    ips[i] = 1;
  } else {
    if (ips[i] == -1) ips[i] = 0;
    else if (ips[i] == 1) ips[i] = -1;
    else ips[i] = 0;
  }
}

void pwnagotchi_face() {
  display.clear();
  updateAndDrawStars();
  displayTimeAndDate();
  display.setFont(ArialMT_Plain_24);
  drawPwnagotchiFace(animState);

  if (seconds - old_seconds > 1) {
    moveX = moveX + random(-5, 5);
    if (moveX > 20) moveX = 5;
    if (moveX < -20) moveX = -5;

    old_seconds = seconds;
    animState++;
    if (animState > 5) animState = 0;
  }
  display.display();
  display.setFont(ArialMT_Plain_10);
}

void drawPwnagotchiFace(int state) {
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_24);
  display.drawString(64 + moveX, 30, honeypotTriggered ? 
    (state == 0 ? pwnagotchiFaceSad :
    state == 1 ? pwnagotchiFaceSad2 :
    state == 2 ? pwnagotchiFaceSuspicious :
    state == 3 ? pwnagotchiFaceSuspicious2 :
    state == 4 ? pwnagotchiFaceHit :
    pwnagotchiFaceHit2) :
    (state == 0 ? pwnagotchiFace :
    state == 1 ? pwnagotchiFace2 :
    state == 2 ? pwnagotchiFaceBlink :
    state == 3 ? pwnagotchiFaceSleep :
    state == 4 ? pwnagotchiFaceSurprised :
    pwnagotchiFaceHappy));
}
