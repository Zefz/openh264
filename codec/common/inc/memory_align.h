/*!
 * \copy
 *     Copyright (c)  2013, Cisco Systems
 *     All rights reserved.
 *
 *     Redistribution and use in source and binary forms, with or without
 *     modification, are permitted provided that the following conditions
 *     are met:
 *
 *        * Redistributions of source code must retain the above copyright
 *          notice, this list of conditions and the following disclaimer.
 *
 *        * Redistributions in binary form must reproduce the above copyright
 *          notice, this list of conditions and the following disclaimer in
 *          the documentation and/or other materials provided with the
 *          distribution.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *     "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *     FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *     COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *     INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *     BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *     LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *     ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *     POSSIBILITY OF SUCH DAMAGE.
 *
 */

#pragma once

#include "typedefs.h"
#include <cstdio>
#include <memory>

namespace WelsCommon {

class CMemoryAlign {
public:
    CMemoryAlign (const uint32_t kuiCacheLineSize);

    void* WelsMallocz (const uint32_t kuiSize, const char* kpTag);
    void WelsFree (void* pPointer, const char* kpTag);

protected:
    uint32_t        m_nCacheLineSize;
};

#define  WELS_NEW_OP(object, type)   \
  (type*)(new object);

#define  WELS_DELETE_OP(p) \
  if(p) delete p;            \
  p = NULL;

static inline void* WelsMallocz(const uint32_t kuiSize, const char* kpTag = nullptr) {
#ifdef __linux__
    printf("[%s] zMalloc %ul bytes", kpTag == nullptr ? "unknown" : kpTag, kuiSize);
#endif
  return calloc(kuiSize, sizeof(uint8_t));
}

static inline void WelsFree(void *memory) {
    free(memory);
}

}
