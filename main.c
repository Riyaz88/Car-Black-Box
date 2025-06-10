/*
 * File:   main.c
 * Author: RIYAZ AHMAED
 *
 * Created on January 22, 2025, 9:36 PM
 */


#include <xc.h>
#include <string.h>
#include "cbb.h"
#include "clcd.h"
#include "digital_keypad.h"
#include "adc.h"
#include "ds1307.h"
#include "i2c.h"
#include "timers.h"


#pragma config WDTE = OFF

char *gear[] = {"GN","GR","G1","G2","G3","G4"};
unsigned char return_time ;

void init_config(void)
{
    init_clcd();
    init_digital_keypad();
    init_i2c(100000);
    init_ds1307();
    init_adc();
    init_timer2();
    
   // Interrupt Enable
    PEIE = 1;
    GIE = 1;
    
}

void main(void) {
    
    init_config();
    
    unsigned char key;
    char event[3] = "ON";  // First event
    int gr = 0;  // Gear Box initial
    unsigned char speed = 0; // speed initial
    char control_flag = DASH_BOARD_FLAG; // Starting Screen initial
    char reset_flag = RESET_NOTHING;  // Reset
    int menu_pos = 0; // Menu Position Index
    int wait_time = 0; // Waiting delay
    
    
  
    log_car_event(event,speed);
    
    while(1)
    {
        
        key = read_digital_keypad(STATE);
        // Bouncing Effect Delay
        for(unsigned char wait = 250;wait--;);
        
        speed = (unsigned char)(read_adc(CHANNEL0) / 10);
        if(speed > 99)
            speed = 99;
        
        if(key == SW1)
        {
            strcpy (event," C");
            log_car_event(event,speed);
        }
        else if(key == SW2)
        {
            if(gr < 5)
            {
                gr++;
            }
            strcpy (event,gear[gr]);
            log_car_event(event,speed);
        }
        else if(key == SW3)
        {
            if(gr > 0)
            {
                gr--;
            }
            strcpy (event,gear[gr]);
            log_car_event(event,speed);
        }
        
       // check if sw4 or sw5 pressed then change the screen        
        else if((key == SW4 || key == SW5) && control_flag == DASH_BOARD_FLAG )
        {
            control_flag = LOGIN_FLAG;
            reset_flag = RESET_PASSWORD;
            clear_screen();
            clcd_print("Enter Password  ",LINE1(0));
            clcd_write(DISP_ON_AND_CURSOR_ON,INST_MODE);
            
            /* Switching on the Timer2 */
            TMR2ON = 1;
        }
        
        // menu screeen detect long press, based on menu pos change screen
        else if(control_flag == MAIN_MENU_FLAG && key == L_SW4)
        {
            switch(menu_pos)
            {
                case 0:
                    clear_screen();
                    control_flag = VEIW_LOG_FLAG;
                    reset_flag = RESET_VIEW_LOG_POS;
                    break;
                case 1:
                    clear_screen();
                    control_flag = CLEAR_LOG_FLAG;
                    break;
                case 2:
                    break;
                case 3:
                    clear_screen();
                    control_flag = SET_TIME_FLAG;
                    reset_flag = RESET_SET_TIME;
                    break;
                case 4:
                    clear_screen();
                    control_flag = CHANGE_PASSWORD_FLAG;
                    reset_flag = RESET_PASSWORD;
                    break;
                    
            }
        }
        else if(control_flag == MAIN_MENU_FLAG && key == L_SW5)
        {
            control_flag = DASH_BOARD_FLAG;                                              
            clear_screen();
        }
        else if(control_flag == VEIW_LOG_FLAG && (key == L_SW5 || key == L_SW4 ))
        {
            control_flag = MAIN_MENU_FLAG;                                             
            clear_screen();
        }
        else if(control_flag == SET_TIME_FLAG && key == L_SW4 )
        {
            TMR2ON = 0;
            save_time();
            control_flag = MAIN_MENU_FLAG;
            clear_screen();
        }
        else if(control_flag == SET_TIME_FLAG && key == L_SW5 )
        {
            TMR2ON = 0;
            control_flag = MAIN_MENU_FLAG;
            clear_screen();
        }
        switch(control_flag)
        {
            case DASH_BOARD_FLAG :
                                       display_dash_board(event,speed);
                                       break;
            case LOGIN_FLAG :
                                       switch(login_screen(key,reset_flag))
                                       {
                                           case RETURN_BACK:
                                                //go back to dashboard
                                                control_flag = DASH_BOARD_FLAG;                                              
                                                clear_screen();                                              
                                                clcd_write(DISP_ON_AND_CURSOR_OFF,INST_MODE);            
                                                TMR2ON = 0;
                                                break;
                                                
                                           case LOGIN_SUCCESS:
                                                // go to menu screen
                                                control_flag = MAIN_MENU_FLAG;
                                                reset_flag = RESET_LOGIN_MENU;
                                                clear_screen();                                               
                                                clcd_write(DISP_ON_AND_CURSOR_OFF,INST_MODE);           
                                                TMR2ON = 0;
                                                menu_screen(key,reset_flag);
                                                break;
                                           case RETURN_NOTHING:
                                                break;
                                       }
                                       
                                       break;
            case MAIN_MENU_FLAG:
                                       // menu screen
                                       menu_pos = menu_screen(key,reset_flag);
                                       reset_flag = RESET_NOTHING;
                                       break;
            case VEIW_LOG_FLAG:
                                       view_log(key,reset_flag);
                                       break;
            case CLEAR_LOG_FLAG:
                                       clear_log();
                                       control_flag = MAIN_MENU_FLAG;
                                       reset_flag = RESET_LOGIN_MENU;
                                       menu_screen(key,reset_flag);
                                       break;
            case SET_TIME_FLAG:         
                                       TMR2ON = 1;
                                       set_time(key,reset_flag);
                                       break;
            case CHANGE_PASSWORD_FLAG:
                                    if(change_passwd(key,reset_flag) == TASK_SUCESS)
                                            control_flag = MAIN_MENU_FLAG;                            
      
                                    break;   
        }
         reset_flag = RESET_NOTHING;
    }
    return;
}
