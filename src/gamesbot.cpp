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

#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <rsl/net/socket/ipv4.h>
#include <rsl/net/socket/socketstream.h>
#include <rsl/net/socket/select.h>
#include <rsl/net/irc/message.h>
#include <rsl/net/irc/user.h>
#include "commands.h"
#include "database.h"
#include "gamesbot.h"
#include "highscore.h"
#include "keys.h"
#include "timers.h"

using namespace Rsl::Net::IRC;
using namespace Rsl::Net::Socket;

/**
 ** Callbacks
 **/
static int do_numeric(IRCClient* irc, const IRCUser* source, const IRCMessage* _msg)
{
  const IRCMessageNumeric* msg = dynamic_cast<const IRCMessageNumeric *>(_msg);

  if (msg)
  {
    switch (msg->GetNumeric())
    {
      case 1:
      {
        GamesBot::Instance()->OnConnect();
        break;
      }
    }
  }

  return 0;
}

static int do_ping(IRCClient* irc, const IRCUser* source, const IRCMessage* _msg)
{
  const IRCMessagePing* msg = dynamic_cast<const IRCMessagePing *>(_msg);

  if (msg)
  {
    irc->Send(IRCMessagePong(msg->GetText()));
  }

  return 0;
}

static int do_privmsg(IRCClient* irc, const IRCUser* source, const IRCMessage* _msg)
{
  const IRCMessagePrivmsg* msg = dynamic_cast<const IRCMessagePrivmsg *>(_msg);

  if (msg)
  {
    GamesBot* bot = GamesBot::Instance();

    const std::string& text = msg->GetText().GetText();
    if (text[0] == '!')
      CommandHandler::Instance()->Handle(source, (const IRCUser *)&msg->GetDest(), text);
    else if (*(bot->GetGame()) != '\0')
      bot->SendToGame(source->GetName().c_str(), text.c_str());
  }

  return 0;
}


/**
 ** Singleton
 **/
GamesBot* GamesBot::Instance()
{
  static GamesBot* instance = 0;
  if (instance == 0)
    instance = new GamesBot();
  return instance;
}


/**
 ** Bot source code
 **/
GamesBot::GamesBot()
  : m_errno(0), m_error(""), m_game(0), m_gamesPath("")
{
}

GamesBot::~GamesBot()
{
  UnloadGames();
}

bool GamesBot::Ok() const
{
  return !m_errno;
}

int GamesBot::Errno() const
{
  return m_errno;
}

const char* GamesBot::Error() const
{
  return m_error.c_str();
}

