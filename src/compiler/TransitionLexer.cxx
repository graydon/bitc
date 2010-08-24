/**************************************************************************
 *
 * Copyright (C) 2010, Jonathan S. Shapiro
 * Portions Copyright (C) 2008, Johns Hopkins University
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

#include <assert.h>
#include <string.h>
#include <string>

#include <unicode/uchar.h>

#include <libsherpa/utf8.hxx>
#include <libsherpa/LexLoc.hxx>

#include "BUILD/TransitionParser.hxx"

using namespace sherpa;

#include "TransitionLexer.hxx"
#include "LitValue.hxx"

bool
TransitionLexer::valid_ident_start(ucs4_t ucs4)
{
  // Extended characters are only permitted as the first
  // identifier character in lisp identifier mode.
  return (u_hasBinaryProperty(ucs4,UCHAR_XID_START)
          || (ucs4 == '_'));
}

bool
TransitionLexer::valid_ident_continue(ucs4_t ucs4)
{
  // For the moment, extended characters are permitted as 
  // continue characters.
  return (u_hasBinaryProperty(ucs4,UCHAR_XID_CONTINUE)
          || (ucs4 == '_'));
}

bool
TransitionLexer::valid_ascii_symbol(ucs4_t ucs4)
{
  switch (ucs4) {
  case '#':                     // until S-expressions are gone
  case '_':                     // because it is extended alphabetic
    return false;

  case '!':
  case '$':
  case '%':
  case '&':
  case '*':
  case '+':
  case '-':
  case '/':
  case '<':
  case '>':
  case '=':
  case '?':
  case '@':
  case '^':
  case '|':
  case '~':
    return true;

  default:
    return false;
  }
}

bool
TransitionLexer::valid_operator_start(ucs4_t ucs4)
{

  // Extended characters are only permitted as the first
  // identifier character in lisp identifier mode.
  return (valid_ascii_symbol(ucs4));
}

bool
TransitionLexer::valid_operator_continue(ucs4_t ucs4)
{
  // For the moment, extended characters are permitted as 
  // continue characters.
  return (valid_ascii_symbol(ucs4));
}

bool
TransitionLexer::valid_ifident_start(ucs4_t ucs4)
{
  return (isalpha(ucs4) || ucs4 == '_');
  //  return (u_hasBinaryProperty(ucs4,UCHAR_XID_START));
}

bool
TransitionLexer::valid_ifident_continue(ucs4_t ucs4)
{
  return (isalpha(ucs4) || isdigit(ucs4) || ucs4 == '_' || ucs4 == '-');
  //  return (u_hasBinaryProperty(ucs4,UCHAR_XID_CONTINUE) ||
  //valid_ifident_punct(ucs4));
}

bool
TransitionLexer::valid_tv_ident_start(ucs4_t ucs4)
{
  return (u_hasBinaryProperty(ucs4,UCHAR_XID_START) || 
          ucs4 == '_');
}

bool
TransitionLexer::valid_tv_ident_continue(ucs4_t ucs4)
{
  return (u_hasBinaryProperty(ucs4,UCHAR_XID_CONTINUE) ||
          ucs4 == '_');
}

TransitionLexer::KeyWord::KeyWord(const char *_nm, LangFlags _whichLang, int _tokValue)
{
  nm = _nm;
  whichLang = _whichLang;

  sexprTokValue = (_whichLang & (lf_sexpr|lf_version)) ? _tokValue : tk_SxpIdent;
  blockTokValue = (_whichLang & (lf_block|lf_version)) ? _tokValue : tk_BlkIdent;

  if (sexprTokValue == tk_ReservedWord)
    sexprTokValue = tk_SxpReservedWord;
}

TransitionLexer::KeyWord::KeyWord(const char *_nm, LangFlags _whichLang, 
                                  int _sexprTokValue, int _blockTokValue)
{
  nm = _nm;
  whichLang = _whichLang;
  sexprTokValue = _sexprTokValue;
  blockTokValue = _blockTokValue;
}

/// @brief Whether keyword table has been sorted.
///
/// I added this because the emacs sort-lines function doesn't
/// do the right thing for trailing punctuation in keywords. This is
/// true because '!' sorts lower than '"', so
///
///    "set"  sorts lower than
///    "set!
///
/// I finally got tired of forgetting to fix this by hand.
static bool keywords_sorted = false;

struct TransitionLexer::KeyWord TransitionLexer::keywords[] = {
  TransitionLexer::KeyWord( "!",                lf_block,             '!' ),
  TransitionLexer::KeyWord( "~",                lf_block,             '~' ),
  TransitionLexer::KeyWord( "!=",               lf_block,             tk_NOTEQUALS ),
  TransitionLexer::KeyWord( "%",                lf_block,             '%' ),
  TransitionLexer::KeyWord( "&",                lf_block,             '&' ),
  TransitionLexer::KeyWord( "&&",               lf_block,             tk_AND ),
  TransitionLexer::KeyWord( "*",                lf_transition,        tk_SxpIdent, '*' ),
  TransitionLexer::KeyWord( "+",                lf_transition,        tk_SxpIdent, '+' ),
  TransitionLexer::KeyWord( "-",                lf_transition,        tk_SxpIdent, '-' ),
  TransitionLexer::KeyWord( "->",               lf_transition,        tk_FNARROW ),
  TransitionLexer::KeyWord( "/",                lf_transition,        tk_SxpIdent, '/' ),
  TransitionLexer::KeyWord( "<",                lf_block,             '<' ),
  TransitionLexer::KeyWord( "<<",               lf_block,             tk_LSHIFT ),
  TransitionLexer::KeyWord( "<=",               lf_block,             tk_LE ),
  TransitionLexer::KeyWord( "=",                lf_transition,        tk_SxpIdent, '=' ),
  TransitionLexer::KeyWord( "^",                lf_transition,        '^' ),
  TransitionLexer::KeyWord( "==",               lf_block,             tk_EQUALS ),
  TransitionLexer::KeyWord( ">",                lf_block,             '>' ),
  TransitionLexer::KeyWord( ">=",               lf_block,             tk_GE ),
  TransitionLexer::KeyWord( ">>",               lf_block,             tk_RSHIFT ),
  TransitionLexer::KeyWord( "and",              lf_transition,        tk_AND ),
  TransitionLexer::KeyWord( "apply",            lf_transition,        tk_APPLY ),
  TransitionLexer::KeyWord( "array",            lf_transition,        tk_ARRAY ),
  TransitionLexer::KeyWord( "ArrayRef",         lf_transition,        tk_ARRAY_REF ),
  TransitionLexer::KeyWord( "as",               lf_transition,        tk_AS ),
  TransitionLexer::KeyWord( "assert",           lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "begin",            lf_transition,        tk_BEGIN ),
  TransitionLexer::KeyWord( "bitc",             lf_version,           tk_BITC ),
  TransitionLexer::KeyWord( "bitfield",         lf_transition,        tk_BITFIELD ),
  TransitionLexer::KeyWord( "bitsizeof",        lf_transition,        tk_BITSIZEOF ),
  TransitionLexer::KeyWord( "block",            lf_transition,        tk_BLOCK ),
  TransitionLexer::KeyWord( "bool",             lf_transition,        tk_BOOL ),
  TransitionLexer::KeyWord( "boxed",            lf_block,             tk_BOXED ),
  TransitionLexer::KeyWord( "break",            lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "ByRef",            lf_transition,        tk_BY_REF ),
  TransitionLexer::KeyWord( "case",             lf_transition,        tk_CASE ),
  TransitionLexer::KeyWord( "catch",            lf_transition,        tk_CATCH ),
  TransitionLexer::KeyWord( "char",             lf_transition,        tk_CHAR ),
  TransitionLexer::KeyWord( "check",            lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "closed",           lf_transition,        tk_CLOSED ),
  TransitionLexer::KeyWord( "coindset",         lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "cond",             lf_transition,        tk_COND ),
  TransitionLexer::KeyWord( "const",            lf_transition,        tk_CONST ),
  TransitionLexer::KeyWord( "constrain",        lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "continue",         lf_transition,        tk_CONTINUE ),
  TransitionLexer::KeyWord( "declare",          lf_transition,        tk_DECLARE ),
  TransitionLexer::KeyWord( "deep-const",       lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "def",              lf_transition,        tk_DEF ),
  TransitionLexer::KeyWord( "defequiv",         lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "defexception",     lf_transition,        tk_DEFEXCEPTION ),
  TransitionLexer::KeyWord( "define",           lf_transition,        tk_DEFINE ),
  TransitionLexer::KeyWord( "definstance",      lf_sexpr,             tk_DEFINSTANCE ),
  TransitionLexer::KeyWord( "definvariant",     lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "defobject",        lf_sexpr,             tk_DEFOBJECT ),
  TransitionLexer::KeyWord( "defrefine",        lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "defrepr",          lf_transition,        tk_DEFREPR ),
  TransitionLexer::KeyWord( "defstruct",        lf_sexpr,             tk_DEFSTRUCT ),
  TransitionLexer::KeyWord( "deftheory",        lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "defthm",           lf_transition,        tk_DEFTHM ),
  TransitionLexer::KeyWord( "deftypeclass",     lf_sexpr,             tk_DEFTYPECLASS ),
  TransitionLexer::KeyWord( "defunion",         lf_transition,        tk_DEFUNION ),
  TransitionLexer::KeyWord( "defvariant",       lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "deref",            lf_transition,        tk_DEREF ),
  TransitionLexer::KeyWord( "disable",          lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "do",               lf_transition,        tk_DO ),
  TransitionLexer::KeyWord( "do*",              lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "double",           lf_transition,        tk_DOUBLE ),
  TransitionLexer::KeyWord( "dup",              lf_transition,        tk_DUP ),
  TransitionLexer::KeyWord( "else",             lf_block,             tk_ELSE ),
  TransitionLexer::KeyWord( "enable",           lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "exception",        lf_transition,        tk_EXCEPTION ),
  TransitionLexer::KeyWord( "external",         lf_transition,        tk_EXTERNAL ),
  TransitionLexer::KeyWord( "false",            lf_block,             tk_FALSE ),
  TransitionLexer::KeyWord( "fill",             lf_transition,        tk_FILL ),
  TransitionLexer::KeyWord( "float",            lf_transition,        tk_FLOAT ),
  TransitionLexer::KeyWord( "fn",               lf_transition,        tk_FN ),
  TransitionLexer::KeyWord( "forall",           lf_transition,        tk_FORALL ),
  TransitionLexer::KeyWord( "from",             lf_transition,        tk_FROM ),
  TransitionLexer::KeyWord( "giving",           lf_transition,        tk_GIVING ),
  TransitionLexer::KeyWord( "if",               lf_transition,        tk_IF ),
  TransitionLexer::KeyWord( "import",           lf_transition,        tk_IMPORT ),
  TransitionLexer::KeyWord( "import!",          lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "impure",           lf_transition,        tk_IMPURE ),
  TransitionLexer::KeyWord( "in",               lf_block,             tk_IN ),
  TransitionLexer::KeyWord( "indset",           lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "InnerRef",         lf_transition,        tk_INNER_REF ),
  TransitionLexer::KeyWord( "instance",         lf_block,             tk_INSTANCE ),
  TransitionLexer::KeyWord( "int16",            lf_transition,        tk_INT16 ),
  TransitionLexer::KeyWord( "int32",            lf_transition,        tk_INT32 ),
  TransitionLexer::KeyWord( "int64",            lf_transition,        tk_INT64 ),
  TransitionLexer::KeyWord( "int8",             lf_transition,        tk_INT8 ),
  TransitionLexer::KeyWord( "interface",        lf_transition,        tk_INTERFACE ),
  TransitionLexer::KeyWord( "label",            lf_block,             tk_LABEL ),
  TransitionLexer::KeyWord( "lambda",           lf_transition,        tk_LAMBDA ),
  TransitionLexer::KeyWord( "let",              lf_transition,        tk_LET ),
  TransitionLexer::KeyWord( "let*",             lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "letrec",           lf_transition,        tk_LETREC ),
  TransitionLexer::KeyWord( "location",         lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "loop",             lf_transition,        tk_LOOP ),
  TransitionLexer::KeyWord( "MakeVector",       lf_transition,        tk_MAKE_VECTOR ),
  TransitionLexer::KeyWord( "member",           lf_transition,        tk_MEMBER ),   /* REDUNDANT */
  TransitionLexer::KeyWord( "method",           lf_transition,        tk_METHOD ),
  TransitionLexer::KeyWord( "module",           lf_transition,        tk_MODULE ),
  TransitionLexer::KeyWord( "mutable",          lf_transition,        tk_MUTABLE ),
  TransitionLexer::KeyWord( "namespace",        lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "not",              lf_block,             tk_NOT ),
  TransitionLexer::KeyWord( "nth",              lf_sexpr,             tk_NTH ),
  TransitionLexer::KeyWord( "object",           lf_block,             tk_OBJECT ),
  TransitionLexer::KeyWord( "opaque",           lf_transition,        tk_OPAQUE ),
  TransitionLexer::KeyWord( "or",               lf_transition,        tk_OR ),
  TransitionLexer::KeyWord( "otherwise",        lf_transition,        tk_OTHERWISE ),
  TransitionLexer::KeyWord( "proclaim",         lf_transition,        tk_PROCLAIM ),
  TransitionLexer::KeyWord( "provide",          lf_transition,        tk_PROVIDE ),
  TransitionLexer::KeyWord( "provide!",         lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "pure",             lf_transition,        tk_PURE ),
  TransitionLexer::KeyWord( "quad",             lf_transition,        tk_QUAD ),
  TransitionLexer::KeyWord( "read-only",        lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "ref",              lf_transition,        tk_REF ),
  TransitionLexer::KeyWord( "reference",        lf_block,             tk_PTR ),
  TransitionLexer::KeyWord( "repr",             lf_block,             tk_REPR ),
  TransitionLexer::KeyWord( "require",          lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "reserved",         lf_transition,        tk_RESERVED ),
  TransitionLexer::KeyWord( "return",           lf_transition,        tk_RETURN ),
  TransitionLexer::KeyWord( "sensory",          lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "set!",             lf_transition,        tk_SET ),
  TransitionLexer::KeyWord( "sizeof",           lf_transition,        tk_SIZEOF ),
  TransitionLexer::KeyWord( "string",           lf_transition,        tk_STRING ),
  TransitionLexer::KeyWord( "struct",           lf_block,             tk_STRUCT ),
  TransitionLexer::KeyWord( "super",            lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "suspend",          lf_transition,        tk_SUSPEND ),  
  TransitionLexer::KeyWord( "switch",           lf_transition,        tk_SWITCH ),
  TransitionLexer::KeyWord( "tag",              lf_transition,        tk_TAG ),
  TransitionLexer::KeyWord( "the",              lf_transition,        tk_THE ),
  TransitionLexer::KeyWord( "then",             lf_block,             tk_THEN ),
  TransitionLexer::KeyWord( "throw",            lf_transition,        tk_THROW ),
  TransitionLexer::KeyWord( "trait",            lf_block,             tk_TRAIT ),
  TransitionLexer::KeyWord( "true",             lf_block,             tk_TRUE ),
  TransitionLexer::KeyWord( "try",              lf_transition,        tk_TRY ),
  TransitionLexer::KeyWord( "tycon",            lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "tyfn",             lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "typecase",         lf_block,             tk_TYPECASE ),
  TransitionLexer::KeyWord( "uint16",           lf_transition,        tk_UINT16 ),
  TransitionLexer::KeyWord( "uint32",           lf_transition,        tk_UINT32 ),
  TransitionLexer::KeyWord( "uint64",           lf_transition,        tk_UINT64 ),
  TransitionLexer::KeyWord( "uint8",            lf_transition,        tk_UINT8 ),
  TransitionLexer::KeyWord( "unboxed",          lf_block,             tk_UNBOXED ),
  TransitionLexer::KeyWord( "union",            lf_block,             tk_UNION ),
  TransitionLexer::KeyWord( "until",            lf_block,             tk_UNTIL ),
  TransitionLexer::KeyWord( "using",            lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "val",              lf_transition,        tk_VAL ),
  TransitionLexer::KeyWord( "value-at",         lf_transition,        tk_ReservedWord ),
  TransitionLexer::KeyWord( "vector",           lf_transition,        tk_VECTOR ),
  TransitionLexer::KeyWord( "version",          lf_version,           tk_VERSION ),
  TransitionLexer::KeyWord( "when",             lf_transition,        tk_WHEN ),
  TransitionLexer::KeyWord( "where",            lf_transition,        tk_WHERE ),
  TransitionLexer::KeyWord( "word",             lf_transition,        tk_WORD ),
  TransitionLexer::KeyWord( "|",                lf_block,             '|' ),
  TransitionLexer::KeyWord( "||",               lf_block,             tk_OR )
};

