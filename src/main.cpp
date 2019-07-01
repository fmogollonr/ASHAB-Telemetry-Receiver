#define SERIAL_RX_BUFFER_SIZE 255

#include "Arduino.h"
#define U8G2_USE_LARGE_FONTS
#include <U8g2lib.h>
#include <SPI.h>
#include <RH_RF95.h>
#include <SD.h>
#include "logo.xbm"
#include <WiFi.h>
#include <WebServer.h>

/* Put your SSID & Password */
const char* ssid = "ESP32";  // Enter SSID here
const char* password = "12345678";  //Enter Password here

#define DEBUG_SERIAL Serial

#define PKT_LEN		255

#define LED		25

// OLED
#define OLED_CLK 	22 // 15
#define OLED_DAT 	21
#define OLED_RST 	23 // 16
#define LOG_PATH "/lora_recv.log"

// LoRa
#define LORA_SS		18
#define LORA_INT	26

// INTERFACE
#define LINE8_0	8+1
#define LINE8_1	16+1
#define LINE8_2	24+1
#define LINE8_3 32+1
#define LINE8_4 40+1
#define LINE8_5 48+1
#define LINE8_6 56+1
#define LINE8_7 64+1


#define SD_CS 13
#define SD_SCK 14
#define SD_MOSI 15
#define SD_MISO 2

#define FONT_7t	u8g2_font_artossans8_8r
#define FONT_16n u8g2_font_logisoso16_tn

/* Put IP Address details */
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);


WebServer server(80);

uint8_t LED1pin = 4;
bool LED1status = LOW;

uint8_t LED2pin = 5;
bool LED2status = LOW;


// Objects
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ OLED_CLK, /* data=*/ OLED_DAT, /* reset=*/ OLED_RST);
RH_RF95 rf95(LORA_SS, LORA_INT);

// receive buffer
uint8_t buf[PKT_LEN];

int size=5;

