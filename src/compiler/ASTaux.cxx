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

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

#include "UocInfo.hxx"
#include "AST.hxx"
#include "Type.hxx"
#include "TypeInfer.hxx"
#include "inter-pass.hxx"
#include "FQName.hxx"

using namespace boost;
using namespace sherpa;

shared_ptr<Type>
AST::getType()
{
  return symType->getType();
}

shared_ptr <const Type>
AST::getType() const
{
  return symType->getType();
}

shared_ptr<AST>
AST::genIdent(const char *label, const bool isTV)
{
  shared_ptr<AST> id = AST::make(at_ident);

  std::stringstream ss;
  if (isTV)
    ss << "'__";
  else
    ss << "__";
  ss << label << id->ID;

  id->s = ss.str();

  return id;
}

shared_ptr<AST>
AST::genSym(shared_ptr<AST> ast, const char *label, const bool isTV)
{
  shared_ptr<AST> id = genIdent(label, isTV);
  // FQN to be set by the next call to the ersolver

  id->identType = ast->identType;
  id->flags = ast->flags | ID_IS_GENSYM;
  id->symType = ast->symType;
  id->scheme = ast->scheme;

  return id;
}

std::string
AST::asString() const
{
  std::stringstream ss;
  PrettyPrint(ss, pp_NONE);
  return ss.str();
}

std::string
AST::asString(PrettyPrintFlags flags) const
{
  std::stringstream ss;
  PrettyPrint(ss, flags);
  return ss.str();
}

bool
AST::isFnxn()
{
  return (symType->getBareType()->typeTag == ty_fn);
}

size_t
AST::nBits()
{
  if (field_bits != 0)
    return field_bits;
  else
    return tagType->nBits();
}


shared_ptr<AST>
AST::Use()
{
  assert(astType == at_ident);
  assert(!symbolDef || isIdentType(id_tvar));
  shared_ptr<AST> idUse = getDeepCopy();
  idUse->flags  &= ~MASK_FLAGS_FROM_USE;
  idUse->symbolDef = shared_from_this();
  if (symType)
    idUse->symType = symType->getDCopy();
  return idUse;
}


AST::AST(shared_ptr<AST> ast, bool shallowCopyChildren)
{
  astType = ast->astType;
  ID = ++(AST::astCount);
  identType = ast->identType;
  s = ast->s;
  loc = ast->loc;
  fqn = ast->fqn;
  flags = ast->flags;
  externalName = ast->externalName;
  symbolDef = ast->symbolDef;
  defn = ast->defn;
  decl = ast->decl;
  symType = ast->symType;
  scheme = ast->scheme;
  envs = ast->envs;
  defForm = ast->defForm;
  defbps = ast->defbps;
  litValue = ast->litValue;
  litBase = ast->litBase;
  isDecl = ast->isDecl;
  printVariant = ast->printVariant;
  tagType = ast->tagType;
  field_bits = ast->field_bits;
  unin_discm = ast->unin_discm;
  total_fill = ast->total_fill;

  if (shallowCopyChildren)
    children = ast->children;
}


shared_ptr<AST>
AST::getTrueCopy()
{
  shared_ptr<AST> to = AST::make(shared_from_this(), false);

  for (size_t i=0; i < children.size(); i++)
    to->children.push_back(child(i)->getTrueCopy());

  return to;
}

shared_ptr<AST>
AST::getDeepCopy()
{
  shared_ptr<AST> to = AST::make(shared_from_this(), false);
  to->symbolDef = GC_NULL;
  to->defn = GC_NULL;
  to->decl = GC_NULL;
  to->symType = GC_NULL;
  to->scheme = GC_NULL;
  to->envs = envs;
  to->defForm = GC_NULL;
  to->defbps = GC_NULL;

  for (size_t i=0; i<children.size(); i++)
    to->children.push_back(child(i)->getDeepCopy());
  return to;
}
