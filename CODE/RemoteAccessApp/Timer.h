// 1ms timer

#ifndef _TIMER_H
#define _TIMER_H

#include "global.h"

class Timer
{
  public:
    Timer(void);
    ~Timer(void);
    /**************************************************************************
    DOES:    This function reads a 1 millisecond timer tick. The timer tick
             must be a UNSIGNED16 and must be incremented once per millisecond.
    RETURNS: 1 millisecond timer tick
    **************************************************************************/
    UNSIGNED16 GetTime(void);
    /**************************************************************************
    DOES:    This function compares a UNSIGNED16 timestamp to the internal 
             timer tick and returns 1 if the timestamp expired/passed.
    RETURNS: 1 if timestamp expired/passed
             0 if timestamp is not yet reached
    NOTES:   The maximum timer runtime measurable is 0x8000 (about 32 seconds).
             For the usage in MicroCANopen that is sufficient. 
    **************************************************************************/
    UNSIGNED8 IsTimeExpired(UNSIGNED16 timestamp);

    static void Sleep(unsigned long milliseconds);

  private:
#ifdef WIN32
    // multimedia timer configuration
    TIMECAPS timecaps;
    // multimedia timer handle
    MMRESULT mmtimerhandle;
#endif // WIN32
};

#endif // _TIMER_H
