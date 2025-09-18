/*
Basic Code to run the OpenDrop V4.1, Research platfrom for digital microfluidics
Object codes are defined in the OpenDrop.h library
Written by Urs Gaudenz from GaudiLabs, 2021
*/

#include <SPI.h>
#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <OpenDrop.h>
#include <OpenDropAudio.h>

#include "hardware_def.h"

#define RESERVOIR_PERIOD 50 // in ticks
#define JOYSTICK_PERIOD 10  // in ticks

OpenDrop device = OpenDrop();
Drop *myDrop = device.getDrop();

enum JoystickAction
{
  UP,
  DOWN,
  LEFT,
  RIGHT,
  HOLD,
  NONE
} joystick_state;

void tickJoystick();
void tickReservoir();

bool FluxCom[16][8];
bool FluxBack[16][8];

int ControlBytesIn[16];
int ControlBytesOut[24];
int readbyte;
int writebyte;

int JOY_value;
int joy_x, joy_y;
int x, y;
int del_counter = 0;
int del_counter2 = 0;

bool SWITCH_state = true;
bool SWITCH_state2 = true;
bool idle = true;

bool Magnet1_state = false;
bool Magnet2_state = false;

int j = 0;

// the setup function runs once when you press reset or power the board
void setup()
{
  Serial.begin(115200);
  device.begin("c10");

  ControlBytesOut[23] = device.get_ID();
  Serial.println(device.get_ID());
  // OpenDropDevice.set_voltage(240,false,1000);

  device.set_Fluxels(FluxCom);

  pinMode(JOY_pin, INPUT);

  OpenDropAudio.begin(16000);
  OpenDropAudio.playMe(2);
  delay(2000);

  device.drive_Fluxels();
  device.update_Display();
  Serial.println("Welcome to OpenDrop");

  myDrop->begin(7, 4);
  device.update();

  del_counter = millis();
}

/*Left is 680, Right is 0, Down is 510, Up is 720, Default is 1023 */
void loop()
{
  delay(500);
  if (Serial.available() > 0) // receive data from App
  {
    readbyte = Serial.read();
    if (x < FluxlPad_width)
      for (y = 0; y < 8; y++)
        FluxCom[x][y] = (((readbyte) >> (y)) & 0x01);
    else
      ControlBytesIn[x - FluxlPad_width] = readbyte;

    x++;
    digitalWrite(LED_Rx_pin, HIGH);
    if (x == (FluxlPad_width + 16))
    {
      device.set_Fluxels(FluxCom);
      device.drive_Fluxels();
      device.update_Display();

      if ((ControlBytesIn[0] & 0x2) && (Magnet1_state == false))
      {
        Magnet1_state = true;
        device.set_Magnet(0, HIGH);
      };

      if (!(ControlBytesIn[0] & 0x2) && (Magnet1_state == true))
      {
        Magnet1_state = false;
        device.set_Magnet(0, LOW);
      };

      if ((ControlBytesIn[0] & 0x1) && (Magnet2_state == false))
      {
        Magnet2_state = true;
        device.set_Magnet(1, HIGH);
      };

      if (!(ControlBytesIn[0] & 0x1) && (Magnet2_state == true))
      {
        Magnet2_state = false;
        device.set_Magnet(1, LOW);
      };

      for (int x = 0; x < (FluxlPad_width); x++)
      {
        writebyte = 0;
        for (int y = 0; y < FluxlPad_heigth; y++)
          writebyte = (writebyte << 1) + (int)device.get_Fluxel(x, y);
        ControlBytesOut[x] = writebyte;
      }

      device.set_Temp_1(ControlBytesIn[10]);
      device.set_Temp_2(ControlBytesIn[11]);
      device.set_Temp_3(ControlBytesIn[12]);

      device.show_feedback(ControlBytesIn[8]);

      ControlBytesOut[17] = device.get_Temp_L_1();
      ControlBytesOut[18] = device.get_Temp_H_1();
      ControlBytesOut[19] = device.get_Temp_L_2();
      ControlBytesOut[20] = device.get_Temp_H_2();
      ControlBytesOut[21] = device.get_Temp_L_3();
      ControlBytesOut[22] = device.get_Temp_H_3();

      for (x = 0; x < 24; x++)
        Serial.write(ControlBytesOut[x]);
      x = 0;
    };
  }
  else
    digitalWrite(LED_Rx_pin, LOW);
  del_counter--;

  /*del_counter is updating the display every 2000 miliseconds*/
  if (millis() - del_counter > 2000)
  { // update Display
    device.update_Display();
    del_counter = millis();
  }

  SWITCH_state = digitalRead(SW1_pin);
  SWITCH_state2 = digitalRead(SW2_pin);

  if (!SWITCH_state) // activate Menu
  {
    OpenDropAudio.playMe(1);
    Menu(device);
    device.update_Display();
    del_counter2 = 200;
  }

  if (!SWITCH_state2) // activate Reservoirs
  {
    tickReservoir(0);
    magnet(0);
  }

  JOY_value = analogRead(JOY_pin); // navigate using Joystick

  Serial.println(JOY_value);
  Serial.println("del_counter:");
  Serial.println(del_counter);
  Serial.println("del_counter2:");
  Serial.println(del_counter2);

  tickJoystick();

  device.update_Drops();
  device.update();

  if (JOY_value > 950)
  {
    del_counter2 = 0;
    idle = true;
  }
  if (del_counter2 > 0)
    del_counter2--;

} // main loop

