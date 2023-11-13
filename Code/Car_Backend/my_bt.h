#ifndef MY_BT.H
#define MY_BT.H

#include "protothreads.h"
#include "ArduinoBLE.h"
#include "my_arducam.h"
#include "base.h"

BLEService car_control("38e7bf14-7170-4bc3-935a-d5cb99622342"); //Random UUID
BLEStringCharacteristic command("38e7bf15-7170-4bc3-935a-d5cb99622342", BLERead | BLEWrite, 128);
BLECharacteristic photo_data("38e7bf16-7170-4bc3-935a-d5cb99622342", BLERead | BLENotify, sizeof(buff), true);

BLEDevice central;
pt pt_bt_peripheral;
int bluetooth_receiver(struct pt *pt);

void init_bluetooth();
void test_bt();
void send_data(char* data, size_t len);

#endif //MY_BT.H
