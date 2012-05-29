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

#include <string.h>
#include "database.h"

Database* Database::Instance()
{
  static Database* instance = 0;
  if (!instance)
    instance = new Database();
  return instance;
}

Database::Database()
  : m_errno(0), m_error(""), m_handle(0)
{
}

Database::~Database()
{
  if (m_handle)
    sqlite3_close(m_handle);
}

Database::Database(const char* path)
  : m_errno(0), m_error(""), m_handle(0)
{
  Create(path);
}

bool Database::Create(const char* path)
{
  if (m_handle)
    return Ok();

  int rc = sqlite3_open_v2(path, &m_handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 0);
  if (rc)
  {
    m_error = sqlite3_errmsg(m_handle);
    m_errno = sqlite3_errcode(m_handle);;
    sqlite3_close(m_handle);
    m_handle = 0;
    return false;
  }

  return true;
}

bool Database::Ok() const
{
  return !m_errno && m_handle;
}

int Database::Errno() const
{
  return m_errno;
}

const char* Database::Error() const
{
  return m_error.c_str();
}

int Database::ChangedRows()
{
  return sqlite3_changes(m_handle);
}

int Database::SQLite_cbk(void* _res, int argc, char** argv, char** colNames)
{
  DatabaseResult* res = (DatabaseResult *)_res;

  res->m_numRows++;
  res->m_numCols = argc;

  res->InsertRow(argc, colNames, argv);

  return 0;
}

DatabaseResult* Database::Query(const char* query, ...)
{
  if (!Ok()) return 0;

  DatabaseResult* res = new DatabaseResult();
  char* errMsg;

  va_list vl;
  char queryStr[4096];
  va_start(vl, query);
  vsnprintf(queryStr, sizeof(queryStr), query, vl);
  va_end(vl);

  int rc = sqlite3_exec(m_handle, queryStr, Database::SQLite_cbk, res, &errMsg);
  if (rc != SQLITE_OK)
  {
    delete res;
    res = 0;
    m_errno = sqlite3_errcode(m_handle);
    m_error = errMsg;
  }
  res->m_curRow = res->m_firstRow;

  return res;
}


/**
 ** Result
 **/
DatabaseResult::DatabaseResult()
  : m_numRows(0UL), m_numCols(0UL), m_firstRow(0), m_lastRow(0), m_curRow(0)
{
}

DatabaseResult::~DatabaseResult()
{
  DatabaseRow* next;

  for (DatabaseRow* cur = m_firstRow; cur != 0; cur = next)
  {
    next = cur->next;
    delete cur;
  }
}

unsigned long DatabaseResult::NumRows() const
{
  return m_numRows;
}

void DatabaseResult::Reset()
{
  m_curRow = m_firstRow;
}

void DatabaseResult::InsertRow(unsigned long numCols, char** indices, char** values)
{
  DatabaseRow* row = new DatabaseRow(numCols, indices, values);

  if (!m_firstRow)
  {
    m_firstRow = row;
    m_lastRow = row;
  }
  else
  {
    m_lastRow->next = row;
    m_lastRow = row;
  }
}

const DatabaseRow* DatabaseResult::FetchRow()
{
  DatabaseRow* ret = m_curRow;
  if (ret)
    m_curRow = m_curRow->next;
  return ret;
}


/**
 ** Row
 **/
DatabaseRow::DatabaseRow(unsigned long numCols, char** indices, char** values)
  : m_numCols(numCols), m_indices(0), m_values(0), next(0)
{
  m_indices = new char*[numCols];
  m_values = new char*[numCols];

  for (unsigned long i = 0; i < numCols; i++)
  {
    m_indices[i] = new char[strlen(indices[i])];
    strcpy(m_indices[i], indices[i]);
    m_values[i] = new char[strlen(values[i])];
    strcpy(m_values[i], values[i]);
  }
}

DatabaseRow::~DatabaseRow()
{
  for (unsigned long i = 0; i < m_numCols; i++)
  {
    delete [] m_values[(m_numCols - i - 1)];
    delete [] m_indices[(m_numCols - i - 1)];
  }
  delete m_values;
  delete m_indices;
}

const char* DatabaseRow::operator[](const char* idx) const
{
  for (unsigned long i = 0; i < m_numCols; i++)
  {
    if (!strcmp(idx, m_indices[i]))
      return m_values[i];
  }
  return 0;
}

const char* DatabaseRow::operator[](unsigned long idx) const
{
  if (idx > m_numCols)
    return 0;

  return m_values[idx];
}

