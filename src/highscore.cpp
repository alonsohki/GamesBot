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

#include <stdlib.h>
#include "highscore.h"

HighScore* HighScore::Instance()
{
  static HighScore* instance = 0;
  if (!instance)
    instance = new HighScore();
  return instance;
}

HighScore::HighScore()
{
  Database* db = Database::Instance();
  delete db->Query("CREATE TABLE IF NOT EXISTS highscore "
                   "( nickname VARCHAR(64), game VARCHAR(64), score INTEGER )");
  delete db->Query("CREATE INDEX IF NOT EXISTS idxHighscore ON highscore(nickname, game)");
}

HighScore::~HighScore()
{
}

int HighScore::GetScore(const char* nickname, const char* game)
{
  int score = 0;
  Database* db = Database::Instance();
  DatabaseResult* res = db->Query("SELECT score FROM highscore WHERE nickname LIKE '%s' AND game LIKE '%s'",
                                  nickname, game);
  if (res)
  {
    const DatabaseRow* row = res->FetchRow();
    if (row)
      score = atoi((*row)["score"]);
    delete res;
  }

  return score;
}

void HighScore::SetScore(const char* nickname, const char* game, int score)
{
  Database* db = Database::Instance();
  delete db->Query("UPDATE highscore SET score='%d' WHERE nickname LIKE '%s' AND game LIKE '%s'",
                   score, nickname, game);
  if (db->ChangedRows() == 0)
  {
    delete db->Query("INSERT INTO highscore(nickname, game, score) VALUES ('%s', '%s', '%d')",
                     nickname, game, score);
  }
}

void HighScore::GetGameTop(const char* game, std::vector<std::string>& nicknames, std::vector<int>& scores, int limit)
{
  Database* db = Database::Instance();
  DatabaseResult* res;
 
  if (limit == -1)
    res = db->Query("SELECT nickname,score FROM highscore WHERE game LIKE '%s' ORDER BY score DESC", game);
  else
    res = db->Query("SELECT nickname,score FROM highscore WHERE game LIKE '%s' ORDER BY score DESC LIMIT %d", game, limit);

  if (res)
  {
    const DatabaseRow* row;

    while ((row = res->FetchRow()) != 0)
    {
      nicknames.push_back((*row)["nickname"]);
      scores.push_back(atoi((*row)["score"]));
    }
    delete res;
  }
}

