#include <Adafruit_SSD1306.h>
#include <OpenDropAudio.h>
#include <SAMD_AnalogCorrection.h>
#include "adapterI2C.h"
#include "FlashStorage.h"
#include "hardware_def.h"
#include "bitmap.h"
#include "Adafruit_GFX.h"

#include "OpenDrop.h"
#include "tasks.h"

// access global display from OpenDrop.cpp
extern Adafruit_SSD1306 display;

void ButtonTask::tickButton(uint64_t tick, bool button_state)
{
    if (tick % this->period != 0)
    {
        return;
    }

    switch (this->state)
    {
    case ButtonTask::RELEASED:
        if (button_state)
        {
            this->state = ButtonTask::DOWN_PRESS;
        }
        break;
    case ButtonTask::DOWN_PRESS:
        if (button_state)
        {
            this->state = ButtonTask::PRESSED;
        }
        else
        {
            this->state = ButtonTask::UP_RELEASE;
        }
        break;
    case ButtonTask::PRESSED:
        if (!button_state)
        {
            this->state = ButtonTask::UP_RELEASE;
        }
        break;
    case ButtonTask::UP_RELEASE:
        if (!button_state)
        {
            this->state = ButtonTask::RELEASED;
        }
        else
        {
            this->state = ButtonTask::DOWN_PRESS;
        }
        break;
    }
}

void JoystickTask::tickJoystick(uint64_t tick)
{
    if (tick % this->period != 0)
    {
        return;
    }
    uint32_t JOY_value = analogRead(JOY_pin);
    

    switch (this->state)
    {
    case JoystickTask::HOLD:
        if (JOY_value < 950)
        {
            // state transition per the value of the analog read
            if (JOY_value > 725)
            {
                this->state = JoystickTask::UP;
            }
            else if (JOY_value > 597)
            {
                this->state = JoystickTask::LEFT;
            }
            else if (JOY_value > 256)
            {
                this->state = JoystickTask::DOWN;
            }
            else
            {
                this->state = JoystickTask::RIGHT;
            }
        }
        else
        {
            this->state = JoystickTask::HOLD;
        }
        break;
    case JoystickTask::UP:
        if (!((JOY_value < 950) && (JOY_value > 725)))
        {
            this->state = JoystickTask::HOLD;
        }
        break;
    case JoystickTask::DOWN:
        if (!((JOY_value < 597) && (JOY_value > 256)))
        {
            this->state = JoystickTask::HOLD;
        }
        break;
    case JoystickTask::LEFT:
        if (!((JOY_value < 725) && (JOY_value > 597)))
        {
            this->state = JoystickTask::HOLD;
        }
        break;
    case JoystickTask::RIGHT:
        if (!((JOY_value < 256) && (JOY_value > 0)))
        {
            this->state = JoystickTask::HOLD;
        }
        break;
     default:
         break;
    }
};

void JoystickTask::moveDroplet(uint64_t tick, Drop *drop)
{ // navigate using Joystick
    // If someone intentionally moves the joystick
    switch (this->state)
    {
    case JoystickTask::RIGHT:
        drop->move_right();
        break;
    case JoystickTask::UP:
        drop->move_up();
        break;
    case JoystickTask::LEFT:
        drop->move_left();
        break;
    case JoystickTask::DOWN:
        drop->move_down();
        break;
    case JoystickTask::HOLD:
        // Serial.println("Hold");
        break;
    default:
        break;
    }
}

