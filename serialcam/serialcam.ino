// Copyright (c) 2017 Kevin Murray <kdmfoss@gmail.com>:w
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.


#include <ArduCAM.h>
#include <Wire.h>
#include <SPI.h>
#include "memorysaver.h"

#define DELAY_MSEC  10000
#define PIN_CS_CAM  7
#define SERIAL_SPD  115200
#define POWERSAVE

/* WIRING:
Camera:
SCL, SDA -> SCL, SDA
MOSI, MISO, SCK -> 11, 12, 13
CS -> PIN_CS_CAM
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
    //set the CS as an output:
    pinMode(PIN_CS_CAM, OUTPUT);

    // initialize SPI:
    SPI.begin();
    while(1) {
        //Check if the ArduCAM SPI bus is OK
        myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
        if (myCAM.read_reg(ARDUCHIP_TEST1) != 0x55) {
            delay(1000);
            continue;
        } else {
            break;
        }
    }

    while(1) {
        //Check if the camera module type is OV5642
        myCAM.wrSensorReg16_8(0xff, 0x01);
        myCAM.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
        myCAM.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
        if((vid != 0x56) || (pid != 0x42)) {
            delay(1000);
            continue;
        } else {
            break;
        }
    }
    myCAM.set_format(JPEG);
    myCAM.InitCAM();
    myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);   //VSYNC is active HIGH
    myCAM.OV5642_set_JPEG_size(OV5642_640x480);
    myCAM.write_reg(ARDUCHIP_FRAMES,0x00);
    PWRDOWN(myCAM);
}

void loop()
{
    uint32_t time_ms = millis();
    uint32_t length = 0;

    // Turn on camera
    PWRUP(myCAM);

    // Prepare for capture
    myCAM.flush_fifo();
    myCAM.clear_fifo_flag();
    // Capture
    myCAM.start_capture();
    while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));

    length = myCAM.read_fifo_length();
    if (length >= MAX_FIFO_SIZE) { //384K
        return ;
    } else if (length == 0 ) { //0 kb
        return ;
    }

    myCAM.CS_LOW();
    myCAM.set_fifo_burst();
    for (size_t i = 0; i < length; i++) {
        Serial.write(SPI.transfer(0));
    }
    myCAM.CS_HIGH();
    PWRDOWN(myCAM);

    int32_t to_delay = DELAY_MSEC - (millis() - time_ms);
    if (to_delay > 0) delay(to_delay);
}
