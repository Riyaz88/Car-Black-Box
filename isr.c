#include <xc.h>

extern unsigned char return_time;
extern unsigned char sec;
extern char set_time_base[7],set_time_cpy[7];
extern unsigned char set_time_flag;

void __interrupt() isr(void)
{
    static unsigned int count = 0;
     
    if (TMR2IF == 1)
    {
        if (++count == 1250)
        {
            count = 0;
            
            if(sec > 0)
                sec--;
            
            if(return_time > 0)
                return_time--;
            
            if(set_time_flag == 0)
            {
                set_time_flag = 1;
            }
            else if(set_time_flag == 1)
            {
                set_time_flag = 0;
            }
        }
        
        
        TMR2IF = 0;
    }
}