#include <ArduCAM.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "memorysaver.h"

#define PIN_CS_SD   4
#define PIN_CS_CAM  7
#define SERIAL_SPD  115200

ArduCAM myCAM(OV5642, PIN_CS_CAM);

void setup()
{
    uint8_t vid, pid;
    Wire.begin();
    Serial.begin(SERIAL_SPD);
    Serial.println(F("ArduCAM Start!"));
    //set the CS as an output:
    pinMode(PIN_CS_CAM, OUTPUT);

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
    myCAM.write_reg(ARDUCHIP_FRAMES,0x00);
    myCAM.set_bit(ARDUCHIP_GPIO,GPIO_PWDN_MASK);
    delay(1000);
}

void loop()
{
    char filename[32];
    byte buf[256];
    static int i = 0;
    static int k = 0;
    uint8_t temp = 0,temp_last=0;
    uint32_t length = 0;
    bool is_header = false;
    File outFile;
    myCAM.clear_bit(ARDUCHIP_GPIO,GPIO_PWDN_MASK);
    //Flush the FIFO
    myCAM.flush_fifo();
    //Clear the capture done flag
    myCAM.clear_fifo_flag();
    //Start capture
    myCAM.start_capture();
    Serial.println(F("start Capture"));
    // Wait for capture to finish
    while(!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
    Serial.println(F("Capture Done."));
    length = myCAM.read_fifo_length();
    Serial.print(F("The fifo length is :"));
    Serial.println(length, DEC);
    if (length >= MAX_FIFO_SIZE) { //384K
        Serial.println(F("Over size."));
        return ;
    }
    if (length == 0 ) { //0 kb
        Serial.println(F("Size is 0."));
        return ;
    }
    //Construct a file name
    k = k + 1;
    itoa(k, filename, 10);
    strcat(filename, ".jpg");
    //Open the new file
    outFile = SD.open(filename, O_WRITE | O_CREAT | O_TRUNC);
    if(!outFile) {
        Serial.println(F("File open faild"));
        return;
    }
    Serial.print(F("Saving to: "));
    Serial.println(filename);
    myCAM.CS_LOW();
    myCAM.set_fifo_burst();
/*
    while ( length-- ) {
        temp_last = temp;
        temp =  SPI.transfer(0x00);
        //Read JPEG data from FIFO
        if ( (temp == 0xD9) && (temp_last == 0xFF) ) { //If find the end ,break while,
            buf[i++] = temp;  //save the last  0XD9
            //Write the remain bytes in the buffer
            myCAM.CS_HIGH();
            outFile.write(buf, i);
            //Close the file
            outFile.close();
            Serial.println(F("Image save OK."));
            is_header = false;
            i = 0;
        }
        if (is_header == true) {
            //Write image data to buffer if not full
            if (i < 256)
                buf[i++] = temp;
            else {
                //Write 256 bytes image data to file
                myCAM.CS_HIGH();
                outFile.write(buf, 256);
                i = 0;
                buf[i++] = temp;
                myCAM.CS_LOW();
                myCAM.set_fifo_burst();
            }
        } else if ((temp == 0xD8) & (temp_last == 0xFF)) {
            is_header = true;
            buf[i++] = temp_last;
            buf[i++] = temp;
        }
    }
*/
    myCAM.set_bit(ARDUCHIP_GPIO,GPIO_PWDN_MASK);
    delay(10000);
}


