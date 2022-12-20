#ifndef WIFI_N_BT

#include "my_bt.h"

void init_bluetooth(){
  println_debug("Init Bluetooth!");
  if (!BLE.begin()) {
      Serial.println("Error in bluetooth init!");
      while (1);
    }

    BLE.setLocalName("car_control"); //name for advertisement
    BLE.setDeviceName("car_control"); //name of build in characteristic
    BLE.setAdvertisedService(car_control);
    car_control.addCharacteristic(command);
    car_control.addCharacteristic(photo_data);

    BLE.addService(car_control);

    command.writeValue("Init!");
    photo_data.writeValue(buff, sizeof(buff), true);

    println_debug("Start advertise!");
    if (!BLE.advertise()){
      println_debug("ERROR:");
      println_debug("Could not start advertising!");
    }
    print_debug("Started BT service with UUID: ");
    println_debug(car_control.uuid());
    println_debug("With characteristics: ");
    print_debug("Command: ");
    println_debug(command.uuid());
    print_debug("Photo_data: ");
    println_debug(photo_data.uuid());
    
    println_debug("Init done!");
}

void test_bt(){
  println_debug("Starting testing bluetooth!");
  String backup = command.value();
  String test_string = "Test!";
  command.writeValue(test_string);
  String read_string = command.value();

  if (test_string != read_string){
    println_debug("ERROR:");
    println_debug("Could not write or read to command characteristic!"); 
    print_debug("Test string: ");
    print_debug(test_string);
    print_debug(" ,but read: ");
    println_debug(read_string);
    while(1);
  }else{
    command.writeValue(backup);
  }

  photo_data.writeValue(buff, sizeof(buff), true);
  const uint8_t* cmp_buff = photo_data.value();
  for (int i=0; i<sizeof(buff); i++){
    if (buff[i] != cmp_buff[i]){
      println_debug("ERROR:");
      println_debug("Could not write or read to charcteristic!");
    }
  }
  println_debug("Finished testing bluetooth!");
}

void send_data(char* data, size_t len){
    photo_data.writeValue((void*) data, (int) len, true);
}
#endif //WIFI_N_BT
