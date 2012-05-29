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

#include <signal.h>
#include "config.h"
#include "database.h"
#include "gamesbot.h"
#include "highscore.h"
#include "timers.h"

void ShowHelp(int argc, char* argv[], char* envp[])
{
  printf("%s\n", PACKAGE_STRING);
  printf("Usage: %s [OPTION]\n", argv[0]);
  printf("\n");
  printf("Possible options are:\n");
  printf("\t-v, --verbose\tShow debug information\n");
  printf("\t-f, --conffile\tSpecify the configuration file location\n");
  printf("\t-d, --dbpath\tSpecify the location for the database file\n");
  printf("\t-g, --gamespath\tSpecify the location for the games to load\n");
  printf("\n");
  printf("Report bugs to: <%s>\n", PACKAGE_BUGREPORT);
}

const char* GetPackageName()
{
  return PACKAGE;
}

static inline void DeleteInstances()
{
  GamesBot::Instance()->UnloadGames();
  delete Timers::Instance();
  delete HighScore::Instance();
  delete Database::Instance();
  delete GamesBot::Instance();
}

static void sighandler(int signum)
{
  switch (signum)
  {
    case SIGINT:
    {
      GamesBot* bot = GamesBot::Instance();
      bot->Quit("Got SIGINT! Closing...");
      DeleteInstances();
      exit(EXIT_SUCCESS);
      break;
    }
  }
}

int main(int argc, char* argv[], char* envp[])
{
  GamesBot* bot = GamesBot::Instance();

  signal(SIGINT, sighandler);

  puts("Initializing bot ...");
  if (!bot->Initialize(argc, argv, envp))
  {
    printf("Error initializing bot: %s\n", bot->Error());
    DeleteInstances();
    return EXIT_FAILURE;
  }

  puts("Running bot ...");
  if (!bot->Run())
  {
    printf("Error running bot: %s\n", bot->Error());
    DeleteInstances();
    return EXIT_FAILURE;
  }

  DeleteInstances();

  return EXIT_SUCCESS;
}
