#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include <assert.h>
#include <WiFiNINA.h>
#include "memorysaver.h"
#include "arduino_secrets.h"

#define MOTOR_L 1
#define MOTOR_R 2
#define MOTOR_L_PIN 4
#define MOTOR_R_PIN 5
#define MOTOR_L_PLUS 0
#define MOTOR_L_MINUS 1
#define MOTOR_R_PLUS 21
#define MOTOR_R_MINUS 20

//General stuff
bool debug = true;
bool two_cam = false;
void tests();

//Arducam stuff
const int CS1 = 6;
const int CS2 = 7;
ArduCAM Cam1(OV5642, CS1);
ArduCAM Cam2(OV5642, CS2);
int take_photo(char sel);
void init_arducam(ArduCAM* cam_nr);

//Motor stuff
void set_motor(char sel, int pwr_per);
void init_motor();
void test_motor();
void update_motor(int motor_plus, int motor_minus, int motor_dir, int pwr_per); 


//Wifi server stuff
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key index number (needed only for WEP)

int status = WL_IDLE_STATUS;
WiFiServer server(80); //port 80
void print_wifi_status();
void init_wifi();
void wifi_server_loop();
void match_com(String in);
void wifi_send(uint8_t * buff, size_t sze);


void setup() {
  // put your setup code here, to run once:
  init_motor();
  if (debug){
    Serial.begin(9600);
    while(!Serial);
    Serial.println("Starting MKRWifi1010!");
  }

  init_wifi();
  
  //Initializing for Arducam
  if (debug){
    Serial.println("Initializing Arducam!");
  }
  SPI.begin();
  
  //Check SPI connection for Cam 1
  digitalWrite(CS1, HIGH);
  digitalWrite(CS2, HIGH);

  //init_arducam(&Cam1);
  
  if (two_cam){
    //init_arducam(&Cam2);
  }
  if (debug){
    Serial.println("Setup done!");
  }
}

void loop() {
  //tests();

  wifi_server_loop();
}

void tests(){
  test_motor();
}

/*----------------Motor function declarations----------------*/

void set_motor(char sel, int pwr_per){
  assert(pwr_per <= 100 && pwr_per >= -100);
  switch (sel){
    case MOTOR_L:
      update_motor(MOTOR_L_PLUS, MOTOR_L_MINUS, MOTOR_L_PIN, pwr_per);
      break;
    case MOTOR_R:
      update_motor(MOTOR_R_PLUS, MOTOR_R_MINUS, MOTOR_R_PIN, pwr_per);
      break;
  }
}

void init_motor(){
  pinMode(MOTOR_L_PIN, OUTPUT);
  pinMode(MOTOR_R_PIN, OUTPUT);
  pinMode(MOTOR_L_MINUS, OUTPUT);
  pinMode(MOTOR_L_PLUS, OUTPUT);
  pinMode(MOTOR_R_MINUS, OUTPUT);
  pinMode(MOTOR_R_PLUS, OUTPUT);

  analogWrite(MOTOR_L_PIN, 0);
  analogWrite(MOTOR_R_PIN, 0);

  digitalWrite(MOTOR_L_MINUS, HIGH);
  digitalWrite(MOTOR_L_PLUS, HIGH);

  digitalWrite(MOTOR_R_MINUS, HIGH);
  digitalWrite(MOTOR_R_PLUS, HIGH);
}

void test_motor(){
  if (debug){
    Serial.println("Starting motor tests!");
  }
  for (int i = -100; i<=100; i+=10){
    if (debug){
      Serial.print("Setting pwr to: ");
      Serial.println(i);
    }
    set_motor(MOTOR_L, i);
    delay(2000);
    //set_motor(MOTOR_R, i);
    //delay(2000);
  }
}

void update_motor(int motor_plus, int motor_minus, int motor_dir, int pwr_per){
  int pwm = 255 * abs(pwr_per)/100 * 3.55/5; //100 percent equals 3.55V -> max v out of opamp
  if (pwr_per == 0){
        analogWrite(motor_dir, 0);
        digitalWrite(motor_minus, HIGH);
        digitalWrite(motor_plus, HIGH);
      }else if (pwr_per < 0){
        digitalWrite(motor_plus, LOW);
        digitalWrite(motor_minus, HIGH);
        analogWrite(motor_dir, pwm);
      }else {
        digitalWrite(motor_minus, LOW);
        digitalWrite(motor_plus, HIGH);
        analogWrite(motor_dir, pwm);
  }
}

/*----------------Wifi function declarations----------------*/

void print_wifi_status(){
  if (debug){
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
  
    // print your board's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);
  
    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
  }
}

void init_wifi(){
  if (debug){
    Serial.println("Initiliaizing wifi connection!");
  }
  if (WiFi.status() == WL_NO_MODULE) {
    if (debug){
      Serial.println("Communication with WiFi module failed!"); 
    }
    while (true);
  }
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION && debug) {
    Serial.println("Please upgrade the firmware");
  }

  while (status != WL_CONNECTED) {
    if (debug){
      Serial.print("Attempting to connect to Network named: ");
      Serial.println(ssid);                   
    }
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    unsigned char ip[] = {192, 168, 1, 240};
    WiFi.config(ip);
    status = WiFi.begin(ssid, pass);
    delay(10000);
  }
  server.begin();                          
  if (debug){
    Serial.println("Successfully connected!");
    print_wifi_status();                        // you're connected now, so print out the status
  }
}

