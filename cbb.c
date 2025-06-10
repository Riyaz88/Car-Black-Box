/*
 * File:   cbb.c
 * Author: RIYAZ AHMAED
 *
 * Created on January 22, 2025, 9:37 PM
 */


#include <xc.h>
#include "cbb.h"
#include "i2c.h"
#include "ds1307.h"
#include "clcd.h"
#include "EEprom.h"
#include "string.h"
#include "digital_keypad.h"

unsigned char clock_reg[3];
unsigned char log[11];
unsigned char time[7],set_time_base[7],set_time_cpy[7];  // "HH:MM:SS"
unsigned char pos,limit = NOT_REACHED_10;
extern unsigned char return_time ;
unsigned char sec,set_time_flag;
unsigned char user_passwd[4],saved_passwd[4];

char *menu[] = {"View Log","Clear Log","Download Log","Set Time","Change Pwd"};

static void get_time(unsigned char *clock_reg)
{
    clock_reg[0] = read_ds1307(HOUR_ADDR); // HH -> BCD 
    clock_reg[1] = read_ds1307(MIN_ADDR); // MM -> BCD 
    clock_reg[2] = read_ds1307(SEC_ADDR); // SS -> BCD 
    
    /* To store the time in HH:MM:SS format */
    
    // HH
    time[0] = ((clock_reg[0] >> 4) & 0x03) + '0';
    time[1] = (clock_reg[0] & 0x0F) + '0';
   
    // MM 
    time[2] = ((clock_reg[1] >> 4) & 0x07) + '0';
    time[3] = (clock_reg[1] & 0x0F) + '0';
    
    // SS
    time[4] = ((clock_reg[2] >> 4) & 0x07) + '0';
    time[5] = (clock_reg[2] & 0x0F) + '0';
    time[6] = '\0';
   
}

void display_dash_board(char event[],unsigned short speed)
{
    clcd_print("  TIME    EV  SP",LINE1(0));
    get_time(clock_reg);
    
    //HH
    clcd_putch(time[0],LINE2(0));
    clcd_putch(time[1],LINE2(1));
    clcd_putch(':',LINE2(2));
    //MM
    clcd_putch(time[2],LINE2(3));
    clcd_putch(time[3],LINE2(4));
    clcd_putch(':',LINE2(5));
    //SS
    clcd_putch(time[4],LINE2(6));
    clcd_putch(time[5],LINE2(7));
    
    clcd_print(event,LINE2(10));
    clcd_putch(speed / 10 + '0',LINE2(14));
    clcd_putch(speed % 10 + '0',LINE2(15));
}
void log_event(void)
{
    unsigned char addr;
    if(++pos == 10)
    {
        limit = REACHED_10;
        pos = 0;
    }
    addr = pos * 10 + 5;
    ext_eeprom_24C02_str_write(addr,log);
    
}
void log_car_event (char *event,unsigned short speed)
{
    get_time(clock_reg);
    strncpy(log,time,6);
    
    strncpy(&log[6],event,2);
    
    log[8] = speed / 10 + '0';
    log[9] = speed % 10 + '0';
    
    log[10] = '\0';
    
    log_event();
}

unsigned char login_screen(unsigned char key,unsigned char reset_flag)
{
    static unsigned char i,attempt;
    
    if(reset_flag == RESET_PASSWORD)
    {
        attempt = 3;
        i = 0;
//        user_passwd[0] = '\0';
//        user_passwd[1] = '\0';
//        user_passwd[2] = '\0';
//        user_passwd[3] = '\0';       
        saved_passwd[0] = ext_eeprom_24C02_read(0x01);
        saved_passwd[1] = ext_eeprom_24C02_read(0x02);
        saved_passwd[2] = ext_eeprom_24C02_read(0x03);
        saved_passwd[3] = ext_eeprom_24C02_read(0x04);
        return_time = 5;
        key = 0;
    }
    
    if(return_time == 0)
    {
        return RETURN_BACK;
    }
    //read password
    if(key == SW4 && i < 4)
    {
        user_passwd[i] = '1';
        i++;
        clcd_putch('*',LINE2(i));
        return_time = 5;
    }
    else if(key == SW5 && i < 4)
    {
        user_passwd[i] = '0';
        i++;
        clcd_putch('*',LINE2(i));
        return_time = 5;
    }
    
    //compare password
    if(i == 4)
    {
        if(strncmp(user_passwd,saved_passwd,4) == 0)
        {
            clear_screen();
            clcd_write(DISP_ON_AND_CURSOR_OFF,INST_MODE);
            __delay_us(100);
            clcd_print("Login Success",LINE1(0));
            __delay_ms(1500);
            
            TMR2ON = 0;
            return LOGIN_SUCCESS;
        }
        else
        {
            // reduce attempt
            attempt--;
            
            if(attempt == 0)
            {
                // block screen for 60 sec
                clear_screen();
                clcd_write(DISP_ON_AND_CURSOR_OFF,INST_MODE);
                __delay_us(100);
                clcd_print("you are blocked",LINE1(0));
                clcd_print("Wait for",LINE2(0));
                sec = 60;
                while(sec != 0)
                {
                    clcd_putch(sec / 10 + '0',LINE2(9));
                    clcd_putch(sec % 10 + '0',LINE2(10));
                }
                 clcd_print("secs",LINE2(12));
                 attempt = 3;
            }   
            else
            {
                // If attempts left display info
                clear_screen();
                clcd_write(DISP_ON_AND_CURSOR_OFF,INST_MODE);
                __delay_us(100);
                clcd_print("Wrong Password",LINE1(0));
                clcd_print("Attempts left",LINE2(0));
                clcd_putch(attempt + '0',LINE2(14));
                __delay_ms(1500);
                
            }
            clear_screen();
            clcd_print("Enter Password",LINE1(0));
            clcd_write(DISP_ON_AND_CURSOR_ON,INST_MODE);
            i = 0;
            return_time = 5;
//            return RETURN_BACK;
        }
    }
    return RETURN_NOTHING;
}

