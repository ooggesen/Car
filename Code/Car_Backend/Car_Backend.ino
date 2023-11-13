//#define WIFI_N_BT //Define if to use wifi instead of bluetooth, bluetooth not yet tested
#define DEBUG //define for debug feedback through Serial

#include "base.h"

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
//-----------------------------------setup-------------------------------------------------------------
void setup() {
  // put your setup code here, to run once:
  PT_INIT(&pt_send_photo);
  cam_state = idle;
  cam_event = def;
  
  init_motor();
  #ifdef DEBUG
    Serial.begin(9600);
    while(!Serial);
    Serial.println("\nStarting MKRWifi1010!");
  #endif

  #ifdef WIFI_N_BT
    PT_INIT(&pt_wifi_server);
    init_wifi();
  #else
    init_bluetooth();
  #endif
  
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
  #ifdef WIFI_N_BT
    PT_SCHEDULE(wifi_server_loop(&pt_wifi_server));
  #else
//    BLE.poll();
//    match_com((String)command.value());
    PT_SCHEDULE(bluetooth_receiver(&pt_bt_peripheral));
  #endif
  cam_fsm();
}

//------------------------------- debug stuff ----------------------------------
void tests(){
  println_debug("Starting tests!");
  //test_motor();
  test_arducam();
  #ifndef WIFI_N_BT
    test_bt();
  #endif
  println_debug("Test finished!\n");
}


void print_debug(String text){
  #ifdef DEBUG
    Serial.print(text);
    //Serial.flush();
  #endif
}

void println_debug(String text){
  #ifdef DEBUG
    Serial.println(text);
    //Serial.flush();
  #endif
}
