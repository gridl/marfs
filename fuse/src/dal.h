#ifndef _MARFS_DAL_H
#define _MARFS_DAL_H

/*
This file is part of MarFS, which is released under the BSD license.


Copyright (c) 2015, Los Alamos National Security (LANS), LLC
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

-----
NOTE:
-----
MarFS uses libaws4c for Amazon S3 object communication. The original version
is at https://aws.amazon.com/code/Amazon-S3/2601 and under the LGPL license.
LANS, LLC added functionality to the original work. The original work plus
LANS, LLC contributions is found at https://github.com/jti-lanl/aws4c.

GNU licenses can be found at <http://www.gnu.org/licenses/>.


From Los Alamos National Security, LLC:
LA-CC-15-039

Copyright (c) 2015, Los Alamos National Security, LLC All rights reserved.
Copyright 2015. Los Alamos National Security, LLC. This software was produced
under U.S. Government contract DE-AC52-06NA25396 for Los Alamos National
Laboratory (LANL), which is operated by Los Alamos National Security, LLC for
the U.S. Department of Energy. The U.S. Government has rights to use,
reproduce, and distribute this software.  NEITHER THE GOVERNMENT NOR LOS
ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR
ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  If software is
modified to produce derivative works, such modified software should be
clearly marked, so as not to confuse it with the version available from
LANL.

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/


// ---------------------------------------------------------------------------
// MarFS  Data Abstraction Layer (DAL)
//
// This is an abstract interface used for interaction with the Storage part
// of MarFS.  Just as the MDAL provides an abstract interface to the MarFS
// metadata, this abstraction is used when marfs writes actual data to
// storage.  The DAL implementations need only do the simplest part of
// what their interface suggests.
// ---------------------------------------------------------------------------


#include "marfs_configuration.h" // DAL_Type

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <fcntl.h>
#include <attr/xattr.h>
#include <dirent.h>
#include <utime.h>

#include <stdio.h>


#  ifdef __cplusplus
extern "C" {
#  endif


// DAL_Context
//
// This is per-file-handle DAL-state.  Think of it as equivalent to a
// file-descriptor. Individual implementations might need extra state, to
// be shared across some or all calls with the same file-handle (e.g. POSIX
// needs a file-descriptor).  Our first cut at this is to provide an extra
// Context argument to all DAL calls.  Individual implementations can
// allocate storage here for whatever state they need.
//
// There is also global-state in each DAL struct (not to be confused with
// DAL_Context).  This can be initialized (if needed) in dal_ctx_init().
// It will initially be NULL.  If desired, one could use dal_ctx_init()
// to associate individual contexts to have a link to the global state.
//
// The MarFS calls to the DAL implementation functions will not touch the
// contents of the Context, except that we call init_dal_context() when
// MarFS file-handles are created, and delete_dal_context() when MarFS
// file-handles are destroyed.
// 

typedef struct {
   uint32_t  flags;
   union {
      void*     ptr;
      size_t    sz;
      ssize_t   ssz;
      uint32_t  u;
      int32_t   i;
   } data;
} DAL_Context;


// DAL function signatures



// fwd-decl
struct DAL;


// initialize/destroy context, if desired.
//
//   -- init    is called before any other ops (per file-handle).
//   -- destroy is called when a file-handle is being destroyed.
//
#if 0
typedef  int     (*dal_ctx_init)   (DAL_Context* ctx, struct DAL* dal);
typedef  int     (*dal_ctx_destroy)(DAL_Context* ctx, struct DAL* dal);

#else
typedef  int     (*dal_ctx_init)   (DAL_Context* ctx, struct DAL* dal, void* os);
typedef  int     (*dal_ctx_destroy)(DAL_Context* ctx, struct DAL* dal);
#endif


// --- storage ops

// return NULL from dal_open(), for failure 
// This value is only checked for NULL/non-NULL
//
// TBD: This should probably use va_args, to improve generality For now
//      we're just copying the object_stream inteface verbatim.
//
typedef int      (*dal_open) (DAL_Context* ctx,
                              int          is_put,
                              size_t       content_length,
                              uint8_t      preserve_write_count,
                              uint16_t     timeout);

typedef int      (*dal_put)  (DAL_Context*  ctx,
                              const char*   buf,
                              size_t        size);

typedef ssize_t  (*dal_get)  (DAL_Context*  ctx,
                              char*         buf,
                              size_t        size);

typedef int      (*dal_sync) (DAL_Context*  ctx);
typedef int      (*dal_abort)(DAL_Context*  ctx);

typedef int      (*dal_close)(DAL_Context*  ctx);



// This is a collection of function-ptrs
// They capture a given implementation of interaction with an MDFS.
typedef struct DAL {
   // DAL_Type             type;
   const char*          name;
   size_t               name_len;

   void*                global_state;

   dal_ctx_init         init;
   dal_ctx_destroy      destroy;

   dal_open             open;
   dal_sync             sync;
   dal_abort            abort;
   dal_close            close;

   dal_put              put;
   dal_get              get;

} DAL;


// insert a new DAL, if there are no name-conflicts
int  install_DAL(DAL* dal);

// find a DAL with the given name
DAL* get_DAL(const char* name);




// exported for building custom DAL
int     default_dal_ctx_init(DAL_Context* ctx, DAL* dal, void* os);
int     default_dal_ctx_destroy(DAL_Context* ctx, DAL* dal);



#  ifdef __cplusplus
}
#  endif

#endif
