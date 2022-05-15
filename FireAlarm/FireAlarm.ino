/*
*针脚连接：
*A0-MQ2烟雾传感器AO
*D5-扬声器IO
*D6-火焰传感器DO
*/

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AliyunIoTSDK.h>

//Ticker计时器
Ticker ticker1; //读取A0引脚气体传感器数据
Ticker ticker2; ////读取并处理明火状态
Ticker ticker3; //明火报警计时器
Ticker ticker4; //读取并处理可燃气体状态
Ticker ticker5; //气体报警计时器

static WiFiClient espClient;
//填入从阿里云获取的设备ID
#define PRODUCT_KEY " "
#define DEVICE_NAME " "
#define DEVICE_SECRET " "
#define REGION_ID "cn-shanghai"

// 设置 wifi 信息
#define WIFI_SSID "名称"
#define WIFI_PASSWD "密码"
        
int val = 0;  //A0的数值
int speaker = D5; //扬声器针脚D5
int fire = D6;  //火焰传感器针脚D6
bool isfire = 0;  //是否有明火
bool issmoke = 0;   //是否有可燃气体

void setup() {
  Serial.begin(115200);
  //设置针脚模式
  pinMode(fire, INPUT);
  pinMode(speaker, OUTPUT);
  // 初始化 wifi
  wifiInit(WIFI_SSID, WIFI_PASSWD);

  // 初始化设备，发送设备信息，连接到阿里云
  AliyunIoTSDK::begin(espClient, PRODUCT_KEY, DEVICE_NAME, DEVICE_SECRET, REGION_ID);
  
  // 发送初始数据到阿里云
  AliyunIoTSDK::send("RSSI", 1); 
  AliyunIoTSDK::send("flameStatus", fireState()); //明火检测
  AliyunIoTSDK::send("smokeStatus", fireState()); //可燃气体检测
  AliyunIoTSDK::bindData("PowerSwitch", reCallback); //重置回调，用于解除明火警报
  //用ticker循环执行
  ticker1.attach(1.0, setval);  //每秒调用一次setval
  ticker2.attach(1.0, setdigFire);  
  ticker4.attach(1.0, setdigSmoke);
  
}

//暂不需要loop函数
void loop() {
  AliyunIoTSDK::loop();
}

//读取A0读数，在串口监视器输出数值，并发送到阿里云
void setval() {
  val = analogRead(A0);
  Serial.println(val);
  AliyunIoTSDK::send("RSSI", val);
}

//返回明火状态
bool fireState(){
  isfire=!digitalRead(fire);  //读取火焰传感器数据
  return isfire;
}

//返回可燃气体状态
bool smokestate(){
  if(val>=550){ //当读取A0的值大于550时视为检测到可燃气体，该值可根据实际测试调整
    issmoke=1;
    return issmoke;
  }else{
    issmoke=0;
    return issmoke;
  }
}

//读取并处理明火状态
void setdigFire(){
  if(fireState()==1){
    Serial.println("isfire=1");
    AliyunIoTSDK::send("flameStatus", fireState()); //发送明火状态为1
    ticker3.attach_ms(900,speakerON); //开启扬声器警报(明火)
  }else{
    Serial.println("isfire=0");
  }
}

//重置回调，app内按下重置按钮后执行
void reCallback(JsonVariant p){
  int PowerSwitch = p["PowerSwitch"];
   if (PowerSwitch == 1)
    {
      AliyunIoTSDK::send("flameStatus", 0);//将明火状态重置为0
      ticker3.detach(); //停止扬声器警报(明火)
    } 
}

//读取并处理可燃气体状态
void setdigSmoke(){
  if(smokestate()==1){
    Serial.println("issmoke=1");
    AliyunIoTSDK::send("smokeStatus", smokestate());  //发送气体状态为1
    ticker5.attach_ms(900,speakerON); //开启扬声器警报(气体)
  }else{
    Serial.println("issmoke=0");
    AliyunIoTSDK::send("smokeStatus", smokestate());  //发送气体状态为0
    if(isfire==0){
      ticker5.detach(); //停止扬声器警报(气体)
    } 
  }
}

//扬声器响警报
void speakerON(){
      tone(speaker, 950,800); //音高 时长   
}

// wifi 连接初始化 
void wifiInit(const char *ssid, const char *passphrase) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, passphrase);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("WiFi not Connect");
  }
  Serial.println("Connected to AP");
}