unsigned char menu_screen(unsigned char key,unsigned char reset_flag)
{
    
    static unsigned char menu_pos;
    static unsigned char select_pos;
    
    if(reset_flag == RESET_LOGIN_MENU)
    {
        menu_pos = 0;
        select_pos = 1;
        key = 0;
    }
    
    if(key == SW4 && menu_pos < 4)
    {
        clear_screen();
        menu_pos++;
        if(select_pos == 1)
            select_pos = 2;
    }
    else if(key == SW5 && menu_pos > 0)
    {
        clear_screen();
        menu_pos--;
        if(select_pos == 2)
            select_pos = 1;
    }
  
    
    if(select_pos == 1)
    {   
        clcd_print("->",LINE1(0));
        clcd_print(menu[menu_pos],LINE1(2));
        clcd_print(menu[menu_pos+1],LINE2(2));
    }
    else 
    {
        clcd_print("->",LINE2(0));
        clcd_print(menu[menu_pos-1],LINE1(2));
        clcd_print(menu[menu_pos],LINE2(2));
    }
    
    return menu_pos;
    
}

void view_log(unsigned char key,unsigned char reset_flag)
{
    static unsigned char shift;
    char rlog[11];
//    unsigned char add;
    
    //read logs from eeprom and display it on clcd
    if(reset_flag == RESET_VIEW_LOG_POS)
    {
        shift = 0;
        
    }
    else if(key == SW4 && limit == NOT_REACHED_10)
    {
        if(shift < pos)
        {
            shift++;
            clear_screen();
        }
    }
    else if(key == SW4 && shift < 10)
    {
        shift++;
        clear_screen();
    }
    else if(key == SW5 && shift > 0)
    {
        shift--;
        clear_screen();
    }
     
    for(unsigned char i = 0;i < 10;i++)
    {
       rlog[i] = ext_eeprom_24C02_read(i + (shift * 10) + 5);  
    }
    
    clcd_print("NO  TIME   EV SP",LINE1(0));
    get_time(clock_reg);
    
    
    clcd_putch(shift + '0',LINE2(0));
    //HH
    clcd_putch(rlog[0],LINE2(2));
    clcd_putch(rlog[1],LINE2(3));
    clcd_putch(':',LINE2(4));
    //MM
    clcd_putch(rlog[2],LINE2(5));
    clcd_putch(rlog[3],LINE2(6));
    clcd_putch(':',LINE2(7));
    //SS
    clcd_putch(rlog[4],LINE2(8));
    clcd_putch(rlog[5],LINE2(9));
    
    clcd_putch(rlog[6],LINE2(11));
    clcd_putch(rlog[7],LINE2(12));
    
    clcd_putch(rlog[8],LINE2(14));
    clcd_putch(rlog[9],LINE2(15));
}

void clear_log()
{
    pos = 0;
    limit = NOT_REACHED_10;
    
    clcd_print("LOGS CLEARED",LINE1(3));
    clcd_print("SUCCESSFULLY",LINE2(3));
    
    for(unsigned char wait = 250;wait--;);
    clear_screen();
        
}


void edit_time(char *time_cpy,char pos)
{
    if(pos == 's')
    {
        time_cpy[4] = ' ';
        time_cpy[5] = ' ';
    }
    else if(pos == 'm')
    {
        time_cpy[3] = ' ';
        time_cpy[2] = ' ';
    }
    else if(pos == 'h')
    {
        time_cpy[1] = ' ';
        time_cpy[0] = ' ';
    }
    
}
void display_set_time(char time[])
{
    //HH
    clcd_putch(time[0],LINE2(0));
    clcd_putch(time[1],LINE2(1));
    clcd_putch(':',LINE2(2));
    //MM
    clcd_putch(time[2],LINE2(3));
    clcd_putch(time[3],LINE2(4));
    clcd_putch(':',LINE2(5));
    //SS
    clcd_putch(time[4],LINE2(6));
    clcd_putch(time[5],LINE2(7));
}

unsigned char char_to_dec(unsigned char ones,unsigned char tens)
   {
       return (tens - '0') * 10 + (ones - '0');
   }