void wifi_server_loop(){
  WiFiClient client = server.available();

  if (client) {                             // if you get a client,
    if (debug){
      Serial.println("new client");           // print a message out the serial port
    }
    String currentLine = "";                
    while (client.connected()) {           
      if (client.available()) {            
        char c = client.read();            
        if (debug){
          //Serial.write(c);                    
        }
        if (c == '\n') {                    
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            client.println(
              "Send GET PHOTO_F to receive a photo from the front camera!");
            client.println(
              "Send GET PHOTO_T to receive a photo from the top camera!");
            client.println(
              "Send SET MOTOR_L NUM to to set the power of the left motor to NUM percent!");
            client.println(
              "Send SET MOTOR_R NUM to to set the power of the left motor to NUM percent!");
           
            client.println();
            break;
          } else {
            match_com(currentLine);
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop();
    if (debug){
      Serial.println("client disconnected");
    }
  }
}

void match_com(String in){
  if (in.indexOf("GET PHOTO_F") >= 0) {                                          //TODO send the photos to the http client
    //client.write(); //Send data TODO
  }
  if (in.indexOf("GET PHOTO_T") >= 0) {
    //client.write(); //Send data TODO
  }
  if (in.indexOf("SET MOTOR_R") >= 0) {
    unsigned int index =  in.indexOf("SET MOTOR_R") + 12;
    int pwr = in.substring(index).toInt();
    set_motor(MOTOR_R, pwr);
    if (debug){
      Serial.print("Set right motor to ");
      Serial.print(pwr);
      Serial.print(" power.\n");
    }
  }
  if (in.indexOf("SET MOTOR_L") >= 0) {
    unsigned int index =  in.indexOf("SET MOTOR_L") + 12;
    int pwr = in.substring(index).toInt();
    set_motor(MOTOR_L, pwr);
    if (debug){
      Serial.print("Set left motor to ");
      Serial.print(pwr);
      Serial.print(" power.\n");
    }
  }
}

void wifi_send(uint8_t * buff, size_t sze){
  //TODO
}


/*----------------Arducam function declarations----------------*/

void init_arducam(ArduCAM* cam_nr){
  uint8_t vid, pid;
  uint8_t temp; 
  
  cam_nr->write_reg(0x07, 0x80);
  delay(100);
  cam_nr->write_reg(0x07, 0x00);
  delay(100);
  
  while(1){
    //Check if the ArduCAM SPI bus is OK
    cam_nr->write_reg(ARDUCHIP_TEST1, 0x55);
    temp = cam_nr->read_reg(ARDUCHIP_TEST1);
    if (temp != 0x55){
      if (debug){
        Serial.println(F("Cam1: ACK CMD SPI interface Error! END")); 
      }      
      delay(1000);continue;
    }else{
      if (debug){
        Serial.println(F("Cam1: ACK CMD SPI interface OK. END")); 
      }   
      break;  
    }
  }

  //Check if correct module is connected for Cam1
  while(1){
    //Check if the camera module type is OV5642
    cam_nr->wrSensorReg16_8(0xff, 0x01);
    cam_nr->rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
    cam_nr->rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
    if((vid != 0x56) || (pid != 0x42)){
      if (debug){
        Serial.println(F("Cam1: ACK CMD Can't find OV5642 module! END")); 
      }    
      delay(1000);continue;
    }
    else{
      if (debug){
        Serial.println(F("Cam1: ACK CMD OV5642 detected. END"));  
      }
      break;
    } 
  }
  //Change to JPEG capture mode and initialize Cam1
  cam_nr->set_format(JPEG);
  cam_nr->InitCAM();

  cam_nr->write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);   //VSYNC is active HIGH
  cam_nr->OV5642_set_JPEG_size(OV5642_1600x1200);
  delay(1000);
  cam_nr->clear_fifo_flag();
  cam_nr->write_reg(ARDUCHIP_FRAMES,0x00);  
}

int take_photo(ArduCAM myCAM){
  uint8_t temp = 0, temp_last = 0;
  uint32_t length = 0;
  bool is_header = false;

  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();
  //Start capture
  myCAM.start_capture();
  if (myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK)){
    delay(50);
    length = myCAM.read_fifo_length();
    
    if (debug){
      Serial.println(length, DEC);
    }
    if (length >= MAX_FIFO_SIZE) //512 kb
    {
      if (debug){
        Serial.println(F("ACK CMD Over size. END"));
      }
      return -1;
    }
    if (length == 0 ) //0 kb
    {
      if (debug){
        Serial.println(F("ACK CMD Size is 0. END"));
      }
      return -1;
    }
    myCAM.CS_LOW();
    myCAM.set_fifo_burst();//Set fifo burst mode
    temp =  SPI.transfer(0x00);
    length --;
    while ( length-- )
    {
      temp_last = temp;
      temp =  SPI.transfer(0x00);
      if (is_header == true)
      {
        if (debug){
          Serial.write(temp);                         //TODO tmp probably contains the picture data and it needs to be send per Wifi to the client.
        }
      }
      else if ((temp == 0xD8) & (temp_last == 0xFF))
      {
        is_header = true;
        if (debug){
          Serial.println(F("ACK IMG END"));
          Serial.write(temp_last);
          Serial.write(temp);
        }
      }
      if ( (temp == 0xD9) && (temp_last == 0xFF) ) //If find the end ,break while,
      break;
      delayMicroseconds(15);
    }
    myCAM.CS_HIGH();
    is_header = false;
    myCAM.clear_fifo_flag();
    }
  
  return 0;
}