String SendHTML(uint8_t led1stat,uint8_t led2stat){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>LED Control</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-on {background-color: #3498db;}\n";
  ptr +=".button-on:active {background-color: #2980b9;}\n";
  ptr +=".button-off {background-color: #34495e;}\n";
  ptr +=".button-off:active {background-color: #2c3e50;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>ESP32 Web Server</h1>\n";
  ptr +="<h3>Using Access Point(AP) Mode</h3>\n";
  
   if(led1stat)
  {ptr +="<p>LED1 Status: ON</p><a class=\"button button-off\" href=\"/led1off\">OFF</a>\n";}
  else
  {ptr +="<p>LED1 Status: OFF</p><a class=\"button button-on\" href=\"/led1on\">ON</a>\n";}

  if(led2stat)
  {ptr +="<p>LED2 Status: ON</p><a class=\"button button-off\" href=\"/led2off\">OFF</a>\n";}
  else
  {ptr +="<p>LED2 Status: OFF</p><a class=\"button button-on\" href=\"/led2on\">ON</a>\n";}

  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}

void handle_OnConnect() {
  LED1status = LOW;
  LED2status = LOW;
  Serial.println("GPIO4 Status: OFF | GPIO5 Status: OFF");
  server.send(200, "text/html", SendHTML(LED1status,LED2status)); 
}

void handle_led1on() {
  LED1status = HIGH;
  Serial.println("GPIO4 Status: ON");
  server.send(200, "text/html", SendHTML(true,LED2status)); 
}

void handle_led1off() {
  LED1status = LOW;
  Serial.println("GPIO4 Status: OFF");
  server.send(200, "text/html", SendHTML(false,LED2status)); 
}

void handle_led2on() {
  LED2status = HIGH;
  Serial.println("GPIO5 Status: ON");
  server.send(200, "text/html", SendHTML(LED1status,true)); 
}

void handle_led2off() {
  LED2status = LOW;
  Serial.println("GPIO5 Status: OFF");
  server.send(200, "text/html", SendHTML(LED1status,false)); 
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}



void setup()
{

	  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
  
  server.on("/", handle_OnConnect);
  server.on("/led1on", handle_led1on);
  server.on("/led1off", handle_led1off);
  server.on("/led2on", handle_led2on);
  server.on("/led2off", handle_led2off);
  server.onNotFound(handle_NotFound);
  
  server.begin();

	// initialize LED digital pin as an output.
	pinMode(LED, OUTPUT);

	// init SPI (ESP32)
	pinMode(18, OUTPUT);
	SPI.begin(5, 19, 27, 18);

	// init Display and show splash screen
	u8g2.begin();
	u8g2.drawXBM((128-logo_width)/2, (64-logo_height)/2, logo_width, logo_height, logo_bits);
	u8g2.sendBuffer();
	delay(3000);

	// simple interface
	u8g2.clearBuffer();
	u8g2.setFont(FONT_7t);
	u8g2.drawStr(2, LINE8_0,"ASHAB Receiver");
	u8g2.drawStr(2, LINE8_3,"Last RSSI:");
	u8g2.drawStr(2, LINE8_5,"Last Len:");
	u8g2.sendBuffer();

	// Serial port 
	DEBUG_SERIAL.begin(115200);
	//DEBUG_SERIAL.buffer(255);

	SPIClass sd_spi(HSPI);

	sd_spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

    if(!SD.begin(SD_CS, sd_spi)){
		u8g2.drawStr(2, LINE8_2,"SD NOK");
    }
	else
	{
		u8g2.drawStr(2, LINE8_2,"SD OK");
	}
	
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
		u8g2.drawStr(2, LINE8_2,"No SD card");
	}
	else
	{
		u8g2.drawStr(2, LINE8_2,"OTHER SD");
	}

	uint8_t buffer[size];
	memcpy(buffer, "lora", size);
	

	File test = SD.open(LOG_PATH, FILE_APPEND);
	 if (!test) {
            u8g2.drawStr(2, LINE8_2,"SD failed");
        } else {
            u8g2.drawStr(2, LINE8_2,"SD Append");
            test.write(buffer,32);
            //test.printf("\n\n");
            test.close();
}

	// Init LoRa
	if (!rf95.init())
	{
		DEBUG_SERIAL.println("#RF95 Init failed");
		u8g2.drawStr(2, LINE8_1,"LoRa not found");
		u8g2.sendBuffer();
	}
	else
	{
		DEBUG_SERIAL.println("#RF95 Init OK");  
		u8g2.drawStr(2, LINE8_1,"LoRa OK");
		u8g2.sendBuffer();
	}


	// receiver, low power	
	rf95.setFrequency(868.5);
	rf95.setTxPower(5,false);
	memset(buf, 0, sizeof(buf));

}




void loop()
{
	if (rf95.available())
	{
		// Should be a message for us now   
		uint8_t len = sizeof(buf);
		if (rf95.recv(buf, &len))
		{
			// packet received
			digitalWrite(LED, HIGH);
			// send it through serial port
			DEBUG_SERIAL.write(buf, len);
			digitalWrite(LED, LOW);
			// clear buffer
			memset(buf, 0, sizeof(buf));

			// show RSSI
			u8g2.drawStr(2, LINE8_4, "           ");
			u8g2.setCursor(2, LINE8_4);
			u8g2.print(rf95.lastRssi());
			// show len
			u8g2.drawStr(2, LINE8_6, "           ");
			u8g2.setCursor(2, LINE8_6);
			u8g2.print(len);
			u8g2.sendBuffer();

		}
		else
		{
			// failed packet?
			DEBUG_SERIAL.println("#Recv failed");
		}
	}

server.handleClient();
  if(LED1status)
  {digitalWrite(LED1pin, HIGH);}
  else
  {digitalWrite(LED1pin, LOW);}
  
  if(LED2status)
  {digitalWrite(LED2pin, HIGH);}
  else
  {digitalWrite(LED2pin, LOW);}
}


