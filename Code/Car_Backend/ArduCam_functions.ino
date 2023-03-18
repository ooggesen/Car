#include "my_arducam.h"

bool is_header;
uint32_t length;
uint8_t tmp, tmp_last;
unsigned i;

void init_buff(){
  for (int j=0; i<1024; i++) buff[j] = 0; 
  i = 0;
}

int read_and_send_fifo_arducam(ArduCAM* cam_nr, struct pt* pt){
  PT_BEGIN(pt);

  init_buff(); i= 2; is_header = false;
  
  length = cam_nr->read_fifo_length();

  println_debug("Starting transmission!");
  #ifdef DEBUG
    print_debug("Size of photo in bytes: ");
    Serial.println(length, DEC);
  #endif
  
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

  while (length--) //no breaks due to PT usage, use length = 0; continue; instead
  {
    #ifdef WIFI_N_BT
      if (!current_client.connected()){ 
        length = 0;
        continue;
      }
    #else
    #endif
    
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
        send_data((char*)&buff, (size_t) ((i) * sizeof(char)));
        is_header = false;
        length = 0;
        continue;
      }
      if (i>=sizeof(buff)){
        send_data((char*)&buff, (size_t) (sizeof(buff) * sizeof(char))); 
        i = 0;
      }
    }
    PT_YIELD(pt);
  }
  
  cam_nr->CS_HIGH();
  println_debug("Finished transmission!");
  cam_event = clear_fifo;
  
  PT_END(pt);
}

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
  cam_nr->OV5642_set_JPEG_size(OV5642_1024x768); //OV5642_2592x1944, OV5642_2048x1536, OV5642_1600x1200, OV5642_1280x960, OV5642_1024x768
  //cam_nr->OV5642_set_hue(degree_180);
  //cam_nr->OV5642_set_Compress_quality(low_quality); //low_quality, default_quality
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