static int
kwstrcmp(const void *vKey, const void *vCandidate)
{
  const char *key = ((const TransitionLexer::KeyWord *) vKey)->nm;
  const char *candidate = ((const TransitionLexer::KeyWord *) vCandidate)->nm;

  return strcmp(key, candidate);
}

int
TransitionLexer::kwCheck(const char *s, int identType)
{
  if (ifIdentMode) {
    if (!valid_ifident_start(*s))
      return (currentLang & lf_sexpr) ? tk_SxpReservedWord : tk_ReservedWord;

    for (++s; *s; s++) {
      if (!valid_ifident_continue(*s))
        return (currentLang & lf_sexpr) ? tk_SxpReservedWord : tk_ReservedWord;
    }

    return (currentLang & lf_sexpr) ? tk_SxpIdent : tk_BlkIdent;
  }

  KeyWord key = KeyWord(s, lf_transition, 0);
  KeyWord *entry = 
    (KeyWord *)bsearch(&key, keywords, // &OK
                       sizeof(keywords)/sizeof(keywords[0]), 
                       sizeof(keywords[0]), kwstrcmp);

  // If it is in the token table, and it is accepted in the prevailing
  // language variant, return the indicated token type. Note a trick
  // here that the very first token accepted may be accepted under the
  // lf_version sub-language. Once the first token has been accepted,
  // we disable that sub-language.
  if (entry) {
    if (currentLang & entry->whichLang) {
      currentLang &= ~LangFlags(lf_version);

      return (currentLang & lf_sexpr) ? entry->sexprTokValue : entry->blockTokValue;
    }
  }

  // Otherwise, check for various reserved words:

  // Things starting with "__":
  if (s[0] == '_' && s[1] == '_') {
    if (!isRuntimeUoc)
      return (currentLang & lf_sexpr) ? tk_SxpReservedWord : tk_ReservedWord;
  }

  // Things starting with "def" are reserved:
  if (s[0] == 'd' && s[1] == 'e' && s[2] == 'f')
    return (currentLang & lf_sexpr) ? tk_SxpReservedWord : tk_ReservedWord;

  // Things starting with "#" are reserved:
  if (s[0] == '#')
    return (currentLang & lf_sexpr) ? tk_SxpReservedWord : tk_ReservedWord;

  if (currentLang & lf_sexpr) 
    return tk_SxpIdent;
  else return identType;
}

