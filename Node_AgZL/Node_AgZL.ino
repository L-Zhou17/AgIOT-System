#include <WiFi.h>
#include <Wire.h>
#include <FS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SSD1306.h"
#include "images.h"
#include "EEPROM.h"
#define EEPROM_SIZE 64

int sht = 0x40;
#define humidity       0xf5
#define temperature    0xf3
int BH1750address = 0x23;
byte buffx[2];
byte buff[2];
char p1[10];
char p2[10];
char p3[10];
float b,t,h;

SSD1306  display(0x3c, 4, 15);
typedef void (*Demo)(void);

String backPage = "<!DOCTYPE HTML><html><center><h1>Success</h1><br><a href=\"/conf\">Go back</a></center></html>";
String ssid = "";
String pswd = "";
String svip = "";
String port = "";
char buf[50];
String list;
String list2;
///////////////////////    WiFi config   /////////////////////////
const char *localssid = "ESP32WiFiName";
const char *password = "12345678";
//////////////////////////////////////////////////////////////////

AsyncWebServer server(80);
WiFiClient client;

void setup()
{
  Serial.begin(115200);
  Wire.begin(4, 15);
  delay(10);
  Oled_Logo();
  EEPROM_init();
  bool activated = Node_Init_chk();
  
  char str1[30];
  char str2[30];
  sprintf(str1,"SSID:%s",ssid);
  sprintf(str2,"Password:%s",pswd);
  display.clear();
  drawFont(str1,str2);
  display.display();
  delay(2000);
  
  APscan();
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid.c_str(), pswd.c_str());
  int iii=0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    iii+=1;
    Serial.print(".");
    if(iii>20) break;
  }
  if(WiFi.status() != WL_CONNECTED)
    Serial.print("Network not found.");
  else
    Serial.print("Network connected.");

  IPAddress softLocal(192,168,4,1);  
  IPAddress softGateway(192,168,4,1);
  IPAddress softSubnet(255,255,255,0);
  WiFi.softAPConfig(softLocal, softGateway, softSubnet);  
  WiFi.softAP(localssid, password);

  /////////////////////////////////////////////////////////////////////////////
  server.on("/wel", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Welcome, Agriserver!");
  });
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Accessed.");
    request->send(200, "text/html", SensorPage());
  });
  server.on("/conf", HTTP_GET, [](AsyncWebServerRequest *request){
    int paramsNr = request->params();
    if(paramsNr == 0)  request->send(200, "text/html", prepareHtmlPage());
    else
    {
        request->send(200, "text/html", backPage); 
        EEPROM_clear();
        ssid = "";
        pswd = "";
        svip = "";
        port = "";
        for(int i=0;i<paramsNr;i++)
        { 
            AsyncWebParameter* p = request->getParam(i);
            if(p->name()=="select") ssid += p->value();
            if(p->name()=="pswd") pswd += p->value();
            if(p->name()=="svip") svip += p->value();
            if(p->name()=="port") port += p->value();
        }
        EEPROM_write_Flag();
        EEPROM_write();
        delay(10);
        ESP.restart();
    }
  });
  server.begin();
  Serial.println("Init OK");
}

void loop()
{
    const uint16_t pport = port.toInt();
    char aa[50];
    b= BH1750read();
    t= SHT2xread(temperature);
    h= SHT2xread(humidity);
    char a1[20];
    char b1[20];
    char c1[20];
    sprintf(a1,"Temp  %.2f C",t);
    sprintf(b1,"Humi  %.2f %%",h);
    sprintf(c1,"Light  %.2f Lx",b);
    sprintf(aa,"$AgGP,THL,ESP32,9,1,99,%.2f,%.2f,%.2f,#",t,h,b);
    display.clear();
    drawFontFaceDemo(a1,b1,c1);
    display.display();
    int jj=0;
    if(WiFi.status() != WL_CONNECTED) {
        delay(7000);
        char str1[20];
        char str2[20];
        sprintf(str1,"SSID:%s",ssid);
        sprintf(str2,"was not available!");
        display.clear();
        drawFont(str1,str2);
        display.display();
        delay(3000);
        return;
    }
    if(!client.connected())
    {
      if (!client.connect(svip.c_str(),pport )) 
      {
        Serial.println("connection failed");
        Serial.println("wait 5 sec...");
        delay(5000);
        return;
      }
    }
    else
    {
      client.println(aa);
      delay(30000);
    }
}

