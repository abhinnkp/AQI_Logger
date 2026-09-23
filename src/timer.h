/*
This library is primarily designed to facilitate the creation of timer-based interrupts within code, as well as to manage flags and execute operations accordingly.
Note    : Timer must be enable after complition of all initialization task in setup function
NOTE(2) : The code does not incorporate any functionality /library based on a Real-Time Operating System (RTOS).
          Consequently, after a certain period, when the timer experiences a rollback and resets the counter, there may be a slight drift in the time scale.
          LCM concept for ROLLBACK implemented to avoid this (ROLL BACK VALUE is divisible without reminder for all TICK COUNTERs)
          This drift is automatically corrected in the next minute.
*/

#ifndef __TIMER_H
#define __TIMER_H

#include <Arduino.h>


#define  SSL_TIMER_INTERVAL_MICROSECOND                 1000LL                                                                                  //1000 Microsecond based Timer interval - 1msec
#define  SSL_TIMER_PRESCALER                            80LL                                                                                    //TIMER PRESCALER - 80 FOR 1uS tick - On the ESP32, the hardware timers are typically driven by the APB clock, which runs at 80 MHz by default
#define  ONE_SECOND_TICK_VALUE                          (uint64_t)((long long)(1000LL/(long long)SSL_TIMER_INTERVAL_MICROSECOND)*1000LL)        //TICK VALUE WHEN 1 Second elapsed
#define  FIVE_SECOND_TICK_VALUE                         (uint64_t)((long long)ONE_SECOND_TICK_VALUE * 5LL)                                      //TICK VALUE WHEN 5 Second elapsed
#define  TEN_SECOND_TICK_VALUE                          (uint64_t)((long long)ONE_SECOND_TICK_VALUE * 10LL)                                     //TICK VALUE WHEN 10 Second elapsed
#define  THIRTY_SECOND_TICK_VALUE                       (uint64_t)((long long)ONE_SECOND_TICK_VALUE * 30LL)                                     //TICK VALUE WHEN 30 Second elapsed
#define  FOURTY_SECOND_TICK_VALUE                       (uint64_t)((long long)ONE_SECOND_TICK_VALUE * 40LL)                                     //TICK VALUE WHEN 40 Second elapsed
#define  FIFTY_SECOND_TICK_VALUE                        (uint64_t)((long long)ONE_SECOND_TICK_VALUE * 50LL)                                     //TICK VALUE WHEN 50 Second elapsed

//Lets Find LCM for 10,30,40,50 SECOND_TICK_VALUE, So during ROLLBACK of tick counter we will not miss any tick TASK for next one minute cycle
//Answer of LCM = 600000 (Approx 10 min)
#define  SSL_TICK_COUNTER_ROLLBACK_THRESOLD             (uint64_t)(600000LL)                                                                    //Rollback after 10 min (approx..)
//Timer Related structure
typedef struct
{
    volatile uint64_t                                    RUNNING_TICK_COUNTER;       //Value increase on every Timer interrupt and Rollback to 0,when cross thresold limit mentioned above
    bool                                                 TIMER_ENABLE_FLAG;          //TRUE After enable Timer setup

    //Below Flags are for Time base operation performance
    bool                                                 FIVE_SECOND_ELAPSED_FLAG;
    bool                                                 TEN_SECOND_ELAPSED_FLAG;
    bool                                                 THIRTY_SECOND_ELAPSED_FLAG;
    bool                                                 FOURTY_SECOND_ELAPSED_FLAG;
    bool                                                 FIFTY_SECOND_ELAPSED_FLAG;

}ssl_timer_t;

//-------------------------------------------------------------------------------------------------
// Stored Function inside RAM instead of FLASH, For Reduces latency and better real time operation
// Interrupt Service Routine (ISR)
extern void IRAM_ATTR onTimer();
//-------------------------------------------------------------------------------------------------



//-------------------------------------------------------------------------------------------------
//Below Function use for initialize and begin timer as per define interval inside function
//Argument : Pointer of hw_timer_t and ssl_timer_t
//Return   : void
void init_hw_ssl_timer(hw_timer_t *, ssl_timer_t*);
//-------------------------------------------------------------------------------------------------



#endif