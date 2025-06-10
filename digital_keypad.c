#include <xc.h>
#include "digital_keypad.h"

void init_digital_keypad(void)
{
    /* Set Keypad Port as input */
    KEYPAD_PORT_DDR = KEYPAD_PORT_DDR | INPUT_LINES;
}

unsigned char read_digital_keypad(unsigned char mode)
{
    static unsigned char once = 1,long_press,prev_key,key;
    
    if (mode == LEVEL_DETECTION)
    {
        return KEYPAD_PORT & INPUT_LINES;
    }
    else
    {
        key = KEYPAD_PORT & INPUT_LINES;
        if ((key != ALL_RELEASED) && once)
        {
            once = 0;
            long_press = 0;
            prev_key = key;
//            return KEYPAD_PORT & INPUT_LINES;
        }
        else if(once == 0 && prev_key == key && long_press < 25)
        {
            long_press++;
        }
        else if(long_press == 25)
        {
            long_press++;
            return prev_key | 0x80;
        }
        else if (key == ALL_RELEASED && once == 0)
        {
            once = 1;
            if(long_press < 25)
                return prev_key;
        }
    }
    
    return ALL_RELEASED;
  
}