void
TransitionLexer::ReportParseError()
{
  errStream << lastTokenPosition
            << ": syntax error (via yyerror)" << '\n';
  num_errors++;
}
 
void
TransitionLexer::ReportParseError(const LexLoc& where, std::string msg)
{
  errStream << where
            << ": "
            << msg << std::endl;

  num_errors++;
}

void
TransitionLexer::ReportParseWarning(const LexLoc& where, std::string msg)
{
  errStream << where
            << ": "
            << msg << std::endl;
}

TransitionLexer::TransitionLexer(std::ostream& _err, std::istream& _in, 
                       const std::string& origin,
                       bool commandLineInput)
  :here(origin, 1, 0), inStream(_in), errStream(_err)
{
  inStream.unsetf(std::ios_base::skipws);

  if (!keywords_sorted) {
    qsort(keywords,
         sizeof(keywords)/sizeof(keywords[0]), 
         sizeof(keywords[0]), kwstrcmp);
    keywords_sorted = true;
  }

  // Don't accept block syntax keywords until we see the new version syntax,
  // which is accepted under lf_version
  currentLang = lf_sexpr | lf_version;
  lispParenDepth = 0;
  num_errors = 0;
  isRuntimeUoc = false;
  ifIdentMode = false;
  isCommandLineInput = commandLineInput;
  debug = false;
  nModules = 0;

  showNextError = true;

  havePushbackToken = false;
  lastToken = LToken(EOF, "end of file");
}

