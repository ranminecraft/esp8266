#include <TimeLib.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <Arduino.h>

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

const char ssid[] = "Nope";
const char pass[] = "22022202";

static const char ntpServerName[] = "ntp1.aliyun.com";
const int timeZone = 8;

const unsigned long HTTP_TIMEOUT = 6000;
WiFiUDP Udp;
unsigned int localPort = 8888;

time_t getNtpTime();
boolean isNTPConnected = false;

// 星
const unsigned char xing[] U8X8_PROGMEM = {
  0x00, 0x00, 0xF8, 0x0F, 0x08, 0x08, 0xF8, 0x0F, 0x08, 0x08, 0xF8, 0x0F, 0x80, 0x00, 0x88, 0x00,
  0xF8, 0x1F, 0x84, 0x00, 0x82, 0x00, 0xF8, 0x0F, 0x80, 0x00, 0x80, 0x00, 0xFE, 0x3F, 0x00, 0x00
};
// 六
const unsigned char liu[] U8X8_PROGMEM = {
  0x40, 0x00, 0x80, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0xFF, 0x7F, 0x00, 0x00, 0x00, 0x00,
  0x20, 0x02, 0x20, 0x04, 0x10, 0x08, 0x10, 0x10, 0x08, 0x10, 0x04, 0x20, 0x02, 0x20, 0x00, 0x00
};

// 小电视
const unsigned char bilibilitv_24u[] U8X8_PROGMEM = { 0x00, 0x00, 0x02, 0x00, 0x00, 0x03, 0x30, 0x00, 0x01, 0xe0, 0x80, 0x01,
                                                      0x80, 0xc3, 0x00, 0x00, 0xef, 0x00, 0xff, 0xff, 0xff, 0x03, 0x00, 0xc0, 0xf9, 0xff, 0xdf, 0x09, 0x00, 0xd0, 0x09, 0x00, 0xd0, 0x89, 0xc1,
                                                      0xd1, 0xe9, 0x81, 0xd3, 0x69, 0x00, 0xd6, 0x09, 0x91, 0xd0, 0x09, 0xdb, 0xd0, 0x09, 0x7e, 0xd0, 0x0d, 0x00, 0xd0, 0x4d, 0x89, 0xdb, 0xfb,
                                                      0xff, 0xdf, 0x03, 0x00, 0xc0, 0xff, 0xff, 0xff, 0x78, 0x00, 0x1e, 0x30, 0x00, 0x0c };

const int slaveSelect = 5;
const int scanLimit = 7;

int times = 0;
int hours = -1;
String weather = "天气获取中...";

// dnspod
String domain = "ranmc.cc";
String subDomain = "ddns";
String loginToken = "336053,3536c7dd873558b7a2e3b8b9edd78816";
String recordId;
String address;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;

  u8g2.begin();
  u8g2.enableUTF8Print();

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  u8g2.setCursor(0, 15);
  u8g2.print("小阿然专属多功能屏");
  u8g2.setCursor(0, 29);
  u8g2.print("By Ranica");
  u8g2.setCursor(0, 43);
  u8g2.print("www.ranmc.cn");
  u8g2.setCursor(0, 57);
  u8g2.print("version 2.5(透明探索)");
  u8g2.sendBuffer();

  Serial.print("Connecting WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  // 每300秒同步一次时间
  setSyncInterval(300);
  isNTPConnected = true;
}


void loop() {

  if (timeStatus() != timeNotSet) {
    oledClockDisplay();
  }
  if (WiFi.status() == WL_CONNECTED) {
    if (times % 80 == 0) getBtc();
    if (times % 100 == 0) getBili();
    if (times == 10) ddns();
    if (times == 20) getMc();
  }

  times += 1;
  if (times >= 1200) {
    times = 0;
  }
  delay(1000);
}
/*
void temp() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  u8g2.setCursor(0, 15);
  String line1 = "室内温度: ";
  line1 += t;
  line1 += " °C";
  Serial.println(line1);
  u8g2.print(line1);
  u8g2.setCursor(0, 30);
  String line2 = "室内湿度: ";
  line2 += h;
  line2 += " %";
  Serial.println(line2);
  u8g2.print(line2);
  u8g2.sendBuffer();
  delay(5000);
}*/

