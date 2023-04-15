#include <UrlEncode.h>
#include <ArduinoJson.h>
#include <UniversalTelegramBot.h>
#include "SD.h"
#include "SPI.h"
#include "arduino_secrets.h"
#include "discord.h"
#include <ESPAsyncWebServer.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP_Mail_Client.h>

#define SD_CS 5
File file;
#define BOTtoken "5612269105:AAG4ZlXtKCibXmHHe2N1mtXyOPdi1VoucF8"
#define CHAT_ID "5751887235"
WiFiUDP ntpudp;
const long utcOffsetInSeconds = 25200;
NTPClient timeClient(ntpudp, "time.google.com", utcOffsetInSeconds);
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;
bool isMoreDataAvailable();
byte getNextByte();
int gassensorAnalog;
AsyncWebServer server(80);
SMTPSession smtp;

void smtpCallback(SMTP_Status status);

void MenulisDataFileSD(const char* pesanlog){
  file = SD.open("/KIR/LOG.txt", FILE_APPEND);
  if(!file){
        Serial.println("Gagal Membuka File");
        return;
    }
    if(file.println(pesanlog)){
        ;
    } else {
        Serial.println("Gagal menambah data");
    }
    file.close();
}
bool isMoreDataAvailable()
{
  return file.available();
}

byte getNextByte()
{
  return file.read();
}

String mongas() {
gassensorAnalog = analogRead(35);

    return String(gassensorAnalog);
  
}
void gas(){
    gassensorAnalog = analogRead(35);
    Serial.println(gassensorAnalog);
}

String LogInfo(){
  timeClient.update();
  return timeClient.UntukLogging() + "[INFO] ";
}

String LogWarning() {
  timeClient.update();
  return timeClient.UntukLogging() + "[WARNING] ";
}

String LogError() {
  timeClient.update();
  return timeClient.UntukLogging() + "[ERROR] ";
}

void finalLOG(String pesan) {
  Serial.println(pesan);
  MenulisDataFileSD(pesan.c_str());
}
void whatsapp(String pesan){
  String url = "https://api.callmebot.com/whatsapp.php?phone=6285755921415&text=" + urlEncode(pesan) + "&apikey=7266615";      
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpResponseCode = http.POST(url);
  if (httpResponseCode == 200){
    Serial.print("Pesan Whatsapp berhasil terkirim");
  }
  else{
    Serial.println("Error mengirim pesan");
    Serial.print("HTTP respon code: ");
    Serial.println(httpResponseCode);
  }
http.end();  
}

void finalPesan(String pesan){
    sendDiscord(pesan);
    whatsapp(pesan);
    bot.sendMessage(CHAT_ID, pesan, "");
}
void setup() {
  // put your setup code here, to run once:
  
  #ifdef ESP8266
    configTime(0, 0, "pool.ntp.org");      
    client.setTrustAnchors(&cert);
  #endif
  #ifdef ESP32
    client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  #endif
  connectWIFI();
  Serial.begin(9600);
  pinMode(22, OUTPUT);      
  SD.begin(SD_CS);
  timeClient.begin();  
    if (!SD.begin(SD_CS)) {
    Serial.println(+ "Gagal Memuat Kartu SD");
    return;
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("Tidak Ada Kartu SD");
    return;
  }
  
  if(!SD.exists("/KIR")){
    Serial.println("Folder tidak ditemukan");
    if(SD.mkdir("/KIR")){
      Serial.println("Folder Berhasil dibuat");
    }
    else{
      Serial.println("Folder Gagal Dibuat");
    }
  }
  if(!SD.exists("/KIR/LOG.txt")){
     Serial.println("LOG file tidak ditemukan");
    file = SD.open("/KIR/LOG.txt", FILE_WRITE);
    if(!file){
        Serial.println("Gagal membuka File");
        return;
    }
    if(file.print("LOG file telah dibuat")){
        Serial.println("LOG File telah di tulis");
    } else {
        Serial.println("LOG File Gagal di tulis");
    }
  }
server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SD, "/KIR/index.html");
  });
server.on("/gasdat", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", mongas().c_str());
}); 
smtp.debug(0);
smtp.callback(smtpCallback);
ESP_Mail_Session session;
session.server.host_name = "smtp.gmail.com";
  session.server.port = 465;
  session.login.email = "jokolopo.2007@gmail.com";
  session.login.password = "jsaijdwalmsda";
  session.login.user_domain = "";
  SMTP_Message message;
  session.time.ntp_server = "time.google.com,time.nist.gov";
