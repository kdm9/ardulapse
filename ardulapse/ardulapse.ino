// Copyright (c) 2017 Kevin Murray <kdmfoss@gmail.com>:w
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.


#include <ArduCAM.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "memorysaver.h"

#define DELAY_MSEC  10000
#define PIN_CS_SD   4
#define PIN_CS_CAM  7
#define SERIAL_SPD  115200
// #define POWERSAVE
#define PROPER_JPEG
#define NAME_BY_SERIES

#define FNAME_BUFSIZE 32
#define IMG_BUFSIZE 256

/* WIRING:
Camera:
SCL, SDA -> SCL, SDA
MOSI, MISO, SCK -> 11, 12, 13
CS -> PIN_CS_CAM

SD card:
MOSI, MISO, SCK -> 11, 12, 13
CS -> PIN_CS_SD
*/


#ifdef POWERSAVE
#define PWRDOWN(m) m.set_bit(ARDUCHIP_GPIO,GPIO_PWDN_MASK)
#define PWRUP(m) m.clear_bit(ARDUCHIP_GPIO,GPIO_PWDN_MASK)
#else
#define PWRDOWN(m) while(0)
#define PWRUP(m) while(0)
#endif


ArduCAM myCAM(OV5642, PIN_CS_CAM);

void setup()
{
    uint8_t vid, pid;
    Wire.begin();
    Serial.begin(SERIAL_SPD);
    Serial.println(F("ArduCAM Start!"));
    //set the CS as an output:
    pinMode(PIN_CS_CAM, OUTPUT);
    pinMode(PIN_CS_SD, OUTPUT);

    // initialize SPI:
    SPI.begin();
    while(1) {
        //Check if the ArduCAM SPI bus is OK
        myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
        if (myCAM.read_reg(ARDUCHIP_TEST1) != 0x55) {
            Serial.println(F("SPI interface Error!"));
            delay(1000);
            continue;
        } else {
            Serial.println(F("SPI interface OK."));
            break;
        }
    }

    //Initialize SD Card
    while(!SD.begin(PIN_CS_SD)) {
        Serial.println(F("SD Card Error!"));
        delay(1000);
    }
    Serial.println(F("SD Card detected."));

    while(1) {
        //Check if the camera module type is OV5642
        myCAM.wrSensorReg16_8(0xff, 0x01);
        myCAM.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
        myCAM.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
        if((vid != 0x56) || (pid != 0x42)) {
            Serial.println(F("Can't find OV5642 module!"));
            delay(1000);
            continue;
        } else {
            Serial.println(F("OV5642 detected."));
            break;
        }
    }
    myCAM.set_format(JPEG);
    myCAM.InitCAM();
    myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);   //VSYNC is active HIGH
    myCAM.OV5642_set_JPEG_size(OV5642_2592x1944);
    //myCAM.OV5642_set_JPEG_size(OV5642_320x240);
    myCAM.write_reg(ARDUCHIP_FRAMES,0x00);
    PWRDOWN(myCAM);
}

#ifdef NAME_BY_SERIES
static uint32_t imgn = 0;
#endif

void loop()
{
    char filename[FNAME_BUFSIZE] = "";
    byte buf[IMG_BUFSIZE];
    uint32_t bufpos = 0;
    uint32_t time_ms = millis();
    uint32_t length = 0;
    File outFile;

    Serial.print(F("\n\nBEGIN at "));
    Serial.println(time_ms/1000, DEC);

    // Turn on camera
    PWRUP(myCAM);
    myCAM.clear_fifo_flag();

    Serial.print(F("Start capture -- "));
    // Prepare for capture
    memset(buf, 0, IMG_BUFSIZE);
    myCAM.flush_fifo();
    // Capture
    myCAM.start_capture();
    while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
    Serial.println(F("Done."));

    length = myCAM.read_fifo_length();
    Serial.print(F("The fifo length is "));
    Serial.print(length, DEC);
    if (length >= MAX_FIFO_SIZE) { //384K
        Serial.println(F(" -- Over size."));
        return ;
    } else if (length == 0 ) { //0 kb
        Serial.println(F(" -- Size is 0."));
        return ;
    }
    Serial.println(F(" -- OK"));

    //Construct a file name
#ifdef NAME_BY_SERIES
    itoa(++imgn, filename, 10);
#else
    itoa((int)(time_ms / 1000), filename, 10);
#endif
    strcat(filename, ".jpg");

    //Open the new file
    Serial.print(F("Open "));
    Serial.print(filename);
    outFile = SD.open(filename, O_WRITE | O_CREAT | O_TRUNC);
    if(!outFile) {
        Serial.println(F(" -- Failed"));
        delay(1000);
        return;
    }
    Serial.println(F(" -- OK"));

    // Write image
    Serial.print(F("Saving image -- "));
    uint32_t remaining = length;
    uint32_t wrote = 0;
#ifdef PROPER_JPEG
    // Find JPG header
    myCAM.CS_LOW();
    myCAM.set_fifo_burst();
    while (remaining > 0) {
        if (buf[0] == 0xFF && buf[1] == 0xD8) break;
        buf[0] = buf[1];
        buf[1] = SPI.transfer(0); remaining--;
        bufpos = 2;
    }
    if (remaining == 0) {
        Serial.println("Invalid JPEG/JIF!");
        return;
    }
#endif
    while (remaining) {
        myCAM.CS_LOW();
        myCAM.set_fifo_burst();
        while (bufpos < IMG_BUFSIZE && remaining > 0) {
            buf[bufpos] =  SPI.transfer(0); remaining--;
            #ifdef PROPER_JPEG
            if (buf[bufpos-1] == 0xFF && buf[bufpos] == 0xD9) break;
            #endif
            bufpos++;
        }
        myCAM.CS_HIGH();
        if (wrote % 1024 == 0) {
            Serial.print(F("\033[1G\033[2KSaving image -- "));
            Serial.print(wrote / 1024, DEC);
            Serial.print(F("kb"));
        }
        wrote += bufpos;
        outFile.write(buf, bufpos);
        bufpos = 0;
    }
    outFile.close();
    Serial.println(F("\033[1G\033[2KSaved sucessfully"));

//    myCAM.clear_fifo_flag();
//    myCAM.flush_fifo();
    PWRDOWN(myCAM);

    int32_t to_delay = DELAY_MSEC - (millis() - time_ms);
    if (to_delay > 0) delay(to_delay);
}
