/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini
 
   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
   And has a characteristic of: beb5483e-36e1-4688-b7f5-ea07361b26a8
 
   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.
 
   A connect hander associated with the server starts a background task that performs notification
   every couple of seconds.
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
 

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
 
// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
 
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
 
 
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };
 
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

///////////////////////////////////////////////////////////////////////////////

/// 温度計

#include "max6675.h"

int thermoDO = 12;
int thermoCS = 15;
int thermoCLK = 14;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);


///////////////////////////////////////////////////////////////////////////////

/// OLED

#include <Arduino.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // Adafruit Feather ESP8266/32u4 Boards + FeatherWing OLED


///////////////////////////////////////////////////////////////////////////////


void setup() {
  Serial.begin(115200);

  u8g2.begin();
  // フォントのデータがどこにあるか。よくわからない。
  // https://github.com/olikraus/u8g2/wiki/fntlistall#16-pixel-height のを指定してもダメな場合がある
  //u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.setFont(u8g2_font_ncenB14_tf);

  // Create the BLE Device
  BLEDevice::init("K-Thermometer");
 
  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
 
  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
 
  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );
 
  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());
 
  // Start the service
  pService->start();
 
  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

///////////////////////////////////////////////////////////////////////////////

// 過去60秒間の温度データ
float temperature_logs[60];
int log_pos = 0;  // 次が何番目か

#define OLED_STRING_LINE_SPACE 4

void loop() {

  unsigned long time_millis = 0;
  time_millis = millis();

  // データを生成
  {
    unsigned int second = (time_millis / 1000) % 60;
    unsigned int minute = (time_millis / 1000 / 60) % 60;
    unsigned int hour = (time_millis / 1000 / 3600) % 100;

    // 温度
    float temperature = thermocouple.readCelsius();

    // ログ
    temperature_logs[log_pos % 60] = temperature;
    log_pos++;

    // 送信データ
    char send_buffer[32];
    sprintf(send_buffer, "%02d:%02d:%02d,%3.1f\n", hour, minute, second, temperature);
    Serial.printf(send_buffer);

    // 表示
    char disp_time_buffer[256];
    sprintf(disp_time_buffer, "%02d:%02d:%02d", hour, minute, second);

    char disp_current_buffer[256];
    if (log_pos > 59) {
      sprintf(disp_current_buffer, "%3.1f C  %+3.1f", temperature, temperature - temperature_logs[log_pos % 60]);
    } else {
      sprintf(disp_current_buffer, "%3.1f C", temperature);
    }

    // https://github.com/olikraus/u8g2/wiki/u8g2reference#drawstr
    u8g2.clearBuffer();                 // clear the internal memory
    u8g2.drawStr(0, u8g2.getFontAscent(), disp_time_buffer);
    u8g2.drawStr(0, u8g2.getFontAscent() * 2 + OLED_STRING_LINE_SPACE, disp_current_buffer);
    u8g2.sendBuffer();                  // transfer internal memory to the display


    // BLE
    if (deviceConnected) {
      // https://github.com/espressif/arduino-esp32/blob/master/libraries/BLE/src/BLECharacteristic.h
      pCharacteristic->setValue(send_buffer);

      pCharacteristic->notify();
      //pCharacteristic->indicate();
    }

  }

  // なるべく1秒毎にする
  unsigned long time_spent = 0;
  time_spent = millis();
  delay(1000 - (time_millis - time_millis));
}