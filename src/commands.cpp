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
 *     * Neither the name of the Mantis Bot nor the names of its contributors may be used to endorse or
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
#include "commands.h"
#include "gamesbot.h"
#include "keys.h"

using namespace Rsl::Net::IRC;

#define COMMAND(x) void CommandHandler::cmd_ ## x (GamesBot* bot, const IRCUser* source, const IRCUser* dest, const std::vector<std::string>& params)


COMMAND(games)
{
  bot->Send(IRCText("%C03List of commands:%C"));
  bot->Send(IRCText("%C12!list%C           Lists the available games"));
  bot->Send(IRCText("%C12!start <game>%C   Starts a game"));
  bot->Send(IRCText("%C12!stop%C           Stops the current game"));
  bot->Send(IRCText("%C12!refresh%C        Reloads the available games"));
}

COMMAND(list)
{
  bot->Send(IRCText("%C03Available games:%C"));
  const std::vector<std::string>& availGames = bot->ListGames();

  for (std::vector<std::string>::const_iterator i = availGames.begin();
       i != availGames.end();
       i++)
  {
    if ((*i) == bot->GetGame())
      bot->Send(IRCText("%B%C02*%C%B %s", (*i).c_str()));
    else
      bot->Send((*i).c_str());
  }
}

COMMAND(start)
{
  if (params.size() < 2)
    return;

  const char* curGame = bot->GetGame();
  if (*curGame != '\0')
  {
    bot->Send(IRCText("%C04Error:%C There is an already started game, use %C12!stop%C to stop it."));
  }
  else
  {
    bot->Send(IRCText("Starting game %C12%s%C ...", params[1].c_str()));
    if (!bot->StartGame(params[1].c_str()))
      bot->Send(IRCText("%C04Error:%C Unable to start game \"%s\"", params[1].c_str()));
  }
}

COMMAND(stop)
{
  const char* curGame = bot->GetGame();
  if (*curGame == '\0')
  {
    bot->Send(IRCText("%C04Error:%C There are not running games"));
  }
  else
  {
    bot->Send(IRCText("Stopping game %C12%s%C...", curGame));
    bot->StopGame();
  }
}

COMMAND(refresh)
{
  bot->Send(IRCText("Refreshing games ..."));
  bot->ReloadGames();
  bot->Send(IRCText("Done!"));
}
/* */





CommandHandler* CommandHandler::Instance()
{
  static CommandHandler* instance = 0;
  if (instance == 0)
    instance = new CommandHandler();
  return instance;
}

CommandHandler::CommandHandler()
{
#define ADDCOMMAND(x) RegisterCommand( #x , CommandHandler::cmd_ ##x )
  ADDCOMMAND(games);
  ADDCOMMAND(list);
  ADDCOMMAND(start);
  ADDCOMMAND(stop);
  ADDCOMMAND(refresh);
#undef ADDCOMMAND
}

void CommandHandler::RegisterCommand(const std::string& name, cmd_t callback)
{
  Command cmd;
  cmd.name = name;
  cmd.callback = callback;
  m_commands.push_back(cmd);
}

static inline void Split(const std::string& str, std::vector<std::string>& dest)
{
  int p;
  int i = 0;

  do
  {
    while (str[i] == ' ')
    {
      i++;
    }

    p = str.find(" ", i);
    if (p > -1)
    {
      dest.push_back(str.substr(i, p - i));
      i = p + 1;
    }
  } while (p > -1);

  if (str[i] != '\0')
  {
    dest.push_back(str.substr(i));
  }
}

void CommandHandler::Handle(const IRCUser* source, const IRCUser* dest, const std::string& text)
{
  if ( text.length() < 2 || text[0] != '!' )
    return;

  std::vector<std::string> params;
  Split(text.c_str() + 1, params);

  if ( params.size() > 0 )
  {
    for (std::vector<Command>::iterator i = m_commands.begin();
         i != m_commands.end();
         i++)
    {
      if (!strcasecmp((*i).name.c_str(), params[0].c_str()))
      {
        (*i).callback(GamesBot::Instance(), source, dest, params);
        break;
      }
    }
  }
}

#undef COMMAND