void tickJoystick()
{
  JOY_value = analogRead(JOY_pin); // navigate using Joystick
  // If someone intentionally moves the joystick
  if ((JOY_value < 950))
  {
    // state transition per the value of the analog read
    if ((JOY_value > 725))
    {
      joystick_state = UP;
    }
    else if ((JOY_value > 597))
    {
      joystick_state = LEFT;
    }
    else if ((JOY_value > 256))
    {
      joystick_state = DOWN;
    }
    else
    {
      joystick_state = RIGHT;
    }
    // take action based on new state
    switch (joystick_state)
    {
    case RIGHT:
      Serial.println("Right");
      myDrop->move_right();
      break;
    case UP:
      Serial.println("Up");
      myDrop->move_up();
      break;
    case LEFT:
      Serial.println("Left");
      myDrop->move_left();
      break;
    case DOWN:
      Serial.println("Down");
      myDrop->move_down();
      break;
    default:
      break;
    }
  }
  // return to the hold position.
  joystick_state = HOLD;
}

void tickReservoir(uint8_t tick)
{
  if (tick % RESERVOIR_PERIOD != 0)
    return;

  if ((myDrop->position_x() == 15) && (myDrop->position_y() == 3))
  {
    myDrop->begin(14, 1);
    device.dispense(1, 1200);
  }
  if ((myDrop->position_x() == 15) && (myDrop->position_y() == 4))
  {
    myDrop->begin(14, 6);
    device.dispense(2, 1200);
  }

  if ((myDrop->position_x() == 0) && (myDrop->position_y() == 3))
  {
    myDrop->begin(1, 1);
    device.dispense(3, 1200);
  }
  if ((myDrop->position_x() == 0) && (myDrop->position_y() == 4))
  {
    myDrop->begin(1, 6);
    device.dispense(4, 1200);
  }

  device.top_left_reservoir.updateState();
  device.top_right_reservoir.updateState();
  device.bottom_left_reservoir.updateState();
  device.bottom_right_reservoir.updateState();
}

void magnet(uint8_t tick)
{
  if ((myDrop->position_x() == 10) && (myDrop->position_y() == 2))
  {
    if (Magnet1_state)
    {
      device.set_Magnet(0, LOW);
      Magnet1_state = false;
    }
    else
    {
      device.set_Magnet(0, HIGH);
      Magnet1_state = true;
    }
    while (!digitalRead(SW2_pin))
      ;
  }

  if ((myDrop->position_x() == 5) && (myDrop->position_y() == 2))
  {
    if (Magnet2_state)
    {
      device.set_Magnet(1, LOW);
      Magnet2_state = false;
    }
    else
    {
      device.set_Magnet(1, HIGH);
      Magnet2_state = true;
    }
    while (!digitalRead(SW2_pin))
      ;
  }
}