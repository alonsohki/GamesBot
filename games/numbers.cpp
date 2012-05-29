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

#include <string>
#include <vector>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <rsl/net/irc/text.h>
#include "gamesbot.h"
#include "game.h"
#include "highscore.h"
#include "timers.h"

using namespace Rsl::Net::IRC;

static GamesBot* bot = 0;
static Timers* timers = 0;
static HighScore* highscore = 0;

static const unsigned int numbersTable[] = {
 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 25, 50,
 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 25, 75,
 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 25, 75,
 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 25, 75,
 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 25, 100,
 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 50, 100,
 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 50, 500,
 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 50, 500,
};

#define numbersTableLength sizeof(numbersTable) / sizeof(const unsigned int)

/**
 ** infix -> postfix
 **/
#define STACKSIZE 500

static const unsigned int operatorPrecedenceTable[256] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 2, 1, 0, 1, 0, 2, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0
};
static inline bool isOperator(unsigned char c)
{
  if (!operatorPrecedenceTable[c])
    return false;
  return true;
}
static inline unsigned int getOperatorPrecedence(unsigned char c)
{
  return operatorPrecedenceTable[c];
}


/* Shunting yard algorithm to transform a infix expression into a postfix expression.
 * Algorithm extracted from http://en.wikipedia.org/wiki/Shunting_yard_algorithm
 */
static inline char* parseExpression(const char* expr, char* outputQueue, char** errMsg = 0)
{
  char operatorQueue[STACKSIZE];
  char* oq = operatorQueue - 1;
  char* p = outputQueue;
  unsigned int curOperatorPrecedence;

  while (*expr != '\0')
  {
    /* Read the next token */
    while (*expr == ' ')
      expr++;
    if (*expr == '\0')
      break;

    if (isdigit(*expr))
    {
      /* Parse a numeric token */
      *p++ = *expr;
      while (isdigit(*++expr))
        *p++ = *expr;
      *p++ = ' ';
    }

    else if (*expr == '(')
    {
      *++oq = '(';
      expr++;
    }

    else if (*expr == ')')
    {
      while (oq >= operatorQueue && *oq != '(')
      {
        *p++ = *oq--;
        *p++ = ' ';
      }

      if (oq < operatorQueue)
      {
        if (errMsg)
          *errMsg = strdup("Mismatched parenthesis ')'");
        return 0;
      }
      else
        oq--;

      expr++;
    }

    else if (isOperator((unsigned char)*expr))
    {
      /* Parse an operator respecting precedence */
      curOperatorPrecedence = getOperatorPrecedence((unsigned char)*expr);

      while (oq >= operatorQueue && curOperatorPrecedence <= getOperatorPrecedence((unsigned char)*oq))
      {
        *p++ = *oq--;
        *p++ = ' ';
      }
      *++oq = *expr++;
    }

    else
    {
      if (errMsg)
      {
        char msg[256];
        snprintf(msg, sizeof(msg), "Unknown token: '%c'", *expr);
        *errMsg = strdup(msg);
      }
      return 0;
    }
  }

  /* Dump the remaining operators from the operator queue to the output queue */
  while (oq >= operatorQueue)
  {
    if (*oq == '(')
    {
      if (errMsg)
        *errMsg = strdup("Mismatched parenthesis '('");
      return 0;
    }
    *p++ = *oq--;
    *p++ = ' ';
  }

  /* Remove extra spaces in the end of the output queue */
  if (p == outputQueue)
    *p = '\0';
  else
    p[-1] = '\0';

  return outputQueue;
}


class GameNumbers : public Game
{
public:
  static GameNumbers* Instance()
  {
    static GameNumbers* instance = 0;
    if (!instance)
      instance = new GameNumbers();
    return instance;
  }

public:
  GameNumbers()
    : m_roundStarted(false)
  {
    m_randomness = open("/dev/urandom", O_RDONLY);
  }

  virtual ~GameNumbers()
  {
    close(m_randomness);
    timers->Destroy(m_timer);
  }

  const char* GetName() { return "numbers"; }

  void Start()
  {
    bot->Send(IRCText("%C03Numbers game started!%C"));
    StartRound();
  }

  void Stop()
  {
    bot->Send(IRCText("%C03Numbers game stopped!%C"));
    timers->Destroy(m_timer);
    m_roundStarted = false;
  }

