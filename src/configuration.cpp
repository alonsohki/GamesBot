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

#include <strings.h>
#include "configuration.h"

Configuration::Configuration()
  : m_errno(0), m_error("")
{
}

Configuration::~Configuration()
{
}

Configuration::Configuration(const char* filename)
  : m_errno(0), m_error("")
{
  Create(filename);
}

bool Configuration::Create(const char* filename)
{
  m_parser.SetFilename(filename);

  if (!m_parser.Ok())
  {
    m_error = m_parser.Error();
    m_errno = m_parser.Errno();
    return false;
  }

 const char* v;
#define SAFE_LOAD(section, entry, dest) do { \
  v = m_parser.GetValue(#section, #entry); \
  if (v == 0) \
  { \
    m_errno = -1; \
    m_error = "Unable to load entry '" #entry "' from section '" #section "'"; \
    return false; \
  } \
  (dest) = v; \
} while ( false )

  /* ircserver */
  SAFE_LOAD(ircserver, address, this->IRCServer.address);
  SAFE_LOAD(ircserver, service, this->IRCServer.service);
  SAFE_LOAD(ircserver, password, this->IRCServer.password);
  SAFE_LOAD(ircserver, ssl, v);
  this->IRCServer.useSSL = (!strcasecmp(v, "true") ? true : false);
  SAFE_LOAD(ircserver, sslcert, v);
  this->IRCServer.sslCert = (*v != '\0' ? v : 0);

  /* bot */
  SAFE_LOAD(bot, nickname, this->Bot.nickname);
  SAFE_LOAD(bot, username, this->Bot.username);
  SAFE_LOAD(bot, fullname, this->Bot.fullname);
  SAFE_LOAD(bot, password, this->Bot.password);
  SAFE_LOAD(bot, channel, this->Bot.channel);

#undef SAFE_LOAD
  return true;
}

bool Configuration::Ok() const
{
  return !m_errno;
}

int Configuration::Errno() const
{
  return m_errno;
}

const char* Configuration::Error() const
{
  return m_error.c_str();
}

