/*
 * Copyright (c) 2007, Alberto Alonso Pinto
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions
 *       and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 *       and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Games Bot nor the names of its contributors may be used to endorse or
 *       promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/time.h>
#include <time.h>
#include "timers.h"

class Timer
{
  friend class Timers;

public:
  Timer(TimerCbk_t cbk, int nrep, unsigned int ms, void* userData)
    : next(0), prev(0), m_cbk(cbk), m_nrep(nrep), m_ms(ms), m_userdata(userData)
  {
    SaveCurrentTime();
  }

  ~Timer()
  {
    Timers::Instance()->DeleteFromList(this);
  }

  inline const timeval& GetNextExecution() { return m_nextExecution; }
  inline int GetRemainingRepetitions() { return m_nrep; }

  Timer* next;
  Timer* prev;

  inline TimerCbk_t GetCallback() { return m_cbk; }
  inline void* GetUserdata() { return m_userdata; }

private:
  inline void Update()
  {
    if (m_nrep > 0)
      m_nrep--;
    SaveCurrentTime();
  }

  inline void SaveCurrentTime()
  {
    gettimeofday(&m_lastExecution, 0);

    m_nextExecution.tv_sec = m_lastExecution.tv_sec + (unsigned int)(m_ms / 1000);
    long us = (m_ms % 1000) * 1000;
    if (m_lastExecution.tv_usec + us > 999999)
    {
      m_nextExecution.tv_sec++;
      us = us - 1000000;
    }
    m_nextExecution.tv_usec = m_lastExecution.tv_usec + us;
  }

  TimerCbk_t m_cbk;
  int m_nrep;
  unsigned long m_ms;
  void* m_userdata;
  timeval m_lastExecution;
  timeval m_nextExecution;
};

Timers* Timers::Instance()
{
  static Timers* instance = 0;
  if (!instance)
    instance = new Timers();
  return instance;
}

Timers::Timers()
  : m_firstTimer(0), m_lastTimer(0)
{
}

Timers::~Timers()
{
  for (Timer* s = m_firstTimer; s != 0; s = m_firstTimer)
    delete s;
}

static inline bool CompareTimevals(const timeval& t1, const timeval& t2)
{
  if (t1.tv_sec > t2.tv_sec)
    return true;
  else if (t1.tv_sec == t2.tv_usec && t1.tv_usec > t2.tv_usec)
    return true;
  else
    return false;
}

void Timers::InsertTimer(Timer* timer)
{
  if (!m_firstTimer)
  {
    m_firstTimer = timer;
    m_lastTimer = timer;
  }
  else
  {
    Timer* s;
    Timer* s2 = 0;
    const timeval& tv = timer->GetNextExecution();

    for (s = m_firstTimer; s != 0; s = s->next)
    {
      if (CompareTimevals(s->GetNextExecution(), tv))
        break;
      s2 = s;
    }

    if (!s2)
    {
      /* First timer to be executed */
      m_firstTimer->prev = timer;
      timer->next = m_firstTimer;
      m_firstTimer = timer;
    }
    else if (!s)
    {
      /* Last timer to be executed */
      m_lastTimer->next = timer;
      timer->prev = m_lastTimer;
      m_lastTimer = timer;
    }
    else
    {
      /* Intermediate */
      s2->next = timer;
      s->prev = timer;
      timer->next = s;
      timer->prev = s2;
    }
  }
}

Timer* Timers::Create(TimerCbk_t cbk, int nrep, unsigned int ms, void* userData)
{
  if (ms < 10)
    return 0;

  Timer* newTimer = new Timer(cbk, nrep, ms, userData);
  InsertTimer(newTimer);

  return newTimer;
}

void Timers::Destroy(Timer* timer)
{
  if (FindTimer(timer))
  {
    DeleteFromList(timer);
    delete timer;
  }
}

void Timers::DeleteFromList(Timer* timer)
{
  /* Make sure that the timer is in the list */
  if (!FindTimer(timer))
    return;


  /* Unlink the timer */
  if (timer->next)
    timer->next->prev = timer->prev;
  if (timer->prev)
    timer->prev->next = timer->next;

  if (timer == m_firstTimer)
    m_firstTimer = timer->next;
  if (timer == m_lastTimer)
    m_lastTimer = timer->prev;
}

long Timers::GetNextExecution() const
{
  if (m_firstTimer)
  {
    const timeval& nextExecution = m_firstTimer->GetNextExecution();
    timeval curTime;

    gettimeofday(&curTime, 0);
    if (CompareTimevals(curTime, nextExecution))
    {
      return 1;
    }
    else
    {
      long remaining;
      remaining = (nextExecution.tv_sec - curTime.tv_sec) * 1000;
      if (nextExecution.tv_usec < curTime.tv_usec)
      {
        remaining -= 1000;
        remaining += (nextExecution.tv_usec + 1000000 - curTime.tv_usec) / 1000;
      }
      else
        remaining += (nextExecution.tv_usec - curTime.tv_usec) / 1000;

      return remaining;
    }
  }
  else
    return -1;
}

void Timers::Execute()
{
  timeval curTime;
  gettimeofday(&curTime, 0);

  for (Timer* s = m_firstTimer; s != 0; s = m_firstTimer)
  {
    if (CompareTimevals(s->GetNextExecution(), curTime))
      break;

    TimerCbk_t cbk = s->GetCallback();
    void* userdata = s->GetUserdata();
    s->Update();
    int remainingReps = s->GetRemainingRepetitions();

    cbk(userdata);

    if (FindTimer(s))
    {
      DeleteFromList(s);
      if (remainingReps == 0)
        delete s;
      else
        InsertTimer(s);
    }
  }
}

Timer* Timers::FindTimer(Timer* timer)
{
  for (Timer* s = m_firstTimer; s != 0; s = s->next)
  {
    if (timer == s)
      return s;
  }
  return 0;
}