ucs4_t
TransitionLexer::getChar()
{
  char utf[8];
  unsigned char c;

  long ucs4 = pushBackStack.pop();

  if (ucs4 != -1) {
    utf8_encode(ucs4, utf);
    goto checkDigit;
  }

  memset(utf, 0, 8);

  utf[0] = inStream.get();
  c = utf[0];
  if (utf[0] == EOF)
    return EOF;

  if (c <= 127)
    goto done;

  utf[1] = inStream.get();
  if (utf[1] == EOF)
    return EOF;

  if (c <= 223)
    goto done;

  utf[2] = inStream.get();
  if (utf[2] == EOF)
    return EOF;

  if (c <= 239)
    goto done;

  utf[3] = inStream.get();
  if (utf[3] == EOF)
    return EOF;

  if (c <= 247)
    goto done;
 
  utf[4] = inStream.get();
  if (utf[4] == EOF)
    return EOF;

  if (c <= 251)
    goto done;

  utf[5] = inStream.get();
  if (utf[5] == EOF)
    return EOF;

 done:
  ucs4 = utf8_decode(utf, 0);
 checkDigit:
  thisToken += utf;

  return ucs4;
}

void
TransitionLexer::ungetChar(ucs4_t c)
{
  char utf[8];

  // Never bother to push back EOF, since it is reproduced by the
  // input.
  if (c == EOF)
    return;

  pushBackStack.push(c);

  unsigned len = utf8_encode(c, utf);
  thisToken.erase( thisToken.length() - len);
}

void
TransitionLexer::ungetThisToken()
{
  ucs4_t ucsToken[8];
  const char *s = thisToken.c_str();
  const char *snext = s;
  size_t i = 0;

  for( ; *snext && i < 8; i++) {
    ucsToken[i] = utf8_decode(s, &snext);
    s = snext;
  }

  // If the following assert fails, we are backing out something
  // excessively large...
  assert(*snext == 0);

  for (; i > 0; i--) {
    ucs4_t c = ucsToken[i-1];
    pushBackStack.push(c);
  }

  thisToken.erase();
}

static bool
isWhiteSpace(ucs4_t c)
{
  switch (c) {
  case ' ':
  case '\t':
  case '\n':
  case '\r':
    return true;
  default:
    return false;
  }
}

static bool
isCharDelimiter(ucs4_t c)
{
  if (c == ')') return true;

  return isWhiteSpace(c);
}

int
TransitionLexer::lex(ParseType *lvalp)
{
  if (!havePushbackToken)
    here = skipWhiteSpaceAndComments();

  LToken tok = getNextToken();
  if (debug) {
    errStream << "TOKEN " << tok.tokType << ": " << tok.loc << ' '
              << '"' << tok.str << '"' << '\n';
  }

  lvalp->tok = tok;
  here = tok.endLoc;
  lastToken = tok;
  lastTokenPosition = tok.loc;
  return lastToken.tokType;
}

