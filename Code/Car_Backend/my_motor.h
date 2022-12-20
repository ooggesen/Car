#ifndef MY_MOTOR.H
#define MY_MOTOR.H

#include <assert.h>

#define MOTOR_L 1
#define MOTOR_R 2
#define MOTOR_L_PIN 4
#define MOTOR_R_PIN 5
#define MOTOR_L_PLUS 0
#define MOTOR_L_MINUS 1
#define MOTOR_R_PLUS 21
#define MOTOR_R_MINUS 20

void set_motor(char sel, int pwr_per);
void init_motor();
void test_motor();
void update_motor(int motor_plus, int motor_minus, int motor_dir, int pwr_per); 
void init_buff();

#endif //MY_MOTOR.H
