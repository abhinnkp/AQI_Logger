/*
This library is primarily designed to facilitate the creation of timer-based interrupts within code, as well as to manage flags and execute operations accordingly.
Note    : Timer must be enable after complition of all initialization task in setup function
NOTE(2) : The code does not incorporate any functionality /library based on a Real-Time Operating System (RTOS).
          Consequently, after a certain period, when the timer experiences a rollback and resets the counter, there may be a slight drift in the time scale.
          LCM concept for ROLLBACK implemented to avoid this (ROLL BACK VALUE is divisible without reminder for all TICK COUNTERs)
          This drift is automatically corrected in the next minute.
*/


#include <timer.h>

extern hw_timer_t                              *timer;                       //Pointer to point timer related varaibles - (Hardware Timer)
extern ssl_timer_t                             ssl_timer;                    //Timer structure varaible



//Below Variables for getting correct Time stamps
uint64_t  LAST_FIVE_SECOND_TIME_VALUE               = 0;
uint64_t  LAST_TEN_SECOND_TIME_VALUE                = 0;
uint64_t  LAST_THIRTY_SECOND_TIME_VALUE             = 0;
uint64_t  LAST_FOURTY_SECOND_TIME_VALUE             = 0;
uint64_t  LAST_FIFTY_SECOND_TIME_VALUE              = 0;

//-------------------------------------------------------------------------------------------------
// Stored Function inside RAM instead of FLASH, For Reduces latency and better real time operation
// Interrupt Service Routine (ISR)- Dont use any Serial.print or delay function
void IRAM_ATTR onTimer()
{
    //Increase Tick counter
    ssl_timer.RUNNING_TICK_COUNTER++;

    if(((ssl_timer.RUNNING_TICK_COUNTER - LAST_FIVE_SECOND_TIME_VALUE)) >= FIVE_SECOND_TICK_VALUE)
    {
         //Five Second Flag
         ssl_timer.FIVE_SECOND_ELAPSED_FLAG = true;

         LAST_FIVE_SECOND_TIME_VALUE = ssl_timer.RUNNING_TICK_COUNTER;

    }

    if(((ssl_timer.RUNNING_TICK_COUNTER - LAST_TEN_SECOND_TIME_VALUE)) >= TEN_SECOND_TICK_VALUE)
    {
        //Ten Second Flag
        ssl_timer.TEN_SECOND_ELAPSED_FLAG = true;

        LAST_TEN_SECOND_TIME_VALUE = ssl_timer.RUNNING_TICK_COUNTER;

    }

    if(((ssl_timer.RUNNING_TICK_COUNTER - LAST_THIRTY_SECOND_TIME_VALUE)) >= THIRTY_SECOND_TICK_VALUE)
    {
       //Thirty Second Flag
       ssl_timer.THIRTY_SECOND_ELAPSED_FLAG = true;

       LAST_THIRTY_SECOND_TIME_VALUE = ssl_timer.RUNNING_TICK_COUNTER;

    }

    if(((ssl_timer.RUNNING_TICK_COUNTER - LAST_FOURTY_SECOND_TIME_VALUE)) >= FOURTY_SECOND_TICK_VALUE)
    {
        //Fourty Second Flag
        ssl_timer.FOURTY_SECOND_ELAPSED_FLAG = true;

        LAST_FOURTY_SECOND_TIME_VALUE = ssl_timer.RUNNING_TICK_COUNTER;

    }

    if(((ssl_timer.RUNNING_TICK_COUNTER - LAST_FIFTY_SECOND_TIME_VALUE)) >= FIFTY_SECOND_TICK_VALUE)
    {
        //Fifty second flag
        ssl_timer.FIFTY_SECOND_ELAPSED_FLAG =  true;

        LAST_FIFTY_SECOND_TIME_VALUE = ssl_timer.RUNNING_TICK_COUNTER;

    }





    if(ssl_timer.RUNNING_TICK_COUNTER > (SSL_TICK_COUNTER_ROLLBACK_THRESOLD))
    {
        uint64_t diff_five = ssl_timer.RUNNING_TICK_COUNTER - LAST_FIVE_SECOND_TIME_VALUE;
        uint64_t diff_ten = ssl_timer.RUNNING_TICK_COUNTER - LAST_TEN_SECOND_TIME_VALUE;
        uint64_t diff_thirty = ssl_timer.RUNNING_TICK_COUNTER - LAST_THIRTY_SECOND_TIME_VALUE;
        uint64_t diff_fourty = ssl_timer.RUNNING_TICK_COUNTER - LAST_FOURTY_SECOND_TIME_VALUE;
        uint64_t diff_fifty = ssl_timer.RUNNING_TICK_COUNTER - LAST_FIFTY_SECOND_TIME_VALUE;

        ssl_timer.RUNNING_TICK_COUNTER = 0U;

        LAST_FIVE_SECOND_TIME_VALUE = 0ULL - (diff_five % FIVE_SECOND_TICK_VALUE);
        LAST_TEN_SECOND_TIME_VALUE = 0ULL - (diff_ten % TEN_SECOND_TICK_VALUE);
        LAST_THIRTY_SECOND_TIME_VALUE = 0ULL - (diff_thirty % THIRTY_SECOND_TICK_VALUE);
        LAST_FOURTY_SECOND_TIME_VALUE = 0ULL - (diff_fourty % FOURTY_SECOND_TICK_VALUE);
        LAST_FIFTY_SECOND_TIME_VALUE = 0ULL - (diff_fifty % FIFTY_SECOND_TICK_VALUE);
    }
}
//-------------------------------------------------------------------------------------------------




