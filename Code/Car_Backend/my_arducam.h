#ifndef MY_ARDUCAM.H
#define MY_ARDUCAM.H

#include "protothreads.h"
#include "my_wifi.h"
#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>

#define CS1 6
#define CS2 7

enum cam_event_t{start_cap1, start_cap2, cap_done, clear_fifo, done, def};
enum cam_state_t{idle, init1, init2, photo_s, terminate};
enum dec_t{photo, video, eof};

enum cam_event_t cam_event;
enum cam_state_t cam_state;
enum cam_state_t transition[5][6]=
  { {init1    , init2    , idle       , idle     , idle   , idle}, //idle
    {init1    , init1    , photo_s    , init1    , init1  , init1}, //init1
    {init2    , init2    , photo_s    , init2    , init2  , init2}, //init2
    {photo_s  , photo_s  , photo_s    , terminate, photo_s, photo_s}, //photo_s
    {terminate, terminate, terminate  , terminate, idle   , terminate} //terminate
    };

ArduCAM Cam1(OV5642, CS1);
ArduCAM Cam2(OV5642, CS2);
bool two_cam = false;
ArduCAM *Curr_cam;

uint8_t buff[128]; //512 bytes is maximum size of bluetooth characteristik

pt pt_send_photo;

int take_photo(ArduCAM *cam_nr);
void init_arducam(ArduCAM* cam_nr);
void test_arducam();
int take_and_send_photo();
void cam_fsm();
void start_capture(ArduCAM *cam_nr);
void init_buff();
int read_and_send_fifo_arducam(ArduCAM* cam_nr, struct pt* pt);

#endif //MY_ARDUCAM.H
