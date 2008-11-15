#ifndef LITVALUE_HXX
#define LITVALUE_HXX

/**************************************************************************
 *
 * Copyright (C) 2008, Johns Hopkins University.
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

#include <inttypes.h>
#include <string>
#include <libsherpa/BigNum.hxx>


enum LitRepr {
  lr_none,
  lr_bool,
  lr_char,
  lr_int,
  lr_float,
  lr_string
};

struct LitValue {
  LitRepr lr;

  bool   b;			/* boolean Values */
  unsigned long c;		/* utf32 code points */
  sherpa::BigNum i;		/* large precision integers */
  double d;			/* doubles, floats          */

  // FIX: (shap) the original input is being saved in AST.s for replay
  // purposes. String literals need to be stored here as a vector of
  // character representations.
  std::string s;		/* String Literals          */

  static uint32_t DecodeStringCharacter(const char *s, const char **next);
  static uint32_t DecodeRawCharacter(const char *s, const char **next);
  static uint32_t DecodeCharacter(const std::string&);

  LitValue() {
    lr = lr_none;
  }
};

#endif /* LITVALUE_HXX */