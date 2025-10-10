/*--------------------------------------------------------------------
  This file is part of the OpenDrop library
  by Urs Gaudenz, GaudiLabs 2016
  --------------------------------------------------------------------*/

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#include <pins_arduino.h>
#endif

#include "Adafruit_GFX.h"
// #include <tuple>

#define max_drops 8
#define max_magnets 3

class OpenDrop;

//<-
class Magnet
{
  private:
    Position loc;
    bool state;
    int ID;


  public:
    Magnet (): ID(0), state(false), loc({-1, -1}) {}

    Magnet(int id, Position loc) : ID(id), state(false), loc(loc) {}
    Position getLocation() const { return loc; }
    int getID() const { return ID; }

  bool toggle()
  {
    if (state)
      state = LOW;
    else
      state = HIGH;

    return state;
  }
  

};


class Drop
{

public:
  // Constructor: number of LEDs, pin number, LED type
  Drop(void);

  friend class OpenDrop;

  void begin(int x, int y);
  void move_right();
  void move_left();
  void move_up();
  void move_down();
  void go(int x, int y);
  int position_x();
  int position_y();
  int goal_x();
  int goal_y();
  int next_x();
  int next_y();
  int num();
  bool is_moving();

private:
  uint8_t _pos_x;
  uint8_t _pos_y;
  uint8_t _goal_x;
  uint8_t _goal_y;
  uint8_t _next_x;
  uint8_t _next_y;
  uint8_t _dropnum;
  bool _moving;
};

//STATE
enum DispenseState
{
  UPDATE,
  STEP_1,
  STEP_2,
  STEP_3,
  STEP_4,
  STEP_5,
  STEP_6,
};

struct Position
{
  int x;
  int y;

  bool operator ==(const Position& a) const
  {
    return (a.x == x && a.y == y);
  }
};

//STATE
class Reservoir
{
public:
  Reservoir(int reservoir);
  Position getDispenseAnimationPosition(int droplet);
  enum Location
  {
    TOP_RIGHT,
    BOTTOM_RIGHT,
    TOP_LEFT,
    BOTTOM_LEFT,
  };
  void updateState() { dispenseState = static_cast<DispenseState>((dispenseState + 1) % 7);};

private:
  int reservoir;
  DispenseState dispenseState;
};

class OpenDrop
{
public:
  friend class Magnet;
  OpenDrop(uint8_t addr = 0x60);
  friend class Drop;
  Reservoir top_left_reservoir = Reservoir(Reservoir::TOP_LEFT);
  Reservoir top_right_reservoir = Reservoir(Reservoir::TOP_RIGHT);
  Reservoir bottom_left_reservoir = Reservoir(Reservoir::BOTTOM_LEFT);
  Reservoir bottom_right_reservoir = Reservoir(Reservoir::BOTTOM_RIGHT);
  void begin(char code_str[]);
  bool run(void);
  void dispense(Reservoir reservoir, int delay_us);
  void update(void);
  void set_Fluxels(bool fluxels_array[][8]);
  bool get_Fluxel(int x, int y);
  void update_Display(void);
  void update_Drops(void);
  void drive_Fluxels(void);
  void read_Fluxels(void);
  void set_joy(uint8_t x, uint8_t y);
  Position get_joy();
  void show_joy(boolean val);
  void show_feedback(boolean val);
  void set_voltage(uint16_t voltage, bool AC_on, uint16_t frequence);
  void set_Pin(uint8_t pin, boolean val);
  void set_Magnet(uint8_t magnet, bool state);
  void toggle_Magnet(uint8_t magnet);
  uint8_t get_ID(void);

  float get_Temp_1(void);
  float get_Temp_2(void);
  float get_Temp_3(void);

  void set_Temp_1(uint8_t temperature);
  void set_Temp_2(uint8_t temperature);
  void set_Temp_3(uint8_t temperature);

  uint8_t get_Temp_L_1(void);
  uint8_t get_Temp_H_1(void);
  uint8_t get_Temp_L_2(void);
  uint8_t get_Temp_H_2(void);
  uint8_t get_Temp_L_3(void);
  uint8_t get_Temp_H_3(void);

  Drop *getDrop();

private:
  uint8_t _addr;
  uint16_t _freq;
  Drop drops[max_drops];
  Magnet magnets[max_magnets];
  uint8_t drop_count = 0;
  int _runing;
  Position _joy;
  bool _show_joy;
  //  PWM _pwm;
};

void Menu(OpenDrop &theOpenDrop);