session.time.gmt_offset = 7;
session.time.day_light_offset = 0;
  message.addRecipient("Muhammad Zuhair Zuhdi","jokolopo.20@gmail.com");
   message.sender.name = "ESP";
  message.sender.email = "jokolopo.2007@gmail.com"; 
  message.subject = "Gas Sensor LPG";
  String htmlMsg = "<div style=\"color:#2f4468;\"><h1>ESP32 Report</h1><p>Gas LPG Terdeteksi dengan data " + String(gassensorAnalog) + ". </p></div>";
  message.html.content = htmlMsg.c_str();
  message.html.content = htmlMsg.c_str();
  message.text.charSet = "us-ascii";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
  if (!smtp.connect(&session))
return;
  delay(5000);
  server.begin();
  timeClient.update();   
  finalLOG(LogInfo() + "ESP32 Siap");
  finalPesan("Alat Menyala");  
}

void handleNewMessages(int numNewMessages) {  
  for (int i=0; i<numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;  
    }    
    String text = bot.messages[i].text;
    finalLOG(LogInfo() + text);
    String from_name = bot.messages[i].from_name; 
  finalLOG(LogInfo() + "menghandle pesan baru");
  finalLOG(LogInfo() + String(numNewMessages));
 if (text == "/start") {
      String welcome = "Hai, " + from_name + ".\n";
      welcome += "Command\n\n";
      welcome += "/localip untuk mendapatkan local ip esp32\n";
      welcome += "/log mendapatkan log file \n";
      welcome += "/gasstatus Mendapatkan status gas \n";
      bot.sendMessage(chat_id, welcome, "");
    }
    if(text == "/gasstatus"){
      gassensorAnalog = analogRead(35);
      bot.sendMessage(chat_id, String(gassensorAnalog), "");
    }
    if (text == "/localip") {
      bot.sendMessage(chat_id, localIp() , "");
    }
/*    next project
    if(text == "/log"){
      file = SD.open("/KIR/LOG.txt");
      if(file){
      String kirim = bot.sendMultipartFormDataToTelegram("sendDocument", "document", file.name() , "txt", CHAT_ID, file.size(), isMoreDataAvailable, getNextByte, nullptr, nullptr);
       if (kirim){
      finalLOG(LogInfo() + "Berhasil Mengirim " + file.name() + " file ke Telegram");
    }
    else{
      finalLOG(LogError() + "Gagal Mengirim Log file ke Telegram");
    }
      }
      else{
        finalLOG(LogError() + "Gagal membuka Log file");
      }        
    file.close();      
    }*/
    
    if(text == "/tesbuzzer"){ 
      bot.sendMessage(chat_id, "Tes Buzzer", "");  
      digitalWrite(22, HIGH);
      delay(5000);
      digitalWrite(22, LOW);        
    } 
    if(text == "/tespesan"){
      finalPesan("tes Pesan");
    }
  }    
}
 
        
void loop() {
  // Hanya Core 1, Core 0 tidak bisa digunakan di loop 
gas();  
    if (gassensorAnalog >= 4000){      
      finalPesan("Gas Terdeteksi dengan data " + String(gassensorAnalog));
      if (!MailClient.sendMail(&smtp, &message)){
Serial.println("Error mengirim Email, " + smtp.errorReason());
      }
      digitalWrite(22, HIGH);
      delay(8000);
      digitalWrite(22, LOW);      
    }
  if (millis() - lastTimeBotRan > botRequestDelay)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages)
    {
      Serial.println("Mendapatkan respon");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
ESP_MAIL_PRINTF("Free Heap: %d\n", MailClient.getFreeHeap());

smtp.sendingResult.clear();  
  delay(700);
}
void smtpCallback(SMTP_Status status)
{
Serial.println(status.info());

if (status.success())

{

Serial.println("----------------");

ESP_MAIL_PRINTF("Pesan berhasil dikirim: %d\n", status.completedCount());

ESP_MAIL_PRINTF("Pesan berhasil dikirim: %d\n", status.failedCount());

Serial.println("----------------\n");

struct tm dt;

for (size_t i = 0; i < smtp.sendingResult.size(); i++)

{

SMTP_Result result = smtp.sendingResult.getItem(i);

time_t ts = (time_t)result.timestamp;

localtime_r(&ts, &dt);

ESP_MAIL_PRINTF("Message No: %d\n", i + 1);

ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "berhasil" : "gagal");

ESP_MAIL_PRINTF("tanggal/wktu: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);

ESP_MAIL_PRINTF("email yang di tuju: %s\n", result.recipients);

}

Serial.println("----------------\n");

smtp.sendingResult.clear();

}

}
