#ifndef MY_WIFI.H
#define MY_WIFI.H

#include "protothreads.h"
#include <WiFiNINA.h>
#include "arduino_secrets.h"
#include "my_arducam.h"
#include "base.h"

pt pt_wifi_server;
int wifi_server_loop(struct pt* pt);

WiFiServer server(80); //port 80
WiFiClient current_client;

void print_wifi_status();
void init_wifi();
void send_data(char* data, size_t len);

#endif //MY_WIFI.H