void SerialCommTask::tickSerial(uint64_t tick, bool FluxCom[FLUXPAD_WIDTH][FLUXPAD_HEIGHT], uint8_t ControlBytesIn[NUM_CONTROL_BYTES_IN], uint8_t ControlBytesOut[NUM_CONTROL_BYTES_OUT], OpenDrop &device)
{

    switch (this->state)
    {
    case SerialCommTask::WAITING:
        // if we have data to read, move to reading,
        // if we have read enough data, move to writing,
        // otherwise keep waiting
        if (Serial.available() && this->bytes_read < (FLUXPAD_WIDTH + 16))
        {
            this->state = SerialCommTask::READING;
        }
        else if (Serial.available() && this->bytes_read == (FLUXPAD_WIDTH + 16))
        {
            this->state = SerialCommTask::WRITING;
        }
        else
        {
            this->state = SerialCommTask::WAITING;
        }
        break;
    case SerialCommTask::READING:
        // if available and we have read enough bytes, we can move to writing,
        // if available and we have not read enough bytes, keep reading,
        // otherwise move to waiting
        if (Serial.available() && this->bytes_read == (FLUXPAD_WIDTH + 16))
        {
            this->state = SerialCommTask::WRITING;
        }
        else if (Serial.available())
        {
            this->state = SerialCommTask::READING;
        }
        else
        {
            this->state = SerialCommTask::WAITING;
        }
        break;
    case SerialCommTask::WRITING:
        // writing, we can move to waiting
        this->state = SerialCommTask::WAITING;
        this->bytes_read = 0;

        device.show_feedback(ControlBytesIn[8]);
        device.set_Temp_1(ControlBytesIn[10]);
        device.set_Temp_2(ControlBytesIn[11]);
        device.set_Temp_3(ControlBytesIn[12]);
        break;
    default:
        this->state = SerialCommTask::WAITING;
        break;
    }

    switch (this->state)
    {
    case SerialCommTask::READING:
        // read byte, first populate electrode data, then control bytes
        uint8_t read_byte = Serial.read();
        uint8_t x = this->bytes_read;

        if (x < FLUXPAD_WIDTH)
        {
            for (uint8_t y = 0; y < FLUXPAD_HEIGHT; y++)
            {
                FluxCom[x][y] = (((read_byte) >> (y)) & 0x01);
            }
        }
        else
        {
            ControlBytesIn[x - FLUXPAD_WIDTH] = read_byte;
        }

        this->bytes_read++;
        digitalWrite(LED_Rx_pin, HIGH);
        break;
    case SerialCommTask::WRITING:
        device.set_Fluxels(FluxCom);
        device.drive_Fluxels();
        device.update_Display();

        // TODO: Magnet Control

        // Condense electrode data into single byte per column for writing back to the app
        uint8_t write_byte = 0;
        for (uint8_t x = 0; x < FLUXPAD_WIDTH; x++)
        {
            write_byte = 0;
            for (uint8_t y = 0; y < FLUXPAD_HEIGHT; y++)
            {
                write_byte = (write_byte << 1) + (int)device.get_Fluxel(x, y);
            }
            ControlBytesOut[x] = write_byte;
        }

        ControlBytesOut[17] = device.get_Temp_L_1();
        ControlBytesOut[18] = device.get_Temp_H_1();
        ControlBytesOut[19] = device.get_Temp_L_2();
        ControlBytesOut[20] = device.get_Temp_H_2();
        ControlBytesOut[21] = device.get_Temp_L_3();
        ControlBytesOut[22] = device.get_Temp_H_3();

        for (uint8_t i = 0; i < NUM_CONTROL_BYTES_OUT; i++)
        {
            Serial.write(ControlBytesOut[i]);
        }
        break;
    }
};

void DisplayTask::tickDisplay(uint64_t tick, bool menu_visible, OpenDrop &device)
{
    if (tick % this->period != 0)
    {
        return;
    }

    switch (this->state)
    {
    case DisplayTask::HIDDEN:
        if (!menu_visible)
        {
            this->state = DisplayTask::VISIBLE;
        }
        break;
    case DisplayTask::VISIBLE:
        if (menu_visible)
        {
            this->state = DisplayTask::HIDDEN;
        }
        break;
    case DisplayTask::DEBUG:
        break;
    default:
        this->state = DisplayTask::HIDDEN;
        break;
    }

    switch (this->state)
    {
    case DisplayTask::HIDDEN:
        break;
    case DisplayTask::VISIBLE:
        device.update_Display();
        break;
    case DisplayTask::DEBUG:
        // TODO: add debug info display functionality
        break;
    default:
        break;
    }
};

