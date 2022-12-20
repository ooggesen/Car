#include "my_motor.h"

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