  void ParseText(const char* source, const char* text)
  {
    char postfix[STACKSIZE];
    if (m_roundStarted && parseExpression(text, postfix))
    {
      int numNumbers = sizeof(m_roundNumbers) / sizeof(int);
      int copyNumbers[numNumbers];
      memcpy(copyNumbers, m_roundNumbers, sizeof(m_roundNumbers));

      /* Check if the expression numbers are valid */
      for (char* p = postfix; *p != '\0'; p++)
      {
        while (!isdigit(*p) && *p != '\0')
          p++;
        if (*p == '\0')
          break;
        
        char* p2;
        int number = strtol(p, &p2, 10);
        p = p2;

        bool found = false;
        for (int i = 0; i < numNumbers; i++)
        {
          if (copyNumbers[i] == number)
          {
            copyNumbers[i] = -1;
            found = true;
            break;
          }
        }
        if (!found)
        {
          bot->Send(IRCText("%s: Invalid number '%d'", source, number));
          return;
        }
      }

      /* Process his expression */
      ProcessExpression(source, postfix);
    }
  }

  std::string GetNumberList()
  {
    std::string ret;
    int numNumbers = sizeof(m_roundNumbers) / sizeof(int);
    for (int i = 0; i < numNumbers; i++)
    {
      char tmp[256];
      snprintf(tmp, sizeof(tmp), "%d, ", m_roundNumbers[i]);
      ret += tmp;
    }
    ret = ret.substr(0, ret.length() - 2);
    return ret;
  }

  static int compare(const void* a_, const void* b_)
  {
    int* a = (int *)a_;
    int* b = (int *)b_;

    if (*a < *b) return -1;
    else if (*a == *b) return 0;
    else return 1;
  }

  void StartRound()
  {
    /* Choose the random numbers */
    int numNumbers = sizeof(m_roundNumbers) / sizeof(int);
    for (int i = 0; i < numNumbers; i++)
    {
      int randvalue;
      bool alreadyUsed;
      do
      {
        alreadyUsed = false;
        read(m_randomness, (void *)&randvalue, sizeof(int));
        if (randvalue < 0)
          randvalue = -randvalue;
        randvalue = randvalue % numbersTableLength;

        for (int ii = 0; ii < i; ii++)
        {
          if (m_roundNumbers[ii] == randvalue)
          {
            alreadyUsed = true;
            break;
          }
        }
      } while (alreadyUsed);
      m_roundNumbers[i] = randvalue;
    }

    read(m_randomness, (void *)&m_target, sizeof(int));
    if (m_target < 0)
      m_target = -m_target;
    m_target = m_target % 1000;

    /* Transform the number list into the real numbers, not just references to the table */
    for (int i = 0; i < numNumbers; i++)
    {
      m_roundNumbers[i] = numbersTable[m_roundNumbers[i]];
    }
    qsort(m_roundNumbers, numNumbers, sizeof(int), GameNumbers::compare);

    m_timeRemaining = 120;
    m_winner = "";
    m_winnerValue = 0;
    bot->Send(IRCText("Round time: %C042 minutes%C"));
    bot->Send(IRCText("Use the numbers %C12%s%C to get the target %C03%d%C", GetNumberList().c_str(), m_target));
    m_timer = timers->Create(GameNumbers::StaticRoundStep, 3, 40000);
    m_roundStarted = true;
  }

  void RoundStep()
  {
    m_timeRemaining -= 40;
    if (m_timeRemaining > 0)
    {
      bot->Send(IRCText("Time remaining: %C04%d seconds%C", m_timeRemaining));
      bot->Send(IRCText("Use the numbers %C12%s%C to get the target %C03%d%C", GetNumberList().c_str(), m_target));
    }
    else
    {
      if (m_winner == "")
      {
        bot->Send(IRCText("%C04Time is over!%C Good luck in the next round..."));
      }
      else
      {
        bot->Send(IRCText("%C04Time is over!%C The winner is %C12%s%C (%C03%d%C)", m_winner.c_str(), m_winnerValue));
        int diff = m_target - m_winnerValue;
        if (diff < 0)
          diff = -diff;
        if (diff > 5)
          bot->Send(IRCText("%C12Difference is bigger than 5, so no point for you%C"));
        else
          SetWinner(m_winner.c_str());
      }
      m_timer = timers->Create(GameNumbers::StaticRoundStart, 1, 4500);
      m_roundStarted = false;
    }
  }

