#ifndef INOSTREAM_HXX
#define INOSTREAM_HXX

/**************************************************************************
 *
 * Copyright (C) 2006, Johns Hopkins University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 *   - Redistributions of source code must contain the above 
 *     copyright notice, this list of conditions, and the following
 *     disclaimer. 
 *
 *   - Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions, and the following
 *     disclaimer in the documentation and/or other materials 
 *     provided with the distribution.
 *
 *   - Neither the names of the copyright holders nor the names of any
 *     of any contributors may be used to endorse or promote products
 *     derived from this software without specific prior written
 *     permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **************************************************************************/

#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <string>
#include <assert.h>
#include <sstream>

using namespace sherpa;
using namespace std;

// This is a completely sleazy way of making an automatically indenting 
// stream.
struct INOstream {
  size_t depth;
  size_t col;
  bool needIndent;
  std::ostream &ostrm;
  //void doIndent();

  INOstream(std::ostream &os) 
    :ostrm(os)
  {
    depth = 0;
    col = 0;
    needIndent = true;
  }

  inline void indent(int i)
  {
    depth += i;
  }

  inline int indentToHere()
  {
    size_t old_depth = depth;
    depth = (depth > col) ? depth : col;
    return old_depth;
  }
  inline void setIndent(size_t theCol)
  {
    depth = theCol;
  }

  inline void more()
  {
    indent(2);
  }

  inline void less()
  {
    indent(-2);
  }

  void
  doIndent()
  {
    //std::cout << "Indent = " << depth << "; needIndent = " << needIndent << endl;
    if (needIndent) {
      while (col < depth) {
	ostrm << ' ';
	col++;
      }
    }
    needIndent = false;
  }

  inline
  INOstream& operator<<(const char c)
  {
    INOstream& inostrm = *this;
    inostrm.doIndent();

    inostrm.ostrm << c;
    inostrm.col++;

    if (c == '\n') {
      inostrm.col = 0;
      inostrm.needIndent = true;
    }
    return inostrm;
  }

  inline
  INOstream& operator<<(const unsigned char c)
  {
    INOstream& inostrm = *this;
    inostrm.doIndent();

    inostrm.ostrm << c;
    inostrm.col++;
    if (c == '\n') {
      inostrm.col = 0;
      inostrm.needIndent = true;
    }
    return inostrm;
  }

  // This is where the magic happens. The way that a C++ stream recognizes
  // things like std::endl is by overloading this argument type. std::endl 
  // is actually a procedure!
  //
  // Note, however, that INOstream does not forward the endl. We need
  // it in order to recognize when a fill is required, but we don't
  // really intend to flush the underlying output stream.
  inline
  INOstream& operator<<(ostream& (*pf)(ostream&))
  {
    INOstream& inostrm = *this;
    if (pf == (ostream& (*)(ostream&)) std::endl) {
      inostrm << '\n';
    }
    else {
      inostrm.doIndent();
      inostrm.ostrm << pf;
    }
    return inostrm;
  }

  inline
  INOstream& operator<<(ios_base& (*pf)(ios_base&))
  {
    INOstream& inostrm = *this;
    inostrm.doIndent();
    inostrm.ostrm << pf;
    return inostrm;
  }

  inline
  INOstream& operator<<(const char *s)
  {
    INOstream& inostrm = *this;

    for (size_t i = 0; i < strlen(s); i++)
      inostrm << s[i];

    return inostrm;
  }

#if 0
  inline
  INOstream& operator<<(const unsigned long long ull)
  {
    char digits[64];		// sufficient in all cases
    sprintf(digits, "%ull", ull);

    INOstream& inostrm = *this;
    inostrm << digits;

    return inostrm;
  }

  inline
  INOstream& operator<<(const long long ll)
  {
    char digits[64];		// sufficient in all cases
    sprintf(digits, "%ll", ll);

    INOstream& inostrm = *this;
    inostrm << digits;

    return inostrm;
  }

  inline
  INOstream& operator<<(const unsigned long ul)
  {
    char digits[64];		// sufficient in all cases
    sprintf(digits, "%ul", ul);

    INOstream& inostrm = *this;
    inostrm << digits;

    return inostrm;
  }

  inline
  INOstream& operator<<(const long l)
  {
    char digits[64];		// sufficient in all cases
    sprintf(digits, "%l", l);

    INOstream& inostrm = *this;
    inostrm << digits;

    return inostrm;
  }

  INOstream& operator<<(const unsigned int ui)
  {
    char digits[64];		// sufficient in all cases
    sprintf(digits, "%ul", ui);

    INOstream& inostrm = *this;
    inostrm << digits;

    return inostrm;
  }

  inline
  INOstream& operator<<(const int i)
  {
    char digits[64];		// sufficient in all cases
    sprintf(digits, "%l", i);

    INOstream& inostrm = *this;
    inostrm << digits;

    return inostrm;
  }
#endif

  inline
  INOstream& operator<<(const std::string& s)
  {
    INOstream& inostrm = *this;

    for (size_t i = 0; i < s.size(); i++)
      inostrm << s[i];

    return inostrm;
  }

  template<typename T>
  inline
  INOstream& operator<<(T ob)
  {
    stringstream ss;
    ss << ob;

    INOstream& inostrm = *this;
    inostrm << ss.str();
    return inostrm;
  }

};

#endif /* INOSTREAM_HXX */