void MenuTask::tickMenu(uint64_t tick, JoystickTask::State curr_dir, JoystickTask::State prev_dir, ButtonTask::State button, OpenDrop &device)
{
    if (tick % this->period != 0)
    {
        return;
    }
    switch (this->state)
    {
    case MenuTask::HIDDEN:
        // open menu on button press
        if (button == ButtonTask::DOWN_PRESS)
        {
            this->state = MenuTask::SELECT_OPTION;
            this->visible = true;
            this->menu_position = 1;
            this->voltage = device.getVoltageSet();
            this->frequency = device.getACFrequency();
            this->AC_state = device.getACFlag();
            this->set_sound = device.getSoundFlag();
            this->set_feedback = device.getFeedbackFlag();
            this->save_settings = false;
            display.dim(false);
        }
        break;
    case MenuTask::SELECT_OPTION:
        // render menu
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE, BLACK);
        display.setCursor(15, 2);
        if (this->AC_state)
            display.print("MODE:     AC");
        else
            display.print("MODE:     DC");
        display.setCursor(15, 12);
        display.print("VOLTAGE:  ");
        display.print(this->voltage);
        display.print(" V");
        display.setCursor(15, 22);
        display.print("FREQUENCY:");
        if (this->frequency < 1000)
            display.print(" ");
        display.print(this->frequency);
        display.print(" Hz");
        display.setCursor(15, 32);
        if (this->set_sound)
            display.print("SOUND:    ON");
        else
            display.print("SOUND:    OFF");
        display.setCursor(15, 42);
        if (this->set_feedback)
            display.print("FEEDBACK: ON");
        else
            display.print("FEEDBACK: OFF");

        if (!this->save_settings)
        {
            display.setCursor(15, 55);
            display.println("SET:      CANCEL");
        }
        else
        {
            display.setCursor(15, 55);
            display.println("SET:      OK");
        }

        display.setCursor(0, 10 * this->menu_position - 8);
        if (this->menu_position == 6)
            display.setCursor(0, 55);
        display.print(">");
        display.display();

        // navigation using joystick edges
        if (curr_dir == JoystickTask::UP && prev_dir != JoystickTask::UP)
        {
            if (this->menu_position > 1)
            {
                this->menu_position--;
                this->save_settings = false;
            }
        }
        if (curr_dir == JoystickTask::DOWN && prev_dir != JoystickTask::DOWN)
        {
            if (this->menu_position < 6)
            {
                this->menu_position++;
            }
        }

        // adjust values per menu item on left/right edges
        if (curr_dir == JoystickTask::LEFT && prev_dir != JoystickTask::LEFT)
        {
            switch (this->menu_position)
            {
            case 1:
                this->AC_state = false;
                break;
            case 2:
                if (this->voltage > 50)
                    this->voltage -= 10;
                break;
            case 3:
                if (this->frequency > 100)
                    this->frequency -= 50;
                break;
            case 4:
                this->set_sound = true;
                break; // left -> ON (matches original mapping)
            case 5:
                this->set_feedback = false;
                break;
            case 6:
                this->save_settings = false;
                break;
            default:
                break;
            }
        }
        if (curr_dir == JoystickTask::RIGHT && prev_dir != JoystickTask::RIGHT)
        {
            switch (this->menu_position)
            {
            case 1:
                this->AC_state = true;
                break;
            case 2:
                if (this->voltage < 280)
                    this->voltage += 10;
                break;
            case 3:
                if (this->frequency < 1500)
                    this->frequency += 50;
                break;
            case 4:
                this->set_sound = false;
                break; // right -> OFF (matches original mapping)
            case 5:
                this->set_feedback = true;
                break;
            case 6:
                this->save_settings = true;
                break;
            default:
                break;
            }
        }

        // confirm via button press or SW3 pin
        if (button == ButtonTask::DOWN_PRESS)
        {
            if (this->save_settings)
            {
                this->state = MenuTask::SAVE_SETTINGS;
            }
            else
            {
                this->state = MenuTask::HIDDEN;
            }
        }

        break;
    case MenuTask::SAVE_SETTINGS:
        device.saveSettings(this->AC_state, this->voltage, this->frequency, this->set_sound, this->set_feedback);
        this->visible = false;
        this->state = MenuTask::HIDDEN;
        break;
    default:
        this->state = MenuTask::HIDDEN;
        break;
    }
}

void DispenseTask::tickDispense(uint64_t tick, ButtonTask::State button, Drop* myDrop)
{
    if (tick % this->period != 0)
    {
        return;
    }

    switch (this->state)
    {    
        case DispenseTask::IDLE:
            if (button == ButtonTask::UP_RELEASE)
            {
                this->state = DispenseTask::STEP1;
            }
            break;
        case DispenseTask::STEP1:
            this->state = DispenseTask::STEP2;
            break;
        case DispenseTask::STEP2:
            this->state = DispenseTask::STEP3;
            break;
        case DispenseTask::STEP3:
            this->state = DispenseTask::STEP4;
            break;
        case DispenseTask::STEP4:
            this->state = DispenseTask::STEP5;
            break;
        case DispenseTask::STEP5:
            this->state = DispenseTask::STEP6;
            break;
        case DispenseTask::STEP6:
            this->state = DispenseTask::IDLE;
            break;
        default:
            this->state = DispenseTask::IDLE;
            break;
    }

    switch (this->state)
    {
        case DispenseTask::IDLE:
            break;
        case DispenseTask::STEP1:
            myDrop->go(this->reservoir.getDispenseAnimationPosition(0));
            break;
        case DispenseTask::STEP2:
            myDrop->go(this->reservoir.getDispenseAnimationPosition(1));
            break;
        case DispenseTask::STEP3:
            myDrop->go(this->reservoir.getDispenseAnimationPosition(2));
            break;
        case DispenseTask::STEP4:
            myDrop->go(this->reservoir.getDispenseAnimationPosition(3));
            break;
        case DispenseTask::STEP5:
            myDrop->go(this->reservoir.getDispenseAnimationPosition(4));
            break;
        case DispenseTask::STEP6:
            myDrop->go(this->reservoir.getDispenseAnimationPosition(5));
            break;
        default:
            break;
    }
}

void MagnetTask::tickMagnet(uint64_t tick, ButtonTask::State button, Drop* myDrop, OpenDrop &device)
{
    if (tick % this->period != 0)
    {
        return;
    }

    switch (this->state)
    {
        case MagnetTask::OFF:
            if (button == ButtonTask::UP_RELEASE )
            {
                this->state = MagnetTask::ON;
            }
            break;
        case MagnetTask::ON:
            if (button == ButtonTask::UP_RELEASE)
            {
                this->state = MagnetTask::OFF;
            }
            break;
        default:
            this->state = MagnetTask::OFF;
            break;
    }

    switch (this->state)
    {
        case MagnetTask::OFF:
        case MagnetTask::ON:
            device.toggle_Magnet(this->magnet.getID());
            break;
        default:
            break;
    }
}