  static void StaticRoundStep(void*)
  {
    GameNumbers::Instance()->RoundStep();
  }

  static void StaticRoundStart(void*)
  {
    GameNumbers::Instance()->StartRound();
  }

  void ProcessExpression(const char* source, char* expr)
  {
    int valueStack[STACKSIZE];
    int *stackP = valueStack - 1;

    #define GET_STACKSIZE() (stackP + 1 - valueStack)

    while (*expr != '\0')
    {
      while (*expr == ' ')
        expr++;
      if (*expr == '\0')
        break;

      if (isdigit(*expr))
      {
        char* p = expr;
        char* p2;
        int value = strtol(p, &p2, 10);
        expr = p2;
        *++stackP = value;
      }
      else if (*expr == '+')
      {
        if (GET_STACKSIZE() < 2) return;
        stackP[-1] += stackP[0];
        stackP--;
        expr++;
      }
      else if (*expr == '-')
      {
        if (GET_STACKSIZE() < 2) return;
        stackP[-1] -= stackP[0];
        stackP--;
        expr++;
      }
      else if (*expr == '/')
      {
        if (GET_STACKSIZE() < 2) return;
        stackP[-1] /= stackP[0];
        stackP--;
        expr++;
      }
      else if (*expr == '*')
      {
        if (GET_STACKSIZE() < 2) return;
        stackP[-1] *= stackP[0];
        stackP--;
        expr++;
      }
      else
        return; /* Unknown token */
    }

    if (GET_STACKSIZE() != 1) return;

    if (valueStack[0] == m_target)
    {
      /* Exact value */
      timers->Destroy(m_timer);
      m_timer = timers->Create(GameNumbers::StaticRoundStart, 1, 4500);
      m_roundStarted = false;

      bot->Send(IRCText("%B%C03%s calculated the exact value! Congratulations%C%B", source));
      SetWinner(source);

      return;
    }

    int diff = m_target - valueStack[0];
    if (diff < 0)
      diff = -diff;
    int winnerDiff = m_target - m_winnerValue;
    if (winnerDiff < 0)
      winnerDiff = -winnerDiff;

    bot->Send(IRCText("%s: %d", source, valueStack[0]));
    if (diff < winnerDiff)
    {
      m_winner = source;
      m_winnerValue = valueStack[0];
      bot->Send(IRCText("%C06New nearest value for %s!%C", source));
    }

#undef GET_STACKSIZE
  }

  void SetWinner(const char* nickname)
  {
    int curScore = highscore->GetScore(nickname, "numbers");
    highscore->SetScore(nickname, "numbers", curScore + 1);

    std::vector<std::string> nicknames;
    std::vector<int> scores;

    highscore->GetGameTop("numbers", nicknames, scores, 5);

    std::string topList("%BHigh scores:%B ");
    bool appearsNickInTop = false;
    for (unsigned int i = 0; i < nicknames.size(); i++)
    {
      char tmp[256];
      if (!strcasecmp(nicknames[i].c_str(), nickname))
      {
        appearsNickInTop = true;
        snprintf(tmp, sizeof(tmp), "%%C%02d%%B%s: %d%%B%%C, ", 5+i, nicknames[i].c_str(), scores[i]);
      }
      else
        snprintf(tmp, sizeof(tmp), "%%C%02d%s: %d%%C, ", 5+i, nicknames[i].c_str(), scores[i]);

      topList += tmp;
    }
    topList = topList.substr(0, topList.length() - 2);

    if (!appearsNickInTop)
    {
      char tmp[256];
      snprintf(tmp, sizeof(tmp), " ... %%B%s: %d%%B", nickname, curScore + 1);
      topList += tmp;
    }

    bot->Send(IRCText(topList));
  }

private:
  std::string m_winner;
  int m_winnerValue;
  Timer* m_timer;
  int m_timeRemaining;
  int m_randomness;
  int m_target;
  int m_roundNumbers[7];
  bool m_roundStarted;
};


extern "C" Game* startup()
{
  bot = GamesBot::Instance();
  timers = Timers::Instance();
  highscore = HighScore::Instance();

  return GameNumbers::Instance();
}

extern "C" void cleanup()
{
  delete GameNumbers::Instance();
}

