#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include <assert.h>
#include <WiFiNINA.h>
#include "memorysaver.h"
#include "arduino_secrets.h"
#include "protothreads.h"

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
ArduCAM *Curr_cam;
enum dec_t{
  photo,
  video, 
  eof
};

volatile uint8_t buff[2048];
volatile uint32_t length;
volatile uint8_t tmp, tmp_last;
int take_and_send_photo();
unsigned i;
int take_photo(ArduCAM *cam_nr);
void init_arducam(ArduCAM* cam_nr);
void test_arducam();
//uint8_t read_and_send_fifo_arducam(ArduCAM* cam_nr);
enum cam_event_t{start_cap1, start_cap2, cap_done, clear_fifo, done, def} cam_event;
enum cam_state_t{idle, init1, init2, photo_s, terminate} cam_state;
enum cam_state_t transition[5][6]=
  { {init1    , init2    , idle       , idle     , idle   , idle}, //idle
    {init1    , init1    , photo_s    , init1    , init1  , init1}, //init1
    {init2    , init2    , photo_s    , init2    , init2  , init2}, //init2
    {photo_s  , photo_s  , photo_s    , terminate, photo_s, photo_s}, //photo_s
    {terminate, terminate, terminate  , terminate, idle   , terminate} //terminate
    };
void cam_fsm();
void start_capture(ArduCAM *cam_nr);
bool is_header;

//Motor stuff
void set_motor(char sel, int pwr_per);
void init_motor();
void test_motor();
void update_motor(int motor_plus, int motor_minus, int motor_dir, int pwr_per); 
void init_buff();


//Wifi server stuff
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;  // your network key index number (needed only for WEP)
String currentLine;
char c;

int status = WL_IDLE_STATUS;
WiFiServer server(80); //port 80
WiFiClient current_client;
void print_wifi_status();
void init_wifi();
//int wifi_server_loop();
void match_com(String in);
void wifi_send(byte data);

//Debug stuff
void print_debug(String text);
void println_debug(String text);