// #define LAYOUT_BLOCK_DEBUG
void
TransitionLexer::beginBlock(bool implicit)
{
#ifdef LAYOUT_BLOCK_DEBUG
  errStream << "  LAYOUT: " << here 
            << (implicit ? ": implicit " : ": explicit ")
            << "beginBlock()" << std::endl;
#endif

  // Until we find the first token and update it, the semicolon
  // comment for a new block is the same as it's parent.
  unsigned column = layoutStack ? layoutStack->column : 0;

  boost::shared_ptr<LayoutFrame> lf = 
    LayoutFrame::make(lastToken.tokType, implicit, column);

  //  expectingLeftBrace = false; // we're about to return it.
  //  learnBlockIndent = true;

  lf->next = layoutStack;
  layoutStack = lf;
}

void
TransitionLexer::endBlock(bool implicit)
{
  if (implicit && ! layoutStack->implicit)
    ReportParseError(here, "Implicit close brace balances explicit open brace.");
  else if (!implicit && layoutStack->implicit) 
    ReportParseError(here, "Explicit close brace balances implicit open brace.");

  // considerLineStart = false; // we've done it
  layoutStack = layoutStack->next;

#ifdef LAYOUT_BLOCK_DEBUG
  errStream << "  LAYOUT: " << here 
            << (implicit ? ": implicit " : ": explicit ")
            << "endBlock()";
  if (layoutStack)
    errStream << " leaving indent at " << layoutStack->column;
  errStream << std::endl;
#endif
}

void
TransitionLexer::closeToOpeningToken(int closingToken)
{
  /// closeToOpeningToken is always followed by multiple, serially
  /// reentrant calls to TrimLayoutStack. If we are currently in the
  /// trimming state, then we have already done the work that
  /// closeToOpeningToken was supposed to do, and we should just return.
  if (layoutFlags & TRIMMING_LAYOUT_STACK)
    return;

  boost::shared_ptr<LayoutFrame> ls = layoutStack;

  while (ls && ls->implicit) {
    ls->dead = true;

    bool foundOpeningToken = false;

    switch(closingToken) {
    case EOF:
      if (ls->precedingToken == EOF)
        foundOpeningToken = true;
      break;

    case tk_IN:
      if (   ls->precedingToken == tk_LET
          || ls->precedingToken == tk_LETREC
          || ls->precedingToken == tk_CASE)
        foundOpeningToken = true;
      break;
    }

    if (foundOpeningToken)
      break;

    ls = ls->next;
  }

  layoutFlags |= TRIMMING_LAYOUT_STACK;
  return;
}

void
TransitionLexer::closeToOffset(unsigned offset)
{
  // Note that in contrast to closeToOpeningToken, this can safely
  // (and usefully) be called multiple times.

  // It is possible for us to be called on the first token in the
  // file, or on junk tokens after end of module. In both cases we
  // won't have a layout stack yet.
  if (!layoutStack)
    return;

#ifdef LAYOUT_BLOCK_DEBUG
  unsigned count = 0;

  errStream << "  LAYOUT: " << here 
            << ": closeToOffsetToken called: ";
#endif

  boost::shared_ptr<LayoutFrame> ls = layoutStack;

  // Make sure we don't do this at top level, where there *isn't* a
  // containing offset.
  while (ls && ls->implicit && (offset < ls->column)) {
    ls->dead = true;
    ls = ls->next;
#ifdef LAYOUT_BLOCK_DEBUG
    count++;
#endif
  }
    
#ifdef LAYOUT_BLOCK_DEBUG
  errStream << count << " frames closed" << std::endl;
#endif

  layoutFlags |= TRIMMING_LAYOUT_STACK;
  return;
}

bool
TransitionLexer::conditionallyInsertSemicolon(unsigned offset)
{
  // It is possible for us to be called on the first token in the
  // file, or on junk tokens after end of module. In both cases we
  // won't have a layout stack yet.
  if (!layoutStack)
    return false;

  assert(layoutStack);
  if (here.offset <= layoutStack->column)
    return true;
  return false;
}

bool
TransitionLexer::trimLayoutStack()
{
  if (layoutStack && layoutStack->dead) {
    endBlock(true);
    return true;
  }

  layoutFlags &= ~LayoutFlags(TRIMMING_LAYOUT_STACK);
  return false;
}

LexLoc
TransitionLexer::skipWhiteSpaceAndComments()
{
  ucs4_t c;

  LexLoc pos = here;

 startOver:
  thisToken.erase();

  c = getChar();

  if (c == '/') {
    c = getChar();
    if (c == '/') {
      do {
        c = getChar();
      } while (c != '\n' && c != '\r');
      // Back out the EOL. We'll handle that with white space
      // processing below to simplify layout processing.
      ungetChar(c);
      pos.updateWith(thisToken);
      goto startOver;
    }
    else if (c == '*') {
      for (;;) {
        c = getChar();
        if (c == '*') {
          c = getChar();
          if (c == '/') {
            break;
          }
          else if (c == EOF) {
            pos.updateWith(thisToken);
            return pos;
          }
          else
            ungetChar(c);
        }
        else if (c == '\n' || c == '\r') {
          /* If a multi-line comment spans lines, the next token after
             the comment is the first token on that line */
          layoutFlags |= LayoutFlags(CHECK_FIRST_TOKEN);
        }
        else if (c == EOF) {
          pos.updateWith(thisToken);
          return pos;
        }
      }

      pos.updateWith(thisToken);
      goto startOver;
    }
    else {
      ungetChar(c);
      c = '/';
    }
  }

  if (isWhiteSpace(c)) {
    while (isWhiteSpace(c)) {
      if (c == '\n' || c == '\r')
        layoutFlags |= LayoutFlags(CHECK_FIRST_TOKEN);

      c = getChar();
    }
    ungetChar(c);

    pos.updateWith(thisToken);
    goto startOver;
  }

  ungetChar(c);

  return pos;
}

