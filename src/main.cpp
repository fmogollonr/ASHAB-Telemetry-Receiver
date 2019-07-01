#define SERIAL_RX_BUFFER_SIZE 255

#include "Arduino.h"
#define U8G2_USE_LARGE_FONTS
#include <U8g2lib.h>
#include <SPI.h>
#include <RH_RF95.h>
#include <SD.h>
#include "logo.xbm"

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

// Objects
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ OLED_CLK, /* data=*/ OLED_DAT, /* reset=*/ OLED_RST);
RH_RF95 rf95(LORA_SS, LORA_INT);

// receive buffer
uint8_t buf[PKT_LEN];

int size=5;

void setup()
{

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

}