void set_time(unsigned char key,unsigned char reset_flag)
{
   static unsigned char units[] = {'s','m','h'};
   static unsigned char index = 0;
   unsigned char value;
   
    if(reset_flag == RESET_SET_TIME)
    {
        strcpy(set_time_base,time);
        strcpy(set_time_cpy,set_time_base);
        edit_time(set_time_cpy,'s');
        key = 0;
    }
  
    if(key == SW5)
    {
        strcpy(set_time_cpy,set_time_base);
        index++;
        if(index == 3)
            index = 0;
        edit_time(set_time_cpy,units[index]);
        
    }
    else if(key == SW4)
    {
        if(index == 0)
        {
            value = char_to_dec(set_time_base[5],set_time_base[4]);
            if(value == 59)
                value = 0;
            else
                value++;
            set_time_base[5] = value % 10 + '0';
            set_time_base[4] = value / 10 + '0';
        }
        else if(index == 1)
        {
            value = char_to_dec(set_time_base[3],set_time_base[2]);
            if(value == 59)
                value = 0;
            else
                value++;
            set_time_base[3] = value % 10 + '0';
            set_time_base[2] = value / 10 + '0';
        }
        else if(index == 2)
        {
            value = char_to_dec(set_time_base[1],set_time_base[0]);
            if(value == 23)
                value = 0;
            else
                value++;
            set_time_base[1] = value % 10 + '0';
            set_time_base[0] = value / 10 + '0';
            
        }
     strcpy(set_time_cpy,set_time_base);      
     edit_time(set_time_cpy,units[index]);
    }
    
    if(set_time_flag == 0)
    {
        clcd_print("TIME (HH:MM:SS)",LINE1(0));
        display_set_time(set_time_base);
    }
    else
    {
        clcd_print("TIME (HH:MM:SS)",LINE1(0));
        display_set_time(set_time_cpy);
    }
    
}


unsigned char dec_to_bcd(unsigned char ones,unsigned char tens)
{
    return (tens - '0' << 4) + (ones - '0');
}

void save_time()
{
    write_ds1307(0x00,dec_to_bcd(set_time_base[5],set_time_base[4]));
    write_ds1307(0x01,dec_to_bcd(set_time_base[3],set_time_base[2]));
    write_ds1307(0x02,dec_to_bcd(set_time_base[1],set_time_base[0]));   
    
    clear_screen();
    clcd_print("TIME UPDATED",LINE1(1));
    clcd_print("SUCCESSFULLY",LINE2(1));
    
    for(unsigned int wait = 500;wait--;);
}

unsigned char change_passwd(unsigned char key,unsigned char reset_flag)
{
    static unsigned char index = 0,screen_flag ;
    
    if(reset_flag == RESET_PASSWORD)
    {
        clcd_print("Enter old Passwd",LINE1(0));
        clcd_write(DISP_ON_AND_CURSOR_ON,INST_MODE);
         __delay_us(100);
        key = 0;
        screen_flag = 0;
    }

    if(key == SW4 && screen_flag == 0)
    {
        user_passwd[index] = '1';
        clcd_putch('*',LINE2(index));
        index++;
    }
    else if(key == SW5 && screen_flag == 0)
    {
        user_passwd[index] = '0';
        clcd_putch('*',LINE2(index));
        index++;
    }
    else if(key == SW4 && screen_flag == 1)
    {
        saved_passwd[index] = '1';
        clcd_putch('*',LINE2(index));
        index++;
    }
    else if(key == SW5 && screen_flag == 1)
    {
        saved_passwd[index] = '0';
        clcd_putch('*',LINE2(index));
        index++;
    }

    if(index == 4 && screen_flag == 0)
    {
        index = 0;
        if(strncmp(user_passwd,saved_passwd,4) == 0)
        {
            clear_screen();
            clcd_print("Enter new passwd",LINE1(0));
            screen_flag = 1;
            return RETURN_NOTHING;
        }
        else
        {
            char wait = 25;
            clear_screen();
             clcd_write(DISP_ON_AND_CURSOR_OFF,INST_MODE);
         __delay_us(100);
            while(wait)
            {
               clcd_print("Wrong Passwd",LINE1(2));
               clcd_print("Try Again",LINE2(2));
               wait--;
            }     
            clear_screen();
            return TASK_SUCESS;
        }
    }
    else if(index == 4 && screen_flag == 1)
    {
        char wait = 25;
        index = 0;
        clear_screen();
         clcd_write(DISP_ON_AND_CURSOR_OFF,INST_MODE);
         __delay_us(100);
         ext_eeprom_24C02_byte_write(0x01,saved_passwd[0]);
         ext_eeprom_24C02_byte_write(0x02,saved_passwd[1]);
         ext_eeprom_24C02_byte_write(0x03,saved_passwd[2]);
         ext_eeprom_24C02_byte_write(0x04,saved_passwd[3]);
        while(wait)
        {
        clcd_print("Passwd Updated",LINE1(2));
        clcd_print("SUCCESSFULLY",LINE2(2));
        wait--;
        }
            
        clear_screen();
        return TASK_SUCESS;
    }
    return RETURN_NOTHING;
}