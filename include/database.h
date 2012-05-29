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

#ifndef __DATABASE_H
#define __DATABASE_H

#include <sqlite3.h>
#include <string>

class DatabaseRow
{
  friend class DatabaseResult;

public:
  virtual ~DatabaseRow();

  const char* operator[](const char* index) const;
  const char* operator[](unsigned long index) const;
  unsigned long NumCols() const;

private:
  DatabaseRow(unsigned long numCols, char** indices, char** values);

  unsigned long m_numCols;
  char** m_indices;
  char** m_values;
  DatabaseRow* next;
};

class DatabaseResult
{
  friend class Database;

public:
  virtual ~DatabaseResult();

  unsigned long NumRows() const;
  unsigned long NumCols() const;
  const DatabaseRow* FetchRow();
  void Reset();

private:
  DatabaseResult();
  void InsertRow(unsigned long numCols, char** indices, char** values);

  unsigned long m_numRows;
  unsigned long m_numCols;
  DatabaseRow* m_firstRow;
  DatabaseRow* m_lastRow;
  DatabaseRow* m_curRow;
};

class Database
{
public:
  static Database* Instance();

public:
  Database();
  Database(const char* path);
  virtual ~Database();

  bool Create(const char* path);

  bool Ok() const;
  int Errno() const;
  const char* Error() const;

  DatabaseResult* Query(const char* query, ...);
  int ChangedRows();

private:
  static int SQLite_cbk(void* _res, int argc, char** argv, char** colNames);
  int m_errno;
  std::string m_error;
  sqlite3* m_handle;
};

#endif /* #ifndef __DATABASE_H */