void getBili() {
  Serial.print("getBili");
  String response = httpsGet("https://api.bilibili.com/x/relation/stat?vmid=233623132");
  DynamicJsonDocument json(1024);
  deserializeJson(json, response);
  JsonObject data = json["data"];
  int follower = data["follower"];
  Serial.print(" bilibili: ");
  Serial.println(follower);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_chinese2);
  u8g2.drawXBMP(52, 0, 24, 24, bilibilitv_24u);
  u8g2.setFont(u8g2_font_crox4t_tn);
  u8g2.setCursor(48, 42);
  u8g2.print(follower);
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  u8g2.setCursor(32, 61);
  u8g2.print("阔耐的阿然");
  u8g2.sendBuffer();
  delay(5000);
}

void getMc() {
  Serial.print("getMc");
  String response = httpsGet("https://list.fansmc.com/api/ranmc.cn");
  DynamicJsonDocument json(1024);
  deserializeJson(json, response);
  int online = json["p"];
  int maxPlayers = json["mp"];
  String text = "";
  text += online;
  text += "/";
  text += maxPlayers;
  Serial.print(text);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_bubble_tn);
  u8g2.setCursor(20, 27);
  u8g2.println(text);
  u8g2.setFont(u8g2_font_courB14_tn);
  u8g2.setCursor(32, 44);
  u8g2.print("1.19.2");
  u8g2.setFont(u8g2_font_courB14_tf);
  u8g2.setCursor(25, 58);
  u8g2.println("ranmc.cn");
  u8g2.sendBuffer();
  delay(10000);
}

void getBtc() {
  Serial.print("getBtc");
  String response = httpsGet("https://chain.so/api/v2/get_info/btc");
  DynamicJsonDocument json(1024);
  deserializeJson(json, response);
  JsonObject data = json["data"];
  int price = data["price"];
  Serial.print("BTC/USDT: ");
  Serial.print(price);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB24_tn);
  u8g2.setCursor(15, 30);
  u8g2.print(price);
  u8g2.setFont(u8g2_font_bubble_tr);
  u8g2.setCursor(4, 61);
  u8g2.print("btc");
  u8g2.setFont(u8g2_font_crox4hb_tf);
  u8g2.print("/USDT");
  u8g2.setCursor(57, 54);
  u8g2.sendBuffer();
  delay(5000);
}

void getWeather() {
  Serial.print("getWeather");
  String response = httpsGet("https://devapi.heweather.net/v7/weather/now?gzip=n&location=101281701&key=29098938f66545ee97cd2c9540173b5b");
  DynamicJsonDocument json(1024);
  deserializeJson(json, response);
  JsonObject now = json["now"];
  weather = "天气 ";
  String text = now["text"];
  weather += text;
  weather += " ";
  int temp = now["temp"];
  weather += temp;
  weather += "°   ";
  if (text == "null") {
    weather = "天气获取失败...";
  } else if (temp > 32) {
    weather += "(?_?)";
  } else if (temp > 29) {
    weather += "(X_X)";
  } else if (temp > 24) {
    weather += "(^_^)";
  } else if (temp < 19) {
    weather += "(o_o)";
  } else if (temp < 14) {
    weather += "(o_O)";
  } else {
    weather += "(^v^)";
  }
  Serial.print(weather);
}

void ddns() {
  Serial.print("ddns");

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  u8g2.setCursor(0, 14);
  u8g2.print("DDNS状态");
  u8g2.setCursor(0, 30);
  u8g2.print("域名: " + subDomain + "." + domain);
  u8g2.setCursor(0, 46);
  u8g2.print("当前IP: ");
  u8g2.setCursor(0, 62);
  u8g2.print("状态: ");
  u8g2.sendBuffer();

  address = httpsGet("https://ddns.oray.com/checkip");
  address.replace("Current IP Address: ", "");
  Serial.print("IP:");
  Serial.print(address);
  if (address.length() > 15 || address.length() < 7) {
    Serial.print("IP获取失败");
    Serial.print(address);
    address = "获取失败";
    u8g2.setCursor(0, 46);
    u8g2.print("当前IP: 获取失败");
    u8g2.setCursor(0, 62);
    u8g2.print("状态: 网络错误");
    u8g2.sendBuffer();
  } else {
    u8g2.setCursor(0, 46);
    u8g2.print("当前IP: " + address);
    u8g2.sendBuffer();
    u8g2.setCursor(0, 62);
    String path = "login_token=" + loginToken + "&domain=" + domain + "&sub_domain=" + subDomain;
    String response = httpsPost("dnsapi.cn", "/Record.List", path);
    DynamicJsonDocument json(1024);
    deserializeJson(json, response);
    JsonArray records = json["records"];
    if (records.size() >= 1) {
      JsonObject record = records[0];
      recordId = String(record["id"]);
      Serial.print("ID:");
      Serial.print(recordId);
      String value = record["value"];
      if (value == address) {
        u8g2.print("状态: 未变更");
      } else {
        path += "&record_id=" + recordId + "&value=" + address + "&record_type=A&record_line=默认&mx=0";
        String response = httpsPost("dnsapi.cn", "/Record.Modify", path);
        deserializeJson(json, response);
        JsonObject status = json["status"];
        int code = status["code"];
        if (code == 1) {
          u8g2.print("状态: 已同步");
        } else {
          u8g2.print("状态: 同步失败");
        }
        Serial.print(response);
      }
    } else {

      
      u8g2.print("状态: 记录不存在");
    }
    u8g2.sendBuffer();
  }
  delay(5000);
}