//-----------------------------------ProtoThread stuff-------------------------------------------------------------
pt pt_wifi_server;
int wifi_server_loop(struct pt* pt){
  PT_BEGIN(pt);

  current_client = server.available();

  if (current_client) {                    
    println_debug("\nnew client");
    current_client.flush();         
    
    currentLine = "";                
    while (current_client.connected()) { 
      if (current_client.available()) {            
        c = current_client.read();            
        if (debug){
          //Serial.write(c);                    
        }
        if (c == '\n') {                    
          if (currentLine.length() == 0) {
            current_client.println("HTTP/1.1 200 OK");
            current_client.println("Content-type:text/html");
            current_client.println();

            current_client.println(
              "Send GET PHOTO_1 to receive a photo from the first camera!"
              );
            current_client.println(
              "Send GET PHOTO_T to receive a photo from the top camera!"
              );
            current_client.println(
              "Send SET MOTOR_L NUM to to set the power of the left motor to NUM percent!"
              );
            current_client.println(
              "Send SET MOTOR_R NUM to to set the power of the left motor to NUM percent!"
              );
           
            current_client.println();
            break;
          } else {
            match_com(currentLine);     //Executes commands
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
      PT_YIELD(pt);
    }
    current_client.stop();

    println_debug("client disconnected");
  }

  PT_END(pt);
}

pt pt_send_photo;
int read_and_send_fifo_arducam(ArduCAM* cam_nr, struct pt* pt){
  PT_BEGIN(pt);

  init_buff(); i= 2; is_header = false;
  
  length = cam_nr->read_fifo_length();

  println_debug("Starting transmission!");
  if (debug){
    print_debug("Size of photo in bytes: ");
    Serial.println(length, DEC);
  }
  if (current_client.connected()){
    current_client.write((char*)&length, (size_t) sizeof(length));
  }
  
  if (length >= MAX_FIFO_SIZE) //512 kb
  {
    println_debug(F("ACK CMD Over size. END"));
    return 1;
  }else if (length == 0 ) //0 kb
  {
    println_debug(F("ACK CMD Size is 0. END"));
    return 1;
  }
  cam_nr->CS_LOW();
  cam_nr->set_fifo_burst();//Set fifo burst mode

  while (length-- && current_client.connected())
  {
//    buff[i] =  SPI.transfer(0x00);
//    i++;
    if (is_header == false){
      buff[0] = buff[1];
      buff[1] = SPI.transfer(0x00); 
      if ((buff[0] == 0xFF) && (buff[1] == 0xD8))
        is_header = true;
    }
   
    if (is_header == true)
    {
      tmp_last = tmp;
      tmp =  SPI.transfer(0x00);
      buff[i] = tmp;
      i++;

      if ( (tmp == (char)0xD9) && (tmp_last == (char)0xFF)){
        println_debug("Found img end!");
        current_client.write((char*)&buff, (size_t) ((i) * sizeof(char)));
        is_header = false;
        length = 0;
      }
      if (i>=sizeof(buff)){
        current_client.write((char*)&buff, (size_t) (sizeof(buff) * sizeof(char))); 
        i = 0;
      }
    }
    
    //current_client.write(buff[0]);
    PT_YIELD(pt);
  }
  
  cam_nr->CS_HIGH();
  println_debug("Finished transmission!");
  cam_event = clear_fifo;
  
  PT_END(pt);
}

void init_buff(){
  for (int j=0; i<1024; i++) buff[j] = 0; 
  i = 0;
}
//-----------------------------------setup-------------------------------------------------------------
void setup() {
  // put your setup code here, to run once:
  PT_INIT(&pt_wifi_server);
  PT_INIT(&pt_send_photo);
  cam_state = idle;
  cam_event = def;
  
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

//-----------------------------------loop-------------------------------------------------------------

void loop() {
  PT_SCHEDULE(wifi_server_loop(&pt_wifi_server));
  cam_fsm();
}

void tests(){
  println_debug("Starting tests!");
  //test_motor();
  test_arducam();
  println_debug("Test finished!\n");
}

//--------------------------Motor function declarations--------------------------

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

//--------------------------Wifi function declarations--------------------------

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

void match_com(String in){
  if (in.indexOf("GET PHOTO_1") >= 0) {  
    println_debug("CMD: photo 1st camera!");
    cam_event = start_cap1;
  }else if (in.indexOf("GET PHOTO_2") >= 0 && two_cam) {
    println_debug("CMD: photo 2nd camera!");
    cam_event = start_cap2;
  }else if (in.indexOf("SET MOTOR_R") >= 0) {
    unsigned int index =  in.indexOf("SET MOTOR_R") + 12;
    int pwr = in.substring(index).toInt();
    set_motor(MOTOR_R, pwr);
    
    print_debug("CMD: r motor ");
    print_debug((String) pwr);
    print_debug(" \% power.\n");
  }else if (in.indexOf("SET MOTOR_L") >= 0) {
    unsigned int index =  in.indexOf("SET MOTOR_L") + 12;
    int pwr = in.substring(index).toInt();
    set_motor(MOTOR_L, pwr);
    
    print_debug("CMD: l motor ");
    print_debug((String) pwr);
    print_debug(" \% power.\n");
  }
}

void wifi_send(byte data){
  current_client.write(data);
}

//--------------------------Arducam function declarations-------------------------- 

void cam_fsm(){
  //println_debug((String) cam_event);
  cam_state = transition[cam_state][cam_event];
  cam_event = def;
  
  switch(cam_state){
    case idle:
      break;
    case init1:
      //println_debug((String)cam_state);
      Curr_cam = &Cam1;
      take_and_send_photo(Curr_cam);
      break;
    case init2:
      //println_debug((String)cam_state);
      Curr_cam = &Cam2;
      take_and_send_photo(Curr_cam);
      break;
    case photo_s:
      //println_debug((String)cam_state);
      PT_SCHEDULE(read_and_send_fifo_arducam(Curr_cam, &pt_send_photo)); 
      break;
    case terminate:
      //println_debug((String)cam_state);
      Curr_cam->clear_fifo_flag();
      current_client.flush();
      cam_event = done;
      break;
  }
}

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
  cam_nr->OV5642_set_JPEG_size(OV5642_2592x1944); //OV5642_2592x1944, OV5642_2048x1536, OV5642_1600x1200, OV5642_1280x960, OV5642_1024x768
  //cam_nr->OV5642_set_hue(degree_180);
  //cam_nr->OV5642_set_Compress_quality(low_quality); //low_quality, default_quality
  //myCAM.OV5642_set_Sharpness(Auto_Sharpness_default);
  delay(1000);
  cam_nr->clear_fifo_flag();
  cam_nr->write_reg(ARDUCHIP_FRAMES,0x00);  
}

int take_and_send_photo(ArduCAM *cam_nr){
  start_capture(cam_nr);
  //PT_SCHEDULE(read_and_send_fifo_arducam(cam_nr, &pt_send_photo));
  cam_event = cap_done;
  
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
  println_debug("Capture finished!");
  delay(50);
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

//------------------------------- debug stuff ----------------------------------

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