void
TransitionLexer::pushTokenBack(LToken& tok)
{
  pushbackToken = tok;
  havePushbackToken = true;
}

LToken
TransitionLexer::getNextToken()
{
  LToken tok = getNextInputToken();

#if 1
  // The transitional syntax requires curly brace insertion at start
  // and end of file, but otherwise we don't want to do layout
  // processing when we are handling the S-expression syntax:
  if (((currentLang & lf_block) == 0) &&
      (lastToken.tokType != EOF) &&     // beginning of file
      (tok.tokType != EOF))     // end of file
    return tok;
#endif

  LexLoc startLoc = here;
  LexLoc endLoc = here;

  // Rule 1: A left curly-brace is automatically inserted after
  // certain tokens:
  //
  // FIX: Once I clean up the surface syntax, this should also be done
  // for tk_DO.
  bool curlyRequired = ((lastToken.tokType == EOF)        // beginning of file
                        || (lastToken.tokType == tk_CASE)
                        || (lastToken.tokType == tk_LET)
                        || (lastToken.tokType == tk_LETREC)
                        || (lastToken.tokType == tk_IN)
                        || (lastToken.tokType == tk_DO)
                        || false);

  if (curlyRequired && (tok.tokType != '{')) {
    pushTokenBack(tok);
    beginBlock(true);
    
    // No position update for inserted token.
    return LToken('{', startLoc, endLoc, "{");
  }

#ifdef LAYOUT_BLOCK_DEBUG
  if (lastToken < 256 && isprint(lastToken)) {
    errStream << "  LAYOUT: " << startLoc 
              << ": last token was '" << (char)lastToken << "'." << std::endl;
  }
  else {
    errStream << "  LAYOUT: " << startLoc 
              << ": last token was " << lastToken << "." << std::endl;
  }
#endif

  // Rule 2: If the last token was a '{', then the next token is
  // processed specially. Either:
  //
  //   a) The next token is more indented than the prevailing indent
  //      level, in which case it establishes the indent level for the
  //      newly introduced block, OR
  //   b) The next token is NOT more indented, in which case the
  //      block is presumed to have been empty and a '} is immediately
  //      inserted. 
  if (lastToken.tokType == '{') {
    assert(layoutStack);

#ifdef LAYOUT_BLOCK_DEBUG
    errStream << "  LAYOUT: " << startLoc 
              << ": Processing post-{..." << std::endl;
#endif

    if ((startLoc.offset > layoutStack->column) ||
        // Top-level context needs special handling if the curly brace
        // was implicitly inserted:
        (layoutStack->implicit && !layoutStack->next)) {
      // Valid indent. Establish indent level for new block:
      layoutStack->column = startLoc.offset;
#ifdef LAYOUT_BLOCK_DEBUG
      errStream << "  LAYOUT: " << startLoc 
                << ": set indent to "
                << startLoc.offset
                << ", disable check first token." << std::endl;
#endif
    }
    else if (layoutStack->implicit && layoutStack->next) {
#ifdef LAYOUT_BLOCK_DEBUG
      errStream << "  LAYOUT: " << startLoc 
                << " : close empty implicit block." << std::endl;
#endif
      // First token after '{' is at the previous (or earlier) indent
      // level, and it was an implicit open block. Close it immediately:
      pushTokenBack(tok);
      endBlock(true);

      return LToken('}', startLoc, endLoc, "}");
    }

    layoutFlags &= ~LayoutFlags(CHECK_FIRST_TOKEN);
  }

  // Rule 3: If this is the first token on a line, then WHILE the
  // current indent level is less than the prevailing indent level,
  // close implicit blocks.
  if (layoutFlags & LayoutFlags(CHECK_FIRST_TOKEN)) {
    // Consider possible right curly insertion and/or semicolon
    // insertion.

#ifdef LAYOUT_BLOCK_DEBUG
    errStream << "  LAYOUT: " << startLoc 
              << ": processing first token..." << std::endl;
#endif

    // FIX: Consider NOT doing start of line processing in various cases:

    closeToOffset(startLoc.offset);
    if (trimLayoutStack()) {
      pushTokenBack(tok);
      return LToken('}', startLoc, endLoc, "}");
    }
  }

  // Rule 4: At end of file, any outstanding implicit blocks are closed.
  if (tok.tokType == EOF) {
    closeToOpeningToken(EOF);
    if (trimLayoutStack()) {
      pushTokenBack(tok);
      return LToken('}', startLoc, endLoc, "}");
    }

    return tok;
  }

  // Rule 5: If this is the first token on a line, and the indent
  // level is the SAME as the prevailing indent level, insert a
  // semicolon UNLESS:
  //   a) we just saw one, or
  //   b) we are closing the block explicitly.

  if (layoutFlags & LayoutFlags(CHECK_FIRST_TOKEN)) {
    bool wantAutoSemi = !(lastToken.tokType == ';' ||
                          tok.tokType == ';' ||
                          tok.tokType == '}' ||
                          tok.tokType == tk_THEN ||
                          tok.tokType == tk_ELSE ||
                          tok.tokType == tk_IN ||
                          tok.tokType == tk_DO ||
                          false);

    if (wantAutoSemi) {
      if (conditionallyInsertSemicolon(startLoc.offset)) {
#ifdef LAYOUT_BLOCK_DEBUG
        errStream << "  LAYOUT: " << startLoc 
                  << ": semicolon inserted at offset " 
                  << startLoc.offset << std::endl;
#endif
        pushTokenBack(tok);
        return LToken(';', startLoc, endLoc, ";");
      }
    }

    layoutFlags &= ~LayoutFlags(CHECK_FIRST_TOKEN);
  }

  // Rule 6: When we see IN, close implicit braces up to the
  // appropriate preceding keyword. This is special-cased because IN
  // frequently appears as a de-facto close-bracket token on the same
  // line as the corresponding open-bracket token.
  //
  // FIX: Once I clean up the surface syntax, this should also be done
  // for tk_DO.
  if (tok.tokType == tk_IN) {
    closeToOpeningToken(tk_IN);
    if (trimLayoutStack()) {
      pushTokenBack(tok);
      return LToken('}', startLoc, endLoc, "}");
    }
  }

  // Open and close layout contexts when we see explicit open/close
  // curly braces:
  if (tok.tokType == '{')
    beginBlock(false);

  if (tok.tokType == '}')
    endBlock(false);

  return tok;
}