// Network
String httpsPost(String host, String uri, String path) {
  String response = "{}";

  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;
  https.begin(*client, host, 443, uri, true);
  https.addHeader("Content-Type", "application/x-www-form-urlencoded");
  https.addHeader("User-Agent", "BuildFailureDetectorESP8266");
  https.addHeader("Host", String(uri + ":443"));
  https.addHeader("Content-Length", String(path.length()));
  https.POST(path);

  /*
  int httpCode = https.POST(path);  
  if (httpCode > 0 && httpCode == HTTP_CODE_OK) {  */

  response = https.getString();
  https.end();
  return response;
}

String httpsGet(String url) {
  String response = "{}";
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;
  https.begin(*client, url);
  https.GET();
  response = https.getString();
  https.end();
  return response;
  return "{}";
}

// Time

void oledClockDisplay() {
  int years, months, days, minutes, seconds, weekdays;
  years = year();
  months = month();
  days = day();
  if (hours != hour() || weather == "天气获取失败...") {
    getWeather();
  }
  hours = hour();
  minutes = minute();
  seconds = second();
  weekdays = weekday();
  //Serial.printf("%d/%d/%d %d:%d:%d Weekday:%d\n", years, months, days, hours, minutes, seconds, weekdays);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  u8g2.setCursor(0, 14);
  u8g2.print(weather);
  String currentTime = "";
  if (hours < 10)
    currentTime += 0;
  currentTime += hours;
  currentTime += ":";
  if (minutes < 10)
    currentTime += 0;
  currentTime += minutes;
  currentTime += ":";
  if (seconds < 10)
    currentTime += 0;
  currentTime += seconds;
  String currentDay = "";
  currentDay += years;
  currentDay += "/";
  if (months < 10)
    currentDay += 0;
  currentDay += months;
  currentDay += "/";
  if (days < 10)
    currentDay += 0;
  currentDay += days;
  u8g2.setFont(u8g2_font_logisoso24_tr);
  u8g2.setCursor(5, 44);
  u8g2.print(currentTime);
  u8g2.setCursor(0, 61);
  u8g2.setFont(u8g2_font_unifont_t_chinese2);
  u8g2.print(currentDay);
  u8g2.drawXBM(80, 48, 16, 16, xing);
  u8g2.setCursor(95, 62);
  u8g2.print("期");
  if (weekdays == 1)
    u8g2.print("日");
  else if (weekdays == 2)
    u8g2.print("一");
  else if (weekdays == 3)
    u8g2.print("二");
  else if (weekdays == 4)
    u8g2.print("三");
  else if (weekdays == 5)
    u8g2.print("四");
  else if (weekdays == 6)
    u8g2.print("五");
  else if (weekdays == 7)
    u8g2.drawXBM(111, 49, 16, 16, liu);
  u8g2.sendBuffer();
}


// NTP

const int NTP_PACKET_SIZE = 48;      // NTP时间在消息的前48个字节里
byte packetBuffer[NTP_PACKET_SIZE];  // 输入输出包的缓冲区


time_t getNtpTime() {
  Serial.println("getNtpTime");
  IPAddress ntpServerIP;
  while (Udp.parsePacket() > 0)
    ;
  // 从池中获取随机服务器
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      isNTPConnected = true;
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // 将数据包读取到缓冲区
      unsigned long secsSince1900;
      // 将从位置40开始的四个字节转换为长整型，只取前32位整数部分
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      Serial.println(secsSince1900);
      Serial.println(secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR);
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("NTP FAIL");
  isNTPConnected = false;
  return 0;  //如果未得到时间则返回0
}


// 向给定地址的时间服务器发送NTP请求
void sendNTPpacket(IPAddress& address) {
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;  // LI, Version, Mode
  packetBuffer[1] = 0;           // Stratum, or type of clock
  packetBuffer[2] = 6;           // Polling Interval
  packetBuffer[3] = 0xEC;        // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  Udp.beginPacket(address, 123);  //NTP需要使用的UDP端口号为123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}