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

#define NUM_RESERVOIRS 4

OpenDrop device = OpenDrop();
Drop *myDrop = device.getDrop();

Drop *dispensed_droplet_array[NUM_RESERVOIRS];

enum JoystickAction
{
  UP,
  DOWN,
  LEFT,
  RIGHT,
  HOLD,
  NONE
} joystick_state;

void tickJoystick(uint8_t tick);
void tickDispense(uint8_t tick);
void tickMagnet(uint8_t tick);
void tickSerial(uint8_t tick);

bool FluxCom[16][8];
bool FluxBack[16][8];

int ControlBytesIn[16];
int ControlBytesOut[24];
int readbyte;
int writebyte;

int x, y;

bool SWITCH1 = true;
bool SWITCH2 = true;
bool idle = true;

//remove?
bool Magnet1_state = false;
bool Magnet2_state = false;


void setup()
{
  Serial.begin(115200);
  device.begin("c10");

  ControlBytesOut[23] = device.get_ID();
  Serial.println(device.get_ID());
  // OpenDropDevice.set_voltage(240,false,1000);

  device.set_Fluxels(FluxCom);

  // Initialize Joystick pin to be an input
  pinMode(JOY_pin, INPUT);

  OpenDropAudio.begin(16000);
  OpenDropAudio.playMe(2);
  
  
  delay(2000);

  device.drive_Fluxels();
  device.update_Display();
  Serial.println("Welcome to OpenDrop");

  myDrop->begin(Position{7,4});

  for (int i = 0; i < NUM_RESERVOIRS; i++)
  {
    dispensed_droplet_array[i] = device.getDrop();
  }

  device.update();

}


uint32_t tick = 0;

void loop()
{
  delay(500);

  SWITCH1 = digitalRead(SW1_pin);
  SWITCH2 = digitalRead(SW2_pin);

  if (!SWITCH1) // activate Menu
  {
    OpenDropAudio.playMe(1);
    Menu(device);
    device.update_Display();
  
  }

  if (!SWITCH2) // activate Reservoirs
  {
    tickDispense(tick);
    tickMagnet(tick);
  }

  tickJoystick(tick);

  device.update_Drops();
  device.update();

  tick++;
} 


void tickJoystick(uint8_t tick)
{
  uint32_t JOY_value = analogRead(JOY_pin); // navigate using Joystick
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

    switch (joystick_state) 
    {
    case RIGHT:
      Serial.println("Right");
      myDrop->move_right();
      device.set_joy(myDrop->pos().x, myDrop->pos().y);
      break;
    case UP:
      Serial.println("Up");
      myDrop->move_up();
      device.set_joy(myDrop->pos().x, myDrop->pos().y);
      break;
    case LEFT:
      Serial.println("Left");
      myDrop->move_left();
      device.set_joy(myDrop->pos().x, myDrop->pos().y);
      break;
    case DOWN:
      Serial.println("Down");
      myDrop->move_down();
      device.set_joy(myDrop->pos().x, myDrop->pos().y);      
      break;
    default:
      break;
    }
  }
  // Return to hold position so if user releases joystick, it doesn't keep moving in the last direction
  joystick_state = HOLD; 
}

void tickDispense(uint8_t tick)
{

  if (tick % RESERVOIR_PERIOD != 0)
    return;

  if ((myDrop->pos() == Position{15, 3}))
  {
    myDrop->begin(Position{14, 1});
    device.dispense(1, 1200);
  }
  if ((myDrop->pos() == Position{15, 4}))
  {
    myDrop->begin(Position{14, 6});
    device.dispense(2, 1200);
  }
  if ((myDrop->pos() == Position{0, 3}))
  {
    myDrop->begin(Position{1, 1});
    device.dispense(3, 1200);
  }
  if ((myDrop->pos()  == Position{0, 4}))
  {
    myDrop->begin(Position{1, 6});
    device.dispense(4, 1200);
  }

  device.top_left_reservoir.updateState();
  device.top_right_reservoir.updateState();
  device.bottom_left_reservoir.updateState();
  device.bottom_right_reservoir.updateState();
}


//STATE? Could turn into a state
void tickMagnet(uint8_t tick)
{
  if ((myDrop->pos() == Position{10, 2}))
  {
    device.toggle_Magnet(0);

    while (!digitalRead(SW2_pin));
  }

  if ((myDrop->pos() == Position{5, 2}))
  {
    device.toggle_Magnet(0);
    while (!digitalRead(SW2_pin));
  }
}

void tickSerial(uint8_t tick)
{
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

}