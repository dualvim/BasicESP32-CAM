/*
* ESP32-CAM_NoSleep.ino     
* --> 
* -->
*/





/*****************************
** Included Header Files    **
*****************************/
#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "fd_forward.h"
#include "fr_forward.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "dl_lib.h"
#include "driver/rtc_io.h"
#include <EEPROM.h>            // read and write from flash memory
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>




/******************************************
** Constants and global scope variables  **  
******************************************/
// define the number of bytes you want to access
#define EEPROM_SIZE 1

// Pin numbers in CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Other constants
#define INTERVAL 60000 // Time interval (in milliseconds) between each picture

// Global scope variables
camera_config_t config; //Stores the camera configuration parameters
unsigned int pictureNumber = 0; //Picture number




/**************************
** Function prototypes   ** 
**************************/
void config_and_initialize_camera( void );
void init_microSD_card( void );
void take_picture( void );






/******************
** setup()       ** 
******************/
void setup() {
      // Disable the 'browout detector'
      WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 

      // Turn off the flash
      pinMode(4, INPUT);
      digitalWrite(4, LOW);
      //rtc_gpio_hold_dis(GPIO_NUM_4);
      
      // Mnitor serial
      Serial.begin(115200);
      delay(2000);
      Serial.println("Initializing...");

      Serial.print("Initializing the camera module...");
      config_and_initialize_camera();
      Serial.println("Ok!");

      //--> Inicializar o cartao MicroSD:
      Serial.print("Initializing  the MicroSD card module...");
      init_microSD_card();
      Serial.println("");

}





/****************
** loop()      **  
****************/
void loop() {
      // --> Take a picture:
      take_picture();

      // Wait '' milliseconds before taking another picture
      delay(INTERVAL);

}



/**************************
** Auxiliary Functions   **
**************************/
// --> 'config_and_initialize_camera()' function
void config_and_initialize_camera( void ){
      // --> Configuracoes da camera
      //camera_config_t config;
      config.ledc_channel = LEDC_CHANNEL_0;
      config.ledc_timer = LEDC_TIMER_0;
      config.pin_d0 = Y2_GPIO_NUM;
      config.pin_d1 = Y3_GPIO_NUM;
      config.pin_d2 = Y4_GPIO_NUM;
      config.pin_d3 = Y5_GPIO_NUM;
      config.pin_d4 = Y6_GPIO_NUM;
      config.pin_d5 = Y7_GPIO_NUM;
      config.pin_d6 = Y8_GPIO_NUM;
      config.pin_d7 = Y9_GPIO_NUM;
      config.pin_xclk = XCLK_GPIO_NUM;
      config.pin_pclk = PCLK_GPIO_NUM;
      config.pin_vsync = VSYNC_GPIO_NUM;
      config.pin_href = HREF_GPIO_NUM;
      config.pin_sscb_sda = SIOD_GPIO_NUM;
      config.pin_sscb_scl = SIOC_GPIO_NUM;
      config.pin_pwdn = PWDN_GPIO_NUM;
      config.pin_reset = RESET_GPIO_NUM;
      config.xclk_freq_hz = 20000000;
      config.pixel_format = PIXFORMAT_JPEG; 


      // --> Frame size:
      if(psramFound()){
            config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
            config.jpeg_quality = 10;
            config.fb_count = 2;
      } else {
            config.frame_size = FRAMESIZE_SVGA;
            config.jpeg_quality = 12;
            config.fb_count = 1;
      }

      // --> Inicialize Camera
      esp_err_t err = esp_camera_init(&config);
      if (err != ESP_OK) {
            Serial.printf("Camera init failed with error 0x%x", err);
            return;
      }
}




// --> 'init_microSD_card()' function
void init_microSD_card( void ){
      Serial.println("Starting SD Card");
      if(!SD_MMC.begin()){
            Serial.println("SD Card Mount Failed");
            return;
      }
      
      uint8_t cardType = SD_MMC.cardType();
      if(cardType == CARD_NONE){
            Serial.println("No SD Card attached");
            return;
      }
      Serial.println("Ok!");
}




// --> 'take_picture()' function
void take_picture( void ){
      camera_fb_t * fb = NULL;
      
      // Take Picture with Camera
      fb = esp_camera_fb_get();  
      if(!fb) {
            Serial.println("Camera capture failed");
            return;
      }
      // initialize EEPROM with predefined size
      EEPROM.begin(EEPROM_SIZE);
      pictureNumber = EEPROM.read(0) + 1;
      
      // Path where new picture will be saved in SD Card
      String path = "/pic_" + String(pictureNumber) +".jpg";
      
      fs::FS &fs = SD_MMC; 
      Serial.printf("Picture file name: %s\n", path.c_str());
  
      File file = fs.open(path.c_str(), FILE_WRITE);
      if(!file){
            Serial.println("Failed to open file in writing mode");
      } 
      else {
            file.write(fb->buf, fb->len); // payload (image), payload length
            Serial.printf("Saved file to path: %s\n", path.c_str());
            EEPROM.write(0, pictureNumber);
            EEPROM.commit();
      }
      file.close();
      esp_camera_fb_return(fb); 
      
      // Turns off the ESP32-CAM white on-board LED (flash) connected to GPIO 4
      digitalWrite(4, LOW);
}