void EEPROM_init()
{
  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM"); delay(1000000);
  }
}
void EEPROM_clear()
{
  char clr='\0';
  for (int i = 1; i < EEPROM_SIZE; i++)
  {
    EEPROM.write(i, clr);
  }
  EEPROM.commit();
}
void EEPROM_read()
{
  int i = 1;
  char temp;
  while(i<EEPROM_SIZE)
  {
    temp = EEPROM.read(i);
    if(temp == '\n') break;
    ssid += temp;
    i++;
  }
  i++;
  while(i<EEPROM_SIZE)
  {
    temp = EEPROM.read(i);
    if(temp == '\n') break;
    pswd += temp;
    i++;
  }
  i++;
  while(i<EEPROM_SIZE)
  {
    temp = EEPROM.read(i);
    if(temp == '\n') break;
    svip += temp;
    i++;
  }
  i++;
  while(i<EEPROM_SIZE)
  {
    temp = EEPROM.read(i);
    if(temp == '\0') break;
    port += temp;
    i++;
  }
}
void EEPROM_write()
{
  String S_sum = ssid +"\n"+ pswd +"\n"+ svip +"\n"+ port;
  int S_len = S_sum.length();
  S_sum.toCharArray(buf, S_len+1);
  for (int i = 1; i <= S_len; i++)
  {
    EEPROM.write(i, buf[i-1]);
  }
  EEPROM.commit();
}
void EEPROM_write_Flag()
{
  EEPROM.write(0, 'A');
  EEPROM.commit();
}
bool Node_Init_chk()
{
  char temp;
  ssid = "";
  pswd = "";
  temp = EEPROM.read(0);
  if(temp == 'A') 
  {
    Serial.println("Activated.");
    EEPROM_read();
  }
  else
  {
    Serial.println("New Node. Use default: ");
    ssid = "default SSID";
    pswd = "default psd";
    svip = "default IP";
    port = "default Port";
  }
  Serial.println(ssid);
  Serial.println(pswd);
  Serial.println(svip);
  Serial.println(port);
}
void rst() {
  pinMode(16,OUTPUT);
  digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
  delay(50); 
  digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high
}
void drawFontFaceDemo(char *aa,char *bb,char *cc) {
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(5, 5, aa);
    display.setFont(ArialMT_Plain_10);
    display.drawString(5, 17, bb);
    display.setFont(ArialMT_Plain_10);
    display.drawString(5, 29, cc);
}
void drawFont(char *aa,char *bb) {
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(5, 5, aa);
    display.setFont(ArialMT_Plain_10);
    display.drawString(5, 17, bb);
}
void drawImageDemo() 
{
    display.drawXbm(34, 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
}
void Oled_Logo()
{
  rst();
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.clear();
  drawImageDemo();
  display.display();
  delay(2000);
}
void APscan()
{
    list ="";
    list2="";
    int n = WiFi.scanNetworks(false,true,false,600);
    if (n == 0) {
        list="";
        list2+= n;
        list2+=" networks found<br>";
    } else {
        list2+= n;
        list2+=" networks found<br>";
        for (int i = 0; i < n; ++i) {
            list+="<option value=\"";
            list+= WiFi.SSID(i);
            list+="\">";
            list+=WiFi.SSID(i);
            list+=" (";
            list+=WiFi.RSSI(i);
            list+=")";
            list+="</option>";
            delay(1);
        }
    }
}
String SensorPage()
{
  b= BH1750read();
  t= SHT2xread(temperature);
  h= SHT2xread(humidity);
  sprintf(p1,"%.2f",t);
  sprintf(p2,"%.2f",h);
  sprintf(p3,"%.2f",b);
  String snPage =
     String("<!DOCTYPE HTML>")+ 
      "<html><style>.ctn {max-width: 340px;margin: 0 auto;position: relative;padding: 20px;border-radius: 5px;box-shadow: 0 5px 50px rgba(0,0,0,0.4);text-align: left;color:#00f}</style>"+
      "<div class = \"ctn\"><center><h2>Sensor List</h2></center>"+
      "<center><table border=\"0\">"+
      "  <tr align=\"left\" valign=\"top\"><td>Air-T:</td><td>"+p1+"</td><td>C</td></tr>"+
      "  <tr align=\"left\" valign=\"top\"><td>Air-RH:</td><td>"+p2+"</td><td>%</td></tr>"+
      "  <tr align=\"left\" valign=\"top\"><td>Light:</td><td>"+p3+"</td><td>Lx</td></tr>"+
      "</table></center></div><center><a href=\"/conf\">Settings</a></center></html>";
  return snPage;
}
String prepareHtmlPage()
{
  //APscan();
  String htmlPage =
     String("<!DOCTYPE HTML>")+ 
      "<html><head><meta charset=UTF-8\"><title>Nodes</title><style>"+
      "body{font-family: Arial,sans-serif;font-size: 25px;}"+
      "input,select,button{font-family:inherit;font-size:inherit;margin: 10px auto;}"+
      ".container {width: 85%;margin: 10px auto;background: white;}"+
      "input.sl,select{width: 250px;}"+
      "input.bt{border:none;line-height:2em;color:#fff;width: 100px;background:green;}"+
      ".footer{width: 100%;height:30px;}</style></head>"+
      "<body style=\'background:#cccccc\'>"+
      "<center><div class=\"container\">"+
      " <h2 id=\"idv\" style=\"color:#000\">NodeConfig.</h2><form action=""><center>"+
      "    <table><tbody>"+
      "       <tr><td>WiFi-SSID</td><td align=\"right\"><input class=\"sl\" id =\"select\" name=\"select\" type=\"text\" value=\""+
      ssid +
      "\" maxlength=32></input></td></tr>"+
      "       <tr><td>Password&nbsp;&nbsp;</td><td align=\"right\"><input class=\"sl\" name=\"pswd\" type=\"text\" value=\""+
      pswd +
      "\" maxlength=32></input></td></tr>"+
      "       <tr><td>Server IP</td><td align=\"right\"><input class=\"sl\" name=\"svip\" type=\"text\" value=\""+
      svip +
      "\" maxlength=32></input></td></tr>"+
      "       <tr><td>Port</td><td align=\"right\"><input class=\"sl\" name=\"port\" type=\"text\" value=\""+
      port +
      "\" maxlength=32></input></td></tr>"+
      "   </tbody></table>"+
      "   <center><label>- - - - - - - - - - - - - - - - - - - - - - - - - - - -</label></center>"+
      "   <center><label style=\"color:#f00\">"+
      list2 +
      "</label></center>"+
      "   <select id=\"wifi\" onchange=\"slch()\">"+
      list +
      "</select></center>"+
      " <div><input class=\"bt\" type=\"submit\" value=\"Submit\"></input><center><a href=\"/\">cancel</a></center></center></div></form></div></center>"+
      "<div class=\"footer\"><center>"+
      "<font color=\"#111111\" size=\"3\">Version 1.0 &nbsp;&nbsp; Copyright &copy; 2018&nbsp;Zhejiang University. All rights reserved.</center></div></body>"+
      "<script stype=\"text/javascript\">function slch(){var objS = document.getElementById(\"wifi\");document.getElementById(\'select\').value = objS.options[objS.selectedIndex].value;}</script></html>"
  +"\r\n";
  return htmlPage;
}
double SHT2xread(int temp)
{
  int i=0;
  long  val=0;
  Wire.beginTransmission(sht);
  Wire.write(temp);
  Wire.endTransmission();  
  delay(200);
  Wire.beginTransmission(sht);
  Wire.requestFrom(sht, 2);
  while(Wire.available())
  {
    buff[i] = Wire.read();
    i++;
  }
  Wire.endTransmission();
  val=((buff[0]<<8)|buff[1]);
  val &= ~0x0003;
  if(temp==0xf3)
    return -46.85+175.72/65536.00*(double)val;
  else
    return -6.0+125.0/65536.00*(double)val;
}
float BH1750read()
{
  int i=0;
  float  val=0;
  //开始I2C读写操作
  Wire.beginTransmission(BH1750address);
  Wire.write(0x10);//1lx reolution 120ms//发送命令
  Wire.endTransmission();  
  
  delay(200);
  //读取数据
  Wire.beginTransmission(BH1750address);
  Wire.requestFrom(BH1750address, 2);
  while(Wire.available()) //
  {
    buffx[i] = Wire.read();  // receive one byte
    i++;
  }
  Wire.endTransmission();
  if(2==i)
  {
   val=((buffx[0]<<8)|buffx[1])/1.2;
   if (val>9999.99)val=9999.99;
  }
  return val;
}
