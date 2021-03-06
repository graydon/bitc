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

#include <stdlib.h>
#include <assert.h>


#include "Options.hxx"
#include "Type.hxx"
#include "Trail.hxx"

using namespace boost;
using namespace sherpa;

/// @brief Link @p from to @p to without regard to whether @p from is
/// actually a type variable.
///
/// This is an obscure case. It arises in structure types when
/// maximizing mutability. The problem here is that we have an 'a that
/// has already been concretized, and we now want to maximize the
/// mutability of the alpha parameter. The way we accomplish that is
/// by setting the link pointer on the original, concrete type so as
/// to point to the new, more mutable type. It's a re-substitution, so
/// it is as if we are re-writing the original concretization with a
/// new alpha variable in-place, and then resolving that alpha
/// variable to the new result.
void
Trail::link(shared_ptr<Type> from, shared_ptr<Type> to)
{  
  from = from->getType();
  to = to->getType();
  
  if (to->boundInType(from)) {
    std::cerr << "CYCLIC SUBSTITUTION "
              << from->asString(Options::debugTvP)
              << " |-> "
              << to->asString(Options::debugTvP)
              << std::endl;
    assert(false);
  }

  DEBUG(TRAIL) 
    std::cerr << "LINKING: "
              << from->asString(Options::debugTvP)
              << " |-> "
              << to->asString(Options::debugTvP)
              << std::endl;
  
  vec.push_back(from);   
  from->link = to; 
} 

void
Trail::subst(shared_ptr<Type> from, shared_ptr<Type> to)
{ 
  from = from->getType();
  to = to->getType();
  
  assert(from->typeTag == ty_tvar || from->typeTag == ty_kvar);
  
  if (to->boundInType(from)) {
    std::cerr << "CYCLIC SUBSTITUTION "
              << from->asString(Options::debugTvP)
              << " |-> "
              << to->asString(Options::debugTvP)
              << std::endl;
    assert(false);
  }

  DEBUG(TRAIL) 
    std::cerr << "SUBSTITUTING: "
              << from->asString(Options::debugTvP)
              << " |-> "
              << to->asString(Options::debugTvP)
              << std::endl;

  
  vec.push_back(from);   
  from->link = to; 
}

void
Trail::rollBack(size_t upto)
{
  assert(upto <= vec.size());
  
  for (size_t i = upto; i < vec.size(); i++) {
    vec[i]->link = GC_NULL;
    DEBUG(TRAIL) 
      std::cerr << "[RB] Releasing: "
                << vec[i]->asString(Options::debugTvP)
                << std::endl;
  }

  vec.erase(vec.begin() + upto, vec.end());
}

void
Trail::release(const size_t n, shared_ptr<Type> rel)
{
  assert(vec[n] == rel); // not getType()
  
  rel->link = GC_NULL;
  
  DEBUG(TRAIL) 
    std::cerr << "[RB] Releasing: "
              << rel->asString(Options::debugTvP)
              << std::endl;

  vec.erase(vec.begin() + n);
}
