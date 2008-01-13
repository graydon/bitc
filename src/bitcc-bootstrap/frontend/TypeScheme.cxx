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

#include <stdint.h>
#include <stdlib.h>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <libsherpa/UExcept.hxx>
#include <libsherpa/CVector.hxx>
#include <libsherpa/avl.hxx>
#include <assert.h>

#include "UocInfo.hxx"
#include "Options.hxx"
#include "AST.hxx"
#include "Type.hxx"
#include "TypeInfer.hxx"
#include "TypeScheme.hxx"
#include "TypeMut.hxx"
#include "Typeclass.hxx"
#include "inter-pass.hxx"
#include "Unify.hxx"

using namespace sherpa;
using namespace std;

TypeScheme::TypeScheme(GCPtr<Type> ty, GCPtr<TCConstraints> _tcc)
{
  tau = ty;
  ast = ty->ast;
  tcc = _tcc;
  ftvs = new CVector<GCPtr<Type> >;
}

TypeScheme::TypeScheme(GCPtr<Type> ty, GCPtr<AST> _ast, GCPtr<TCConstraints> _tcc)
{
  tau = ty;
  ast = _ast;
  tcc = _tcc;
  ftvs = new CVector<GCPtr<Type> >;
}

GCPtr<Type> 
TypeScheme::type_instance_copy() const
{
  CVector<GCPtr<Type> > nftvs;
  
  //std::cout << "Instantiating by copy " << this->asString();

  for(size_t i=0; i<ftvs->size(); i++)
    nftvs.append(newTvar(Ftv(i)->ast));

  GCPtr<CVector<GCPtr<Type> > > cnftvs = new CVector<GCPtr<Type> >;
  GCPtr<CVector<GCPtr<Type> > > cftvs = new CVector<GCPtr<Type> >;
  
  for(size_t i=0; i<ftvs->size(); i++) {
    cftvs->append(Ftv(i));
    cnftvs->append(nftvs[i]);
  }
  
  GCPtr<Type> t = tau->TypeSpecialize(cftvs, cnftvs); 
  //std::cout << " to " << t->asString() << std::endl;

  return t;
}


GCPtr<Type> 
TypeScheme::type_instance() const
{
  //std::cout << "Instantiating " << this->asString();

  GCPtr<Type> t;
  if(ftvs->size() == 0)
    t = tau;
  else
    t = type_instance_copy();

  //std::cout << " to " << t->asString() << std::endl;

  return t;
}


GCPtr<TypeScheme> 
TypeScheme::ts_instance() const
{
  //std::cout << "TS Instantiating " << this->asString();

  GCPtr<TypeScheme> ts = new TypeScheme(tau);
  ts->tau = NULL;

  for(size_t i=0; i<ftvs->size(); i++) {
    GCPtr<Type> thisFtv = Ftv(i)->getType();
    GCPtr<AST> thisAst = thisFtv->ast;
    GCPtr<Type> newFtv = newTvar(thisAst);
    ts->ftvs->append(newFtv); // FIXed
  }

  GCPtr<CVector<GCPtr<Type> > > cnftvs = new CVector<GCPtr<Type> >;
  GCPtr<CVector<GCPtr<Type> > > cftvs = new CVector<GCPtr<Type> >;
  
  for(size_t i=0; i<ftvs->size(); i++) {
    cftvs->append(Ftv(i));
    cnftvs->append(ts->Ftv(i));
  }
  
  if(ftvs->size() == 0)
    ts->tau = tau;
  else
    ts->tau = tau->TypeSpecialize(cftvs, cnftvs);  
  //std::cout << " to " << ts->tau->asString() << std::endl;
 
  if(tcc != NULL) {
    GCPtr<TCConstraints> _tcc = new TCConstraints;
    addConstraints(_tcc);
    
    ts->tcc = new TCConstraints;

    for(size_t i = 0; i < _tcc->pred->size(); i++) {
      GCPtr<Typeclass> pred;
      if(ftvs->size() == 0)
	pred = _tcc->Pred(i);
      else
	pred = _tcc->Pred(i)->TypeSpecialize(cftvs, cnftvs);
      
      ts->tcc->addPred(pred);

      if(_tcc->Pred(i)->fnDeps) {
	assert(pred->fnDeps);
	assert(pred->fnDeps->size() == _tcc->Pred(i)->fnDeps->size());
      }
    }
  }
  
  return ts;
}

GCPtr<TypeScheme> 
TypeScheme::ts_instance_copy() const
{
  //std::cout << "TS Instantiating " << this->asString();

  GCPtr<TypeScheme> ts = new TypeScheme(tau);
  ts->tau = NULL;

  for(size_t i=0; i<ftvs->size(); i++) 
    ts->ftvs->append(newTvar(Ftv(i)->ast)); // FIXed
  

  GCPtr<CVector<GCPtr<Type> > > cnftvs = new CVector<GCPtr<Type> >;
  GCPtr<CVector<GCPtr<Type> > > cftvs = new CVector<GCPtr<Type> >;
  
  for(size_t i=0; i<ftvs->size(); i++) {
    cftvs->append(Ftv(i));
    cnftvs->append(ts->Ftv(i));
  }
  
  ts->tau = tau->TypeSpecialize(cftvs, cnftvs);  
  //std::cout << " to " << ts->tau->asString() << std::endl;

  if(tcc != NULL) {
    GCPtr<TCConstraints> _tcc = new TCConstraints;
    addConstraints(_tcc);
    
    ts->tcc = new TCConstraints;

    for(size_t i = 0; i < _tcc->pred->size(); i++) {
      GCPtr<Typeclass> pred = _tcc->Pred(i)->TypeSpecialize(cftvs, cnftvs);
      ts->tcc->addPred(pred);

      if(_tcc->Pred(i)->fnDeps) {
	assert(pred->fnDeps);
	assert(pred->fnDeps->size() == _tcc->Pred(i)->fnDeps->size());
      }
    }
  }
  
  return ts;
}

void
TypeScheme::addConstraints(GCPtr<TCConstraints> _tcc) const
{
  if(tcc == NULL)
    return;
  
  GCPtr<CVector<GCPtr<Type> > > allftvs = new CVector<GCPtr<Type> >;
  tau->collectAllftvs(allftvs);
  
  for(size_t i = 0; i < tcc->pred->size(); i++)    
    for(size_t j=0; j < allftvs->size(); j++)      
      if(tcc->Pred(i)->boundInType((*allftvs)[j])) {
	_tcc->addPred(tcc->Pred(i));
	break;
      }
  
  //if(tcc->pred.size()) {
  // std::cout << tau->ast->loc << "AddConstraints("
  //	      << tau->asString() << ", ";    
  // for(size_t i = 0; i < tcc->pred.size(); i++)
  //   std::cout << tcc->pred[i]->asString() << ", ";
  // std::cout << ") = ";
  // for(size_t i = 0; i < _tcc->pred.size(); i++)
  //   std::cout << _tcc->pred[i]->asString() << ", ";       
  // std::cout << "."
  //	      << std::endl;
  //}
}

