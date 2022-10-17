#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include <assert.h>
#include <WiFiNINA.h>
#include "memorysaver.h"
#include "arduino_secrets.h"
//#include "protothreads.h"

//Motor def
#define MOTOR_L 1
#define MOTOR_R 2
#define MOTOR_L_PIN 4
#define MOTOR_R_PIN 5
#define MOTOR_L_PLUS 0
#define MOTOR_L_MINUS 1
#define MOTOR_R_PLUS 21
#define MOTOR_R_MINUS 20

//Arducam def
#define CS1 6
#define CS2 7

//General stuff
bool debug = true;
bool two_cam = false;
void tests();

//Arducam stuff
ArduCAM Cam1(OV5642, CS1);
ArduCAM Cam2(OV5642, CS2);
int take_and_send_photo(ArduCAM *cam_nr);
int take_photo(ArduCAM *cam_nr);
void init_arducam(ArduCAM* cam_nr);
void test_arducam();
uint8_t read_and_send_fifo_arducam(ArduCAM* cam_nr);

void start_capture(ArduCAM *cam_nr);

//Motor stuff
void set_motor(char sel, int pwr_per);
void init_motor();
void test_motor();
void update_motor(int motor_plus, int motor_minus, int motor_dir, int pwr_per); 


//Wifi server stuff
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;  // your network key index number (needed only for WEP)

int status = WL_IDLE_STATUS;
WiFiServer server(80); //port 80
WiFiClient *current_client;
void print_wifi_status();
void init_wifi();
void wifi_server_loop();
void match_com(String in);
void wifi_send(byte data);

//Debug stuff
void print_debug(String text);
void println_debug(String text);

void setup() {
  // put your setup code here, to run once:
  init_motor();
  if (debug){
    Serial.begin(9600);
    while(!Serial);
    Serial.println("\nStarting MKRWifi1010!");
  }

  init_wifi();
  
  init_arducam(&Cam1);
  
  if (two_cam){
    println_debug("Cam 2:");
    init_arducam(&Cam2);
  }
  println_debug("Setup done!\n");
  
  tests();
}

void loop() {
  wifi_server_loop();
}

void tests(){
  println_debug("Starting tests!");
  //test_motor();
  test_arducam();
  println_debug("Test finished!\n");
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
  {
    println_debug("Starting motor tests!");
  }
  for (int i = -100; i<=100; i+=10){
    print_debug("Setting pwr to: ");
    println_debug((String) i);  
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
    // print the SSID of the network you're attached to:
    print_debug("SSID: ");
    println_debug(WiFi.SSID());
  
    // print your board's IP address:
    IPAddress ip = WiFi.localIP();
    print_debug("IP Address: ");
    if (debug) {
      Serial.println(ip);
    }
    //println_debug((String)ip);
  
    // print the received signal strength:
    long rssi = WiFi.RSSI();
    print_debug("signal strength (RSSI):");
    print_debug((String) rssi);
    println_debug(" dBm");
}

void init_wifi(){
  println_debug("Initiliaizing wifi connection!");
  if (WiFi.status() == WL_NO_MODULE) {
    println_debug("Communication with WiFi module failed!"); 
    while (true);
  }
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    println_debug("Please upgrade the firmware");
  }

  while (status != WL_CONNECTED) {
    print_debug("Attempting to connect to Network named: ");
    println_debug(ssid);
    unsigned char ip[] = {192, 168, 1, 240};
    WiFi.config(ip);
    status = WiFi.begin(ssid, pass);
    delay(10000);
  }
  server.begin();     
                       
  println_debug("Successfully connected!");
  print_wifi_status();           
}

void wifi_server_loop(){
  WiFiClient client = server.available();

  if (client) {   
    current_client = &client;                  
    println_debug("\nnew client");         
    
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
              "Send GET PHOTO_1 to receive a photo from the first camera!"
              );
            client.println(
              "Send GET PHOTO_T to receive a photo from the top camera!"
              );
            client.println(
              "Send SET MOTOR_L NUM to to set the power of the left motor to NUM percent!"
              );
            client.println(
              "Send SET MOTOR_R NUM to to set the power of the left motor to NUM percent!"
              );
           
            client.println();
            break;
          } else {
            match_com(currentLine);     //Executes commands
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop();

    println_debug("client disconnected");
  }
}

void match_com(String in){
  if (in == "GET PHOTO_1"){//in.indexOf("GET PHOTO_1") >= 0) {  
    println_debug("Take photo with 1st camera!");
    take_and_send_photo(&Cam1);
  }else if (in.indexOf("GET PHOTO_2") >= 0 && two_cam) {
    println_debug("Take photo with 2nd camera!");
    take_and_send_photo(&Cam2);
  }else if (in.indexOf("SET MOTOR_R") >= 0) {
    unsigned int index =  in.indexOf("SET MOTOR_R") + 12;
    int pwr = in.substring(index).toInt();
    set_motor(MOTOR_R, pwr);
    
    print_debug("Set right motor to ");
    print_debug((String) pwr);
    print_debug(" power.\n");
  }else if (in.indexOf("SET MOTOR_L") >= 0) {
    unsigned int index =  in.indexOf("SET MOTOR_L") + 12;
    int pwr = in.substring(index).toInt();
    set_motor(MOTOR_L, pwr);
    
    print_debug("Set left motor to ");
    print_debug((String) pwr);
    print_debug(" power.\n");
  }
}