bool GamesBot::Initialize(int argc, char* argv[], char* envp[])
{
  char* configFile = 0;
  char* dbFile = 0;
  char* gamesPath = 0;
  bool daemonize = true;

  /* Read command-line options */
  static struct option long_options[] = {
    { "conffile",   true,   0,  'f' },
    { "help",       false,  0,  'h' },
    { "verbose",    false,  0,  'v' },
    { "dbpath",     true,   0,  'd' },
    { "gamespath" , true,   0,  'g' },
    { 0,            0,      0,   0  },
  };
  int option_index = 0;
  int getopt_retval = 0;

  while (1)
  {
    getopt_retval = getopt_long(argc, argv, "f:hvd:g:", long_options, &option_index);
    if (getopt_retval == -1)
    {
      break;
    }

    switch (getopt_retval)
    {
      case 'f':
      {
        if (configFile) free(configFile);
        configFile = strdup(optarg);
        break;
      }
      case 'h':
      {
        ShowHelp(argc, argv, envp);
        exit(EXIT_SUCCESS);
        break;
      }
      case 'v':
      {
        daemonize = false;
        break;
      }
      case 'd':
      {
        if (dbFile) free(dbFile);
        dbFile = strdup(optarg);
        break;
      }
      case 'g':
      {
        if (gamesPath) free(gamesPath);
        gamesPath = strdup(optarg);
        break;
      }
    }
  }

  if (daemonize)
  {
    bool fromTerminal = isatty(0) && isatty(1) && isatty(2);
    if (fromTerminal)
    {
      close(0);
      close(1);
      close(2);
    }
    if (fork())
    {
      exit(EXIT_SUCCESS);
    }
  }

  /* Load the configuration */
  if (!configFile)
  {
    char tmp[PATH_MAX + 1];
    snprintf(tmp, sizeof(tmp), "%s/gamesbot.conf", SYSCONFDIR);
    configFile = strdup(tmp);
  }

  m_config.Create(configFile);
  if (!m_config.Ok())
  {
    char errMsg[1024];
    m_errno = m_config.Errno();
    snprintf(errMsg, sizeof(errMsg), "Cannot parse the config file ('%s'): %s", configFile, m_config.Error());
    m_error = errMsg;

    free(configFile);
    return false;
  }
  free(configFile);


  /* Load the database */
  if (!dbFile)
  {
    char dir[PATH_MAX + 1];
    char tmp[PATH_MAX + 1];
    snprintf(dir, sizeof(dir), "%s/.%s", (getenv("HOME") ? getenv("HOME") : getenv("PWD")), GetPackageName());
    mkdir(dir, 0755);
    snprintf(tmp, sizeof(tmp), "%s/%s.db", dir, GetPackageName());
    dbFile = strdup(tmp);
  }

  Database* db = Database::Instance();
  if (!db->Create(dbFile))
  {
    char errMsg[1024];
    m_errno = db->Errno();
    snprintf(errMsg, sizeof(errMsg), "Cannot open the database file ('%s'): %s", dbFile, db->Error());
    m_error = errMsg;

    free(dbFile);
    return false;
  }
  free(dbFile);

  /* Initialize the high scores */
  HighScore::Instance();

  /* Startup timers */
  Timers::Instance();


  /* Load games */
  if (!gamesPath)
    m_gamesPath = GAMESDIR;
  else
  {
    m_gamesPath = gamesPath;
    free(gamesPath);
  }

  if (!ReloadGames())
    return false;


  /* Create the IRC bot */
  IPV4Addr addr(m_config.IRCServer.address, m_config.IRCServer.service);
  IPV4Addr bindAddr("0.0.0.0", "0");
  char password[256];
  if (strlen(m_config.IRCServer.password) > 0)
    keysDecode(m_config.IRCServer.password, password);
  else
    *password = '\0';
  m_client.Create(addr, bindAddr,
                  m_config.Bot.nickname,
                  m_config.Bot.username,
                  m_config.Bot.fullname,
                  password,
                  m_config.IRCServer.useSSL,
                  m_config.IRCServer.sslCert);
  memset(password, 0, sizeof(password));

  if (!m_client.Ok())
  {
    m_errno = m_client.Errno();
    m_error = m_client.Error();
    return false;
  }
  
  m_client.RegisterNumericsCallback(                        do_numeric);
  m_client.RegisterCallback(        new IRCMessagePrivmsg,  do_privmsg);
  m_client.RegisterCallback(        new IRCMessagePing,     do_ping);

  return true;
}

bool GamesBot::Run()
{
  if (!m_client.Connect())
  {
    m_errno = m_client.Errno();
    m_error = m_client.Error();
    return false;
  }

  SocketSelect selector(1);
  selector.Add(&m_client.GetSocket(), RSL_SELECT_EVENT_IN);

  Timers* timers = Timers::Instance();
  selector.SetTimeout(timers->GetNextExecution());

  SocketClient* socks[1];
  int nsocks;
  while ((nsocks = selector.Select(socks, 1)) > -1)
  {
    if (nsocks > 0)
      m_client.Loop();

    timers->Execute();
    selector.SetTimeout(timers->GetNextExecution());
  }

  if (nsocks == -1)
  {
    m_errno = selector.Errno();
    m_error = selector.Error();
    return false;
  }

  return true;
}

void GamesBot::OnConnect()
{
  char identifyMsg[256];
  char password[256];

  if (strlen(m_config.Bot.password) > 0)
  {
    memset(password, 0, sizeof(password));
    memset(identifyMsg, 0, sizeof(identifyMsg));
    keysDecode(m_config.Bot.password, password);
    snprintf(identifyMsg, sizeof(identifyMsg), "IDENTIFY %s", password);
    m_client.Send(IRCMessagePrivmsg("NickServ", identifyMsg));
    memset(password, 0, sizeof(password));
    memset(identifyMsg, 0, sizeof(identifyMsg));
  }

  m_client.Send(IRCMessageUmode(m_client.GetMe(), "+d"));
  m_client.Send(IRCMessageJoin(m_config.Bot.channel));
}

const IRCClient& GamesBot::GetClient() const
{
  return m_client;
}

