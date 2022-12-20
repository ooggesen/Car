#ifdef WIFI_N_BT

#include "my_wifi.h"

//Wifi server stuff
char ssid[] = SECRET_SSID;    
char pass[] = SECRET_PASS;    
String currentLine;
char c;

int status = WL_IDLE_STATUS;

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

void print_wifi_status(){
    // print the SSID of the network you're attached to:
    print_debug("SSID: ");
    println_debug(WiFi.SSID());
  
    // print your board's IP address:
    IPAddress ip = WiFi.localIP();
    print_debug("IP Address: ");
    #ifdef DEBUG
      Serial.println(ip);
    #endif
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


void send_data(char* data, size_t len){
  if (current_client.connected()){
    current_client.write(data, len);
  }
}

#endif //WIFI_N_BT
