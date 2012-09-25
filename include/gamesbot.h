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

#ifndef __GAMESBOT_H
#define __GAMESBOT_H

#include <string>
#include <vector>
#include <rsl/net/irc/client.h>
#include <rsl/net/irc/text.h>
#include "configuration.h"
#include "game.h"

typedef void * MODULEHANDLE;

extern void ShowHelp(int argc, char* argv[], char* envp[]);
extern const char* GetPackageName();

extern "C" class Test
{
public:
  Test(int num);
  int GetNum() const;
private:
  int m_num;
};

class GamesBot
{
public:
  static GamesBot* Instance();

public:
  GamesBot();
  ~GamesBot();

  bool Ok() const;
  int Errno() const;
  const char* Error() const;

  bool Initialize(int argc, char* argv[], char* envp[]);
  bool Run();

  const Rsl::Net::IRC::IRCClient& GetClient() const;

  void OnConnect();
  void Send(const Rsl::Net::IRC::IRCText& msg);
  void Quit(const Rsl::Net::IRC::IRCText& msg);
  void SendToGame(const char* source, const char* dest, const char* text);

  const char* GetGame() const;
  bool StartGame(const char* name);
  void StopGame();
  bool ReloadGames();
  void UnloadGames();
  const std::vector<std::string> ListGames() const;

protected:
  Game* FindGame(const char* name) const;

private:
  int m_errno;
  std::string m_error;
  Configuration m_config;
  Rsl::Net::IRC::IRCClient m_client;
  Game* m_game;
  std::vector<Game *> m_games;
  std::vector<MODULEHANDLE> m_gameModules;
  std::string m_gamesPath;
};

#endif /* #ifndef __GAMESBOT_H */