void GamesBot::Send(const IRCText& msg)
{
  if (m_client.Ok())
    m_client.Send(IRCMessagePrivmsg(m_config.Bot.channel, msg));
}

void GamesBot::Quit(const IRCText& msg)
{
  if (m_client.Ok())
    m_client.Send(IRCMessageQuit(msg));
}

void GamesBot::SendToGame(const char* source, const char* text)
{
  if (m_game)
    m_game->ParseText(source, text);
}

void GamesBot::UnloadGames()
{
  if (m_game)
  {
    m_game->Stop();
    m_game = 0;
  }
  m_games.erase(m_games.begin(), m_games.end());
  for (std::vector<MODULEHANDLE>::iterator i = m_gameModules.begin();
       i != m_gameModules.end();
       i++)
  {
    gameCleanup_t cleanupf = (gameCleanup_t)dlsym((*i), "cleanup");
    if (cleanupf)
      cleanupf();
    dlclose((*i));
  }
  m_gameModules.erase(m_gameModules.begin(), m_gameModules.end());
}

bool GamesBot::ReloadGames()
{
  DIR* gamesDir = opendir(m_gamesPath.c_str());
  dirent* dent;

  if (!gamesDir)
  {
    char errMsg[1024];
    m_errno = errno;
    snprintf(errMsg, sizeof(errMsg), "Unable to open the games directory ('%s'): %s", m_gamesPath.c_str(), strerror(errno));
    m_error = errMsg;
    return false;
  }

  /* Save the current game name and unload all games */
  std::string curGameName("");
  if (m_game)
    curGameName = m_game->GetName();
  UnloadGames();

  /* Reload games */
  errno = 0;
  while ((dent = readdir(gamesDir)) != 0)
  {
    errno = 0;
    if (!strcmp(dent->d_name + strlen(dent->d_name) - 3, ".so"))
    {
      char path[PATH_MAX + 1];
      snprintf(path, sizeof(path), "%s/%s", m_gamesPath.c_str(), dent->d_name);
      MODULEHANDLE gameModule = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
      if (gameModule)
      {
        gameStartup_t startupf = (gameStartup_t)dlsym(gameModule, "startup");
        if (startupf)
        {
          Game* newGame = startupf();
          m_games.push_back(newGame);
          m_gameModules.push_back(gameModule);
        }
        else
        {
          printf("Error loading game startup function from '%s': %s\n", path, dlerror());
          dlclose(gameModule);
        }
      }
      else
      {
        printf("Error opening game '%s': %s\n", path, dlerror());
      }
    }
  }

  if (errno != 0)
  {
    char errMsg[1024];
    m_errno = errno;
    snprintf(errMsg, sizeof(errMsg), "Some error occured when reading the games directory: %s\n", strerror(errno));
    m_error = errMsg;
    closedir(gamesDir);
    return false;
  }

  /* Restart the current game if it's still available */
  if (curGameName != "")
  {
    Game* cur = FindGame(curGameName.c_str());
    if (cur)
    {
      m_game = cur;
      cur->Start();
    }
  }

  closedir(gamesDir);
  return true;
}

const char* GamesBot::GetGame() const
{
  if (m_game)
    return m_game->GetName();
  else
    return "";
}

Game* GamesBot::FindGame(const char* name) const
{
  for (std::vector<Game *>::const_iterator i = m_games.begin();
       i != m_games.end();
       i++)
  {
    Game* cur = (*i);
    if (!strcasecmp(cur->GetName(), name))
      return cur;
  }

  return 0;
}

const std::vector<std::string> GamesBot::ListGames() const
{
  std::vector<std::string> gameList;

  for (std::vector<Game *>::const_iterator i = m_games.begin();
       i != m_games.end();
       i++)
  {
    gameList.push_back((*i)->GetName());
  }

  return gameList;
}

bool GamesBot::StartGame(const char* name)
{
  if (m_game)
    return false;

  Game* game = FindGame(name);
  if (!game)
    return false;
  m_game = game;
  m_game->Start();

  m_client.Send(IRCMessageUmode(m_client.GetMe(), "-d"));

  return true;
}

void GamesBot::StopGame()
{
  if (m_game)
  {
    m_game->Stop();
    m_game = 0;
    m_client.Send(IRCMessageUmode(m_client.GetMe(), "+d"));
  }
}
