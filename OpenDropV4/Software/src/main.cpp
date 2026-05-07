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
#include "tasks.h"

OpenDrop device = OpenDrop();
Drop *myDrop = device.getDrop();

Drop *dispensed_droplet_array[NUM_RESERVOIRS];

bool FluxCom[FLUXPAD_WIDTH][FLUXPAD_HEIGHT];

uint8_t ControlBytesIn[NUM_CONTROL_BYTES_IN];
uint8_t ControlBytesOut[NUM_CONTROL_BYTES_OUT];

ButtonTask switch1, switch2;
JoystickTask joystick;
SerialCommTask serialTask;
MagnetTask magnets[NUM_MAGNETS];
HeatingTask heaters[NUM_HEATERS];
DispenseTask dispensers[NUM_RESERVOIRS];
MenuTask menu;
DisplayTask displayTask;


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

  // Display welcome screen and play welcome sound
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

uint64_t tick = 0;

void loop()
{
    
  serialTask.tickSerial(tick, FluxCom, ControlBytesIn, ControlBytesOut, device);

  // Check for button presses and joystick movement every loop, and update states accordingly
  switch1.tickButton(tick, !digitalRead(SW1_pin));
  switch2.tickButton(tick, !digitalRead(SW2_pin));
  
  // Get joystick state and move droplet accordingly
  joystick.tickJoystick(tick);
  joystick.moveDroplet(tick, myDrop);

  // tick dispensers and magnets
  dispensers[0].tickDispense(tick, myDrop->pos());
  magnets[0].tickMagnet(tick);


  // Update menu and display 
  menu.tickMenu(tick, joystick.state, joystick.prev, switch1.state, device);
  displayTask.tickDisplay(tick, menu.visible, device);
  
  // mandatory update call to move droplets
  device.update();
  tick++;
} 