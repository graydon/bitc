/**************************************************************************
 *
 * Copyright (C) 2008, The EROS Group, LLC
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

#include <gc/gc.h>
#include <inttypes.h>
#include "BUILD/bitc-runtime.h"

/// @addtogroup ProcObjects
///
/// <b>x86_64</b> The X86_64 implementation uses the Load-Immediate
/// method, storing the closure record pointer into
/// <code>@%rax</code>. No other register is touched, and
/// <code>@%rax</code> is reserved on this architecture for use as the
/// return register. Note that <code>@%rax</code> is considered
/// call-clobbered even when the called procedure
/// returns <code>void</code>.
void *
bitc_emit_procedure_object(void *stubP, void *envP)
{
  uint64_t envW = (uint64_t) envP;
  uint64_t stubW = (uint64_t) stubP;

  bitc_Procedure* proc = GC_ALLOC(sizeof(bitc_Procedure));

  /* MOVQ $stubW, %rax */
  proc->code[0] = 0x48;
  proc->code[1] = 0xb8;
  proc->code[2] = stubW;
  proc->code[3] = (stubW >> 8);
  proc->code[4] = (stubW >> 16);
  proc->code[5] = (stubW >> 24);
  proc->code[6] = (stubW >> 32);
  proc->code[7] = (stubW >> 40);
  proc->code[8] = (stubW >> 48);
  proc->code[9] = (stubW >> 56);

  /* PUSH %rax */
  proc->code[10] = 0x50;

  /* NOP, NOP, NOP */
  proc->code[11] = 0x90;
  proc->code[12] = 0x90;
  proc->code[13] = 0x90;

  /* movq $envP,%rax */
  proc->code[14] = 0x48u;
  proc->code[15] = 0xb8u;

  /* [16..23] will be filled in via env.ptr */

  /* RETQ */
  proc->code[24] = 0xc3u;

  proc->env.ptr = envP;

  return proc;

  /* On this architecture, no explicit iCache flush is required as
   * long as a branch appears. Return suffices for a branch, which is why
   * this must not be an inline procedure. */
}