//-------------------------------------------------------------------------------------------------
//Below Function use for initialize and begin timer as per define interval inside function
//Argument : Pointer of hw_timer_t and ssl_timer_t
//Note     : After this function call, Timer will start Automatically
//Return   : void
void init_hw_ssl_timer(hw_timer_t *hw_tim, ssl_timer_t*ssl_tim)
{
    ssl_tim->RUNNING_TICK_COUNTER                               = 0U;
    ssl_tim->TIMER_ENABLE_FLAG                                  = false;
    ssl_tim->FIFTY_SECOND_ELAPSED_FLAG                          = false;
    ssl_tim->TEN_SECOND_ELAPSED_FLAG                            = false;
    ssl_tim->THIRTY_SECOND_ELAPSED_FLAG                         = false;
    ssl_tim->FOURTY_SECOND_ELAPSED_FLAG                         = false;
    ssl_tim->FIFTY_SECOND_ELAPSED_FLAG                          = false;



    //Timer initialization


    //Configure the timer (timer index 0, divider 80 to get 1 microsecond tick)
    //On the ESP32, the hardware timers are typically driven by the APB clock, which runs at 80 MHz by default
    //timerBegin(timerNumber, prescalerValue, countUp) - Here SSL_TIMER_PRESCALER = 80 in ssl_timer.h file
    hw_tim = timerBegin(0, SSL_TIMER_PRESCALER, true);


    //Attach the interrupt to the timer -   link a hardware timer to an Interrupt Service Routine (ISR)
    timerAttachInterrupt(hw_tim, &onTimer, true);


    //Set the alarm to trigger every 1000uS (SSL_TIMER_INTERVAL_MICROSECOND = 1000 microseconds, defined in SSL_TIMER.h) - Sets the alarm interval for timer interrupts
    timerAlarmWrite(hw_tim, SSL_TIMER_INTERVAL_MICROSECOND, true);


    //Start the timer
    timerAlarmEnable(hw_tim);

    //Set Flag True after complete initialization
    ssl_tim->TIMER_ENABLE_FLAG                                  = true;
}
//-------------------------------------------------------------------------------------------------