#include "OpenDrop.h"
struct JoystickTask
{
    enum State
    {
        UP,
        DOWN,
        LEFT,
        RIGHT,
        HOLD
    };

    State state;
    uint64_t period;
    State prev;
    JoystickTask() : state(HOLD), prev(HOLD) {}
    void tickJoystick(uint64_t tick);
    void moveDroplet(uint64_t tick, Drop *drop);
};

struct ButtonTask
{
    enum State
    {
        DOWN_PRESS,
        UP_RELEASE,
        PRESSED,
        RELEASED,
    };

    State state;
    uint64_t period;
    uint32_t ticksPressed = 0;
    ButtonTask() : state(RELEASED) {}
    void tickButton(uint64_t tick, bool button_state);
};

struct MenuTask
{
    enum State
    {
        HIDDEN,
        SELECT_OPTION,
        SAVE_SETTINGS,
    };

    State state;
    uint64_t period;
    bool visible = false;
    // Menu persistent fields
    int menu_position = 1;
    int voltage = 240;
    uint32_t frequency = 1000;
    bool AC_state = false;
    bool save_settings = false;
    bool set_feedback = false;
    bool set_sound = false;

    MenuTask() : state(HIDDEN) {}
    void tickMenu(uint64_t tick, JoystickTask::State curr_dir, JoystickTask::State prev_dir, ButtonTask::State button, OpenDrop& device);
};

struct DispenseTask
{   
    enum State
    {
        STEP1,
        STEP2,
        STEP3,
        STEP4,
        STEP5,
        STEP6,
        IDLE
    } state;

    Reservoir reservoir;
    uint64_t period;
    DispenseTask(Reservoir reservoir) : reservoir(reservoir) {}
    void tickDispense(uint64_t tick, ButtonTask::State button, Drop* myDrop);
};

struct SerialCommTask
{
    enum State
    {
        WAITING,
        READING,
        WRITING
    };

    State state;
    uint8_t bytes_read = 0;

    SerialCommTask() : state(WAITING) {}
    void tickSerial(uint64_t tick, bool FluxCom[FLUXPAD_WIDTH][FLUXPAD_HEIGHT], uint8_t ControlBytesIn[NUM_CONTROL_BYTES_IN], uint8_t ControlBytesOut[NUM_CONTROL_BYTES_OUT], OpenDrop &device);
};

struct MagnetTask
{
    enum State
    {
        ON,
        OFF
    };

    State state;
    Magnet magnet;
    uint64_t period = 0;
    MagnetTask(Magnet magnet) : state(OFF), magnet(magnet) {}
    void tickMagnet(uint64_t tick, ButtonTask::State button, Drop* myDrop, OpenDrop &device);
};

struct HeatingTask
{
    enum State
    {
        ON,
        OFF
    };

    State state;
    uint64_t period;
    HeatingTask() : state(OFF) {}
    void tickHeating(uint64_t tick);
};

struct DisplayTask
{
    enum State
    {
        HIDDEN,
        VISIBLE,
        DEBUG,
    };

    State state;
    uint64_t period = 0;
    DisplayTask() : state(VISIBLE) {}
    void tickDisplay(uint64_t tick, bool menu_visible, OpenDrop &device);
};