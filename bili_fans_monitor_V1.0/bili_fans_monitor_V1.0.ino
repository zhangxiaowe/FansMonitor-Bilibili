/**
 * 哔哩哔哩粉丝计数器V1.0
 * 
 * 实现功能：
 *   LCD2004显示粉丝数
 *   粉丝数变化灯效（红绿两色LED）
 *   
 * 引脚连接：
 *  ESP8266 -- LCD1602
 *    3V        VCC
 *    G         GND
 *    D2        SDA
 *    D1        SCL 
 *    
 * 说明：
 *   参考：
 *     https://www.bilibili.com/video/BV1cb41137UG
 *     https://github.com/flyAkari/bilibili_fans_monitor_595
 *     
 * 发布日期：2022-05-02
*/
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <Ticker.h>
#include <NTPClient.h>
#include <WiFiUdp.h>


//---------------wifi、B站账号配置--------------------
const char* ssid       = "";   //WiFi名
const char* password   = "";   //WiFi密码
String biliuid         = "353300793";   //bilibili UID 用户ID
const int REFRUSH_TIME = 1000;          // 刷新时间间隔
const int timeZone = 8; //东八区的时间
long fans_add = 0;
long fans0 = 0;
const int LED_1 = D5; 
const int LED_2 = D6; 
const int LED_3 = D7;
const int LED_4 = D3;   //RED LED

DynamicJsonDocument JSON_Buffer(400);   //声明一个大小为400B的DynamicJsonDocument对象JSON_Buffer,用于存储反序列化后的（即由字符串转换成JSON格式的）JSON报文，方式声明的对象将存储在堆内存中
JsonObject root;        // 定义一个JSON对象的根节点root 用于获取将JSON字符串经过显示类型转换为JSON对象的报文主体
LiquidCrystal_I2C lcd(0x27,20,4);       // set the LCD address to 0x27 for a 16 chars and 2 line display
WiFiClient client;      //创建WiFi对象
WiFiUDP ntpUDP;
// 你可以指定时间服务器池和偏移量（以秒为单位，以后可以用setTimeOffset()改变）。此外，你还可以指定更新间隔（以毫秒为单位，可以通过setUpdateInterval()来改变）。).
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", timeZone*3600, 60000);

void setup() {
  //串口初始化
  Serial.begin(9600);
  Serial.println("============bili fans monitor_v0.1===========");

  // SPI配置
  SPI.begin();

  // LCD配置
  lcd.init();                      // initialize the lcd
  lcd.backlight();
  
  // WIFI配置
  WiFi.mode(WIFI_STA);              //设置ESP8266工作模式为无线终端模式
  WiFi.begin(ssid, password);       //连接WiFi
  WiFiClient client;                //创建WiFi对象
  connectWiFi();                    //等待WiFi连接,连接成功打印IP
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.print(WiFi.localIP());

  // NTP配置
  timeClient.begin();

  //LED配置
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);
  pinMode(LED_4, OUTPUT);
}


// 连接WiFi
void connectWiFi(){
  Serial.print("Connecting WiFi...");
  while(WiFi.status() != WL_CONNECTED){
    delay(100);;
    Serial.print('.');
  }
  Serial.print("WiFi is ok!");
  lcd.setCursor(0,1);
  lcd.print("WiFi is ok!");
  lcd.setCursor(0,1);
  lcd.print("            ");
}


void HttpRequestFansData(){
  HTTPClient http_1;  //创建 HTTPClient 对象
  String URL = "http://api.bilibili.com/x/relation/stat?vmid=" + biliuid;
  http_1.begin(client, URL);    //配置请求地址
  int httpCode = http_1.GET();  //启动连接并发送HTTP请求
  Serial.print("HttpCode:");
  Serial.println(httpCode);
  if (httpCode == 200) {
    String responsePayload = http_1.getString();
    //Serial.println(responsePayload);
    
    DeserializationError error = deserializeJson(JSON_Buffer, responsePayload); //将JSON_Value字符串反序列化为JSON报文的格式,存入JSON_Buffer中
    if(error){    //若反序列化返回错误,则打印错误信息,然后中止程序
      Serial.println("deserializeJson JSON_Buffer is ERROR!!!");
      return ;
    }
    else{
      root = JSON_Buffer.as<JsonObject>();   //获取将JSON字符串经过显示类型转换为JSON对象的报文主体
      //serializeJsonPretty(JSON_Buffer, Serial);         //打印格式化后的JSON报文
      long code = root["code"];
      if (code == 0) {
        long fans = root["data"]["follower"];
        Serial.print("Fans:");
        Serial.println(fans);
        lcd.setCursor(0,2);
        lcd.print("Fans:");
        lcd.setCursor(5,2);
        lcd.print(fans);
        //判断粉丝增减
        fans_add = fans - fans0;
        if (fans_add == 0){
          lcd.setCursor(0,3);
          lcd.print("               ");         
          digitalWrite(LED_1, LOW);    // 低电平       
          digitalWrite(LED_2, LOW);    // 低电平       
          digitalWrite(LED_3, LOW);    // 低电平       
          digitalWrite(LED_4, LOW);    // 低电平
        }else if(fans_add > 0){
          Serial.print("+");
          Serial.println(fans_add);
          lcd.setCursor(0,3);
          lcd.print("+");
          lcd.print(fans_add);
          if (fans_add == 1){
            digitalWrite(LED_1, HIGH);   // 高电平
          }else if (fans_add == 2){
            digitalWrite(LED_1, HIGH);   // 高电平
            delay(100);
            digitalWrite(LED_2, HIGH);   // 高电平
          }else {
            digitalWrite(LED_1, HIGH);   // 高电平
            delay(100);
            digitalWrite(LED_2, HIGH);   // 高电平
            delay(100);
            digitalWrite(LED_3, HIGH);   // 高电平
          }
        }else{
          Serial.print("-");
          Serial.println(fans_add);
          lcd.setCursor(0,3);
          lcd.print(fans_add);
          digitalWrite(LED_4, HIGH);   // 高电平
        }
        fans0 = fans;
      }else{
        Serial.print("error:请求错误"); //请求错误,可能是UID错误
      }    
    }
  }else{
    Serial.print("error:http无法连接"); //http无法连接，请检查网络是否通常。也可能是API url更新。
    String responsePayload = http_1.getString();
    Serial.println(responsePayload);
  }
}


void NTPRequestNetworkTime(){
  timeClient.update();
  String net_time = timeClient.getFormattedTime();
  Serial.println(net_time);
  lcd.setCursor(6,0);
  lcd.print(net_time);
}


void loop() {
  // 如果ESP8266已连接WiFi，则发送HTTP请求
  if (WiFi.status() == WL_CONNECTED)
  {
    HttpRequestFansData();
    NTPRequestNetworkTime();
    delay(REFRUSH_TIME);
  }else{
    connectWiFi();
  }
}