void wifi_send(byte data){
  current_client->write(data);
}

/*----------------Arducam function declarations----------------*/

void init_arducam(ArduCAM* cam_nr){
  SPI.begin();
  
  //Check SPI connection for Cam 1
  digitalWrite(CS1, HIGH);
  digitalWrite(CS2, HIGH);

  Wire.begin();
  
  uint8_t vid, pid;
  uint8_t temp; 

  println_debug("Initialize Arducam!");
  
  cam_nr->write_reg(0x07, 0x80);
  delay(100);
  cam_nr->write_reg(0x07, 0x00);
  delay(100);

  println_debug("Checking SPI connection!");

  while(1){
    //Check if the ArduCAM SPI bus is OK
    cam_nr->write_reg(ARDUCHIP_TEST1, 0x55);
    temp = cam_nr->read_reg(ARDUCHIP_TEST1);
    if (temp != 0x55){
      println_debug(F("Cam: ACK CMD SPI interface Error! END"));     
      delay(1000);continue;
    }else{
      println_debug(F("Cam: ACK CMD SPI interface OK. END")); 
      break;  
    }
  }

  println_debug("Checking if camera is of type OV5642!");
  while(1){
    println_debug("Writing Register!");
    cam_nr->wrSensorReg16_8(0xff, 0x01);
    println_debug("Reading vid!");
    cam_nr->rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
    println_debug("Reading pid!");
    cam_nr->rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
    
    if((vid != 0x56) || (pid != 0x42)){
      println_debug(F("Cam: ACK CMD Can't find OV5642 module! END"));  
      delay(1000);continue;
    }
    else{
      println_debug(F("Cam: ACK CMD OV5642 detected. END"));  
      break;
    } 
  }

  println_debug("Initialize the settings!");

  cam_nr->set_format(JPEG);
  cam_nr->InitCAM();

  cam_nr->write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);   //VSYNC is active HIGH
  cam_nr->OV5642_set_JPEG_size(OV5642_1024x768);
  //myCAM.OV5642_set_Sharpness(Auto_Sharpness_default);
  delay(1000);
  cam_nr->clear_fifo_flag();
  cam_nr->write_reg(ARDUCHIP_FRAMES,0x00);  
}

int take_and_send_photo(ArduCAM *cam_nr){
  start_capture(cam_nr);
  read_and_send_fifo_arducam(cam_nr);
  cam_nr->clear_fifo_flag();
  
  return 0;
}


int take_photo(ArduCAM *cam_nr){
    start_capture(cam_nr);
    cam_nr->clear_fifo_flag();

    return 0;
}

void start_capture(ArduCAM *cam_nr){
  println_debug("Init fifo!");
  cam_nr->flush_fifo();
  cam_nr->clear_fifo_flag();

  println_debug("Starting capture!");
  cam_nr->start_capture();
  while (!cam_nr->get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK)); //Blocks program execution
  delay(50);
  println_debug("Capture finished!");
}

void test_arducam(){
  println_debug("Making Photo with Cam1!");
  take_photo(&Cam1);
  delay(1000);

  if (two_cam) {
    println_debug("Making Photo with Cam2!");
    take_photo(&Cam2);
    delay(1000);
  }
}

uint8_t read_and_send_fifo_arducam(ArduCAM* cam_nr){
  uint8_t temp = 0, temp_last = 0;
  uint32_t length = cam_nr->read_fifo_length();
  bool is_header = false;

  println_debug("Starting transmission!");
  if (debug){
    print_debug("Size of photo in bytes: ");
    Serial.println(length, DEC);
  }
  current_client->write((char*)&length, (size_t) sizeof(length));
  //delay(100);
  
  if (length >= MAX_FIFO_SIZE) //512 kb
  {
    println_debug(F("ACK CMD Over size. END"));
    return 1;
  }
  if (length == 0 ) //0 kb
  {
    println_debug(F("ACK CMD Size is 0. END"));
    return 1;
  }
  cam_nr->CS_LOW();
  cam_nr->set_fifo_burst();//Set fifo burst mode
  while ( length--)
  {
//    temp_last = temp;
    temp =  SPI.transfer(0x00);
    current_client->write(temp);
//    if (is_header == true)
//    {
//      current_client->write(temp);
//      //delay(10);
//    }
//    else 
//    if ((temp == 0xD8) & (temp_last == 0xFF))
//    {
//      is_header = true;
//      println_debug(F("ACK IMG END"));
//      current_client->write(temp_last);
//      current_client->write(temp);
//      //delay(10);
//    }
//    if ((temp == 0xD9) && (temp_last == 0xFF) ) break;
    delayMicroseconds(15);
  }
  cam_nr->CS_HIGH();
  is_header = false;
  println_debug("Finished transmission!");
  return 0;
}

//--------------------- debug stuff ------------------------

void print_debug(String text){
  if (debug){
    Serial.print(text);
    //Serial.flush();
  }
}

void println_debug(String text){
  if (debug){
    Serial.println(text);
    //Serial.flush();
  }
}
