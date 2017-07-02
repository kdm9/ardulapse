#include <SD.h>
#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include "memorysaver.h"

#if defined(__arm__)
#include <itoa.h>
#endif

#define CS_PIN_SD       4  // SD
#define CS_PIN_CAMERA   10 // CAM
#define BUFFER_SIZE     512

ArduCAM CAM(OV5642, CS_PIN_CAMERA);

void setup()
{
    delay(1000);
    // Init i2c
    Wire.begin();


    // Init serial
    delay(1000);
    Serial.begin(9600);
    Serial.println("Initialising ardulapse!");


    // Set chip select pins to output mode
    delay(100);
    pinMode(10, OUTPUT); // SPI slave override
    pinMode(CS_PIN_CAMERA, OUTPUT);
    pinMode(CS_PIN_SD, OUTPUT);
    // Turn off both devices
    digitalWrite(CS_PIN_CAMERA, HIGH);
    digitalWrite(CS_PIN_SD, HIGH);

    // initialize SPI:
    delay(100);
    SPI.begin();

    /***************
     * CAMERA INIT *
     ***************/
    Serial.println("Init Camera...");

    CAM.CS_LOW();
    // Check if the ArduCAM SPI bus is OK
    delay(1000);
    CAM.write_reg(ARDUCHIP_TEST1, 0x55);
    if (CAM.read_reg(ARDUCHIP_TEST1) != 0x55) {
        Serial.println("SPI interface Error!");
        while(1);
    }
    Serial.println("    -- SPI OK");

    //Check if the camera module type is OV5642
    delay(500);
    uint8_t vid, pid;
    CAM.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
    CAM.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
    if ((vid != 0x56) || (pid != 0x42)) {
        Serial.println("ERR: Can't find OV5642 module!");
        while(1);
    }
    Serial.println("    -- OV5642 detected, configuring");

    //Change to JPEG capture mode and initialize the OV5642 module
    delay(100);
    CAM.set_format(JPEG);
    CAM.InitCAM();
    CAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);
    CAM.OV5642_set_JPEG_size(OV5642_2592x1944);
    CAM.clear_fifo_flag();
    CAM.write_reg(ARDUCHIP_FRAMES, 0x00);
    CAM.set_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK);
    Serial.println("    -- OV5642 configured");

    //Initialize SD Card
    delay(100);
    if (!SD.begin(CS_PIN_SD)) {
        Serial.println("SD Card Error");
        while(1);
    }
    Serial.println("    -- SD Card detected!");
}

int jpeg_end(byte *buf, unsigned index)
{
    // Check if last two bytes in buf are 0xFFD9, the JPEG EOF marker
    if (index < 1 || index > BUFFER_SIZE - 1) return 0;
    if (buf[index - 1] != 0xFF) return 0;
    if (buf[index] != 0xD9) return 0;
    return 1;
}

static unsigned img_count = 0;
void loop()
{
    char img_fname[20] = "img.jpg";
    File img_file;
    byte buf[BUFFER_SIZE];
    unsigned index = 0;
    byte res = 0;
    int start_time = millis();

    delay(5000);

    CAM.clear_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK);
    //Flush the FIFO
    CAM.flush_fifo();
    //Clear the capture done flag
    CAM.clear_fifo_flag();
    //Start capture
    CAM.start_capture();
    Serial.print("Start Capture -- ");
    Serial.println(String(img_count++));

    while (!CAM.get_bit(ARDUCHIP_TRIG , CAP_DONE_MASK));

    Serial.println("Capture Done!");
    Serial.print("Outputting to : ");
    Serial.println(img_fname);
    //Open the new file
    if (SD.exists(img_fname)) SD.remove(img_fname);
    img_file = SD.open(img_fname, O_WRITE | O_CREAT | O_TRUNC);
    if (!img_file) {
        Serial.print("Open file '");
        Serial.print(img_fname);
        Serial.println("' failed!");
        return;
    }

    CAM.CS_LOW();
    CAM.set_fifo_burst();

    //Read JPEG data from FIFO
    while (!jpeg_end(buf, index)) {
        res = SPI.transfer(0x00);;
        //Write image data to buffer if not full
        if (index == BUFFER_SIZE) {
            Serial.print(".");
            //Write 256 bytes image data to file
            CAM.CS_HIGH();
            digitalWrite(CS_PIN_SD, LOW);
            img_file.write(buf, 256);
            digitalWrite(CS_PIN_SD, HIGH);
            CAM.CS_LOW();
            CAM.set_fifo_burst();
            index = 0;
        }
        buf[index++] = res;
    }
    CAM.CS_HIGH();
    //Write the remain bytes in the buffer
    digitalWrite(CS_PIN_SD, LOW);
    if (index > 0) {
        img_file.write(buf, index);
    }
    //Close the file
    img_file.close();
    digitalWrite(CS_PIN_SD, HIGH);

    //Clear the capture done flag
    CAM.clear_fifo_flag();
    CAM.set_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK);

    Serial.print("Total time used:");
    Serial.print(millis() - start_time, DEC);
    Serial.println(" millisecond");
}