LToken
TransitionLexer::getNextInputToken()
{
  if (havePushbackToken) {
    havePushbackToken = false;
    return pushbackToken;
  }

  thisToken.erase();

  ucs4_t c = getChar();

  /////////////////////////////////////////////////////////
  // We've now eaten comments and white space, so whatver
  // location this is, this is where the new token starts:
  /////////////////////////////////////////////////////////
  LexLoc startLoc = here;
  LexLoc endLoc = here;

  switch (c) {
  case '`':
    {
      // back-tick should never appear in any context where a sexpr is
      // legal:
      if (currentLang & lf_sexpr) {
        endLoc.updateWith(thisToken);
        return LToken(EOF, startLoc, endLoc, "end of file");
      }

      endLoc.updateWith(thisToken);
      return LToken(c, startLoc, endLoc, thisToken);
    }

  case ';':
    {
      endLoc.updateWith(thisToken);
      return LToken(c, startLoc, endLoc, thisToken);
    }

  case ':':
    {
      int tokID = ':';

      ucs4_t c2 = getChar();
      if (c2 == '=')
        tokID = tk_ASSIGN;
      else if (c2 == ':')
        tokID = tk_INFIX_CONS;
      else
        ungetChar(c2);

      endLoc.updateWith(thisToken);
      return LToken(tokID, startLoc, endLoc, thisToken);
    }

  case '{':
    {
      endLoc.updateWith(thisToken);
      return LToken(c, startLoc, endLoc, thisToken);
    }
  case '}':
    {
      endLoc.updateWith(thisToken);
      return LToken(c, startLoc, endLoc, thisToken);
    }

  case '.':                        // Single character tokens
  case ',':
  case '[':
  case ']':
  case '(':
  case ')':
    {
      endLoc.updateWith(thisToken);
      return LToken(c, startLoc, endLoc, thisToken);
    }

  case '"':                        // String literal
    {
      do {
        c = getChar();

        if (c == '\\') {
          (void) getChar();        // just ignore it -- will validate later
        }
      } while (c != '"');
      
      unsigned badpos = LitValue::validate_string(thisToken.c_str());

      if (badpos) {
        LexLoc badHere = startLoc;
        badHere.offset += badpos;
        errStream << badHere.asString()
                  << ": Illegal (non-printing) character in string '"
                  << thisToken << "'\n";
        num_errors++;
      }

      endLoc.updateWith(thisToken);
      return LToken(tk_String, startLoc, endLoc, 
                    thisToken.substr(1, thisToken.size()-2));
    }


  case '#':                        // character and boolean literals
    {
      c = getChar();
      switch(c) {
      case 't':
        {
          if ((currentLang & lf_sexpr) == 0) {
            ungetChar('t');
            ungetChar('#');
            return LToken(EOF, startLoc, endLoc, "end of file");
          }
          endLoc.updateWith(thisToken);
          return LToken(tk_TRUE, startLoc, endLoc, thisToken);
        }
      case 'f':
        {
          if ((currentLang & lf_sexpr) == 0) {
            ungetChar('f');
            ungetChar('#');
            return LToken(EOF, startLoc, endLoc, "end of file");
          }

          endLoc.updateWith(thisToken);
          return LToken(tk_FALSE, startLoc, endLoc, thisToken);
        }

      default:
        // FIX: this is bad input
        {
          endLoc.updateWith(thisToken);
          return LToken(EOF, startLoc, endLoc, "end of file");
        }
      }
    }

  case '\'':                        // Type variable or character literal.
    {
      // This can signal a type variable or a UCS4 codepoint literal,
      // depending on whether a close-single-quote is present.
      int tokType = tk_TypeVar;

      int c1 = getChar();       // first character after '
      int c2 = getChar();       // second character after '

      if (c1 == EOF || c2 == EOF) {
        endLoc.updateWith(thisToken);
        return LToken(EOF, startLoc, endLoc, "end of file");
      }

      // Check for simple, one-codepoint character:
      if (c2 == '\'') {
        /* This is of the form '??', where ?? is a single character
           that is not "'" or "\": */
        switch (c1) {
        case '\'':
        case '\\':
          {
            // These are valid_charpunct() but not actually legal in
            // non-escaped characters:
            ungetChar(c2);
            ungetChar(c1);
            endLoc.updateWith(thisToken);
            return LToken(EOF, startLoc, endLoc, "end of file");
          }
        default:
          {
            if (LitValue::DecodeCharacter(thisToken) >= 0) {
              endLoc.updateWith(thisToken);
              return LToken(tk_Char, startLoc, endLoc, thisToken);
            }

            ungetChar(c2);
            ungetChar(c1);
            endLoc.updateWith(thisToken);
            return LToken(EOF, startLoc, endLoc, "end of file");
          }          
        }
      }
      else if (c1 == '\\') {
        // Escaped character. We have already consumed the next
        // character into c2. Scan forward to the matching '\''
        // and then test the result by calling DecodeCharacter to
        // validate it:
        do {
          c = getChar();
          if (c == EOF) {
            endLoc.updateWith(thisToken);
            return LToken(EOF, startLoc, endLoc, "end of file");
          }
        } while (c != '\'');

        if (LitValue::DecodeCharacter(thisToken) >= 0) {
          endLoc.updateWith(thisToken);
          return LToken(tk_Char, startLoc, endLoc, thisToken);
        }

        endLoc.updateWith(thisToken);
        return LToken(EOF, startLoc, endLoc, "end of file");
      }

      // Otherwise it is a type variable.

      // It must be a type variable:

      ungetChar(c2);

      if (c1 == '%') {
        tokType = tk_EffectVar;
        c1 = getChar();
      }

      if (!valid_tv_ident_start(c1)) {
        // FIX: this is bad input
        ungetChar(c1);
        endLoc.updateWith(thisToken);
        return LToken(EOF, startLoc, endLoc, "end of file");
      }

      do {
        c = getChar();
      } while (valid_tv_ident_continue(c));
      ungetChar(c);

      endLoc.updateWith(thisToken);
      return LToken(tokType, startLoc, endLoc, thisToken);
    }

    // Hyphen requires special handling. If it is followed by a digit
    // then it is the beginning of a numeric literal, else it is part
    // of an identifier.
  case '-':

    // We now handle unary negate at the expression level in the block
    // syntax:
    if ((currentLang & lf_sexpr) == 0) {
      ungetChar(c);
      goto identifier_or_operator;
    }

    c = getChar();

    if (LitValue::digitValue(c, 10) < 0) {
      ungetChar(c);
      ungetChar('-');
      goto identifier_or_operator;
    }

    /* ELSE fall through to digit processing */
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    {
      /// @bug Need to remove the legacy radix syntax here. When I do
      /// so, I should add support to accept and ignore '_' in decimal
      /// numbers as a placeholder for commas, e.g. 1_000_000

      int radix = 10;

      do {
        c = getChar();
      } while (LitValue::digitValue(c, 10) >= 0);

      /* At this point we could either discover a radix marker 'r', or
         a decimal point (indicating a floating poing literal or a
         language version). If it is a radix marker, change the radix
         value here so that we match the succeeding digits in the
         correct radix. */
      if (c == 'r') {
        radix = strtol(thisToken.c_str(), 0, 10);
        if (radix < 0) radix = -radix; // leading sign not part of radix

        long count = 0;
        do {
          c = getChar();
          count++;
        } while (LitValue::digitValue(c, radix) >= 0);
        count--;
        /* FIX: if count is 0, number is malformed */
      }

      /* We are either done with the literal, in which case it is an
         integer literal, or we are about to see a decimal point, in
         which case it is either a floating point literal or a
         language version. */
      if (c != '.') {
        ungetChar(c);
        int tokType = (thisToken[0] == '-') ? tk_NegativeInt : tk_Nat;

        endLoc.updateWith(thisToken);
        return LToken(tokType, startLoc, endLoc, thisToken);
      }

      if (currentLang & lf_version) {
        /* Looking for a language version number. */
        long count = 0;
        do {
          c = getChar();
          count++;
        } while (LitValue::digitValue(c, 10) >= 0);
        count--;
        ungetChar(c);
        endLoc.updateWith(thisToken);
        return LToken(tk_VersionNumber, startLoc, endLoc, thisToken);
      }
      else {
        /* We have seen a decimal point, so from here on it must be a
           floating point literal. */
        long count = 0;
        do {
          c = getChar();
          count++;
        } while (LitValue::digitValue(c, radix) >= 0);
        count--;
        /* FIX: if count is 0, number is malformed */
      }

      /* We are either done with this token or we are looking at a '^'
         indicating start of an exponent. */
      if (c != '^') {
        ungetChar(c);
        endLoc.updateWith(thisToken);
        return LToken(tk_Float, startLoc, endLoc, thisToken);
      }

      /* Need to collect the exponent. Revert to radix 10 until
         otherwise proven. */
      c = getChar();
      radix = 10;

      if (c != '-' && LitValue::digitValue(c, radix) < 0) {
        // FIX: Malformed token
      }

      do {
        c = getChar();
      } while (LitValue::digitValue(c, 10) >= 0);

      /* Check for radix marker on exponent */
      if (c == 'r') {
        radix = strtol(thisToken.c_str(), 0, 10);
        if (radix < 0) radix = -radix; // leading sign not part of radix

        long count = 0;
        do {
          c = getChar();
          count++;
        } while (LitValue::digitValue(c, radix) >= 0);
        count--;
        /* FIX: if count is 0, number is malformed */
      }

      ungetChar(c);
      endLoc.updateWith(thisToken);
      return LToken(tk_Float, startLoc, endLoc, thisToken);
    }

  case EOF:
    {
      endLoc.updateWith(thisToken);
      return LToken(EOF, startLoc, endLoc, "end of file");
    }

  default:
    if (valid_ident_start(c) || valid_operator_start(c)) {
      ungetChar(c);
      goto identifier_or_operator;
    }

    // FIX: Malformed token
    endLoc.updateWith(thisToken);
    return LToken(EOF, startLoc, endLoc, "end of file");
  }

 identifier_or_operator:
  do {
    c = getChar();
  } while (c == '_');
  ungetChar(c);

  // Possibly an alpha identifier:
  if (valid_ident_start(c)) {
    do {
      c = getChar();
    } while (valid_ident_continue(c));

    // Transitional handling for set! for S-expression LISP
    // syntax. The conditional will go away once we drop set!
    if (thisToken != "set!")
      ungetChar(c);

  ident_done:
    int tokType = kwCheck(thisToken.c_str(), tk_BlkIdent);
    endLoc.updateWith(thisToken);
    return LToken(tokType, startLoc, endLoc, thisToken);
  }
  else if (valid_operator_start(c)) {
    do {
      c = getChar();
    } while (valid_operator_continue(c));
    ungetChar(c);

    int tokType = kwCheck(thisToken.c_str(), tk_MixIdent);
    endLoc.updateWith(thisToken);
    return LToken(tokType, startLoc, endLoc, thisToken);
  }
}
