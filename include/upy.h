#define MP_CONFIGFILE "mpconfigport.h"
#include <stdarg.h>
#define MICROPY_QSTR_BYTES_IN_LEN (1)
#define MICROPY_QSTR_BYTES_IN_HASH (2)
#define MP_SSIZE_MAX (0x7fffffff)
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_MPCONFIG_H__
#define __MICROPY_INCLUDED_PY_MPCONFIG_H__

// This file contains default configuration settings for MicroPython.
// You can override any of the options below using mpconfigport.h file
// located in a directory of your port.

// mpconfigport.h is a file containing configuration settings for a
// particular port. mpconfigport.h is actually a default name for
// such config, and it can be overriden using MP_CONFIGFILE preprocessor
// define (you can do that by passing CFLAGS_EXTRA='-DMP_CONFIGFILE="<file.h>"'
// argument to make when using standard MicroPython makefiles).
// This is useful to have more than one config per port, for example,
// release vs debug configs, etc. Note that if you switch from one config
// to another, you must rebuild from scratch using "-B" switch to make.

#ifdef MP_CONFIGFILE
#include MP_CONFIGFILE
#else
#include "mpconfigport.h"
#endif

// Any options not explicitly set in mpconfigport.h will get default
// values below.

/*****************************************************************************/
/* Object representation                                                     */

// A Micro Python object is a machine word having the following form:
//  - xxxx...xxx1 : a small int, bits 1 and above are the value
//  - xxxx...xx10 : a qstr, bits 2 and above are the value
//  - xxxx...xx00 : a pointer to an mp_obj_base_t (unless a fake object)
#define MICROPY_OBJ_REPR_A (0)

// A Micro Python object is a machine word having the following form:
//  - xxxx...xx01 : a small int, bits 2 and above are the value
//  - xxxx...xx11 : a qstr, bits 2 and above are the value
//  - xxxx...xxx0 : a pointer to an mp_obj_base_t (unless a fake object)
#define MICROPY_OBJ_REPR_B (1)

// A MicroPython object is a machine word having the following form (called R):
//  - iiiiiiii iiiiiiii iiiiiiii iiiiiii1 small int with 31-bit signed value
//  - 01111111 1qqqqqqq qqqqqqqq qqqqq110 str with 20-bit qstr value
//  - s1111111 10000000 00000000 00000010 +/- inf
//  - s1111111 1xxxxxxx xxxxxxxx xxxxx010 nan, x != 0
//  - seeeeeee efffffff ffffffff ffffff10 30-bit fp, e != 0xff
//  - pppppppp pppppppp pppppppp pppppp00 ptr (4 byte alignment)
// Str and float stored as O = R + 0x80800000, retrieved as R = O - 0x80800000.
// This makes strs easier to encode/decode as they have zeros in the top 9 bits.
// This scheme only works with 32-bit word size and float enabled.

#define MICROPY_OBJ_REPR_C (2)

// A MicroPython object is a 64-bit word having the following form (called R):
//  - seeeeeee eeeeffff ffffffff ffffffff ffffffff ffffffff ffffffff ffffffff 64-bit fp, e != 0x7ff
//  - s1111111 11110000 00000000 00000000 00000000 00000000 00000000 00000000 +/- inf
//  - 01111111 11111000 00000000 00000000 00000000 00000000 00000000 00000000 normalised nan
//  - 01111111 11111101 00000000 00000000 iiiiiiii iiiiiiii iiiiiiii iiiiiii1 small int
//  - 01111111 11111110 00000000 00000000 qqqqqqqq qqqqqqqq qqqqqqqq qqqqqqq1 str
//  - 01111111 11111100 00000000 00000000 pppppppp pppppppp pppppppp pppppp00 ptr (4 byte alignment)
// Stored as O = R + 0x8004000000000000, retrieved as R = O - 0x8004000000000000.
// This makes pointers have all zeros in the top 32 bits.
// Small-ints and strs have 1 as LSB to make sure they don't look like pointers
// to the garbage collector.
#define MICROPY_OBJ_REPR_D (3)

#ifndef MICROPY_OBJ_REPR
#define MICROPY_OBJ_REPR (MICROPY_OBJ_REPR_A)
#endif

/*****************************************************************************/
/* Memory allocation policy                                                  */

// Number of bytes in memory allocation/GC block. Any size allocated will be
// rounded up to be multiples of this.
#ifndef MICROPY_BYTES_PER_GC_BLOCK
#define MICROPY_BYTES_PER_GC_BLOCK (4 * BYTES_PER_WORD)
#endif

// Number of words allocated (in BSS) to the GC stack (minimum is 1)
#ifndef MICROPY_ALLOC_GC_STACK_SIZE
#define MICROPY_ALLOC_GC_STACK_SIZE (64)
#endif

// Number of bytes to allocate initially when creating new chunks to store
// interned string data.  Smaller numbers lead to more chunks being needed
// and more wastage at the end of the chunk.  Larger numbers lead to wasted
// space at the end when no more strings need interning.
#ifndef MICROPY_ALLOC_QSTR_CHUNK_INIT
#define MICROPY_ALLOC_QSTR_CHUNK_INIT (128)
#endif

// Initial amount for lexer indentation level
#ifndef MICROPY_ALLOC_LEXER_INDENT_INIT
#define MICROPY_ALLOC_LEXER_INDENT_INIT (10)
#endif

// Increment for lexer indentation level
#ifndef MICROPY_ALLOC_LEXEL_INDENT_INC
#define MICROPY_ALLOC_LEXEL_INDENT_INC (8)
#endif

// Initial amount for parse rule stack
#ifndef MICROPY_ALLOC_PARSE_RULE_INIT
#define MICROPY_ALLOC_PARSE_RULE_INIT (64)
#endif

// Increment for parse rule stack
#ifndef MICROPY_ALLOC_PARSE_RULE_INC
#define MICROPY_ALLOC_PARSE_RULE_INC (16)
#endif

// Initial amount for parse result stack
#ifndef MICROPY_ALLOC_PARSE_RESULT_INIT
#define MICROPY_ALLOC_PARSE_RESULT_INIT (32)
#endif

// Increment for parse result stack
#ifndef MICROPY_ALLOC_PARSE_RESULT_INC
#define MICROPY_ALLOC_PARSE_RESULT_INC (16)
#endif

// Strings this length or less will be interned by the parser
#ifndef MICROPY_ALLOC_PARSE_INTERN_STRING_LEN
#define MICROPY_ALLOC_PARSE_INTERN_STRING_LEN (10)
#endif

// Number of bytes to allocate initially when creating new chunks to store
// parse nodes.  Small leads to fragmentation, large leads to excess use.
#ifndef MICROPY_ALLOC_PARSE_CHUNK_INIT
#define MICROPY_ALLOC_PARSE_CHUNK_INIT (128)
#endif

// Initial amount for ids in a scope
#ifndef MICROPY_ALLOC_SCOPE_ID_INIT
#define MICROPY_ALLOC_SCOPE_ID_INIT (4)
#endif

// Increment for ids in a scope
#ifndef MICROPY_ALLOC_SCOPE_ID_INC
#define MICROPY_ALLOC_SCOPE_ID_INC (6)
#endif

// Maximum length of a path in the filesystem
// So we can allocate a buffer on the stack for path manipulation in import
#ifndef MICROPY_ALLOC_PATH_MAX
#define MICROPY_ALLOC_PATH_MAX (512)
#endif

// Initial size of module dict
#ifndef MICROPY_MODULE_DICT_SIZE
#define MICROPY_MODULE_DICT_SIZE (1)
#endif

// Whether realloc/free should be passed allocated memory region size
// You must enable this if MICROPY_MEM_STATS is enabled
#ifndef MICROPY_MALLOC_USES_ALLOCATED_SIZE
#define MICROPY_MALLOC_USES_ALLOCATED_SIZE (0)
#endif

// Number of bytes used to store qstr length
// Dictates hard limit on maximum Python identifier length, but 1 byte
// (limit of 255 bytes in an identifier) should be enough for everyone
#ifndef MICROPY_QSTR_BYTES_IN_LEN
#define MICROPY_QSTR_BYTES_IN_LEN (1)
#endif

// Number of bytes used to store qstr hash
#ifndef MICROPY_QSTR_BYTES_IN_HASH
#define MICROPY_QSTR_BYTES_IN_HASH (2)
#endif

// Avoid using C stack when making Python function calls. C stack still
// may be used if there's no free heap.
#ifndef MICROPY_STACKLESS
#define MICROPY_STACKLESS (0)
#endif

// Never use C stack when making Python function calls. This may break
// testsuite as will subtly change which exception is thrown in case
// of too deep recursion and other similar cases.
#ifndef MICROPY_STACKLESS_STRICT
#define MICROPY_STACKLESS_STRICT (0)
#endif

// Don't use alloca calls. As alloca() is not part of ANSI C, this
// workaround option is provided for compilers lacking this de-facto
// standard function. The way it works is allocating from heap, and
// relying on garbage collection to free it eventually. This is of
// course much less optimal than real alloca().
#if defined(MICROPY_NO_ALLOCA) && MICROPY_NO_ALLOCA
#undef alloca
#define alloca(x) micropy_m_malloc(mp_state, x)
#endif

/*****************************************************************************/
/* Micro Python emitters                                                     */

// Whether to support loading of persistent code
#ifndef MICROPY_PERSISTENT_CODE_LOAD
#define MICROPY_PERSISTENT_CODE_LOAD (0)
#endif

// Whether to support saving of persistent code
#ifndef MICROPY_PERSISTENT_CODE_SAVE
#define MICROPY_PERSISTENT_CODE_SAVE (0)
#endif

// Whether generated code can persist independently of the VM/runtime instance
// This is enabled automatically when needed by other features
#ifndef MICROPY_PERSISTENT_CODE
#define MICROPY_PERSISTENT_CODE (MICROPY_PERSISTENT_CODE_LOAD || MICROPY_PERSISTENT_CODE_SAVE || MICROPY_MODULE_FROZEN_MPY)
#endif

// Whether to emit x64 native code
#ifndef MICROPY_EMIT_X64
#define MICROPY_EMIT_X64 (0)
#endif

// Whether to emit x86 native code
#ifndef MICROPY_EMIT_X86
#define MICROPY_EMIT_X86 (0)
#endif

// Whether to emit thumb native code
#ifndef MICROPY_EMIT_THUMB
#define MICROPY_EMIT_THUMB (0)
#endif

// Whether to enable the thumb inline assembler
#ifndef MICROPY_EMIT_INLINE_THUMB
#define MICROPY_EMIT_INLINE_THUMB (0)
#endif

// Whether to enable ARMv7-M instruction support in the Thumb2 inline assembler
#ifndef MICROPY_EMIT_INLINE_THUMB_ARMV7M
#define MICROPY_EMIT_INLINE_THUMB_ARMV7M (1)
#endif

// Whether to enable float support in the Thumb2 inline assembler
#ifndef MICROPY_EMIT_INLINE_THUMB_FLOAT
#define MICROPY_EMIT_INLINE_THUMB_FLOAT (1)
#endif

// Whether to emit ARM native code
#ifndef MICROPY_EMIT_ARM
#define MICROPY_EMIT_ARM (0)
#endif

// Convenience definition for whether any native emitter is enabled
#define MICROPY_EMIT_NATIVE (MICROPY_EMIT_X64 || MICROPY_EMIT_X86 || MICROPY_EMIT_THUMB || MICROPY_EMIT_ARM)

/*****************************************************************************/
/* Compiler configuration                                                    */

// Whether to include the compiler
#ifndef MICROPY_ENABLE_COMPILER
#define MICROPY_ENABLE_COMPILER (1)
#endif

// Whether the compiler is dynamically configurable (ie at runtime)
#ifndef MICROPY_DYNAMIC_COMPILER
#define MICROPY_DYNAMIC_COMPILER (0)
#endif

// Configure dynamic compiler macros
#if MICROPY_DYNAMIC_COMPILER
#define MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE_DYNAMIC (mp_dynamic_compiler.opt_cache_map_lookup_in_bytecode)
#define MICROPY_PY_BUILTINS_STR_UNICODE_DYNAMIC (mp_dynamic_compiler.py_builtins_str_unicode)
#else
#define MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE_DYNAMIC MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE
#define MICROPY_PY_BUILTINS_STR_UNICODE_DYNAMIC MICROPY_PY_BUILTINS_STR_UNICODE
#endif

// Whether to enable constant folding; eg 1+2 rewritten as 3
#ifndef MICROPY_COMP_CONST_FOLDING
#define MICROPY_COMP_CONST_FOLDING (1)
#endif

// Whether to enable lookup of constants in modules; eg module.CONST
#ifndef MICROPY_COMP_MODULE_CONST
#define MICROPY_COMP_MODULE_CONST (0)
#endif

// Whether to enable constant optimisation; id = const(value)
#ifndef MICROPY_COMP_CONST
#define MICROPY_COMP_CONST (1)
#endif

// Whether to enable optimisation of: a, b = c, d
// Costs 124 bytes (Thumb2)
#ifndef MICROPY_COMP_DOUBLE_TUPLE_ASSIGN
#define MICROPY_COMP_DOUBLE_TUPLE_ASSIGN (1)
#endif

// Whether to enable optimisation of: a, b, c = d, e, f
// Cost 156 bytes (Thumb2)
#ifndef MICROPY_COMP_TRIPLE_TUPLE_ASSIGN
#define MICROPY_COMP_TRIPLE_TUPLE_ASSIGN (0)
#endif

/*****************************************************************************/
/* Internal debugging stuff                                                  */

// Whether to collect memory allocation stats
#ifndef MICROPY_MEM_STATS
#define MICROPY_MEM_STATS (0)
#endif

// Whether to build functions that print debugging info:
//   mp_lexer_show_token
//   mp_bytecode_print
//   mp_parse_node_print
#ifndef MICROPY_DEBUG_PRINTERS
#define MICROPY_DEBUG_PRINTERS (0)
#endif

/*****************************************************************************/
/* Optimisations                                                             */

// Whether to use computed gotos in the VM, or a switch
// Computed gotos are roughly 10% faster, and increase VM code size by a little
#ifndef MICROPY_OPT_COMPUTED_GOTO
#define MICROPY_OPT_COMPUTED_GOTO (0)
#endif

// Whether to cache result of map lookups in LOAD_NAME, LOAD_GLOBAL, LOAD_ATTR,
// STORE_ATTR bytecodes.  Uses 1 byte extra RAM for each of these opcodes and
// uses a bit of extra code ROM, but greatly improves lookup speed.
#ifndef MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE
#define MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE (0)
#endif

// Whether to use fast versions of bitwise operations (and, or, xor) when the
// arguments are both positive.  Increases Thumb2 code size by about 250 bytes.
#ifndef MICROPY_OPT_MPZ_BITWISE
#define MICROPY_OPT_MPZ_BITWISE (0)
#endif

/*****************************************************************************/
/* Python internal features                                                  */

// Hook for the VM at the start of the opcode loop (can contain variable
// definitions usable by the other hook functions)
#ifndef MICROPY_VM_HOOK_INIT
#define MICROPY_VM_HOOK_INIT
#endif

// Hook for the VM during the opcode loop (but only after jump opcodes)
#ifndef MICROPY_VM_HOOK_LOOP
#define MICROPY_VM_HOOK_LOOP
#endif

// Hook for the VM just before return opcode is finished being interpreted
#ifndef MICROPY_VM_HOOK_RETURN
#define MICROPY_VM_HOOK_RETURN
#endif

// Whether to include the garbage collector
#ifndef MICROPY_ENABLE_GC
#define MICROPY_ENABLE_GC (0)
#endif

// Whether to enable finalisers in the garbage collector (ie call __del__)
#ifndef MICROPY_ENABLE_FINALISER
#define MICROPY_ENABLE_FINALISER (0)
#endif

// Whether to check C stack usage. C stack used for calling Python functions,
// etc. Not checking means segfault on overflow.
#ifndef MICROPY_STACK_CHECK
#define MICROPY_STACK_CHECK (0)
#endif

// Whether to have an emergency exception buffer
#ifndef MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF
#define MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF (0)
#endif
#if MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF
#   ifndef MICROPY_EMERGENCY_EXCEPTION_BUF_SIZE
#   define MICROPY_EMERGENCY_EXCEPTION_BUF_SIZE (0)   // 0 - implies dynamic allocation
#   endif
#endif

// Prefer to raise KeyboardInterrupt asynchronously (from signal or interrupt
// handler) - if supported by a particular port.
#ifndef MICROPY_ASYNC_KBD_INTR
#define MICROPY_ASYNC_KBD_INTR (0)
#endif

// Whether to include REPL helper function
#ifndef MICROPY_HELPER_REPL
#define MICROPY_HELPER_REPL (0)
#endif

// Whether to include emacs-style readline behavior in REPL
#ifndef MICROPY_REPL_EMACS_KEYS
#define MICROPY_REPL_EMACS_KEYS (0)
#endif

// Whether to implement auto-indent in REPL
#ifndef MICROPY_REPL_AUTO_INDENT
#define MICROPY_REPL_AUTO_INDENT (0)
#endif

// Whether port requires event-driven REPL functions
#ifndef MICROPY_REPL_EVENT_DRIVEN
#define MICROPY_REPL_EVENT_DRIVEN (0)
#endif

// Whether to include lexer helper function for unix
#ifndef MICROPY_HELPER_LEXER_UNIX
#define MICROPY_HELPER_LEXER_UNIX (0)
#endif

// Long int implementation
#define MICROPY_LONGINT_IMPL_NONE (0)
#define MICROPY_LONGINT_IMPL_LONGLONG (1)
#define MICROPY_LONGINT_IMPL_MPZ (2)

#ifndef MICROPY_LONGINT_IMPL
#define MICROPY_LONGINT_IMPL (MICROPY_LONGINT_IMPL_NONE)
#endif

#if MICROPY_LONGINT_IMPL == MICROPY_LONGINT_IMPL_LONGLONG
typedef long long mp_longint_impl_t;
#endif

// Whether to include information in the byte code to determine source
// line number (increases RAM usage, but doesn't slow byte code execution)
#ifndef MICROPY_ENABLE_SOURCE_LINE
#define MICROPY_ENABLE_SOURCE_LINE (0)
#endif

// Whether to include doc strings (increases RAM usage)
#ifndef MICROPY_ENABLE_DOC_STRING
#define MICROPY_ENABLE_DOC_STRING (0)
#endif

// Exception messages are short static strings
#define MICROPY_ERROR_REPORTING_TERSE    (1)
// Exception messages provide basic error details
#define MICROPY_ERROR_REPORTING_NORMAL   (2)
// Exception messages provide full info, e.g. object names
#define MICROPY_ERROR_REPORTING_DETAILED (3)

#ifndef MICROPY_ERROR_REPORTING
#define MICROPY_ERROR_REPORTING (MICROPY_ERROR_REPORTING_NORMAL)
#endif

// Whether issue warnings during compiling/execution
#ifndef MICROPY_WARNINGS
#define MICROPY_WARNINGS (0)
#endif

// Float and complex implementation
#define MICROPY_FLOAT_IMPL_NONE (0)
#define MICROPY_FLOAT_IMPL_FLOAT (1)
#define MICROPY_FLOAT_IMPL_DOUBLE (2)

#ifndef MICROPY_FLOAT_IMPL
#define MICROPY_FLOAT_IMPL (MICROPY_FLOAT_IMPL_NONE)
#endif

#if MICROPY_FLOAT_IMPL == MICROPY_FLOAT_IMPL_FLOAT
#define MICROPY_PY_BUILTINS_FLOAT (1)
#define MICROPY_FLOAT_C_FUN(fun) fun##f
typedef float mp_float_t;
#elif MICROPY_FLOAT_IMPL == MICROPY_FLOAT_IMPL_DOUBLE
#define MICROPY_PY_BUILTINS_FLOAT (1)
#define MICROPY_FLOAT_C_FUN(fun) fun
typedef double mp_float_t;
#else
#define MICROPY_PY_BUILTINS_FLOAT (0)
#endif

#ifndef MICROPY_PY_BUILTINS_COMPLEX
#define MICROPY_PY_BUILTINS_COMPLEX (MICROPY_PY_BUILTINS_FLOAT)
#endif

// Enable features which improve CPython compatibility
// but may lead to more code size/memory usage.
// TODO: Originally intended as generic category to not
// add bunch of once-off options. May need refactoring later
#ifndef MICROPY_CPYTHON_COMPAT
#define MICROPY_CPYTHON_COMPAT (1)
#endif

// Whether POSIX-semantics non-blocking streams are supported
#ifndef MICROPY_STREAMS_NON_BLOCK
#define MICROPY_STREAMS_NON_BLOCK (0)
#endif

// Whether to call __init__ when importing builtin modules for the first time
#ifndef MICROPY_MODULE_BUILTIN_INIT
#define MICROPY_MODULE_BUILTIN_INIT (0)
#endif

// Whether module weak links are supported
#ifndef MICROPY_MODULE_WEAK_LINKS
#define MICROPY_MODULE_WEAK_LINKS (0)
#endif

// Whether frozen modules are supported in the form of strings
#ifndef MICROPY_MODULE_FROZEN_STR
#define MICROPY_MODULE_FROZEN_STR (0)
#endif

// Whether frozen modules are supported in the form of .mpy files
#ifndef MICROPY_MODULE_FROZEN_MPY
#define MICROPY_MODULE_FROZEN_MPY (0)
#endif

// Convenience macro for whether frozen modules are supported
#ifndef MICROPY_MODULE_FROZEN
#define MICROPY_MODULE_FROZEN (MICROPY_MODULE_FROZEN_STR || MICROPY_MODULE_FROZEN_MPY)
#endif

// Whether you can override builtins in the builtins module
#ifndef MICROPY_CAN_OVERRIDE_BUILTINS
#define MICROPY_CAN_OVERRIDE_BUILTINS (0)
#endif

// Whether to check that the "self" argument of a builtin method has the
// correct type.  Such an explicit check is only needed if a builtin
// method escapes to Python land without a first argument, eg
// list.append([], 1).  Without this check such calls will have undefined
// behaviour (usually segfault) if the first argument is the wrong type.
#ifndef MICROPY_BUILTIN_METHOD_CHECK_SELF_ARG
#define MICROPY_BUILTIN_METHOD_CHECK_SELF_ARG (1)
#endif

// Whether to use internally defined errno's (otherwise system provided ones)
#ifndef MICROPY_USE_INTERNAL_ERRNO
#define MICROPY_USE_INTERNAL_ERRNO (0)
#endif

// Support for user-space VFS mount (selected ports)
#ifndef MICROPY_FSUSERMOUNT
#define MICROPY_FSUSERMOUNT (0)
#endif

/*****************************************************************************/
/* Fine control over Python builtins, classes, modules, etc                  */

// Whether to implement attributes on functions
#ifndef MICROPY_PY_FUNCTION_ATTRS
#define MICROPY_PY_FUNCTION_ATTRS (0)
#endif

// Whether to support descriptors (__get__ and __set__)
// This costs some code size and makes all load attrs and store attrs slow
#ifndef MICROPY_PY_DESCRIPTORS
#define MICROPY_PY_DESCRIPTORS (0)
#endif

// Support for async/await/async for/async with
#ifndef MICROPY_PY_ASYNC_AWAIT
#define MICROPY_PY_ASYNC_AWAIT (1)
#endif

// Whether str object is proper unicode
#ifndef MICROPY_PY_BUILTINS_STR_UNICODE
#define MICROPY_PY_BUILTINS_STR_UNICODE (0)
#endif

// Whether str.center() method provided
#ifndef MICROPY_PY_BUILTINS_STR_CENTER
#define MICROPY_PY_BUILTINS_STR_CENTER (0)
#endif

// Whether str.splitlines() method provided
#ifndef MICROPY_PY_BUILTINS_STR_SPLITLINES
#define MICROPY_PY_BUILTINS_STR_SPLITLINES (0)
#endif

// Whether to support bytearray object
#ifndef MICROPY_PY_BUILTINS_BYTEARRAY
#define MICROPY_PY_BUILTINS_BYTEARRAY (1)
#endif

// Whether to support memoryview object
#ifndef MICROPY_PY_BUILTINS_MEMORYVIEW
#define MICROPY_PY_BUILTINS_MEMORYVIEW (0)
#endif

// Whether to support set object
#ifndef MICROPY_PY_BUILTINS_SET
#define MICROPY_PY_BUILTINS_SET (1)
#endif

// Whether to support slice subscript operators and slice object
#ifndef MICROPY_PY_BUILTINS_SLICE
#define MICROPY_PY_BUILTINS_SLICE (1)
#endif

// Whether to support slice attribute read access,
// i.e. slice.start, slice.stop, slice.step
#ifndef MICROPY_PY_BUILTINS_SLICE_ATTRS
#define MICROPY_PY_BUILTINS_SLICE_ATTRS (0)
#endif

// Whether to support frozenset object
#ifndef MICROPY_PY_BUILTINS_FROZENSET
#define MICROPY_PY_BUILTINS_FROZENSET (0)
#endif

// Whether to support property object
#ifndef MICROPY_PY_BUILTINS_PROPERTY
#define MICROPY_PY_BUILTINS_PROPERTY (1)
#endif

// Whether to implement the start/stop/step attributes (readback) on
// the "range" builtin type. Rarely used, and costs ~60 bytes (x86).
#ifndef MICROPY_PY_BUILTINS_RANGE_ATTRS
#define MICROPY_PY_BUILTINS_RANGE_ATTRS (1)
#endif

// Whether to support timeout exceptions (like socket.timeout)
#ifndef MICROPY_PY_BUILTINS_TIMEOUTERROR
#define MICROPY_PY_BUILTINS_TIMEOUTERROR (0)
#endif

// Whether to support complete set of special methods
// for user classes, otherwise only the most used
#ifndef MICROPY_PY_ALL_SPECIAL_METHODS
#define MICROPY_PY_ALL_SPECIAL_METHODS (0)
#endif

// Whether to support compile function
#ifndef MICROPY_PY_BUILTINS_COMPILE
#define MICROPY_PY_BUILTINS_COMPILE (0)
#endif

// Whether to support enumerate function(type)
#ifndef MICROPY_PY_BUILTINS_ENUMERATE
#define MICROPY_PY_BUILTINS_ENUMERATE (1)
#endif

// Whether to support eval and exec functions
// By default they are supported if the compiler is enabled
#ifndef MICROPY_PY_BUILTINS_EVAL_EXEC
#define MICROPY_PY_BUILTINS_EVAL_EXEC (MICROPY_ENABLE_COMPILER)
#endif

// Whether to support the Python 2 execfile function
#ifndef MICROPY_PY_BUILTINS_EXECFILE
#define MICROPY_PY_BUILTINS_EXECFILE (0)
#endif

// Whether to support filter function(type)
#ifndef MICROPY_PY_BUILTINS_FILTER
#define MICROPY_PY_BUILTINS_FILTER (1)
#endif

// Whether to support reversed function(type)
#ifndef MICROPY_PY_BUILTINS_REVERSED
#define MICROPY_PY_BUILTINS_REVERSED (1)
#endif

// Whether to define "NotImplemented" special constant
#ifndef MICROPY_PY_BUILTINS_NOTIMPLEMENTED
#define MICROPY_PY_BUILTINS_NOTIMPLEMENTED (0)
#endif

// Whether to support min/max functions
#ifndef MICROPY_PY_BUILTINS_MIN_MAX
#define MICROPY_PY_BUILTINS_MIN_MAX (1)
#endif

// Whether to set __file__ for imported modules
#ifndef MICROPY_PY___FILE__
#define MICROPY_PY___FILE__ (1)
#endif

// Whether to provide mem-info related functions in micropython module
#ifndef MICROPY_PY_MICROPYTHON_MEM_INFO
#define MICROPY_PY_MICROPYTHON_MEM_INFO (0)
#endif

// Whether to provide "array" module. Note that large chunk of the
// underlying code is shared with "bytearray" builtin type, so to
// get real savings, it should be disabled too.
#ifndef MICROPY_PY_ARRAY
#define MICROPY_PY_ARRAY (1)
#endif

// Whether to support slice assignments for array (and bytearray).
// This is rarely used, but adds ~0.5K of code.
#ifndef MICROPY_PY_ARRAY_SLICE_ASSIGN
#define MICROPY_PY_ARRAY_SLICE_ASSIGN (0)
#endif

// Whether to support attrtuple type (MicroPython extension)
// It provides space-efficient tuples with attribute access
#ifndef MICROPY_PY_ATTRTUPLE
#define MICROPY_PY_ATTRTUPLE (1)
#endif

// Whether to provide "collections" module
#ifndef MICROPY_PY_COLLECTIONS
#define MICROPY_PY_COLLECTIONS (1)
#endif

// Whether to provide "collections.OrderedDict" type
#ifndef MICROPY_PY_COLLECTIONS_ORDEREDDICT
#define MICROPY_PY_COLLECTIONS_ORDEREDDICT (0)
#endif

// Whether to provide "math" module
#ifndef MICROPY_PY_MATH
#define MICROPY_PY_MATH (1)
#endif

// Whether to provide special math functions: math.{erf,erfc,gamma,lgamma}
#ifndef MICROPY_PY_MATH_SPECIAL_FUNCTIONS
#define MICROPY_PY_MATH_SPECIAL_FUNCTIONS (0)
#endif

// Whether to provide "cmath" module
#ifndef MICROPY_PY_CMATH
#define MICROPY_PY_CMATH (0)
#endif

// Whether to provide "gc" module
#ifndef MICROPY_PY_GC
#define MICROPY_PY_GC (1)
#endif

// Whether to return number of collected objects from gc.collect()
#ifndef MICROPY_PY_GC_COLLECT_RETVAL
#define MICROPY_PY_GC_COLLECT_RETVAL (0)
#endif

// Whether to provide "io" module
#ifndef MICROPY_PY_IO
#define MICROPY_PY_IO (1)
#endif

// Whether to provide "io.FileIO" class
#ifndef MICROPY_PY_IO_FILEIO
#define MICROPY_PY_IO_FILEIO (0)
#endif

// Whether to provide "io.BytesIO" class
#ifndef MICROPY_PY_IO_BYTESIO
#define MICROPY_PY_IO_BYTESIO (1)
#endif

// Whether to provide "io.BufferedWriter" class
#ifndef MICROPY_PY_IO_BUFFEREDWRITER
#define MICROPY_PY_IO_BUFFEREDWRITER (0)
#endif

// Whether to provide "struct" module
#ifndef MICROPY_PY_STRUCT
#define MICROPY_PY_STRUCT (1)
#endif

// Whether to provide "sys" module
#ifndef MICROPY_PY_SYS
#define MICROPY_PY_SYS (1)
#endif

// Whether to provide "sys.maxsize" constant
#ifndef MICROPY_PY_SYS_MAXSIZE
#define MICROPY_PY_SYS_MAXSIZE (0)
#endif

// Whether to provide "sys.modules" dictionary
#ifndef MICROPY_PY_SYS_MODULES
#define MICROPY_PY_SYS_MODULES (1)
#endif

// Whether to provide "sys.exc_info" function
// Avoid enabling this, this function is Python2 heritage
#ifndef MICROPY_PY_SYS_EXC_INFO
#define MICROPY_PY_SYS_EXC_INFO (0)
#endif

// Whether to provide "sys.exit" function
#ifndef MICROPY_PY_SYS_EXIT
#define MICROPY_PY_SYS_EXIT (0)
#endif

// Whether to provide sys.{stdin,stdout,stderr} objects
#ifndef MICROPY_PY_SYS_STDFILES
#define MICROPY_PY_SYS_STDFILES (0)
#endif

// Whether to provide sys.{stdin,stdout,stderr}.buffer object
// This is implemented per-port
#ifndef MICROPY_PY_SYS_STDIO_BUFFER
#define MICROPY_PY_SYS_STDIO_BUFFER (0)
#endif

// Whether to provide "uerrno" module
#ifndef MICROPY_PY_UERRNO
#define MICROPY_PY_UERRNO (0)
#endif

// Extended modules

#ifndef MICROPY_PY_UCTYPES
#define MICROPY_PY_UCTYPES (0)
#endif

#ifndef MICROPY_PY_UZLIB
#define MICROPY_PY_UZLIB (0)
#endif

#ifndef MICROPY_PY_UJSON
#define MICROPY_PY_UJSON (0)
#endif

#ifndef MICROPY_PY_URE
#define MICROPY_PY_URE (0)
#endif

#ifndef MICROPY_PY_UHEAPQ
#define MICROPY_PY_UHEAPQ (0)
#endif

#ifndef MICROPY_PY_UHASHLIB
#define MICROPY_PY_UHASHLIB (0)
#endif

#ifndef MICROPY_PY_UBINASCII
#define MICROPY_PY_UBINASCII (0)
#endif

#ifndef MICROPY_PY_URANDOM
#define MICROPY_PY_URANDOM (0)
#endif

// Whether to include: randrange, randint, choice, random, uniform
#ifndef MICROPY_PY_URANDOM_EXTRA_FUNCS
#define MICROPY_PY_URANDOM_EXTRA_FUNCS (0)
#endif

#ifndef MICROPY_PY_MACHINE
#define MICROPY_PY_MACHINE (0)
#endif

// Whether to include: time_pulse_us
#ifndef MICROPY_PY_MACHINE_PULSE
#define MICROPY_PY_MACHINE_PULSE (0)
#endif

#ifndef MICROPY_PY_MACHINE_I2C
#define MICROPY_PY_MACHINE_I2C (0)
#endif

#ifndef MICROPY_PY_USSL
#define MICROPY_PY_USSL (0)
#endif

#ifndef MICROPY_PY_WEBSOCKET
#define MICROPY_PY_WEBSOCKET (0)
#endif

#ifndef MICROPY_PY_FRAMEBUF
#define MICROPY_PY_FRAMEBUF (0)
#endif

/*****************************************************************************/
/* Hooks for a port to add builtins                                          */

// Additional builtin function definitions - see builtintables.c:builtin_object_table for format.
#ifndef MICROPY_PORT_BUILTINS
#define MICROPY_PORT_BUILTINS
#endif

// Additional builtin module definitions - see builtintables.c:builtin_module_table for format.
#ifndef MICROPY_PORT_BUILTIN_MODULES
#define MICROPY_PORT_BUILTIN_MODULES
#endif

// Any module weak links - see builtintables.c:mp_builtin_module_weak_links_table.
#ifndef MICROPY_PORT_BUILTIN_MODULE_WEAK_LINKS
#define MICROPY_PORT_BUILTIN_MODULE_WEAK_LINKS
#endif

// Additional constant definitions for the compiler - see compile.c:mp_constants_table.
#ifndef MICROPY_PORT_CONSTANTS
#define MICROPY_PORT_CONSTANTS
#endif

// Any root pointers for GC scanning - see mpstate.c
#ifndef MICROPY_PORT_ROOT_POINTERS
#define MICROPY_PORT_ROOT_POINTERS
#endif

/*****************************************************************************/
/* Miscellaneous settings                                                    */

// All uPy objects in ROM must be aligned on at least a 4 byte boundary
// so that the small-int/qstr/pointer distinction can be made.  For machines
// that don't do this (eg 16-bit CPU), define the following macro to something
// like __attribute__((aligned(4))).
#ifndef MICROPY_OBJ_BASE_ALIGNMENT
#define MICROPY_OBJ_BASE_ALIGNMENT
#endif

// On embedded platforms, these will typically enable/disable irqs.
#ifndef MICROPY_BEGIN_ATOMIC_SECTION
#define MICROPY_BEGIN_ATOMIC_SECTION() (0)
#endif
#ifndef MICROPY_END_ATOMIC_SECTION
#define MICROPY_END_ATOMIC_SECTION(state) (void)(state)
#endif

// Allow to override static modifier for global objects, e.g. to use with
// object code analysis tools which don't support static symbols.
#ifndef STATIC
#define STATIC static
#endif

#define BITS_PER_BYTE (8)
#define BITS_PER_WORD (BITS_PER_BYTE * BYTES_PER_WORD)
// mp_int_t value with most significant bit set
#define WORD_MSBIT_HIGH (((mp_uint_t)1) << (BYTES_PER_WORD * 8 - 1))

// Make sure both MP_ENDIANNESS_LITTLE and MP_ENDIANNESS_BIG are
// defined and that they are the opposite of each other.
#if defined(MP_ENDIANNESS_LITTLE)
#define MP_ENDIANNESS_BIG (!MP_ENDIANNESS_LITTLE)
#elif defined(MP_ENDIANNESS_BIG)
#define MP_ENDIANNESS_LITTLE (!MP_ENDIANNESS_BIG)
#else
  // Endiannes not defined by port so try to autodetect it.
  #if defined(__BYTE_ORDER__)
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
      #define MP_ENDIANNESS_LITTLE (1)
    #else
      #define MP_ENDIANNESS_LITTLE (0)
    #endif
  #elif defined(__LITTLE_ENDIAN__) || defined(__LITTLE_ENDIAN) || defined (_LITTLE_ENDIAN)
    #define MP_ENDIANNESS_LITTLE (1)
  #elif defined(__BIG_ENDIAN__) || defined(__BIG_ENDIAN) || defined (_BIG_ENDIAN)
    #define MP_ENDIANNESS_LITTLE (0)
  #else
    #include <endian.h>
      #if defined(__BYTE_ORDER)
        #if __BYTE_ORDER == __LITTLE_ENDIAN
          #define MP_ENDIANNESS_LITTLE (1)
        #else
          #define MP_ENDIANNESS_LITTLE (0)
        #endif
      #else
        #error endianness not defined and cannot detect it
      #endif
  #endif
  #define MP_ENDIANNESS_BIG (!MP_ENDIANNESS_LITTLE)
#endif

// Make a pointer to RAM callable (eg set lower bit for Thumb code)
// (This scheme won't work if we want to mix Thumb and normal ARM code.)
#ifndef MICROPY_MAKE_POINTER_CALLABLE
#define MICROPY_MAKE_POINTER_CALLABLE(p) (p)
#endif

// If these MP_PLAT_*_EXEC macros are overridden then the memory allocated by them
// must be somehow reachable for marking by the GC, since the native code
// generators store pointers to GC managed memory in the code.
#ifndef MP_PLAT_ALLOC_EXEC
#define MP_PLAT_ALLOC_EXEC(min_size, ptr, size) do { *ptr = micropy_m_new(mp_state, byte, min_size); *size = min_size; } while (0)
#endif

#ifndef MP_PLAT_FREE_EXEC
#define MP_PLAT_FREE_EXEC(ptr, size) micropy_m_del(mp_state, byte, ptr, size)
#endif

// This macro is used to do all output (except when MICROPY_PY_IO is defined)
#ifndef MP_PLAT_PRINT_STRN
#define MP_PLAT_PRINT_STRN(str, len) micropy_hal_stdout_tx_strn_cooked(mp_state, str, len)
#endif

#ifndef MP_SSIZE_MAX
#define MP_SSIZE_MAX SSIZE_MAX
#endif

// printf format spec to use for mp_int_t and friends
#ifndef INT_FMT
#if defined(__LP64__)
// Archs where mp_int_t == long, long != int
#define UINT_FMT "%lu"
#define INT_FMT "%ld"
#elif defined(_WIN64)
#define UINT_FMT "%llu"
#define INT_FMT "%lld"
#else
// Archs where mp_int_t == int
#define UINT_FMT "%u"
#define INT_FMT "%d"
#endif
#endif //INT_FMT

// Modifier for function which doesn't return
#ifndef NORETURN
#define NORETURN __attribute__((noreturn))
#endif

// Modifier for weak functions
#ifndef MP_WEAK
#define MP_WEAK __attribute__((weak))
#endif

// Condition is likely to be true, to help branch prediction
#ifndef MP_LIKELY
#define MP_LIKELY(x) __builtin_expect((x), 1)
#endif

// Condition is likely to be false, to help branch prediction
#ifndef MP_UNLIKELY
#define MP_UNLIKELY(x) __builtin_expect((x), 0)
#endif

#endif // __MICROPY_INCLUDED_PY_MPCONFIG_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_MISC_H__
#define __MICROPY_INCLUDED_PY_MISC_H__

// a mini library of useful types and functions

/** types *******************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef unsigned char byte;
struct _mp_state_ctx_t;
typedef unsigned int uint;

/** generic ops *************************************************/

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif
#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

/** memomry allocation ******************************************/

// TODO make a lazy m_renew that can increase by a smaller amount than requested (but by at least 1 more element)

#define micropy_m_new(mp_state, type, num) ((type*)(micropy_m_malloc(mp_state, sizeof(type) * (num))))
#define micropy_m_new_maybe(mp_state, type, num) ((type*)(micropy_m_malloc_maybe(mp_state, sizeof(type) * (num))))
#define micropy_m_new0(mp_state, type, num) ((type*)(micropy_m_malloc0(mp_state, sizeof(type) * (num))))
#define micropy_m_new_obj(mp_state, type) (micropy_m_new(mp_state, type, 1))
#define micropy_m_new_obj_maybe(mp_state, type) (micropy_m_new_maybe(mp_state, type, 1))
#define micropy_m_new_obj_var(mp_state, obj_type, var_type, var_num) ((obj_type*)micropy_m_malloc(mp_state, sizeof(obj_type) + sizeof(var_type) * (var_num)))
#define micropy_m_new_obj_var_maybe(mp_state, obj_type, var_type, var_num) ((obj_type*)micropy_m_malloc_maybe(mp_state, sizeof(obj_type) + sizeof(var_type) * (var_num)))
#if MICROPY_ENABLE_FINALISER
#define micropy_m_new_obj_with_finaliser(mp_state, type) ((type*)(micropy_m_malloc_with_finaliser(mp_state, sizeof(type))))
#else
#define micropy_m_new_obj_with_finaliser(mp_state, type) micropy_m_new_obj(mp_state, type)
#endif
#if MICROPY_MALLOC_USES_ALLOCATED_SIZE
#define micropy_m_renew(mp_state, type, ptr, old_num, new_num) ((type*)(micropy_m_realloc(mp_state, (ptr), sizeof(type) * (old_num), sizeof(type) * (new_num))))
#define micropy_m_renew_maybe(mp_state, type, ptr, old_num, new_num, allow_move) ((type*)(micropy_m_realloc_maybe(mp_state, (ptr), sizeof(type) * (old_num), sizeof(type) * (new_num), (allow_move))))
#define micropy_m_del(mp_state, type, ptr, num) micropy_m_free(mp_state, ptr, sizeof(type) * (num))
#define micropy_m_del_var(mp_state, obj_type, var_type, var_num, ptr) (micropy_m_free(mp_state, ptr, sizeof(obj_type) + sizeof(var_type) * (var_num)))
#else
#define micropy_m_renew(mp_state, type, ptr, old_num, new_num) ((type*)(micropy_m_realloc(mp_state, (ptr), sizeof(type) * (new_num))))
#define micropy_m_renew_maybe(mp_state, type, ptr, old_num, new_num, allow_move) ((type*)(micropy_m_realloc_maybe(mp_state, (ptr), sizeof(type) * (new_num), (allow_move))))
#define micropy_m_del(mp_state, type, ptr, num) ((void)(num), micropy_m_free(mp_state, ptr))
#define micropy_m_del_var(mp_state, obj_type, var_type, var_num, ptr) ((void)(var_num), micropy_m_free(mp_state, ptr))
#endif
#define micropy_m_del_obj(mp_state, type, ptr) (micropy_m_del(mp_state, type, ptr, 1))

void *micropy_m_malloc(struct _mp_state_ctx_t *mp_state, size_t num_bytes);
void *micropy_m_malloc_maybe(struct _mp_state_ctx_t *mp_state, size_t num_bytes);
void *micropy_m_malloc_with_finaliser(struct _mp_state_ctx_t *mp_state, size_t num_bytes);
void *micropy_m_malloc0(struct _mp_state_ctx_t *mp_state, size_t num_bytes);
#if MICROPY_MALLOC_USES_ALLOCATED_SIZE
void *micropy_m_realloc(struct _mp_state_ctx_t *mp_state, void *ptr, size_t old_num_bytes, size_t new_num_bytes);
void *micropy_m_realloc_maybe(struct _mp_state_ctx_t *mp_state, void *ptr, size_t old_num_bytes, size_t new_num_bytes, bool allow_move);
void micropy_m_free(struct _mp_state_ctx_t *mp_state, void *ptr, size_t num_bytes);
#else
void *micropy_m_realloc(struct _mp_state_ctx_t *mp_state, void *ptr, size_t new_num_bytes);
void *micropy_m_realloc_maybe(struct _mp_state_ctx_t *mp_state, void *ptr, size_t new_num_bytes, bool allow_move);
void micropy_m_free(struct _mp_state_ctx_t *mp_state, void *ptr);
#endif
void *micropy_m_malloc_fail(struct _mp_state_ctx_t *mp_state, size_t num_bytes);

#if MICROPY_MEM_STATS
size_t micropy_m_get_total_bytes_allocated(struct _mp_state_ctx_t *mp_state);
size_t micropy_m_get_current_bytes_allocated(struct _mp_state_ctx_t *mp_state);
size_t micropy_m_get_peak_bytes_allocated(struct _mp_state_ctx_t *mp_state);
#endif

/** array helpers ***********************************************/

// get the number of elements in a fixed-size array
#define MP_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

// align ptr to the nearest multiple of "alignment"
#define MP_ALIGN(ptr, alignment) (void*)(((uintptr_t)(ptr) + ((alignment) - 1)) & ~((alignment) - 1))

/** unichar / UTF-8 *********************************************/

#if MICROPY_PY_BUILTINS_STR_UNICODE
// with unicode enabled we need a type which can fit chars up to 0x10ffff
typedef uint32_t unichar;
#else
// without unicode enabled we can only need to fit chars up to 0xff
// (on 16-bit archs uint is 16-bits and more efficient than uint32_t)
typedef uint unichar;
#endif

unichar utf8_get_char(const byte *s);
const byte *utf8_next_char(const byte *s);

bool unichar_isspace(unichar c);
bool unichar_isalpha(unichar c);
bool unichar_isprint(unichar c);
bool unichar_isdigit(unichar c);
bool unichar_isxdigit(unichar c);
bool unichar_isident(unichar c);
bool unichar_isupper(unichar c);
bool unichar_islower(unichar c);
unichar unichar_tolower(unichar c);
unichar unichar_toupper(unichar c);
mp_uint_t unichar_xdigit_value(unichar c);
mp_uint_t unichar_charlen(const char *str, mp_uint_t len);
#define UTF8_IS_NONASCII(ch) ((ch) & 0x80)
#define UTF8_IS_CONT(ch) (((ch) & 0xC0) == 0x80)

/** variable string *********************************************/

typedef struct _vstr_t {
    size_t alloc;
    size_t len;
    char *buf;
    bool had_error : 1;
    bool fixed_buf : 1;
} vstr_t;

// convenience macro to declare a vstr with a fixed size buffer on the stack
#define VSTR_FIXED(vstr, alloc) vstr_t vstr; char vstr##_buf[(alloc)]; micropy_vstr_init_fixed_buf(mp_state, &vstr, (alloc), vstr##_buf);

void micropy_vstr_init(struct _mp_state_ctx_t *mp_state, vstr_t *vstr, size_t alloc);
void micropy_vstr_init_len(struct _mp_state_ctx_t *mp_state, vstr_t *vstr, size_t len);
void micropy_vstr_init_fixed_buf(struct _mp_state_ctx_t *mp_state, vstr_t *vstr, size_t alloc, char *buf);
struct _mp_print_t;
void micropy_vstr_init_print(struct _mp_state_ctx_t *mp_state, vstr_t *vstr, size_t alloc, struct _mp_print_t *print);
void micropy_vstr_clear(struct _mp_state_ctx_t *mp_state, vstr_t *vstr);
vstr_t *micropy_vstr_new(struct _mp_state_ctx_t *mp_state);
vstr_t *micropy_vstr_new_size(struct _mp_state_ctx_t *mp_state, size_t alloc);
void micropy_vstr_free(struct _mp_state_ctx_t *mp_state, vstr_t *vstr);
void micropy_vstr_reset(struct _mp_state_ctx_t *mp_state, vstr_t *vstr);
bool micropy_vstr_had_error(struct _mp_state_ctx_t *mp_state, vstr_t *vstr);
char *micropy_vstr_str(struct _mp_state_ctx_t *mp_state, vstr_t *vstr);
size_t micropy_vstr_len(struct _mp_state_ctx_t *mp_state, vstr_t *vstr);
void micropy_vstr_hint_size(struct _mp_state_ctx_t *mp_state, vstr_t *vstr, size_t size);
char *micropy_vstr_extend(struct _mp_state_ctx_t *mp_state, vstr_t *vstr, size_t size);
char *micropy_vstr_add_len(struct _mp_state_ctx_t *mp_state, vstr_t *vstr, size_t len);
char *micropy_vstr_null_terminated_str(struct _mp_state_ctx_t *mp_state, vstr_t *vstr);
void micropy_vstr_add_byte(struct _mp_state_ctx_t *mp_state, vstr_t *vstr, byte v);
void micropy_vstr_add_char(struct _mp_state_ctx_t *mp_state, vstr_t *vstr, unichar chr);
void micropy_vstr_add_str(struct _mp_state_ctx_t *mp_state, vstr_t *vstr, const char *str);
void micropy_vstr_add_strn(struct _mp_state_ctx_t *mp_state, vstr_t *vstr, const char *str, size_t len);
void micropy_vstr_ins_byte(struct _mp_state_ctx_t *mp_state, vstr_t *vstr, size_t byte_pos, byte b);
void micropy_vstr_ins_char(struct _mp_state_ctx_t *mp_state, vstr_t *vstr, size_t char_pos, unichar chr);
void micropy_vstr_cut_head_bytes(struct _mp_state_ctx_t *mp_state, vstr_t *vstr, size_t bytes_to_cut);
void micropy_vstr_cut_tail_bytes(struct _mp_state_ctx_t *mp_state, vstr_t *vstr, size_t bytes_to_cut);
void micropy_vstr_cut_out_bytes(struct _mp_state_ctx_t *mp_state, vstr_t *vstr, size_t byte_pos, size_t bytes_to_cut);
void micropy_vstr_printf(struct _mp_state_ctx_t *mp_state, vstr_t *vstr, const char *fmt, ...);

/** non-dynamic size-bounded variable buffer/string *************/

#define CHECKBUF(buf, max_size) char buf[max_size + 1]; size_t buf##_len = max_size; char *buf##_p = buf;
#define CHECKBUF_RESET(buf, max_size) buf##_len = max_size; buf##_p = buf;
#define CHECKBUF_APPEND(buf, src, src_len) \
        { size_t l = MIN(src_len, buf##_len); \
        memcpy(buf##_p, src, l); \
        buf##_len -= l; \
        buf##_p += l; }
#define CHECKBUF_APPEND_0(buf) { *buf##_p = 0; }
#define CHECKBUF_LEN(buf) (buf##_p - buf)

#ifdef va_start
void micropy_vstr_vprintf(struct _mp_state_ctx_t *mp_state, vstr_t *vstr, const char *fmt, va_list ap);
#endif

// Debugging helpers
int DEBUG_printf(const char *fmt, ...);

extern mp_uint_t mp_verbose_flag;

// This is useful for unicode handling. Some CPU archs has
// special instructions for efficient implentation of this
// function (e.g. CLZ on ARM).
// NOTE: this function is unused at the moment
#ifndef count_lead_ones
static inline mp_uint_t micropy_count_lead_ones(struct _mp_state_ctx_t *mp_state, byte val) {
    mp_uint_t c = 0;
    for (byte mask = 0x80; val & mask; mask >>= 1) {
        c++;
    }
    return c;
}
#endif

/** float internals *************/

#if MICROPY_PY_BUILTINS_FLOAT
#if MICROPY_FLOAT_IMPL == MICROPY_FLOAT_IMPL_DOUBLE
#define MP_FLOAT_EXP_BITS (11)
#define MP_FLOAT_FRAC_BITS (52)
#elif MICROPY_FLOAT_IMPL == MICROPY_FLOAT_IMPL_FLOAT
#define MP_FLOAT_EXP_BITS (8)
#define MP_FLOAT_FRAC_BITS (23)
#endif
#define MP_FLOAT_EXP_BIAS ((1 << (MP_FLOAT_EXP_BITS - 1)) - 1)
#endif // MICROPY_PY_BUILTINS_FLOAT

#endif // __MICROPY_INCLUDED_PY_MISC_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_MPPRINT_H__
#define __MICROPY_INCLUDED_PY_MPPRINT_H__

//#include "py/mpconfig.h"

#define PF_FLAG_LEFT_ADJUST       (0x001)
#define PF_FLAG_SHOW_SIGN         (0x002)
#define PF_FLAG_SPACE_SIGN        (0x004)
#define PF_FLAG_NO_TRAILZ         (0x008)
#define PF_FLAG_SHOW_PREFIX       (0x010)
#define PF_FLAG_SHOW_COMMA        (0x020)
#define PF_FLAG_PAD_AFTER_SIGN    (0x040)
#define PF_FLAG_CENTER_ADJUST     (0x080)
#define PF_FLAG_ADD_PERCENT       (0x100)
#define PF_FLAG_SHOW_OCTAL_LETTER (0x200)

typedef void (*mp_print_strn_t)(struct _mp_state_ctx_t *mp_state, void *data, const char *str, size_t len);
struct _mp_state_ctx_t;

typedef struct _mp_print_t {
    void *data;
    mp_print_strn_t print_strn;
} mp_print_t;

// All (non-debug) prints go through one of the two interfaces below.
// 1) Wrapper for platform print function, which wraps MP_PLAT_PRINT_STRN.
extern const mp_print_t mp_plat_print;
#if MICROPY_PY_IO
// 2) Wrapper for printing to sys.stdout.
extern const mp_print_t mp_sys_stdout_print;
#endif

int micropy_print_str(struct _mp_state_ctx_t *mp_state, const mp_print_t *print, const char *str);
int micropy_print_strn(struct _mp_state_ctx_t *mp_state, const mp_print_t *print, const char *str, size_t len, int flags, char fill, int width);
#if MICROPY_PY_BUILTINS_FLOAT
int micropy_print_float(struct _mp_state_ctx_t *mp_state, const mp_print_t *print, mp_float_t f, char fmt, int flags, char fill, int width, int prec);
#endif

int micropy_printf(struct _mp_state_ctx_t *mp_state, const mp_print_t *print, const char *fmt, ...);
#ifdef va_start
int micropy_vprintf(struct _mp_state_ctx_t *mp_state, const mp_print_t *print, const char *fmt, va_list args);
#endif

#endif // __MICROPY_INCLUDED_PY_MPPRINT_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_NLR_H__
#define __MICROPY_INCLUDED_PY_NLR_H__

// non-local return
// exception handling, basically a stack of setjmp/longjmp buffers

#include <limits.h>
#include <setjmp.h>
#include <assert.h>

//#include "py/mpconfig.h"

typedef struct _nlr_buf_t nlr_buf_t;
struct _nlr_buf_t {
    // the entries here must all be machine word size
    nlr_buf_t *prev;
    void *ret_val; // always a concrete object (an exception instance)
#if !defined(MICROPY_NLR_SETJMP) || !MICROPY_NLR_SETJMP
#if defined(__i386__)
    void *regs[6];
#elif defined(__x86_64__)
  #if defined(__CYGWIN__)
    void *regs[12];
  #else
    void *regs[8];
  #endif
#elif defined(__thumb2__) || defined(__thumb__) || defined(__arm__)
    void *regs[10];
#elif defined(__xtensa__)
    void *regs[10];
#else
    #define MICROPY_NLR_SETJMP (1)
    //#warning "No native NLR support for this arch, using setjmp implementation"
#endif
#endif

#if MICROPY_NLR_SETJMP
    jmp_buf jmpbuf;
#endif
};

#if MICROPY_NLR_SETJMP
//#include "py/mpstate.h"

NORETURN void micropy_nlr_setjmp_jump(struct _mp_state_ctx_t *mp_state, void *val);
// nlr_push() must be defined as a macro, because "The stack context will be
// invalidated if the function which called setjmp() returns."
#define micropy_nlr_push(mp_state, buf) ((buf)->prev = (mp_state)->vm.nlr_top, (mp_state)->vm.nlr_top = (buf), setjmp((buf)->jmpbuf))
#define micropy_nlr_pop(mp_state) { (mp_state)->vm.nlr_top = (mp_state)->vm.nlr_top->prev; }
#define micropy_nlr_jump(mp_state, val) micropy_nlr_setjmp_jump(mp_state, val)
#else
unsigned int micropy_nlr_push(struct _mp_state_ctx_t *mp_state, nlr_buf_t *);
void micropy_nlr_pop(struct _mp_state_ctx_t *mp_state);
NORETURN void micropy_nlr_jump(struct _mp_state_ctx_t *mp_state, void *val);
#endif

// This must be implemented by a port.  It's called by nlr_jump
// if no nlr buf has been pushed.  It must not return, but rather
// should bail out with a fatal error.
void micropy_nlr_jump_fail(struct _mp_state_ctx_t *mp_state, void *val);

// use nlr_raise instead of nlr_jump so that debugging is easier
#ifndef DEBUG
#define micropy_nlr_raise(mp_state, val) micropy_nlr_jump(mp_state, MP_OBJ_TO_PTR(val))
#else
#include "mpstate.h"
#define micropy_nlr_raise(mp_state, val) \
    do { \
        /*printf("nlr_raise: nlr_top=%p\n", (mp_state)->vm.nlr_top); \
        micropy_fflush(mp_state, stdout);*/ \
        void *_val = MP_OBJ_TO_PTR(val); \
        assert(_val != NULL); \
        assert(micropy_obj_is_exception_instance(mp_state, val)); \
        micropy_nlr_jump(mp_state, _val); \
    } while (0)

#if !MICROPY_NLR_SETJMP
#define micropy_nlr_push(mp_state, val) \
    assert((mp_state)->vm.nlr_top != val),nlr_push(val)

/*
#define micropy_nlr_push(mp_state, val) \
    printf("nlr_push: before: nlr_top=%p, val=%p\n", (mp_state)->vm.nlr_top, val),assert((mp_state)->vm.nlr_top != val),nlr_push(val)
#endif
*/
#endif

#endif

#endif // __MICROPY_INCLUDED_PY_NLR_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_QSTR_H__
#define __MICROPY_INCLUDED_PY_QSTR_H__

//#include "py/mpconfig.h"
//#include "py/misc.h"

// See qstrdefs.h for a list of qstr's that are available as constants.
// Reference them as MP_QSTR_xxxx.
//
// Note: it would be possible to define MP_QSTR_xxx as qstr_from_str_static("xxx")
// for qstrs that are referenced this way, but you don't want to have them in ROM.

// first entry in enum will be MP_QSTR_NULL=0, which indicates invalid/no qstr
enum {
#ifndef __QSTR_EXTRACT
#define QDEF(id, str) id,
// This file was automatically generated by makeqstrdata.py

QDEF(MP_QSTR_NULL, (const byte*)"\x00\x00\x00" "")
QDEF(MP_QSTR_, (const byte*)"\x05\x15\x00" "")
QDEF(MP_QSTR__star_, (const byte*)"\x8f\xb5\x01" "*")
QDEF(MP_QSTR__, (const byte*)"\xfa\xb5\x01" "_")
QDEF(MP_QSTR__percent__hash_o, (const byte*)"\x6c\x1a\x03" "%#o")
QDEF(MP_QSTR__percent__hash_x, (const byte*)"\x7b\x1a\x03" "%#x")
QDEF(MP_QSTR__brace_open__colon__hash_b_brace_close_, (const byte*)"\x58\x37\x05" "{:#b}")
QDEF(MP_QSTR__0x0a_, (const byte*)"\xaf\xb5\x01" "\x0a")
QDEF(MP_QSTR_maximum_space_recursion_space_depth_space_exceeded, (const byte*)"\x73\x1e\x20" "maximum recursion depth exceeded")
QDEF(MP_QSTR__lt_module_gt_, (const byte*)"\xbd\x94\x08" "<module>")
QDEF(MP_QSTR__lt_lambda_gt_, (const byte*)"\x80\x8c\x08" "<lambda>")
QDEF(MP_QSTR__lt_listcomp_gt_, (const byte*)"\xd4\x15\x0a" "<listcomp>")
QDEF(MP_QSTR__lt_dictcomp_gt_, (const byte*)"\xcc\x8d\x0a" "<dictcomp>")
QDEF(MP_QSTR__lt_setcomp_gt_, (const byte*)"\x54\x51\x09" "<setcomp>")
QDEF(MP_QSTR__lt_genexpr_gt_, (const byte*)"\x34\x6a\x09" "<genexpr>")
QDEF(MP_QSTR__lt_string_gt_, (const byte*)"\x52\x53\x08" "<string>")
QDEF(MP_QSTR__lt_stdin_gt_, (const byte*)"\xe3\x63\x07" "<stdin>")
QDEF(MP_QSTR_utf_hyphen_8, (const byte*)"\xb7\x82\x05" "utf-8")
QDEF(MP_QSTR___locals__, (const byte*)"\x7b\x6a\x0a" "__locals__")
QDEF(MP_QSTR_BufferError, (const byte*)"\x1d\x59\x0b" "BufferError")
QDEF(MP_QSTR_FileExistsError, (const byte*)"\x5b\x14\x0f" "FileExistsError")
QDEF(MP_QSTR_FileNotFoundError, (const byte*)"\x78\x89\x11" "FileNotFoundError")
QDEF(MP_QSTR_FloatingPointError, (const byte*)"\x01\x34\x12" "FloatingPointError")
QDEF(MP_QSTR_UnboundLocalError, (const byte*)"\x99\x22\x11" "UnboundLocalError")
QDEF(MP_QSTR_xxxx, (const byte*)"\x05\xcd\x04" "xxxx")
QDEF(MP_QSTR_xxx, (const byte*)"\xdd\x91\x03" "xxx")
QDEF(MP_QSTR_function, (const byte*)"\x27\x02\x08" "function")
QDEF(MP_QSTR___del__, (const byte*)"\x68\x37\x07" "__del__")
QDEF(MP_QSTR_errno, (const byte*)"\xc1\x11\x05" "errno")
QDEF(MP_QSTR_uctypes, (const byte*)"\xf8\x71\x07" "uctypes")
QDEF(MP_QSTR_const, (const byte*)"\xc0\xff\x05" "const")
QDEF(MP_QSTR_micropython, (const byte*)"\x0b\x7c\x0b" "micropython")
QDEF(MP_QSTR_bytecode, (const byte*)"\x22\x7d\x08" "bytecode")
QDEF(MP_QSTR_native, (const byte*)"\x84\x0b\x06" "native")
QDEF(MP_QSTR_viper, (const byte*)"\x5d\x23\x05" "viper")
QDEF(MP_QSTR_asm_thumb, (const byte*)"\x43\x6d\x09" "asm_thumb")
QDEF(MP_QSTR_AssertionError, (const byte*)"\x97\x5a\x0e" "AssertionError")
QDEF(MP_QSTR_range, (const byte*)"\x1a\x5e\x05" "range")
QDEF(MP_QSTR___aiter__, (const byte*)"\x4e\x2b\x09" "__aiter__")
QDEF(MP_QSTR___anext__, (const byte*)"\x83\xb4\x09" "__anext__")
QDEF(MP_QSTR_StopAsyncIteration, (const byte*)"\xec\xf0\x12" "StopAsyncIteration")
QDEF(MP_QSTR___aenter__, (const byte*)"\x4c\x84\x0a" "__aenter__")
QDEF(MP_QSTR___aexit__, (const byte*)"\xc4\xcf\x09" "__aexit__")
QDEF(MP_QSTR___repl_print__, (const byte*)"\x00\xbb\x0e" "__repl_print__")
QDEF(MP_QSTR_super, (const byte*)"\xc4\xb2\x05" "super")
QDEF(MP_QSTR___class__, (const byte*)"\x2b\xc5\x09" "__class__")
QDEF(MP_QSTR___doc__, (const byte*)"\x2d\xac\x07" "__doc__")
QDEF(MP_QSTR___name__, (const byte*)"\xe2\x38\x08" "__name__")
QDEF(MP_QSTR___module__, (const byte*)"\xff\x30\x0a" "__module__")
QDEF(MP_QSTR___qualname__, (const byte*)"\x6b\x00\x0c" "__qualname__")
QDEF(MP_QSTR_object, (const byte*)"\x90\x8d\x06" "object")
QDEF(MP_QSTR_bool, (const byte*)"\xeb\x3c\x04" "bool")
QDEF(MP_QSTR_int, (const byte*)"\x16\x53\x03" "int")
QDEF(MP_QSTR_uint, (const byte*)"\xe3\x3d\x04" "uint")
QDEF(MP_QSTR_label, (const byte*)"\x43\xe7\x05" "label")
QDEF(MP_QSTR_align, (const byte*)"\xa8\xfb\x05" "align")
QDEF(MP_QSTR_data, (const byte*)"\x15\xdc\x04" "data")
QDEF(MP_QSTR___main__, (const byte*)"\x8e\x13\x08" "__main__")
QDEF(MP_QSTR___build_class__, (const byte*)"\x42\x88\x0f" "__build_class__")
QDEF(MP_QSTR_keys, (const byte*)"\x01\x13\x04" "keys")
QDEF(MP_QSTR___getitem__, (const byte*)"\x26\x39\x0b" "__getitem__")
QDEF(MP_QSTR_closure, (const byte*)"\x74\xca\x07" "closure")
QDEF(MP_QSTR_generator, (const byte*)"\x96\xc3\x09" "generator")
QDEF(MP_QSTR___next__, (const byte*)"\x02\x73\x08" "__next__")
QDEF(MP_QSTR_send, (const byte*)"\xb9\x76\x04" "send")
QDEF(MP_QSTR_close, (const byte*)"\x33\x67\x05" "close")
QDEF(MP_QSTR_throw, (const byte*)"\xb3\x44\x05" "throw")
QDEF(MP_QSTR___path__, (const byte*)"\xc8\x23\x08" "__path__")
QDEF(MP_QSTR___file__, (const byte*)"\x03\x54\x08" "__file__")
QDEF(MP_QSTR_code, (const byte*)"\x68\xda\x04" "code")
QDEF(MP_QSTR_single, (const byte*)"\x3f\x20\x06" "single")
QDEF(MP_QSTR_exec, (const byte*)"\x1e\xc0\x04" "exec")
QDEF(MP_QSTR_eval, (const byte*)"\x9b\xa6\x04" "eval")
QDEF(MP_QSTR_tuple, (const byte*)"\xfd\x41\x05" "tuple")
QDEF(MP_QSTR_append, (const byte*)"\x6b\x97\x06" "append")
QDEF(MP_QSTR_extend, (const byte*)"\x63\xe8\x06" "extend")
QDEF(MP_QSTR_array, (const byte*)"\x7c\x72\x05" "array")
QDEF(MP_QSTR_bytearray, (const byte*)"\x76\xa3\x09" "bytearray")
QDEF(MP_QSTR_memoryview, (const byte*)"\x69\x44\x0a" "memoryview")
QDEF(MP_QSTR_iterator, (const byte*)"\x47\xbe\x08" "iterator")
QDEF(MP_QSTR_bound_method, (const byte*)"\x97\xa2\x0c" "bound_method")
QDEF(MP_QSTR_real, (const byte*)"\xbf\xf9\x04" "real")
QDEF(MP_QSTR_imag, (const byte*)"\x47\xb7\x04" "imag")
QDEF(MP_QSTR_complex, (const byte*)"\xc5\x9d\x07" "complex")
QDEF(MP_QSTR_dict_view, (const byte*)"\x2d\xa9\x09" "dict_view")
QDEF(MP_QSTR_clear, (const byte*)"\x7c\xa0\x05" "clear")
QDEF(MP_QSTR_copy, (const byte*)"\xe0\xdb\x04" "copy")
QDEF(MP_QSTR_fromkeys, (const byte*)"\x37\xbd\x08" "fromkeys")
QDEF(MP_QSTR_get, (const byte*)"\x33\x3b\x03" "get")
QDEF(MP_QSTR_items, (const byte*)"\xe3\x53\x05" "items")
QDEF(MP_QSTR_pop, (const byte*)"\x2a\x73\x03" "pop")
QDEF(MP_QSTR_popitem, (const byte*)"\xbf\x2c\x07" "popitem")
QDEF(MP_QSTR_setdefault, (const byte*)"\x6c\xa3\x0a" "setdefault")
QDEF(MP_QSTR_update, (const byte*)"\xb4\x76\x06" "update")
QDEF(MP_QSTR_values, (const byte*)"\x7d\xbe\x06" "values")
QDEF(MP_QSTR___setitem__, (const byte*)"\x32\x3e\x0b" "__setitem__")
QDEF(MP_QSTR___delitem__, (const byte*)"\xfd\x35\x0b" "__delitem__")
QDEF(MP_QSTR_dict, (const byte*)"\x3f\xfc\x04" "dict")
QDEF(MP_QSTR_OrderedDict, (const byte*)"\xf0\x7e\x0b" "OrderedDict")
QDEF(MP_QSTR_iterable, (const byte*)"\x25\x92\x08" "iterable")
QDEF(MP_QSTR_start, (const byte*)"\x85\xef\x05" "start")
QDEF(MP_QSTR_enumerate, (const byte*)"\x71\xba\x09" "enumerate")
QDEF(MP_QSTR_args, (const byte*)"\xc2\xc6\x04" "args")
QDEF(MP_QSTR_value, (const byte*)"\x4e\x34\x05" "value")
QDEF(MP_QSTR___init__, (const byte*)"\x5f\xa5\x08" "__init__")
QDEF(MP_QSTR_BaseException, (const byte*)"\x07\x92\x0d" "BaseException")
QDEF(MP_QSTR_filter, (const byte*)"\x25\xbe\x06" "filter")
QDEF(MP_QSTR_float, (const byte*)"\x35\x44\x05" "float")
QDEF(MP_QSTR_from_bytes, (const byte*)"\x35\x74\x0a" "from_bytes")
QDEF(MP_QSTR_to_bytes, (const byte*)"\xd8\x3e\x08" "to_bytes")
QDEF(MP_QSTR_key, (const byte*)"\x32\x6d\x03" "key")
QDEF(MP_QSTR_reverse, (const byte*)"\x25\x2a\x07" "reverse")
QDEF(MP_QSTR_count, (const byte*)"\xa6\x4d\x05" "count")
QDEF(MP_QSTR_index, (const byte*)"\x7b\x28\x05" "index")
QDEF(MP_QSTR_insert, (const byte*)"\x12\x54\x06" "insert")
QDEF(MP_QSTR_remove, (const byte*)"\x63\x8a\x06" "remove")
QDEF(MP_QSTR_sort, (const byte*)"\xbf\x9d\x04" "sort")
QDEF(MP_QSTR_list, (const byte*)"\x27\x1d\x04" "list")
QDEF(MP_QSTR_map, (const byte*)"\xb9\x43\x03" "map")
QDEF(MP_QSTR_module, (const byte*)"\xbf\x99\x06" "module")
QDEF(MP_QSTR_builtins, (const byte*)"\xf7\x31\x08" "builtins")
QDEF(MP_QSTR_uio, (const byte*)"\xb6\x66\x03" "uio")
QDEF(MP_QSTR_ucollections, (const byte*)"\x15\x9a\x0c" "ucollections")
QDEF(MP_QSTR_ustruct, (const byte*)"\x47\x08\x07" "ustruct")
QDEF(MP_QSTR_math, (const byte*)"\x35\xbb\x04" "math")
QDEF(MP_QSTR_cmath, (const byte*)"\xb6\xf4\x05" "cmath")
QDEF(MP_QSTR_sys, (const byte*)"\xbc\x8e\x03" "sys")
QDEF(MP_QSTR_gc, (const byte*)"\x61\x6e\x02" "gc")
QDEF(MP_QSTR_uerrno, (const byte*)"\xb4\xe9\x06" "uerrno")
QDEF(MP_QSTR_uzlib, (const byte*)"\x6d\x9b\x05" "uzlib")
QDEF(MP_QSTR_ujson, (const byte*)"\xe8\x30\x05" "ujson")
QDEF(MP_QSTR_ure, (const byte*)"\x87\x63\x03" "ure")
QDEF(MP_QSTR_uheapq, (const byte*)"\x1d\x43\x06" "uheapq")
QDEF(MP_QSTR_uhashlib, (const byte*)"\x65\x9d\x08" "uhashlib")
QDEF(MP_QSTR_ubinascii, (const byte*)"\xc4\x88\x09" "ubinascii")
QDEF(MP_QSTR_ucrypto, (const byte*)"\x13\x22\x07" "ucrypto")
QDEF(MP_QSTR_utime, (const byte*)"\xe5\x9d\x05" "utime")
QDEF(MP_QSTR_apollo, (const byte*)"\xd4\x46\x06" "apollo")
QDEF(MP_QSTR_urandom, (const byte*)"\xab\xae\x07" "urandom")
QDEF(MP_QSTR_ussl, (const byte*)"\x1c\xf2\x04" "ussl")
QDEF(MP_QSTR_lwip, (const byte*)"\x67\x89\x04" "lwip")
QDEF(MP_QSTR_websocket, (const byte*)"\x90\x8d\x09" "websocket")
QDEF(MP_QSTR__webrepl, (const byte*)"\x21\x95\x08" "_webrepl")
QDEF(MP_QSTR_framebuf, (const byte*)"\x69\x82\x08" "framebuf")
QDEF(MP_QSTR___new__, (const byte*)"\x79\x15\x07" "__new__")
QDEF(MP_QSTR_doc, (const byte*)"\x2d\x1f\x03" "doc")
QDEF(MP_QSTR_getter, (const byte*)"\x90\xb2\x06" "getter")
QDEF(MP_QSTR_setter, (const byte*)"\x04\x59\x06" "setter")
QDEF(MP_QSTR_deleter, (const byte*)"\x6e\xdb\x07" "deleter")
QDEF(MP_QSTR_property, (const byte*)"\xc2\x29\x08" "property")
QDEF(MP_QSTR_NoneType, (const byte*)"\x17\x68\x08" "NoneType")
QDEF(MP_QSTR_stop, (const byte*)"\x9d\x36\x04" "stop")
QDEF(MP_QSTR_step, (const byte*)"\x57\x36\x04" "step")
QDEF(MP_QSTR___reversed__, (const byte*)"\x61\xff\x0c" "__reversed__")
QDEF(MP_QSTR_reversed, (const byte*)"\xa1\x6e\x08" "reversed")
QDEF(MP_QSTR_add, (const byte*)"\x44\x32\x03" "add")
QDEF(MP_QSTR_discard, (const byte*)"\x0f\x71\x07" "discard")
QDEF(MP_QSTR_difference, (const byte*)"\x72\x24\x0a" "difference")
QDEF(MP_QSTR_difference_update, (const byte*)"\x9c\xfa\x11" "difference_update")
QDEF(MP_QSTR_intersection, (const byte*)"\x28\x2a\x0c" "intersection")
QDEF(MP_QSTR_intersection_update, (const byte*)"\x06\xdd\x13" "intersection_update")
QDEF(MP_QSTR_isdisjoint, (const byte*)"\xf7\x68\x0a" "isdisjoint")
QDEF(MP_QSTR_issubset, (const byte*)"\xb9\xc1\x08" "issubset")
QDEF(MP_QSTR_issuperset, (const byte*)"\xfc\xec\x0a" "issuperset")
QDEF(MP_QSTR_symmetric_difference, (const byte*)"\xce\x67\x14" "symmetric_difference")
QDEF(MP_QSTR_symmetric_difference_update, (const byte*)"\x60\xf8\x1b" "symmetric_difference_update")
QDEF(MP_QSTR_union, (const byte*)"\xf6\x7c\x05" "union")
QDEF(MP_QSTR___contains__, (const byte*)"\xc6\x5f\x0c" "__contains__")
QDEF(MP_QSTR_set, (const byte*)"\x27\x8f\x03" "set")
QDEF(MP_QSTR_frozenset, (const byte*)"\xed\x9c\x09" "frozenset")
QDEF(MP_QSTR_Ellipsis, (const byte*)"\xf0\xe0\x08" "Ellipsis")
QDEF(MP_QSTR_NotImplemented, (const byte*)"\x3e\xc6\x0e" "NotImplemented")
QDEF(MP_QSTR_slice, (const byte*)"\xb5\xf4\x05" "slice")
QDEF(MP_QSTR_keepends, (const byte*)"\x62\x8b\x08" "keepends")
QDEF(MP_QSTR_decode, (const byte*)"\xa9\x59\x06" "decode")
QDEF(MP_QSTR_encode, (const byte*)"\x43\xca\x06" "encode")
QDEF(MP_QSTR_find, (const byte*)"\x00\x34\x04" "find")
QDEF(MP_QSTR_rfind, (const byte*)"\xd2\x9c\x05" "rfind")
QDEF(MP_QSTR_rindex, (const byte*)"\xe9\x2b\x06" "rindex")
QDEF(MP_QSTR_join, (const byte*)"\xa7\x5c\x04" "join")
QDEF(MP_QSTR_split, (const byte*)"\xb7\x33\x05" "split")
QDEF(MP_QSTR_splitlines, (const byte*)"\x6a\xd3\x0a" "splitlines")
QDEF(MP_QSTR_rsplit, (const byte*)"\xa5\x00\x06" "rsplit")
QDEF(MP_QSTR_startswith, (const byte*)"\x74\xe8\x0a" "startswith")
QDEF(MP_QSTR_endswith, (const byte*)"\x1b\xa3\x08" "endswith")
QDEF(MP_QSTR_strip, (const byte*)"\x29\x1e\x05" "strip")
QDEF(MP_QSTR_lstrip, (const byte*)"\xe5\xb9\x06" "lstrip")
QDEF(MP_QSTR_rstrip, (const byte*)"\x3b\x95\x06" "rstrip")
QDEF(MP_QSTR_format, (const byte*)"\x26\x33\x06" "format")
QDEF(MP_QSTR_replace, (const byte*)"\x49\x25\x07" "replace")
QDEF(MP_QSTR_partition, (const byte*)"\x87\xe5\x09" "partition")
QDEF(MP_QSTR_rpartition, (const byte*)"\x15\xd0\x0a" "rpartition")
QDEF(MP_QSTR_center, (const byte*)"\x4e\xbf\x06" "center")
QDEF(MP_QSTR_lower, (const byte*)"\xc6\xcb\x05" "lower")
QDEF(MP_QSTR_upper, (const byte*)"\x27\x94\x05" "upper")
QDEF(MP_QSTR_isspace, (const byte*)"\x5b\xf8\x07" "isspace")
QDEF(MP_QSTR_isalpha, (const byte*)"\xeb\x37\x07" "isalpha")
QDEF(MP_QSTR_isdigit, (const byte*)"\xa8\x9a\x07" "isdigit")
QDEF(MP_QSTR_isupper, (const byte*)"\xdd\xa7\x07" "isupper")
QDEF(MP_QSTR_islower, (const byte*)"\xfc\x80\x07" "islower")
QDEF(MP_QSTR_str, (const byte*)"\x50\x8d\x03" "str")
QDEF(MP_QSTR_bytes, (const byte*)"\x5c\xb2\x05" "bytes")
QDEF(MP_QSTR_read, (const byte*)"\xb7\xf9\x04" "read")
QDEF(MP_QSTR_readall, (const byte*)"\x76\x4b\x07" "readall")
QDEF(MP_QSTR_readline, (const byte*)"\xf9\x19\x08" "readline")
QDEF(MP_QSTR_write, (const byte*)"\x98\xa8\x05" "write")
QDEF(MP_QSTR_getvalue, (const byte*)"\x78\xac\x08" "getvalue")
QDEF(MP_QSTR___enter__, (const byte*)"\x6d\xba\x09" "__enter__")
QDEF(MP_QSTR___exit__, (const byte*)"\x45\xf8\x08" "__exit__")
QDEF(MP_QSTR_StringIO, (const byte*)"\x76\x76\x08" "StringIO")
QDEF(MP_QSTR_BytesIO, (const byte*)"\x1a\xb7\x07" "BytesIO")
QDEF(MP_QSTR___str__, (const byte*)"\xd0\xcd\x07" "__str__")
QDEF(MP_QSTR___repr__, (const byte*)"\x10\x0b\x08" "__repr__")
QDEF(MP_QSTR___bool__, (const byte*)"\x2b\x65\x08" "__bool__")
QDEF(MP_QSTR___len__, (const byte*)"\xe2\xb0\x07" "__len__")
QDEF(MP_QSTR___hash__, (const byte*)"\xf7\xc8\x08" "__hash__")
QDEF(MP_QSTR___pos__, (const byte*)"\x29\xf0\x07" "__pos__")
QDEF(MP_QSTR___neg__, (const byte*)"\x69\xd5\x07" "__neg__")
QDEF(MP_QSTR___invert__, (const byte*)"\xf7\x77\x0a" "__invert__")
QDEF(MP_QSTR___eq__, (const byte*)"\x71\x3e\x06" "__eq__")
QDEF(MP_QSTR___add__, (const byte*)"\xc4\x82\x07" "__add__")
QDEF(MP_QSTR___sub__, (const byte*)"\x21\x09\x07" "__sub__")
QDEF(MP_QSTR___mul__, (const byte*)"\x31\x42\x07" "__mul__")
QDEF(MP_QSTR___floordiv__, (const byte*)"\x46\x5f\x0c" "__floordiv__")
QDEF(MP_QSTR___truediv__, (const byte*)"\x88\xef\x0b" "__truediv__")
QDEF(MP_QSTR___iadd__, (const byte*)"\x6d\x4a\x08" "__iadd__")
QDEF(MP_QSTR___isub__, (const byte*)"\x08\x78\x08" "__isub__")
QDEF(MP_QSTR___lt__, (const byte*)"\x5d\x68\x06" "__lt__")
QDEF(MP_QSTR___gt__, (const byte*)"\xb6\x82\x06" "__gt__")
QDEF(MP_QSTR___le__, (const byte*)"\xcc\x13\x06" "__le__")
QDEF(MP_QSTR___ge__, (const byte*)"\xa7\x46\x06" "__ge__")
QDEF(MP_QSTR___dict__, (const byte*)"\x7f\x54\x08" "__dict__")
QDEF(MP_QSTR___get__, (const byte*)"\xb3\x8f\x07" "__get__")
QDEF(MP_QSTR___getattr__, (const byte*)"\x40\xf8\x0b" "__getattr__")
QDEF(MP_QSTR___delete__, (const byte*)"\xdc\xed\x0a" "__delete__")
QDEF(MP_QSTR___set__, (const byte*)"\xa7\xb3\x07" "__set__")
QDEF(MP_QSTR___call__, (const byte*)"\xa7\xf9\x08" "__call__")
QDEF(MP_QSTR___iter__, (const byte*)"\xcf\x32\x08" "__iter__")
QDEF(MP_QSTR_type, (const byte*)"\x9d\x7f\x04" "type")
QDEF(MP_QSTR_staticmethod, (const byte*)"\x62\xaf\x0c" "staticmethod")
QDEF(MP_QSTR_classmethod, (const byte*)"\xb4\x8c\x0b" "classmethod")
QDEF(MP_QSTR_zip, (const byte*)"\xe6\xac\x03" "zip")
QDEF(MP_QSTR_default, (const byte*)"\xce\x7d\x07" "default")
QDEF(MP_QSTR_sep, (const byte*)"\x23\x8f\x03" "sep")
QDEF(MP_QSTR_end, (const byte*)"\x0a\x23\x03" "end")
QDEF(MP_QSTR_file, (const byte*)"\xc3\x34\x04" "file")
QDEF(MP_QSTR___import__, (const byte*)"\x38\x3e\x0a" "__import__")
QDEF(MP_QSTR_abs, (const byte*)"\x95\x32\x03" "abs")
QDEF(MP_QSTR_all, (const byte*)"\x44\x33\x03" "all")
QDEF(MP_QSTR_any, (const byte*)"\x13\x33\x03" "any")
QDEF(MP_QSTR_bin, (const byte*)"\xe0\x48\x03" "bin")
QDEF(MP_QSTR_callable, (const byte*)"\x0d\x70\x08" "callable")
QDEF(MP_QSTR_compile, (const byte*)"\xf4\xc9\x07" "compile")
QDEF(MP_QSTR_chr, (const byte*)"\xdc\x4c\x03" "chr")
QDEF(MP_QSTR_dir, (const byte*)"\xfa\x1e\x03" "dir")
QDEF(MP_QSTR_divmod, (const byte*)"\xb8\x04\x06" "divmod")
QDEF(MP_QSTR_execfile, (const byte*)"\x58\x28\x08" "execfile")
QDEF(MP_QSTR_getattr, (const byte*)"\xc0\x17\x07" "getattr")
QDEF(MP_QSTR_setattr, (const byte*)"\xd4\xa8\x07" "setattr")
QDEF(MP_QSTR_globals, (const byte*)"\x9d\x49\x07" "globals")
QDEF(MP_QSTR_hasattr, (const byte*)"\x8c\xb0\x07" "hasattr")
QDEF(MP_QSTR_hash, (const byte*)"\xb7\x70\x04" "hash")
QDEF(MP_QSTR_hex, (const byte*)"\x70\x50\x03" "hex")
QDEF(MP_QSTR_id, (const byte*)"\x28\x6f\x02" "id")
QDEF(MP_QSTR_isinstance, (const byte*)"\xb6\xbe\x0a" "isinstance")
QDEF(MP_QSTR_issubclass, (const byte*)"\xb5\x7f\x0a" "issubclass")
QDEF(MP_QSTR_iter, (const byte*)"\x8f\x21\x04" "iter")
QDEF(MP_QSTR_len, (const byte*)"\x62\x40\x03" "len")
QDEF(MP_QSTR_locals, (const byte*)"\x3b\xa1\x06" "locals")
QDEF(MP_QSTR_max, (const byte*)"\xb1\x43\x03" "max")
QDEF(MP_QSTR_min, (const byte*)"\xaf\x42\x03" "min")
QDEF(MP_QSTR_next, (const byte*)"\x42\x88\x04" "next")
QDEF(MP_QSTR_oct, (const byte*)"\xfd\x5c\x03" "oct")
QDEF(MP_QSTR_ord, (const byte*)"\x1c\x5e\x03" "ord")
QDEF(MP_QSTR_pow, (const byte*)"\x2d\x73\x03" "pow")
QDEF(MP_QSTR_print, (const byte*)"\x54\xc6\x05" "print")
QDEF(MP_QSTR_repr, (const byte*)"\xd0\xf7\x04" "repr")
QDEF(MP_QSTR_round, (const byte*)"\xe7\x25\x05" "round")
QDEF(MP_QSTR_sorted, (const byte*)"\x5e\x15\x06" "sorted")
QDEF(MP_QSTR_sum, (const byte*)"\x2e\x8d\x03" "sum")
QDEF(MP_QSTR_ArithmeticError, (const byte*)"\x2d\x8c\x0f" "ArithmeticError")
QDEF(MP_QSTR_AttributeError, (const byte*)"\x21\xde\x0e" "AttributeError")
QDEF(MP_QSTR_EOFError, (const byte*)"\x91\xbf\x08" "EOFError")
QDEF(MP_QSTR_Exception, (const byte*)"\xf2\x29\x09" "Exception")
QDEF(MP_QSTR_GeneratorExit, (const byte*)"\x16\x62\x0d" "GeneratorExit")
QDEF(MP_QSTR_ImportError, (const byte*)"\x20\x9c\x0b" "ImportError")
QDEF(MP_QSTR_IndentationError, (const byte*)"\x5c\x20\x10" "IndentationError")
QDEF(MP_QSTR_IndexError, (const byte*)"\x83\xad\x0a" "IndexError")
QDEF(MP_QSTR_KeyboardInterrupt, (const byte*)"\xaf\xe2\x11" "KeyboardInterrupt")
QDEF(MP_QSTR_KeyError, (const byte*)"\xea\x00\x08" "KeyError")
QDEF(MP_QSTR_LookupError, (const byte*)"\xff\x69\x0b" "LookupError")
QDEF(MP_QSTR_MemoryError, (const byte*)"\xdc\x83\x0b" "MemoryError")
QDEF(MP_QSTR_NameError, (const byte*)"\xba\x2d\x09" "NameError")
QDEF(MP_QSTR_NotImplementedError, (const byte*)"\xc6\x98\x13" "NotImplementedError")
QDEF(MP_QSTR_OSError, (const byte*)"\xa1\x65\x07" "OSError")
QDEF(MP_QSTR_OverflowError, (const byte*)"\x81\xe1\x0d" "OverflowError")
QDEF(MP_QSTR_RuntimeError, (const byte*)"\x61\xf1\x0c" "RuntimeError")
QDEF(MP_QSTR_StopIteration, (const byte*)"\xea\x1c\x0d" "StopIteration")
QDEF(MP_QSTR_SyntaxError, (const byte*)"\x94\x8f\x0b" "SyntaxError")
QDEF(MP_QSTR_SystemExit, (const byte*)"\x20\xff\x0a" "SystemExit")
QDEF(MP_QSTR_TypeError, (const byte*)"\x25\x96\x09" "TypeError")
QDEF(MP_QSTR_UnicodeError, (const byte*)"\x22\xd1\x0c" "UnicodeError")
QDEF(MP_QSTR_ValueError, (const byte*)"\x96\x87\x0a" "ValueError")
QDEF(MP_QSTR_ViperTypeError, (const byte*)"\xdd\x05\x0e" "ViperTypeError")
QDEF(MP_QSTR_ZeroDivisionError, (const byte*)"\xb6\x27\x11" "ZeroDivisionError")
QDEF(MP_QSTR_namedtuple, (const byte*)"\x1e\x16\x0a" "namedtuple")
QDEF(MP_QSTR_collect, (const byte*)"\x9b\x65\x07" "collect")
QDEF(MP_QSTR_disable, (const byte*)"\x91\x76\x07" "disable")
QDEF(MP_QSTR_enable, (const byte*)"\x04\xde\x06" "enable")
QDEF(MP_QSTR_isenabled, (const byte*)"\x9a\xe5\x09" "isenabled")
QDEF(MP_QSTR_mem_free, (const byte*)"\xcb\x62\x08" "mem_free")
QDEF(MP_QSTR_mem_alloc, (const byte*)"\x52\x2b\x09" "mem_alloc")
QDEF(MP_QSTR_flush, (const byte*)"\x61\xc1\x05" "flush")
QDEF(MP_QSTR_BufferedWriter, (const byte*)"\xeb\x2c\x0e" "BufferedWriter")
QDEF(MP_QSTR_open, (const byte*)"\xd1\x3a\x04" "open")
QDEF(MP_QSTR_FileIO, (const byte*)"\xc5\x15\x06" "FileIO")
QDEF(MP_QSTR_TextIOWrapper, (const byte*)"\xad\x8d\x0d" "TextIOWrapper")
QDEF(MP_QSTR_e, (const byte*)"\xc0\xb5\x01" "e")
QDEF(MP_QSTR_pi, (const byte*)"\x1c\x70\x02" "pi")
QDEF(MP_QSTR_sqrt, (const byte*)"\x21\x44\x04" "sqrt")
QDEF(MP_QSTR_exp, (const byte*)"\xc8\x24\x03" "exp")
QDEF(MP_QSTR_expm1, (const byte*)"\x74\x72\x05" "expm1")
QDEF(MP_QSTR_log, (const byte*)"\x21\x3f\x03" "log")
QDEF(MP_QSTR_log2, (const byte*)"\x73\x23\x04" "log2")
QDEF(MP_QSTR_log10, (const byte*)"\x40\x91\x05" "log10")
QDEF(MP_QSTR_cosh, (const byte*)"\xd2\xdb\x04" "cosh")
QDEF(MP_QSTR_sinh, (const byte*)"\xb9\xa6\x04" "sinh")
QDEF(MP_QSTR_tanh, (const byte*)"\xd6\xa1\x04" "tanh")
QDEF(MP_QSTR_acosh, (const byte*)"\x13\xa3\x05" "acosh")
QDEF(MP_QSTR_asinh, (const byte*)"\x38\x8f\x05" "asinh")
QDEF(MP_QSTR_atanh, (const byte*)"\x97\x81\x05" "atanh")
QDEF(MP_QSTR_cos, (const byte*)"\x7a\x4c\x03" "cos")
QDEF(MP_QSTR_sin, (const byte*)"\xb1\x90\x03" "sin")
QDEF(MP_QSTR_tan, (const byte*)"\xfe\x61\x03" "tan")
QDEF(MP_QSTR_acos, (const byte*)"\x1b\xa0\x04" "acos")
QDEF(MP_QSTR_asin, (const byte*)"\x50\xe5\x04" "asin")
QDEF(MP_QSTR_atan, (const byte*)"\x1f\xbe\x04" "atan")
QDEF(MP_QSTR_atan2, (const byte*)"\xcd\x81\x05" "atan2")
QDEF(MP_QSTR_ceil, (const byte*)"\x06\xb0\x04" "ceil")
QDEF(MP_QSTR_copysign, (const byte*)"\x33\x14\x08" "copysign")
QDEF(MP_QSTR_fabs, (const byte*)"\x93\x12\x04" "fabs")
QDEF(MP_QSTR_floor, (const byte*)"\x7d\x46\x05" "floor")
QDEF(MP_QSTR_fmod, (const byte*)"\xe5\x44\x04" "fmod")
QDEF(MP_QSTR_frexp, (const byte*)"\x1c\x98\x05" "frexp")
QDEF(MP_QSTR_ldexp, (const byte*)"\x40\x6f\x05" "ldexp")
QDEF(MP_QSTR_modf, (const byte*)"\x25\xc0\x04" "modf")
QDEF(MP_QSTR_isfinite, (const byte*)"\xa6\xab\x08" "isfinite")
QDEF(MP_QSTR_isinf, (const byte*)"\x3e\x11\x05" "isinf")
QDEF(MP_QSTR_isnan, (const byte*)"\x9e\x03\x05" "isnan")
QDEF(MP_QSTR_trunc, (const byte*)"\x5b\x99\x05" "trunc")
QDEF(MP_QSTR_radians, (const byte*)"\x87\x3f\x07" "radians")
QDEF(MP_QSTR_degrees, (const byte*)"\x02\x41\x07" "degrees")
QDEF(MP_QSTR_erf, (const byte*)"\x94\x23\x03" "erf")
QDEF(MP_QSTR_erfc, (const byte*)"\x77\x96\x04" "erfc")
QDEF(MP_QSTR_gamma, (const byte*)"\x02\x90\x05" "gamma")
QDEF(MP_QSTR_lgamma, (const byte*)"\xce\x6c\x06" "lgamma")
QDEF(MP_QSTR_phase, (const byte*)"\x6a\xd5\x05" "phase")
QDEF(MP_QSTR_polar, (const byte*)"\x05\x0c\x05" "polar")
QDEF(MP_QSTR_rect, (const byte*)"\xe5\xf9\x04" "rect")
QDEF(MP_QSTR_mem_total, (const byte*)"\xfd\x6a\x09" "mem_total")
QDEF(MP_QSTR_mem_current, (const byte*)"\x16\xba\x0b" "mem_current")
QDEF(MP_QSTR_mem_peak, (const byte*)"\x40\x25\x08" "mem_peak")
QDEF(MP_QSTR_mem_info, (const byte*)"\xd1\xf1\x08" "mem_info")
QDEF(MP_QSTR_qstr_info, (const byte*)"\xb0\x81\x09" "qstr_info")
QDEF(MP_QSTR_stack_use, (const byte*)"\x97\xf7\x09" "stack_use")
QDEF(MP_QSTR_alloc_emergency_exception_buf, (const byte*)"\x78\x2a\x1d" "alloc_emergency_exception_buf")
QDEF(MP_QSTR_heap_lock, (const byte*)"\xad\x8c\x09" "heap_lock")
QDEF(MP_QSTR_heap_unlock, (const byte*)"\x56\x2d\x0b" "heap_unlock")
QDEF(MP_QSTR_calcsize, (const byte*)"\x4d\x38\x08" "calcsize")
QDEF(MP_QSTR_pack, (const byte*)"\xbc\xd1\x04" "pack")
QDEF(MP_QSTR_pack_into, (const byte*)"\x1f\xa9\x09" "pack_into")
QDEF(MP_QSTR_unpack, (const byte*)"\x07\x3c\x06" "unpack")
QDEF(MP_QSTR_unpack_from, (const byte*)"\x0e\x6d\x0b" "unpack_from")
QDEF(MP_QSTR_name, (const byte*)"\xa2\x75\x04" "name")
QDEF(MP_QSTR_version, (const byte*)"\xbf\xd3\x07" "version")
QDEF(MP_QSTR_version_info, (const byte*)"\x6e\x0a\x0c" "version_info")
QDEF(MP_QSTR_implementation, (const byte*)"\x17\x2d\x0e" "implementation")
QDEF(MP_QSTR_platform, (const byte*)"\x3a\x19\x08" "platform")
QDEF(MP_QSTR_byteorder, (const byte*)"\x61\x99\x09" "byteorder")
QDEF(MP_QSTR_little, (const byte*)"\x89\x6a\x06" "little")
QDEF(MP_QSTR_big, (const byte*)"\xe9\x48\x03" "big")
QDEF(MP_QSTR_maxsize, (const byte*)"\xd4\x70\x07" "maxsize")
QDEF(MP_QSTR_exit, (const byte*)"\x85\xbe\x04" "exit")
QDEF(MP_QSTR_stdin, (const byte*)"\x21\x04\x05" "stdin")
QDEF(MP_QSTR_stdout, (const byte*)"\x08\x83\x06" "stdout")
QDEF(MP_QSTR_stderr, (const byte*)"\xa3\x58\x06" "stderr")
QDEF(MP_QSTR_exc_info, (const byte*)"\x0a\xff\x08" "exc_info")
QDEF(MP_QSTR_print_exception, (const byte*)"\x1c\x22\x0f" "print_exception")
QDEF(MP_QSTR_struct, (const byte*)"\x12\x90\x06" "struct")
QDEF(MP_QSTR_sizeof, (const byte*)"\x49\x73\x06" "sizeof")
QDEF(MP_QSTR_addressof, (const byte*)"\x5a\xf9\x09" "addressof")
QDEF(MP_QSTR_bytes_at, (const byte*)"\xb6\x5d\x08" "bytes_at")
QDEF(MP_QSTR_bytearray_at, (const byte*)"\x9c\x5c\x0c" "bytearray_at")
QDEF(MP_QSTR_NATIVE, (const byte*)"\x04\x8e\x06" "NATIVE")
QDEF(MP_QSTR_LITTLE_ENDIAN, (const byte*)"\xbf\x5b\x0d" "LITTLE_ENDIAN")
QDEF(MP_QSTR_BIG_ENDIAN, (const byte*)"\xff\x51\x0a" "BIG_ENDIAN")
QDEF(MP_QSTR_VOID, (const byte*)"\x31\xf2\x04" "VOID")
QDEF(MP_QSTR_UINT8, (const byte*)"\xbb\xe1\x05" "UINT8")
QDEF(MP_QSTR_INT8, (const byte*)"\xce\xbd\x04" "INT8")
QDEF(MP_QSTR_UINT16, (const byte*)"\xc4\x17\x06" "UINT16")
QDEF(MP_QSTR_INT16, (const byte*)"\x91\x76\x05" "INT16")
QDEF(MP_QSTR_UINT32, (const byte*)"\x82\x17\x06" "UINT32")
QDEF(MP_QSTR_INT32, (const byte*)"\x57\x76\x05" "INT32")
QDEF(MP_QSTR_UINT64, (const byte*)"\x61\x18\x06" "UINT64")
QDEF(MP_QSTR_INT64, (const byte*)"\xf4\x75\x05" "INT64")
QDEF(MP_QSTR_BFUINT8, (const byte*)"\xbf\xaf\x07" "BFUINT8")
QDEF(MP_QSTR_BFINT8, (const byte*)"\x4a\x9a\x06" "BFINT8")
QDEF(MP_QSTR_BFUINT16, (const byte*)"\x40\xa6\x08" "BFUINT16")
QDEF(MP_QSTR_BFINT16, (const byte*)"\x95\xe2\x07" "BFINT16")
QDEF(MP_QSTR_BFUINT32, (const byte*)"\x06\xa6\x08" "BFUINT32")
QDEF(MP_QSTR_BFINT32, (const byte*)"\x53\xe2\x07" "BFINT32")
QDEF(MP_QSTR_BF_POS, (const byte*)"\x52\x9d\x06" "BF_POS")
QDEF(MP_QSTR_BF_LEN, (const byte*)"\x19\xb0\x06" "BF_LEN")
QDEF(MP_QSTR_FLOAT32, (const byte*)"\xb4\x87\x07" "FLOAT32")
QDEF(MP_QSTR_FLOAT64, (const byte*)"\x17\x87\x07" "FLOAT64")
QDEF(MP_QSTR_PTR, (const byte*)"\xb3\x0c\x03" "PTR")
QDEF(MP_QSTR_ARRAY, (const byte*)"\x5c\x7a\x05" "ARRAY")
QDEF(MP_QSTR_dumps, (const byte*)"\x7a\x2d\x05" "dumps")
QDEF(MP_QSTR_loads, (const byte*)"\xb0\xb0\x05" "loads")
QDEF(MP_QSTR_group, (const byte*)"\xba\xb0\x05" "group")
QDEF(MP_QSTR_match, (const byte*)"\x96\x22\x05" "match")
QDEF(MP_QSTR_search, (const byte*)"\xab\xc1\x06" "search")
QDEF(MP_QSTR_DEBUG, (const byte*)"\x34\x6d\x05" "DEBUG")
QDEF(MP_QSTR_heappush, (const byte*)"\x87\x6b\x08" "heappush")
QDEF(MP_QSTR_heappop, (const byte*)"\xd6\x27\x07" "heappop")
QDEF(MP_QSTR_heapify, (const byte*)"\xaf\x2d\x07" "heapify")
QDEF(MP_QSTR_compress, (const byte*)"\xa3\x7a\x08" "compress")
QDEF(MP_QSTR_decompress, (const byte*)"\x62\xfb\x0a" "decompress")
QDEF(MP_QSTR_offzip, (const byte*)"\x89\x1e\x06" "offzip")
QDEF(MP_QSTR_packzip, (const byte*)"\x3f\x80\x07" "packzip")
QDEF(MP_QSTR_CRC_16_BITS, (const byte*)"\xdc\x3b\x0b" "CRC_16_BITS")
QDEF(MP_QSTR_CRC_32_BITS, (const byte*)"\xda\xa4\x0b" "CRC_32_BITS")
QDEF(MP_QSTR_CRC_64_BITS, (const byte*)"\x19\x88\x0b" "CRC_64_BITS")
QDEF(MP_QSTR_crc, (const byte*)"\x17\x4d\x03" "crc")
QDEF(MP_QSTR_crc16, (const byte*)"\xb0\xe8\x05" "crc16")
QDEF(MP_QSTR_crc32, (const byte*)"\x76\xe8\x05" "crc32")
QDEF(MP_QSTR_crc32big, (const byte*)"\x3a\x19\x08" "crc32big")
QDEF(MP_QSTR_crc64_iso, (const byte*)"\x7f\xe9\x09" "crc64_iso")
QDEF(MP_QSTR_crc64_ecma, (const byte*)"\xe0\x66\x0a" "crc64_ecma")
QDEF(MP_QSTR_sha224, (const byte*)"\xcb\x01\x06" "sha224")
QDEF(MP_QSTR_md5, (const byte*)"\x19\x44\x03" "md5")
QDEF(MP_QSTR_md5_xor, (const byte*)"\xe3\x4e\x07" "md5_xor")
QDEF(MP_QSTR_sha1, (const byte*)"\x8e\xac\x04" "sha1")
QDEF(MP_QSTR_sha256, (const byte*)"\x2e\x01\x06" "sha256")
QDEF(MP_QSTR_sha384, (const byte*)"\x80\x04\x06" "sha384")
QDEF(MP_QSTR_sha512, (const byte*)"\x69\xfd\x06" "sha512")
QDEF(MP_QSTR_hmac_sha1, (const byte*)"\x76\x8c\x09" "hmac_sha1")
QDEF(MP_QSTR_pbkdf2_sha1, (const byte*)"\x38\x07\x0b" "pbkdf2_sha1")
QDEF(MP_QSTR_pbkdf2_sha256, (const byte*)"\x58\xc4\x0d" "pbkdf2_sha256")
QDEF(MP_QSTR_sha1_xor64, (const byte*)"\x76\x60\x0a" "sha1_xor64")
QDEF(MP_QSTR_adler16, (const byte*)"\x5c\x0d\x07" "adler16")
QDEF(MP_QSTR_adler32, (const byte*)"\x1a\x0d\x07" "adler32")
QDEF(MP_QSTR_checksum32, (const byte*)"\xe9\xfa\x0a" "checksum32")
QDEF(MP_QSTR_sdbm, (const byte*)"\x3d\x79\x04" "sdbm")
QDEF(MP_QSTR_fnv1, (const byte*)"\xca\x2a\x04" "fnv1")
QDEF(MP_QSTR_wadd, (const byte*)"\xf3\x55\x04" "wadd")
QDEF(MP_QSTR_dwadd, (const byte*)"\xb7\x15\x05" "dwadd")
QDEF(MP_QSTR_qwadd, (const byte*)"\x62\x3a\x05" "qwadd")
QDEF(MP_QSTR_wadd_le, (const byte*)"\x85\x51\x07" "wadd_le")
QDEF(MP_QSTR_dwadd_le, (const byte*)"\x41\x1a\x08" "dwadd_le")
QDEF(MP_QSTR_wsub, (const byte*)"\x16\x1b\x04" "wsub")
QDEF(MP_QSTR_force_crc32, (const byte*)"\x34\x81\x0b" "force_crc32")
QDEF(MP_QSTR_murmur3_32, (const byte*)"\xa8\xab\x0a" "murmur3_32")
QDEF(MP_QSTR_jhash, (const byte*)"\xfd\xc0\x05" "jhash")
QDEF(MP_QSTR_jenkins_oaat, (const byte*)"\x7f\xdc\x0c" "jenkins_oaat")
QDEF(MP_QSTR_lookup3_little2, (const byte*)"\xd5\x95\x0f" "lookup3_little2")
QDEF(MP_QSTR_djb2, (const byte*)"\x9b\xef\x04" "djb2")
QDEF(MP_QSTR_ea_checksum, (const byte*)"\x53\x04\x0b" "ea_checksum")
QDEF(MP_QSTR_ffx_checksum, (const byte*)"\x2f\x3e\x0c" "ffx_checksum")
QDEF(MP_QSTR_ff13_checksum, (const byte*)"\xd5\xd0\x0d" "ff13_checksum")
QDEF(MP_QSTR_kh25_checksum, (const byte*)"\xd3\x45\x0d" "kh25_checksum")
QDEF(MP_QSTR_khcom_checksum, (const byte*)"\x15\xfe\x0e" "khcom_checksum")
QDEF(MP_QSTR_mgs2_checksum, (const byte*)"\x9c\xa5\x0d" "mgs2_checksum")
QDEF(MP_QSTR_mgspw_checksum, (const byte*)"\xc9\x29\x0e" "mgspw_checksum")
QDEF(MP_QSTR_sw4_checksum, (const byte*)"\x47\x0a\x0c" "sw4_checksum")
QDEF(MP_QSTR_toz_checksum, (const byte*)"\x36\x72\x0c" "toz_checksum")
QDEF(MP_QSTR_tiara2_checksum, (const byte*)"\xca\x9c\x0f" "tiara2_checksum")
QDEF(MP_QSTR_castlevania_checksum, (const byte*)"\x0a\x9f\x14" "castlevania_checksum")
QDEF(MP_QSTR_rockstar_checksum, (const byte*)"\x36\x95\x11" "rockstar_checksum")
QDEF(MP_QSTR_dbzxv2_checksum, (const byte*)"\x77\xc6\x0f" "dbzxv2_checksum")
QDEF(MP_QSTR_deadrising_checksum, (const byte*)"\xbb\x7c\x13" "deadrising_checksum")
QDEF(MP_QSTR_hexlify, (const byte*)"\x2a\x7f\x07" "hexlify")
QDEF(MP_QSTR_unhexlify, (const byte*)"\xb1\xb9\x09" "unhexlify")
QDEF(MP_QSTR_a2b_base64, (const byte*)"\x3c\x0b\x0a" "a2b_base64")
QDEF(MP_QSTR_b2a_base64, (const byte*)"\x3c\x8f\x0a" "b2a_base64")
QDEF(MP_QSTR_DECRYPT, (const byte*)"\x88\x0e\x07" "DECRYPT")
QDEF(MP_QSTR_ENCRYPT, (const byte*)"\x22\xbb\x07" "ENCRYPT")
QDEF(MP_QSTR_diablo3, (const byte*)"\xdb\x9b\x07" "diablo3")
QDEF(MP_QSTR_dw8xl, (const byte*)"\x7a\xf9\x05" "dw8xl")
QDEF(MP_QSTR_silent_hill3, (const byte*)"\x81\x05\x0c" "silent_hill3")
QDEF(MP_QSTR_nfs_undercover, (const byte*)"\xa4\xd0\x0e" "nfs_undercover")
QDEF(MP_QSTR_final_fantasy13, (const byte*)"\xe2\x78\x0f" "final_fantasy13")
QDEF(MP_QSTR_borderlands3, (const byte*)"\x8e\x3a\x0c" "borderlands3")
QDEF(MP_QSTR_mgs_pw, (const byte*)"\x24\x87\x06" "mgs_pw")
QDEF(MP_QSTR_mgs_base64, (const byte*)"\xd4\x71\x0a" "mgs_base64")
QDEF(MP_QSTR_mgs, (const byte*)"\xfc\x44\x03" "mgs")
QDEF(MP_QSTR_mgs5_tpp, (const byte*)"\x82\xf9\x08" "mgs5_tpp")
QDEF(MP_QSTR_monster_hunter, (const byte*)"\x76\x93\x0e" "monster_hunter")
QDEF(MP_QSTR_rgg_studio, (const byte*)"\xf8\x01\x0a" "rgg_studio")
QDEF(MP_QSTR_aes_ecb, (const byte*)"\x49\x8c\x07" "aes_ecb")
QDEF(MP_QSTR_aes_cbc, (const byte*)"\xef\x94\x07" "aes_cbc")
QDEF(MP_QSTR_aes_ctr, (const byte*)"\x28\x96\x07" "aes_ctr")
QDEF(MP_QSTR_des_ecb, (const byte*)"\x4c\xea\x07" "des_ecb")
QDEF(MP_QSTR_des3_cbc, (const byte*)"\x39\x03\x08" "des3_cbc")
QDEF(MP_QSTR_blowfish_ecb, (const byte*)"\xbc\xcd\x0c" "blowfish_ecb")
QDEF(MP_QSTR_blowfish_cbc, (const byte*)"\x1a\xd5\x0c" "blowfish_cbc")
QDEF(MP_QSTR_camellia_ecb, (const byte*)"\x1c\x73\x0c" "camellia_ecb")
QDEF(MP_QSTR_endian_swap, (const byte*)"\x06\xc6\x0b" "endian_swap")
QDEF(MP_QSTR_reverse_search, (const byte*)"\xf4\xac\x0e" "reverse_search")
QDEF(MP_QSTR_apply_savewizard, (const byte*)"\x7c\x5e\x10" "apply_savewizard")
QDEF(MP_QSTR_localtime, (const byte*)"\x7d\x46\x09" "localtime")
QDEF(MP_QSTR_mktime, (const byte*)"\x96\x2b\x06" "mktime")
QDEF(MP_QSTR_time, (const byte*)"\xf0\xc1\x04" "time")
QDEF(MP_QSTR_gmtime, (const byte*)"\x5a\x8e\x06" "gmtime")
QDEF(MP_QSTR_strftime, (const byte*)"\x43\xf0\x08" "strftime")
#undef QDEF
#endif
    MP_QSTRnumber_of, // no underscore so it can't clash with any of the above
};

typedef size_t qstr;

typedef struct _qstr_pool_t {
    struct _qstr_pool_t *prev;
    size_t total_prev_len;
    size_t alloc;
    size_t len;
    const byte *qstrs[];
} qstr_pool_t;

#define QSTR_FROM_STR_STATIC(s) (micropy_qstr_from_strn(mp_state, (s), strlen(s)))

void micropy_qstr_init(struct _mp_state_ctx_t *mp_state);

mp_uint_t micropy_qstr_compute_hash(struct _mp_state_ctx_t *mp_state, const byte *data, size_t len);
qstr micropy_qstr_find_strn(struct _mp_state_ctx_t *mp_state, const char *str, size_t str_len); // returns MP_QSTR_NULL if not found

qstr micropy_qstr_from_str(struct _mp_state_ctx_t *mp_state, const char *str);
qstr micropy_qstr_from_strn(struct _mp_state_ctx_t *mp_state, const char *str, size_t len);

byte *micropy_qstr_build_start(struct _mp_state_ctx_t *mp_state, size_t len, byte **q_ptr);
qstr micropy_qstr_build_end(struct _mp_state_ctx_t *mp_state, byte *q_ptr);

mp_uint_t micropy_qstr_hash(struct _mp_state_ctx_t *mp_state, qstr q);
const char *micropy_qstr_str(struct _mp_state_ctx_t *mp_state, qstr q);
size_t micropy_qstr_len(struct _mp_state_ctx_t *mp_state, qstr q);
const byte *micropy_qstr_data(struct _mp_state_ctx_t *mp_state, qstr q, size_t *len);

void micropy_qstr_pool_info(struct _mp_state_ctx_t *mp_state, size_t *n_pool, size_t *n_qstr, size_t *n_str_data_bytes, size_t *n_total_bytes);
void micropy_qstr_dump_data(struct _mp_state_ctx_t *mp_state);

#endif // __MICROPY_INCLUDED_PY_QSTR_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_OBJ_H__
#define __MICROPY_INCLUDED_PY_OBJ_H__

//#include "py/mpconfig.h"
//#include "py/misc.h"
//#include "py/qstr.h"
//#include "py/mpprint.h"

// All Micro Python objects are at least this type
// The bit-size must be at least pointer size

#if MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_D
typedef uint64_t mp_obj_t;
typedef uint64_t mp_const_obj_t;
#else
typedef machine_ptr_t mp_obj_t;
typedef machine_const_ptr_t mp_const_obj_t;
#endif

// This mp_obj_type_t struct is a concrete MicroPython object which holds info
// about a type.  See below for actual definition of the struct.
typedef struct _mp_obj_type_t mp_obj_type_t;

// Anything that wants to be a concrete MicroPython object must have mp_obj_base_t
// as its first member (small ints, qstr objs and inline floats are not concrete).
struct _mp_obj_base_t {
    const mp_obj_type_t *type MICROPY_OBJ_BASE_ALIGNMENT;
};
typedef struct _mp_obj_base_t mp_obj_base_t;

// These fake objects are used to indicate certain things in arguments or return
// values, and should only be used when explicitly allowed.
//
//  - MP_OBJ_NULL : used to indicate the absence of an object, or unsupported operation.
//  - MP_OBJ_STOP_ITERATION : used instead of throwing a StopIteration, for efficiency.
//  - MP_OBJ_SENTINEL : used for various internal purposes where one needs
//    an object which is unique from all other objects, including MP_OBJ_NULL.
//
// For debugging purposes they are all different.  For non-debug mode, we alias
// as many as we can to MP_OBJ_NULL because it's cheaper to load/compare 0.

#ifdef NDEBUG
#define MP_OBJ_NULL             (MP_OBJ_FROM_PTR((void*)0))
#define MP_OBJ_STOP_ITERATION   (MP_OBJ_FROM_PTR((void*)0))
#define MP_OBJ_SENTINEL         (MP_OBJ_FROM_PTR((void*)4))
#else
#define MP_OBJ_NULL             (MP_OBJ_FROM_PTR((void*)0))
#define MP_OBJ_STOP_ITERATION   (MP_OBJ_FROM_PTR((void*)4))
#define MP_OBJ_SENTINEL         (MP_OBJ_FROM_PTR((void*)8))
#endif

// These macros/inline functions operate on objects and depend on the
// particular object representation.  They are used to query, pack and
// unpack small ints, qstrs and full object pointers.

#if MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_A

static inline bool MP_OBJ_IS_SMALL_INT(mp_const_obj_t o)
    { return ((((mp_int_t)(o)) & 1) != 0); }
#define MP_OBJ_SMALL_INT_VALUE(o) (((mp_int_t)(o)) >> 1)
#define MP_OBJ_NEW_SMALL_INT(small_int) ((mp_obj_t)((((mp_uint_t)(small_int)) << 1) | 1))

static inline bool MP_OBJ_IS_QSTR(mp_const_obj_t o)
    { return ((((mp_int_t)(o)) & 3) == 2); }
#define MP_OBJ_QSTR_VALUE(o) (((mp_uint_t)(o)) >> 2)
#define MP_OBJ_NEW_QSTR(qst) ((mp_obj_t)((((mp_uint_t)(qst)) << 2) | 2))

#if MICROPY_PY_BUILTINS_FLOAT
#define mp_const_float_e MP_ROM_PTR(&mp_const_float_e_obj)
#define mp_const_float_pi MP_ROM_PTR(&mp_const_float_pi_obj)
extern const struct _mp_obj_float_t mp_const_float_e_obj;
extern const struct _mp_obj_float_t mp_const_float_pi_obj;

#define micropy_obj_is_float(mp_state, o) MP_OBJ_IS_TYPE((o), &mp_type_float)
mp_float_t micropy_obj_float_get(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in);
mp_obj_t micropy_obj_new_float(struct _mp_state_ctx_t *mp_state, mp_float_t value);
#endif

static inline bool MP_OBJ_IS_OBJ(mp_const_obj_t o)
    { return ((((mp_int_t)(o)) & 3) == 0); }

#elif MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_B

static inline bool MP_OBJ_IS_SMALL_INT(mp_const_obj_t o)
    { return ((((mp_int_t)(o)) & 3) == 1); }
#define MP_OBJ_SMALL_INT_VALUE(o) (((mp_int_t)(o)) >> 2)
#define MP_OBJ_NEW_SMALL_INT(small_int) ((mp_obj_t)((((mp_uint_t)(small_int)) << 2) | 1))

static inline bool MP_OBJ_IS_QSTR(mp_const_obj_t o)
    { return ((((mp_int_t)(o)) & 3) == 3); }
#define MP_OBJ_QSTR_VALUE(o) (((mp_uint_t)(o)) >> 2)
#define MP_OBJ_NEW_QSTR(qst) ((mp_obj_t)((((mp_uint_t)(qst)) << 2) | 3))

#if MICROPY_PY_BUILTINS_FLOAT
#define mp_const_float_e MP_ROM_PTR(&mp_const_float_e_obj)
#define mp_const_float_pi MP_ROM_PTR(&mp_const_float_pi_obj)
extern const struct _mp_obj_float_t mp_const_float_e_obj;
extern const struct _mp_obj_float_t mp_const_float_pi_obj;

#define micropy_obj_is_float(mp_state, o) MP_OBJ_IS_TYPE((o), &mp_type_float)
mp_float_t micropy_obj_float_get(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in);
mp_obj_t micropy_obj_new_float(struct _mp_state_ctx_t *mp_state, mp_float_t value);
#endif

static inline bool MP_OBJ_IS_OBJ(mp_const_obj_t o)
    { return ((((mp_int_t)(o)) & 1) == 0); }

#elif MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_C

static inline bool MP_OBJ_IS_SMALL_INT(mp_const_obj_t o)
    { return ((((mp_int_t)(o)) & 1) != 0); }
#define MP_OBJ_SMALL_INT_VALUE(o) (((mp_int_t)(o)) >> 1)
#define MP_OBJ_NEW_SMALL_INT(small_int) ((mp_obj_t)((((mp_uint_t)(small_int)) << 1) | 1))

#define mp_const_float_e MP_ROM_PTR((mp_obj_t)(((0x402df854 & ~3) | 2) + 0x80800000))
#define mp_const_float_pi MP_ROM_PTR((mp_obj_t)(((0x40490fdb & ~3) | 2) + 0x80800000))

static inline bool mp_obj_is_float(mp_const_obj_t o)
    { return (((mp_uint_t)(o)) & 3) == 2 && (((mp_uint_t)(o)) & 0xff800007) != 0x00000006; }
static inline mp_float_t micropy_obj_float_get(struct _mp_state_ctx_t *mp_state, mp_const_obj_t o) {
    union {
        mp_float_t f;
        mp_uint_t u;
    } num = {.u = ((mp_uint_t)o - 0x80800000) & ~3};
    return num.f;
}
static inline mp_obj_t micropy_obj_new_float(struct _mp_state_ctx_t *mp_state, mp_float_t f) {
    union {
        mp_float_t f;
        mp_uint_t u;
    } num = {.f = f};
    return (mp_obj_t)(((num.u & ~0x3) | 2) + 0x80800000);
}

static inline bool MP_OBJ_IS_QSTR(mp_const_obj_t o)
    { return (((mp_uint_t)(o)) & 0xff800007) == 0x00000006; }
#define MP_OBJ_QSTR_VALUE(o) (((mp_uint_t)(o)) >> 3)
#define MP_OBJ_NEW_QSTR(qst) ((mp_obj_t)((((mp_uint_t)(qst)) << 3) | 0x00000006))

static inline bool MP_OBJ_IS_OBJ(mp_const_obj_t o)
    { return ((((mp_int_t)(o)) & 3) == 0); }

#elif MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_D

static inline bool MP_OBJ_IS_SMALL_INT(mp_const_obj_t o)
    { return ((((mp_int_t)(o)) & 0xffff000000000000) == 0x0001000000000000); }
#define MP_OBJ_SMALL_INT_VALUE(o) (((intptr_t)(o)) >> 1)
#define MP_OBJ_NEW_SMALL_INT(small_int) ((mp_obj_t)(((uintptr_t)(small_int)) << 1) | 0x0001000000000001)

static inline bool MP_OBJ_IS_QSTR(mp_const_obj_t o)
    { return ((((mp_int_t)(o)) & 0xffff000000000000) == 0x0002000000000000); }
#define MP_OBJ_QSTR_VALUE(o) ((((uint32_t)(o)) >> 1) & 0xffffffff)
#define MP_OBJ_NEW_QSTR(qst) ((mp_obj_t)((((mp_uint_t)(qst)) << 1) | 0x0002000000000001))

#if MICROPY_PY_BUILTINS_FLOAT
#define mp_const_float_e {((mp_obj_t)((uint64_t)0x4005bf0a8b125769 + 0x8004000000000000))}
#define mp_const_float_pi {((mp_obj_t)((uint64_t)0x400921fb54442d18 + 0x8004000000000000))}

static inline bool micropy_obj_is_float(struct _mp_state_ctx_t *mp_state, mp_const_obj_t o) {
    return ((uint64_t)(o) & 0xfffc000000000000) != 0;
}
static inline mp_float_t micropy_obj_float_get(struct _mp_state_ctx_t *mp_state, mp_const_obj_t o) {
    union {
        mp_float_t f;
        uint64_t r;
    } num = {.r = o - 0x8004000000000000};
    return num.f;
}
static inline mp_obj_t micropy_obj_new_float(struct _mp_state_ctx_t *mp_state, mp_float_t f) {
    union {
        mp_float_t f;
        uint64_t r;
    } num = {.f = f};
    return num.r + 0x8004000000000000;
}
#endif

static inline bool MP_OBJ_IS_OBJ(mp_const_obj_t o)
    { return ((((uint64_t)(o)) & 0xffff000000000000) == 0x0000000000000000); }
#define MP_OBJ_TO_PTR(o) ((void*)(uintptr_t)(o))
#define MP_OBJ_FROM_PTR(p) ((mp_obj_t)((uintptr_t)(p)))

// rom object storage needs special handling to widen 32-bit pointer to 64-bits
typedef union _mp_rom_obj_t { uint64_t u64; struct { const void *lo, *hi; } u32; } mp_rom_obj_t;
#define MP_ROM_INT(i) {MP_OBJ_NEW_SMALL_INT(i)}
#define MP_ROM_QSTR(q) {MP_OBJ_NEW_QSTR(q)}
#if MP_ENDIANNESS_LITTLE
#define MP_ROM_PTR(p) {.u32 = {.lo = (p), .hi = NULL}}
#else
#define MP_ROM_PTR(p) {.u32 = {.lo = NULL, .hi = (p)}}
#endif

#endif

// Macros to convert between mp_obj_t and concrete object types.
// These are identity operations in MicroPython, but ability to override
// these operations are provided to experiment with other methods of
// object representation and memory management.

// Cast mp_obj_t to object pointer
#ifndef MP_OBJ_TO_PTR
#define MP_OBJ_TO_PTR(o) ((void*)o)
#endif

// Cast object pointer to mp_obj_t
#ifndef MP_OBJ_FROM_PTR
#define MP_OBJ_FROM_PTR(p) ((mp_obj_t)p)
#endif

// Macros to create objects that are stored in ROM.

#ifndef MP_ROM_INT
typedef mp_const_obj_t mp_rom_obj_t;
#define MP_ROM_INT(i) MP_OBJ_NEW_SMALL_INT(i)
#define MP_ROM_QSTR(q) MP_OBJ_NEW_QSTR(q)
#define MP_ROM_PTR(p) (p)
/* for testing
typedef struct _mp_rom_obj_t { mp_const_obj_t o; } mp_rom_obj_t;
#define MP_ROM_INT(i) {MP_OBJ_NEW_SMALL_INT(i)}
#define MP_ROM_QSTR(q) {MP_OBJ_NEW_QSTR(q)}
#define MP_ROM_PTR(p) {.o = p}
*/
#endif

// The macros below are derived from the ones above and are used to
// check for more specific object types.

#define MP_OBJ_IS_TYPE(o, t) (MP_OBJ_IS_OBJ(o) && (((mp_obj_base_t*)MP_OBJ_TO_PTR(o))->type == (t))) // this does not work for checking int, str or fun; use below macros for that
#define MP_OBJ_IS_INT(o) (MP_OBJ_IS_SMALL_INT(o) || MP_OBJ_IS_TYPE(o, &mp_type_int))
#define MP_OBJ_IS_STR(o) (MP_OBJ_IS_QSTR(o) || MP_OBJ_IS_TYPE(o, &mp_type_str))
#define MP_OBJ_IS_STR_OR_BYTES(o) (MP_OBJ_IS_QSTR(o) || (MP_OBJ_IS_OBJ(o) && ((mp_obj_base_t*)MP_OBJ_TO_PTR(o))->type->binary_op == micropy_obj_str_binary_op))
#define MP_OBJ_IS_FUN(o) (MP_OBJ_IS_OBJ(o) && (((mp_obj_base_t*)MP_OBJ_TO_PTR(o))->type->name == MP_QSTR_function))

// Note: inline functions sometimes use much more code space than the
// equivalent macros, depending on the compiler.
//static inline bool MP_OBJ_IS_TYPE(mp_const_obj_t o, const mp_obj_type_t *t) { return (MP_OBJ_IS_OBJ(o) && (((mp_obj_base_t*)(o))->type == (t))); } // this does not work for checking a string, use below macro for that
//static inline bool MP_OBJ_IS_INT(mp_const_obj_t o) { return (MP_OBJ_IS_SMALL_INT(o) || MP_OBJ_IS_TYPE(o, &mp_type_int)); } // returns true if o is a small int or long int
// Need to forward declare these for the inline function to compile.
extern const mp_obj_type_t mp_type_int;
extern const mp_obj_type_t mp_type_bool;
static inline bool micropy_obj_is_integer(struct _mp_state_ctx_t *mp_state, mp_const_obj_t o) { return MP_OBJ_IS_INT(o) || MP_OBJ_IS_TYPE(o, &mp_type_bool); } // returns true if o is bool, small int or long int
//static inline bool MP_OBJ_IS_STR(mp_const_obj_t o) { return (MP_OBJ_IS_QSTR(o) || MP_OBJ_IS_TYPE(o, &mp_type_str)); }


// These macros are used to declare and define constant function objects
// You can put "static" in front of the definitions to make them local

#define MP_DECLARE_CONST_FUN_OBJ(obj_name) extern const mp_obj_fun_builtin_t obj_name

#define MP_DEFINE_CONST_FUN_OBJ_0(obj_name, fun_name) \
    const mp_obj_fun_builtin_t obj_name = \
        {{&mp_type_fun_builtin}, false, 0, 0, .fun._0 = fun_name}
#define MP_DEFINE_CONST_FUN_OBJ_1(obj_name, fun_name) \
    const mp_obj_fun_builtin_t obj_name = \
        {{&mp_type_fun_builtin}, false, 1, 1, .fun._1 = fun_name}
#define MP_DEFINE_CONST_FUN_OBJ_2(obj_name, fun_name) \
    const mp_obj_fun_builtin_t obj_name = \
        {{&mp_type_fun_builtin}, false, 2, 2, .fun._2 = fun_name}
#define MP_DEFINE_CONST_FUN_OBJ_3(obj_name, fun_name) \
    const mp_obj_fun_builtin_t obj_name = \
        {{&mp_type_fun_builtin}, false, 3, 3, .fun._3 = fun_name}
#define MP_DEFINE_CONST_FUN_OBJ_VAR(obj_name, n_args_min, fun_name) \
    const mp_obj_fun_builtin_t obj_name = \
        {{&mp_type_fun_builtin}, false, n_args_min, MP_OBJ_FUN_ARGS_MAX, .fun.var = fun_name}
#define MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(obj_name, n_args_min, n_args_max, fun_name) \
    const mp_obj_fun_builtin_t obj_name = \
        {{&mp_type_fun_builtin}, false, n_args_min, n_args_max, .fun.var = fun_name}
#define MP_DEFINE_CONST_FUN_OBJ_KW(obj_name, n_args_min, fun_name) \
    const mp_obj_fun_builtin_t obj_name = \
        {{&mp_type_fun_builtin}, true, n_args_min, MP_OBJ_FUN_ARGS_MAX, .fun.kw = fun_name}

// These macros are used to define constant map/dict objects
// You can put "static" in front of the definition to make it local

#define MP_DEFINE_CONST_MAP(map_name, table_name) \
    const mp_map_t map_name = { \
        .all_keys_are_qstrs = 1, \
        .is_fixed = 1, \
        .is_ordered = 1, \
        .used = MP_ARRAY_SIZE(table_name), \
        .alloc = MP_ARRAY_SIZE(table_name), \
        .table = (mp_map_elem_t*)(mp_rom_map_elem_t*)table_name, \
    }

#define MP_DEFINE_CONST_DICT(dict_name, table_name) \
    const mp_obj_dict_t dict_name = { \
        .base = {&mp_type_dict}, \
        .map = { \
            .all_keys_are_qstrs = 1, \
            .is_fixed = 1, \
            .is_ordered = 1, \
            .used = MP_ARRAY_SIZE(table_name), \
            .alloc = MP_ARRAY_SIZE(table_name), \
            .table = (mp_map_elem_t*)(mp_rom_map_elem_t*)table_name, \
        }, \
    }

// These macros are used to declare and define constant staticmethond and classmethod objects
// You can put "static" in front of the definitions to make them local

#define MP_DECLARE_CONST_STATICMETHOD_OBJ(obj_name) extern const mp_rom_obj_static_class_method_t obj_name
#define MP_DECLARE_CONST_CLASSMETHOD_OBJ(obj_name) extern const mp_rom_obj_static_class_method_t obj_name

#define MP_DEFINE_CONST_STATICMETHOD_OBJ(obj_name, fun_name) const mp_rom_obj_static_class_method_t obj_name = {{&mp_type_staticmethod}, fun_name}
#define MP_DEFINE_CONST_CLASSMETHOD_OBJ(obj_name, fun_name) const mp_rom_obj_static_class_method_t obj_name = {{&mp_type_classmethod}, fun_name}

// Underlying map/hash table implementation (not dict object or map function)

typedef struct _mp_map_elem_t {
    mp_obj_t key;
    mp_obj_t value;
} mp_map_elem_t;

typedef struct _mp_rom_map_elem_t {
    mp_rom_obj_t key;
    mp_rom_obj_t value;
} mp_rom_map_elem_t;

// TODO maybe have a truncated mp_map_t for fixed tables, since alloc=used
// put alloc last in the structure, so the truncated version does not need it
// this would save 1 ROM word for all ROM objects that have a locals_dict
// would also need a trucated dict structure

typedef struct _mp_map_t {
    mp_uint_t all_keys_are_qstrs : 1;
    mp_uint_t is_fixed : 1;     // a fixed array that can't be modified; must also be ordered
    mp_uint_t is_ordered : 1;   // an ordered array
    mp_uint_t used : (8 * sizeof(mp_uint_t) - 3);
    mp_uint_t alloc;
    mp_map_elem_t *table;
} mp_map_t;

// mp_set_lookup requires these constants to have the values they do
typedef enum _mp_map_lookup_kind_t {
    MP_MAP_LOOKUP = 0,
    MP_MAP_LOOKUP_ADD_IF_NOT_FOUND = 1,
    MP_MAP_LOOKUP_REMOVE_IF_FOUND = 2,
    MP_MAP_LOOKUP_ADD_IF_NOT_FOUND_OR_REMOVE_IF_FOUND = 3, // only valid for mp_set_lookup
} mp_map_lookup_kind_t;

extern const mp_map_t mp_const_empty_map;

static inline bool MP_MAP_SLOT_IS_FILLED(const mp_map_t *map, mp_uint_t pos) { return ((map)->table[pos].key != MP_OBJ_NULL && (map)->table[pos].key != MP_OBJ_SENTINEL); }

void micropy_map_init(struct _mp_state_ctx_t *mp_state, mp_map_t *map, mp_uint_t n);
void micropy_map_init_fixed_table(struct _mp_state_ctx_t *mp_state, mp_map_t *map, mp_uint_t n, const mp_obj_t *table);
mp_map_t *micropy_map_new(struct _mp_state_ctx_t *mp_state, mp_uint_t n);
void micropy_map_deinit(struct _mp_state_ctx_t *mp_state, mp_map_t *map);
void micropy_map_free(struct _mp_state_ctx_t *mp_state, mp_map_t *map);
mp_map_elem_t *micropy_map_lookup(struct _mp_state_ctx_t *mp_state, mp_map_t *map, mp_obj_t index, mp_map_lookup_kind_t lookup_kind);
void micropy_map_clear(struct _mp_state_ctx_t *mp_state, mp_map_t *map);
void micropy_map_dump(struct _mp_state_ctx_t *mp_state, mp_map_t *map);

// Underlying set implementation (not set object)

typedef struct _mp_set_t {
    mp_uint_t alloc;
    mp_uint_t used;
    mp_obj_t *table;
} mp_set_t;

static inline bool MP_SET_SLOT_IS_FILLED(const mp_set_t *set, mp_uint_t pos) { return ((set)->table[pos] != MP_OBJ_NULL && (set)->table[pos] != MP_OBJ_SENTINEL); }

void micropy_set_init(struct _mp_state_ctx_t *mp_state, mp_set_t *set, mp_uint_t n);
mp_obj_t micropy_set_lookup(struct _mp_state_ctx_t *mp_state, mp_set_t *set, mp_obj_t index, mp_map_lookup_kind_t lookup_kind);
mp_obj_t micropy_set_remove_first(struct _mp_state_ctx_t *mp_state, mp_set_t *set);
void micropy_set_clear(struct _mp_state_ctx_t *mp_state, mp_set_t *set);

// Type definitions for methods

typedef mp_obj_t (*mp_fun_0_t)(struct _mp_state_ctx_t *mp_state);
typedef mp_obj_t (*mp_fun_1_t)(struct _mp_state_ctx_t *mp_state, mp_obj_t);
typedef mp_obj_t (*mp_fun_2_t)(struct _mp_state_ctx_t *mp_state, mp_obj_t, mp_obj_t);
typedef mp_obj_t (*mp_fun_3_t)(struct _mp_state_ctx_t *mp_state, mp_obj_t, mp_obj_t, mp_obj_t);
typedef mp_obj_t (*mp_fun_var_t)(struct _mp_state_ctx_t *mp_state, size_t n, const mp_obj_t *);
// mp_fun_kw_t takes mp_map_t* (and not const mp_map_t*) to ease passing
// this arg to mp_map_lookup().
typedef mp_obj_t (*mp_fun_kw_t)(struct _mp_state_ctx_t *mp_state, size_t n, const mp_obj_t *, mp_map_t *);

typedef enum {
    PRINT_STR = 0,
    PRINT_REPR = 1,
    PRINT_EXC = 2, // Special format for printing exception in unhandled exception message
    PRINT_JSON = 3,
    PRINT_RAW = 4, // Special format for printing bytes as an undercorated string
    PRINT_EXC_SUBCLASS = 0x80, // Internal flag for printing exception subclasses
} mp_print_kind_t;

typedef void (*mp_print_fun_t)(struct _mp_state_ctx_t *mp_state, const mp_print_t *print, mp_obj_t o, mp_print_kind_t kind);
typedef mp_obj_t (*mp_make_new_fun_t)(struct _mp_state_ctx_t *mp_state, const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
typedef mp_obj_t (*mp_call_fun_t)(struct _mp_state_ctx_t *mp_state, mp_obj_t fun, size_t n_args, size_t n_kw, const mp_obj_t *args);
typedef mp_obj_t (*mp_unary_op_fun_t)(struct _mp_state_ctx_t *mp_state, mp_uint_t op, mp_obj_t);
typedef mp_obj_t (*mp_binary_op_fun_t)(struct _mp_state_ctx_t *mp_state, mp_uint_t op, mp_obj_t, mp_obj_t);
typedef void (*mp_attr_fun_t)(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, qstr attr, mp_obj_t *dest);
typedef mp_obj_t (*mp_subscr_fun_t)(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, mp_obj_t index, mp_obj_t value);

// Buffer protocol
typedef struct _mp_buffer_info_t {
    // if we'd bother to support various versions of structure
    // (with different number of fields), we can distinguish
    // them with ver = sizeof(struct). Cons: overkill for *micro*?
    //int ver; // ?

    void *buf;      // can be NULL if len == 0
    size_t len;     // in bytes
    int typecode;   // as per binary.h

    // Rationale: to load arbitrary-sized sprites directly to LCD
    // Cons: a bit adhoc usecase
    // int stride;
} mp_buffer_info_t;
#define MP_BUFFER_READ  (1)
#define MP_BUFFER_WRITE (2)
#define MP_BUFFER_RW (MP_BUFFER_READ | MP_BUFFER_WRITE)
typedef struct _mp_buffer_p_t {
    mp_int_t (*get_buffer)(struct _mp_state_ctx_t *mp_state, mp_obj_t obj, mp_buffer_info_t *bufinfo, mp_uint_t flags);
} mp_buffer_p_t;
bool micropy_get_buffer(struct _mp_state_ctx_t *mp_state, mp_obj_t obj, mp_buffer_info_t *bufinfo, mp_uint_t flags);
void micropy_get_buffer_raise(struct _mp_state_ctx_t *mp_state, mp_obj_t obj, mp_buffer_info_t *bufinfo, mp_uint_t flags);

// Stream protocol
typedef struct _mp_stream_p_t {
    // On error, functions should return MP_STREAM_ERROR and fill in *errcode (values
    // are implementation-dependent, but will be exposed to user, e.g. via exception).
    mp_uint_t (*read)(struct _mp_state_ctx_t *mp_state, mp_obj_t obj, void *buf, mp_uint_t size, int *errcode);
    mp_uint_t (*write)(struct _mp_state_ctx_t *mp_state, mp_obj_t obj, const void *buf, mp_uint_t size, int *errcode);
    mp_uint_t (*ioctl)(struct _mp_state_ctx_t *mp_state, mp_obj_t obj, mp_uint_t request, uintptr_t arg, int *errcode);
    mp_uint_t is_text : 1; // default is bytes, set this for text stream
} mp_stream_p_t;

struct _mp_obj_type_t {
    mp_obj_base_t base;
    qstr name;
    mp_print_fun_t print;
    mp_make_new_fun_t make_new;     // to make an instance of the type

    mp_call_fun_t call;
    mp_unary_op_fun_t unary_op;     // can return MP_OBJ_NULL if op not supported
    mp_binary_op_fun_t binary_op;   // can return MP_OBJ_NULL if op not supported

    // implements load, store and delete attribute
    //
    // dest[0] = MP_OBJ_NULL means load
    //  return: for fail, do nothing
    //          for attr, dest[0] = value
    //          for method, dest[0] = method, dest[1] = self
    //
    // dest[0,1] = {MP_OBJ_SENTINEL, MP_OBJ_NULL} means delete
    // dest[0,1] = {MP_OBJ_SENTINEL, object} means store
    //  return: for fail, do nothing
    //          for success set dest[0] = MP_OBJ_NULL
    mp_attr_fun_t attr;

    mp_subscr_fun_t subscr;         // implements load, store, delete subscripting
                                    // value=MP_OBJ_NULL means delete, value=MP_OBJ_SENTINEL means load, else store
                                    // can return MP_OBJ_NULL if op not supported

    mp_fun_1_t getiter;             // corresponds to __iter__ special method
    mp_fun_1_t iternext; // may return MP_OBJ_STOP_ITERATION as an optimisation instead of raising StopIteration() (with no args)

    mp_buffer_p_t buffer_p;
    const mp_stream_p_t *stream_p;

    // these are for dynamically created types (classes)
    struct _mp_obj_tuple_t *bases_tuple;
    struct _mp_obj_dict_t *locals_dict;

    /*
    What we might need to add here:

    len             str tuple list map
    abs             float complex
    hash            bool int none str
    equal           int str

    unpack seq      list tuple
    */
};

// Constant types, globally accessible
extern const mp_obj_type_t mp_type_type;
extern const mp_obj_type_t mp_type_object;
extern const mp_obj_type_t mp_type_NoneType;
extern const mp_obj_type_t mp_type_bool;
extern const mp_obj_type_t mp_type_int;
extern const mp_obj_type_t mp_type_str;
extern const mp_obj_type_t mp_type_bytes;
extern const mp_obj_type_t mp_type_bytearray;
extern const mp_obj_type_t mp_type_memoryview;
extern const mp_obj_type_t mp_type_float;
extern const mp_obj_type_t mp_type_complex;
extern const mp_obj_type_t mp_type_tuple;
extern const mp_obj_type_t mp_type_list;
extern const mp_obj_type_t mp_type_map; // map (the python builtin, not the dict implementation detail)
extern const mp_obj_type_t mp_type_enumerate;
extern const mp_obj_type_t mp_type_filter;
extern const mp_obj_type_t mp_type_dict;
extern const mp_obj_type_t mp_type_ordereddict;
extern const mp_obj_type_t mp_type_range;
extern const mp_obj_type_t mp_type_set;
extern const mp_obj_type_t mp_type_frozenset;
extern const mp_obj_type_t mp_type_slice;
extern const mp_obj_type_t mp_type_zip;
extern const mp_obj_type_t mp_type_array;
extern const mp_obj_type_t mp_type_super;
extern const mp_obj_type_t mp_type_gen_instance;
extern const mp_obj_type_t mp_type_fun_builtin;
extern const mp_obj_type_t mp_type_fun_bc;
extern const mp_obj_type_t mp_type_module;
extern const mp_obj_type_t mp_type_staticmethod;
extern const mp_obj_type_t mp_type_classmethod;
extern const mp_obj_type_t mp_type_property;
extern const mp_obj_type_t mp_type_stringio;
extern const mp_obj_type_t mp_type_bytesio;
extern const mp_obj_type_t mp_type_reversed;
extern const mp_obj_type_t mp_type_polymorph_iter;

// Exceptions
extern const mp_obj_type_t mp_type_BaseException;
extern const mp_obj_type_t mp_type_ArithmeticError;
extern const mp_obj_type_t mp_type_AssertionError;
extern const mp_obj_type_t mp_type_AttributeError;
extern const mp_obj_type_t mp_type_EOFError;
extern const mp_obj_type_t mp_type_Exception;
extern const mp_obj_type_t mp_type_GeneratorExit;
extern const mp_obj_type_t mp_type_ImportError;
extern const mp_obj_type_t mp_type_IndentationError;
extern const mp_obj_type_t mp_type_IndexError;
extern const mp_obj_type_t mp_type_KeyboardInterrupt;
extern const mp_obj_type_t mp_type_KeyError;
extern const mp_obj_type_t mp_type_LookupError;
extern const mp_obj_type_t mp_type_MemoryError;
extern const mp_obj_type_t mp_type_NameError;
extern const mp_obj_type_t mp_type_NotImplementedError;
extern const mp_obj_type_t mp_type_OSError;
extern const mp_obj_type_t mp_type_TimeoutError;
extern const mp_obj_type_t mp_type_OverflowError;
extern const mp_obj_type_t mp_type_RuntimeError;
extern const mp_obj_type_t mp_type_StopAsyncIteration;
extern const mp_obj_type_t mp_type_StopIteration;
extern const mp_obj_type_t mp_type_SyntaxError;
extern const mp_obj_type_t mp_type_SystemExit;
extern const mp_obj_type_t mp_type_TypeError;
extern const mp_obj_type_t mp_type_UnicodeError;
extern const mp_obj_type_t mp_type_ValueError;
extern const mp_obj_type_t mp_type_ViperTypeError;
extern const mp_obj_type_t mp_type_ZeroDivisionError;

// Constant objects, globally accessible
// The macros are for convenience only
#define mp_const_none (MP_OBJ_FROM_PTR(&mp_const_none_obj))
#define mp_const_false (MP_OBJ_FROM_PTR(&mp_const_false_obj))
#define mp_const_true (MP_OBJ_FROM_PTR(&mp_const_true_obj))
#define mp_const_empty_bytes (MP_OBJ_FROM_PTR(&mp_const_empty_bytes_obj))
#define mp_const_empty_tuple (MP_OBJ_FROM_PTR(&mp_const_empty_tuple_obj))
extern const struct _mp_obj_none_t mp_const_none_obj;
extern const struct _mp_obj_bool_t mp_const_false_obj;
extern const struct _mp_obj_bool_t mp_const_true_obj;
extern const struct _mp_obj_str_t mp_const_empty_bytes_obj;
extern const struct _mp_obj_tuple_t mp_const_empty_tuple_obj;
extern const struct _mp_obj_singleton_t mp_const_ellipsis_obj;
extern const struct _mp_obj_singleton_t mp_const_notimplemented_obj;
extern const struct _mp_obj_exception_t mp_const_MemoryError_obj;
extern const struct _mp_obj_exception_t mp_const_GeneratorExit_obj;

// General API for objects

mp_obj_t micropy_obj_new_type(struct _mp_state_ctx_t *mp_state, qstr name, mp_obj_t bases_tuple, mp_obj_t locals_dict);
mp_obj_t micropy_obj_new_none(struct _mp_state_ctx_t *mp_state);
static inline mp_obj_t micropy_obj_new_bool(struct _mp_state_ctx_t *mp_state, mp_int_t x) { return x ? mp_const_true : mp_const_false; }
mp_obj_t micropy_obj_new_cell(struct _mp_state_ctx_t *mp_state, mp_obj_t obj);
mp_obj_t micropy_obj_new_int(struct _mp_state_ctx_t *mp_state, mp_int_t value);
mp_obj_t micropy_obj_new_int_from_uint(struct _mp_state_ctx_t *mp_state, mp_uint_t value);
mp_obj_t micropy_obj_new_int_from_str_len(struct _mp_state_ctx_t *mp_state, const char **str, mp_uint_t len, bool neg, mp_uint_t base);
mp_obj_t micropy_obj_new_int_from_ll(struct _mp_state_ctx_t *mp_state, long long val); // this must return a multi-precision integer object (or raise an overflow exception)
mp_obj_t micropy_obj_new_int_from_ull(struct _mp_state_ctx_t *mp_state, unsigned long long val); // this must return a multi-precision integer object (or raise an overflow exception)
mp_obj_t micropy_obj_new_str(struct _mp_state_ctx_t *mp_state, const char* data, mp_uint_t len, bool make_qstr_if_not_already);
mp_obj_t micropy_obj_new_str_from_vstr(struct _mp_state_ctx_t *mp_state, const mp_obj_type_t *type, vstr_t *vstr);
mp_obj_t micropy_obj_new_bytes(struct _mp_state_ctx_t *mp_state, const byte* data, mp_uint_t len);
mp_obj_t micropy_obj_new_bytearray(struct _mp_state_ctx_t *mp_state, mp_uint_t n, void *items);
mp_obj_t micropy_obj_new_bytearray_by_ref(struct _mp_state_ctx_t *mp_state, mp_uint_t n, void *items);
#if MICROPY_PY_BUILTINS_FLOAT
mp_obj_t micropy_obj_new_int_from_float(struct _mp_state_ctx_t *mp_state, mp_float_t val);
mp_obj_t micropy_obj_new_complex(struct _mp_state_ctx_t *mp_state, mp_float_t real, mp_float_t imag);
#endif
mp_obj_t micropy_obj_new_exception(struct _mp_state_ctx_t *mp_state, const mp_obj_type_t *exc_type);
mp_obj_t micropy_obj_new_exception_arg1(struct _mp_state_ctx_t *mp_state, const mp_obj_type_t *exc_type, mp_obj_t arg);
mp_obj_t micropy_obj_new_exception_args(struct _mp_state_ctx_t *mp_state, const mp_obj_type_t *exc_type, mp_uint_t n_args, const mp_obj_t *args);
mp_obj_t micropy_obj_new_exception_msg(struct _mp_state_ctx_t *mp_state, const mp_obj_type_t *exc_type, const char *msg);
mp_obj_t micropy_obj_new_exception_msg_varg(struct _mp_state_ctx_t *mp_state, const mp_obj_type_t *exc_type, const char *fmt, ...); // counts args by number of % symbols in fmt, excluding %%; can only handle void* sizes (ie no float/double!)
mp_obj_t micropy_obj_new_fun_bc(struct _mp_state_ctx_t *mp_state, mp_obj_t def_args, mp_obj_t def_kw_args, const byte *code, const mp_uint_t *const_table);
mp_obj_t micropy_obj_new_fun_native(struct _mp_state_ctx_t *mp_state, mp_obj_t def_args_in, mp_obj_t def_kw_args, const void *fun_data, const mp_uint_t *const_table);
mp_obj_t micropy_obj_new_fun_viper(struct _mp_state_ctx_t *mp_state, mp_uint_t n_args, void *fun_data, mp_uint_t type_sig);
mp_obj_t micropy_obj_new_fun_asm(struct _mp_state_ctx_t *mp_state, mp_uint_t n_args, void *fun_data, mp_uint_t type_sig);
mp_obj_t micropy_obj_new_gen_wrap(struct _mp_state_ctx_t *mp_state, mp_obj_t fun);
mp_obj_t micropy_obj_new_closure(struct _mp_state_ctx_t *mp_state, mp_obj_t fun, mp_uint_t n_closed, const mp_obj_t *closed);
mp_obj_t micropy_obj_new_tuple(struct _mp_state_ctx_t *mp_state, mp_uint_t n, const mp_obj_t *items);
mp_obj_t micropy_obj_new_list(struct _mp_state_ctx_t *mp_state, mp_uint_t n, mp_obj_t *items);
mp_obj_t micropy_obj_new_dict(struct _mp_state_ctx_t *mp_state, mp_uint_t n_args);
mp_obj_t micropy_obj_new_set(struct _mp_state_ctx_t *mp_state, mp_uint_t n_args, mp_obj_t *items);
mp_obj_t micropy_obj_new_slice(struct _mp_state_ctx_t *mp_state, mp_obj_t start, mp_obj_t stop, mp_obj_t step);
mp_obj_t micropy_obj_new_super(struct _mp_state_ctx_t *mp_state, mp_obj_t type, mp_obj_t obj);
mp_obj_t micropy_obj_new_bound_meth(struct _mp_state_ctx_t *mp_state, mp_obj_t meth, mp_obj_t self);
mp_obj_t micropy_obj_new_getitem_iter(struct _mp_state_ctx_t *mp_state, mp_obj_t *args);
mp_obj_t micropy_obj_new_module(struct _mp_state_ctx_t *mp_state, qstr module_name);
mp_obj_t micropy_obj_new_memoryview(struct _mp_state_ctx_t *mp_state, byte typecode, mp_uint_t nitems, void *items);

mp_obj_type_t *micropy_obj_get_type(struct _mp_state_ctx_t *mp_state, mp_const_obj_t o_in);
const char *micropy_obj_get_type_str(struct _mp_state_ctx_t *mp_state, mp_const_obj_t o_in);
bool micropy_obj_is_subclass_fast(struct _mp_state_ctx_t *mp_state, mp_const_obj_t object, mp_const_obj_t classinfo); // arguments should be type objects
mp_obj_t micropy_instance_cast_to_native_base(struct _mp_state_ctx_t *mp_state, mp_const_obj_t self_in, mp_const_obj_t native_type);

void micropy_obj_print_helper(struct _mp_state_ctx_t *mp_state, const mp_print_t *print, mp_obj_t o_in, mp_print_kind_t kind);
void micropy_obj_print(struct _mp_state_ctx_t *mp_state, mp_obj_t o, mp_print_kind_t kind);
void micropy_obj_print_exception(struct _mp_state_ctx_t *mp_state, const mp_print_t *print, mp_obj_t exc);

bool micropy_obj_is_true(struct _mp_state_ctx_t *mp_state, mp_obj_t arg);
bool micropy_obj_is_callable(struct _mp_state_ctx_t *mp_state, mp_obj_t o_in);
bool micropy_obj_equal(struct _mp_state_ctx_t *mp_state, mp_obj_t o1, mp_obj_t o2);

mp_int_t micropy_obj_get_int(struct _mp_state_ctx_t *mp_state, mp_const_obj_t arg);
mp_int_t micropy_obj_get_int_truncated(struct _mp_state_ctx_t *mp_state, mp_const_obj_t arg);
bool micropy_obj_get_int_maybe(struct _mp_state_ctx_t *mp_state, mp_const_obj_t arg, mp_int_t *value);
#if MICROPY_PY_BUILTINS_FLOAT
mp_float_t micropy_obj_get_float(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in);
void micropy_obj_get_complex(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, mp_float_t *real, mp_float_t *imag);
#endif
//qstr mp_obj_get_qstr(mp_obj_t arg);
void micropy_obj_get_array(struct _mp_state_ctx_t *mp_state, mp_obj_t o, mp_uint_t *len, mp_obj_t **items); // *items may point inside a GC block
void micropy_obj_get_array_fixed_n(struct _mp_state_ctx_t *mp_state, mp_obj_t o, mp_uint_t len, mp_obj_t **items); // *items may point inside a GC block
mp_uint_t micropy_get_index(struct _mp_state_ctx_t *mp_state, const mp_obj_type_t *type, mp_uint_t len, mp_obj_t index, bool is_slice);
mp_obj_t micropy_obj_id(struct _mp_state_ctx_t *mp_state, mp_obj_t o_in);
mp_obj_t micropy_obj_len(struct _mp_state_ctx_t *mp_state, mp_obj_t o_in);
mp_obj_t micropy_obj_len_maybe(struct _mp_state_ctx_t *mp_state, mp_obj_t o_in); // may return MP_OBJ_NULL
mp_obj_t micropy_obj_subscr(struct _mp_state_ctx_t *mp_state, mp_obj_t base, mp_obj_t index, mp_obj_t val);
mp_obj_t micropy_generic_unary_op(struct _mp_state_ctx_t *mp_state, mp_uint_t op, mp_obj_t o_in);

// cell
mp_obj_t micropy_obj_cell_get(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in);
void micropy_obj_cell_set(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, mp_obj_t obj);

// int
// For long int, returns value truncated to mp_int_t
mp_int_t micropy_obj_int_get_truncated(struct _mp_state_ctx_t *mp_state, mp_const_obj_t self_in);
// Will raise exception if value doesn't fit into mp_int_t
mp_int_t micropy_obj_int_get_checked(struct _mp_state_ctx_t *mp_state, mp_const_obj_t self_in);
#if MICROPY_PY_BUILTINS_FLOAT
mp_float_t micropy_obj_int_as_float(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in);
#endif

// exception
#define micropy_obj_is_native_exception_instance(mp_state, o) (micropy_obj_get_type(mp_state, o)->make_new == micropy_obj_exception_make_new)
bool micropy_obj_is_exception_type(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in);
bool micropy_obj_is_exception_instance(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in);
bool micropy_obj_exception_match(struct _mp_state_ctx_t *mp_state, mp_obj_t exc, mp_const_obj_t exc_type);
void micropy_obj_exception_clear_traceback(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in);
void micropy_obj_exception_add_traceback(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, qstr file, size_t line, qstr block);
void micropy_obj_exception_get_traceback(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, size_t *n, size_t **values);
mp_obj_t micropy_obj_exception_get_value(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in);
mp_obj_t micropy_obj_exception_make_new(struct _mp_state_ctx_t *mp_state, const mp_obj_type_t *type_in, size_t n_args, size_t n_kw, const mp_obj_t *args);
mp_obj_t micropy_alloc_emergency_exception_buf(struct _mp_state_ctx_t *mp_state, mp_obj_t size_in);
void micropy_init_emergency_exception_buf(struct _mp_state_ctx_t *mp_state);

// str
bool micropy_obj_str_equal(struct _mp_state_ctx_t *mp_state, mp_obj_t s1, mp_obj_t s2);
qstr micropy_obj_str_get_qstr(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in); // use this if you will anyway convert the string to a qstr
const char *micropy_obj_str_get_str(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in); // use this only if you need the string to be null terminated
const char *micropy_obj_str_get_data(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, mp_uint_t *len);
mp_obj_t micropy_obj_str_intern(struct _mp_state_ctx_t *mp_state, mp_obj_t str);
void micropy_str_print_quoted(struct _mp_state_ctx_t *mp_state, const mp_print_t *print, const byte *str_data, mp_uint_t str_len, bool is_bytes);

#if MICROPY_PY_BUILTINS_FLOAT
// float
mp_obj_t micropy_obj_float_binary_op(struct _mp_state_ctx_t *mp_state, mp_uint_t op, mp_float_t lhs_val, mp_obj_t rhs); // can return MP_OBJ_NULL if op not supported

// complex
void micropy_obj_complex_get(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, mp_float_t *real, mp_float_t *imag);
mp_obj_t micropy_obj_complex_binary_op(struct _mp_state_ctx_t *mp_state, mp_uint_t op, mp_float_t lhs_real, mp_float_t lhs_imag, mp_obj_t rhs_in); // can return MP_OBJ_NULL if op not supported
#else
#define micropy_obj_is_float(mp_state, o) (false)
#endif

// tuple
void micropy_obj_tuple_get(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, mp_uint_t *len, mp_obj_t **items);
void micropy_obj_tuple_del(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in);
mp_int_t micropy_obj_tuple_hash(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in);

// list
struct _mp_obj_list_t;
void micropy_obj_list_init(struct _mp_state_ctx_t *mp_state, struct _mp_obj_list_t *o, mp_uint_t n);
mp_obj_t micropy_obj_list_append(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, mp_obj_t arg);
mp_obj_t micropy_obj_list_remove(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, mp_obj_t value);
void micropy_obj_list_get(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, mp_uint_t *len, mp_obj_t **items);
void micropy_obj_list_set_len(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, mp_uint_t len);
void micropy_obj_list_store(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, mp_obj_t index, mp_obj_t value);
mp_obj_t micropy_obj_list_sort(struct _mp_state_ctx_t *mp_state, size_t n_args, const mp_obj_t *args, mp_map_t *kwargs);

// dict
typedef struct _mp_obj_dict_t {
    mp_obj_base_t base;
    mp_map_t map;
} mp_obj_dict_t;
void micropy_obj_dict_init(struct _mp_state_ctx_t *mp_state, mp_obj_dict_t *dict, mp_uint_t n_args);
mp_uint_t micropy_obj_dict_len(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in);
mp_obj_t micropy_obj_dict_get(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, mp_obj_t index);
mp_obj_t micropy_obj_dict_store(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, mp_obj_t key, mp_obj_t value);
mp_obj_t micropy_obj_dict_delete(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, mp_obj_t key);
mp_map_t *micropy_obj_dict_get_map(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in);

// set
void micropy_obj_set_store(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, mp_obj_t item);

// slice
void micropy_obj_slice_get(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, mp_obj_t *start, mp_obj_t *stop, mp_obj_t *step);

// functions
#define MP_OBJ_FUN_ARGS_MAX (0xffff) // to set maximum value in n_args_max below
typedef struct _mp_obj_fun_builtin_t { // use this to make const objects that go in ROM
    mp_obj_base_t base;
    bool is_kw : 1;
    mp_uint_t n_args_min : 15; // inclusive
    mp_uint_t n_args_max : 16; // inclusive
    union {
        mp_fun_0_t _0;
        mp_fun_1_t _1;
        mp_fun_2_t _2;
        mp_fun_3_t _3;
        mp_fun_var_t var;
        mp_fun_kw_t kw;
    } fun;
} mp_obj_fun_builtin_t;

qstr micropy_obj_fun_get_name(struct _mp_state_ctx_t *mp_state, mp_const_obj_t fun);
qstr micropy_obj_code_get_name(struct _mp_state_ctx_t *mp_state, const byte *code_info);

mp_obj_t micropy_identity(struct _mp_state_ctx_t *mp_state, mp_obj_t self);
MP_DECLARE_CONST_FUN_OBJ(mp_identity_obj);

// module
typedef struct _mp_obj_module_t {
    mp_obj_base_t base;
    qstr name;
    mp_obj_dict_t *globals;
} mp_obj_module_t;
mp_obj_dict_t *micropy_obj_module_get_globals(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in);
// check if given module object is a package
bool micropy_obj_is_package(struct _mp_state_ctx_t *mp_state, mp_obj_t module);

// staticmethod and classmethod types; defined here so we can make const versions
// this structure is used for instances of both staticmethod and classmethod
typedef struct _mp_obj_static_class_method_t {
    mp_obj_base_t base;
    mp_obj_t fun;
} mp_obj_static_class_method_t;
typedef struct _mp_rom_obj_static_class_method_t {
    mp_obj_base_t base;
    mp_rom_obj_t fun;
} mp_rom_obj_static_class_method_t;

// property
const mp_obj_t *micropy_obj_property_get(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in);

// sequence helpers

// slice indexes resolved to particular sequence
typedef struct {
    mp_uint_t start;
    mp_uint_t stop;
    mp_int_t step;
} mp_bound_slice_t;

void micropy_seq_multiply(struct _mp_state_ctx_t *mp_state, const void *items, mp_uint_t item_sz, mp_uint_t len, mp_uint_t times, void *dest);
#if MICROPY_PY_BUILTINS_SLICE
bool micropy_seq_get_fast_slice_indexes(struct _mp_state_ctx_t *mp_state, mp_uint_t len, mp_obj_t slice, mp_bound_slice_t *indexes);
#endif
#define micropy_seq_copy(mp_state, dest, src, len, item_t) memcpy(dest, src, len * sizeof(item_t))
#define micropy_seq_cat(mp_state, dest, src1, len1, src2, len2, item_t) { memcpy(dest, src1, (len1) * sizeof(item_t)); memcpy(dest + (len1), src2, (len2) * sizeof(item_t)); }
bool micropy_seq_cmp_bytes(struct _mp_state_ctx_t *mp_state, mp_uint_t op, const byte *data1, mp_uint_t len1, const byte *data2, mp_uint_t len2);
bool micropy_seq_cmp_objs(struct _mp_state_ctx_t *mp_state, mp_uint_t op, const mp_obj_t *items1, mp_uint_t len1, const mp_obj_t *items2, mp_uint_t len2);
mp_obj_t micropy_seq_index_obj(struct _mp_state_ctx_t *mp_state, const mp_obj_t *items, mp_uint_t len, mp_uint_t n_args, const mp_obj_t *args);
mp_obj_t micropy_seq_count_obj(struct _mp_state_ctx_t *mp_state, const mp_obj_t *items, mp_uint_t len, mp_obj_t value);
mp_obj_t micropy_seq_extract_slice(struct _mp_state_ctx_t *mp_state, mp_uint_t len, const mp_obj_t *seq, mp_bound_slice_t *indexes);
// Helper to clear stale pointers from allocated, but unused memory, to preclude GC problems
#define micropy_seq_clear(mp_state, start, len, alloc_len, item_sz) memset((byte*)(start) + (len) * (item_sz), 0, ((alloc_len) - (len)) * (item_sz))
#define micropy_seq_replace_slice_no_grow(mp_state, dest, dest_len, beg, end, slice, slice_len, item_sz) \
    /*printf("memcpy(%p, %p, %d)\n", dest + beg, slice, slice_len * (item_sz));*/ \
    memcpy(((char*)dest) + (beg) * (item_sz), slice, slice_len * (item_sz)); \
    /*printf("memmove(%p, %p, %d)\n", dest + (beg + slice_len), dest + end, (dest_len - end) * (item_sz));*/ \
    memmove(((char*)dest) + (beg + slice_len) * (item_sz), ((char*)dest) + (end) * (item_sz), (dest_len - end) * (item_sz));

#define micropy_seq_replace_slice_grow_inplace(mp_state, dest, dest_len, beg, end, slice, slice_len, len_adj, item_sz) \
    /*printf("memmove(%p, %p, %d)\n", dest + beg + len_adj, dest + beg, (dest_len - beg) * (item_sz));*/ \
    memmove(((char*)dest) + (beg + len_adj) * (item_sz), ((char*)dest) + (beg) * (item_sz), (dest_len - beg) * (item_sz)); \
    memcpy(((char*)dest) + (beg) * (item_sz), slice, slice_len * (item_sz));

#endif // __MICROPY_INCLUDED_PY_OBJ_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_OBJLIST_H__
#define __MICROPY_INCLUDED_PY_OBJLIST_H__

//#include "py/obj.h"

typedef struct _mp_obj_list_t {
    mp_obj_base_t base;
    mp_uint_t alloc;
    mp_uint_t len;
    mp_obj_t *items;
} mp_obj_list_t;

#endif // __MICROPY_INCLUDED_PY_OBJLIST_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_OBJTUPLE_H__
#define __MICROPY_INCLUDED_PY_OBJTUPLE_H__

//#include "py/obj.h"

typedef struct _mp_obj_tuple_t {
    mp_obj_base_t base;
    mp_uint_t len;
    mp_obj_t items[];
} mp_obj_tuple_t;

typedef struct _mp_rom_obj_tuple_t {
    mp_obj_base_t base;
    mp_uint_t len;
    mp_rom_obj_t items[];
} mp_rom_obj_tuple_t;

void micropy_obj_tuple_print(struct _mp_state_ctx_t *mp_state, const mp_print_t *print, mp_obj_t o_in, mp_print_kind_t kind);
mp_obj_t micropy_obj_tuple_unary_op(struct _mp_state_ctx_t *mp_state, mp_uint_t op, mp_obj_t self_in);
mp_obj_t micropy_obj_tuple_binary_op(struct _mp_state_ctx_t *mp_state, mp_uint_t op, mp_obj_t lhs, mp_obj_t rhs);
mp_obj_t micropy_obj_tuple_subscr(struct _mp_state_ctx_t *mp_state, mp_obj_t base, mp_obj_t index, mp_obj_t value);
mp_obj_t micropy_obj_tuple_getiter(struct _mp_state_ctx_t *mp_state, mp_obj_t o_in);

extern const mp_obj_type_t mp_type_attrtuple;

#define MP_DEFINE_ATTRTUPLE(tuple_obj_name, fields, nitems, ...) \
    const mp_rom_obj_tuple_t tuple_obj_name = { \
        .base = {&mp_type_attrtuple}, \
        .len = nitems, \
        .items = { __VA_ARGS__ , MP_ROM_PTR((void*)fields) } \
    }

#if MICROPY_PY_COLLECTIONS
void micropy_obj_attrtuple_print_helper(struct _mp_state_ctx_t *mp_state, const mp_print_t *print, const qstr *fields, mp_obj_tuple_t *o);
#endif

mp_obj_t micropy_obj_new_attrtuple(struct _mp_state_ctx_t *mp_state, const qstr *fields, mp_uint_t n, const mp_obj_t *items);

#endif // __MICROPY_INCLUDED_PY_OBJTUPLE_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_OBJEXCEPT_H__
#define __MICROPY_INCLUDED_PY_OBJEXCEPT_H__

//#include "py/obj.h"
//#include "py/objtuple.h"

typedef struct _mp_obj_exception_t {
    mp_obj_base_t base;
    mp_uint_t traceback_alloc : (BITS_PER_WORD / 2);
    mp_uint_t traceback_len : (BITS_PER_WORD / 2);
    size_t *traceback_data;
    mp_obj_tuple_t *args;
} mp_obj_exception_t;

#endif // __MICROPY_INCLUDED_PY_OBJEXCEPT_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_MPSTATE_H__
#define __MICROPY_INCLUDED_PY_MPSTATE_H__

#include <stdint.h>

//#include "py/mpconfig.h"
//#include "py/misc.h"
//#include "py/nlr.h"
//#include "py/obj.h"
//#include "py/objlist.h"
//#include "py/objexcept.h"

// This file contains structures defining the state of the Micro Python
// memory system, runtime and virtual machine.  The state is a global
// variable, but in the future it is hoped that the state can become local.

// This structure contains dynamic configuration for the compiler.
#if MICROPY_DYNAMIC_COMPILER
typedef struct mp_dynamic_compiler_t {
    uint8_t small_int_bits; // must be <= host small_int_bits
    bool opt_cache_map_lookup_in_bytecode;
    bool py_builtins_str_unicode;
} mp_dynamic_compiler_t;
extern mp_dynamic_compiler_t mp_dynamic_compiler;
#endif

// This structure hold information about the memory allocation system.
typedef struct _mp_state_mem_t {
    #if MICROPY_MEM_STATS
    size_t total_bytes_allocated;
    size_t current_bytes_allocated;
    size_t peak_bytes_allocated;
    #endif

    byte *gc_alloc_table_start;
    size_t gc_alloc_table_byte_len;
    #if MICROPY_ENABLE_FINALISER
    byte *gc_finaliser_table_start;
    #endif
    byte *gc_pool_start;
    byte *gc_pool_end;

    int gc_stack_overflow;
    size_t gc_stack[MICROPY_ALLOC_GC_STACK_SIZE];
    size_t *gc_sp;
    uint16_t gc_lock_depth;

    // This variable controls auto garbage collection.  If set to 0 then the
    // GC won't automatically run when gc_alloc can't find enough blocks.  But
    // you can still allocate/free memory and also explicitly call gc_collect.
    uint16_t gc_auto_collect_enabled;

    size_t gc_last_free_atb_index;

    #if MICROPY_PY_GC_COLLECT_RETVAL
    size_t gc_collected;
    #endif
} mp_state_mem_t;

// This structure hold runtime and VM information.  It includes a section
// which contains root pointers that must be scanned by the GC.
typedef struct _mp_state_vm_t {
    ////////////////////////////////////////////////////////////
    // START ROOT POINTER SECTION
    // everything that needs GC scanning must go here
    // this must start at the start of this structure
    //

    // Note: nlr asm code has the offset of this hard-coded
    nlr_buf_t *nlr_top;

    qstr_pool_t *last_pool;

    // non-heap memory for creating an exception if we can't allocate RAM
    mp_obj_exception_t mp_emergency_exception_obj;

    // memory for exception arguments if we can't allocate RAM
    #if MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF
    #if MICROPY_EMERGENCY_EXCEPTION_BUF_SIZE > 0
    // statically allocated buf
    byte mp_emergency_exception_buf[MICROPY_EMERGENCY_EXCEPTION_BUF_SIZE];
    #else
    // dynamically allocated buf
    byte *mp_emergency_exception_buf;
    #endif
    #endif

    // dictionary with loaded modules (may be exposed as sys.modules)
    mp_obj_dict_t mp_loaded_modules_dict;

    // pending exception object (MP_OBJ_NULL if not pending)
    volatile mp_obj_t mp_pending_exception;

    // current exception being handled, for sys.exc_info()
    #if MICROPY_PY_SYS_EXC_INFO
    mp_obj_base_t *cur_exception;
    #endif

    // dictionary for the __main__ module
    mp_obj_dict_t dict_main;

    // these two lists must be initialised per port, after the call to mp_init
    mp_obj_list_t mp_sys_path_obj;
    mp_obj_list_t mp_sys_argv_obj;

    // dictionary for overridden builtins
    #if MICROPY_CAN_OVERRIDE_BUILTINS
    mp_obj_dict_t *mp_module_builtins_override_dict;
    #endif

    // include any root pointers defined by a port
    MICROPY_PORT_ROOT_POINTERS

    // root pointers for extmod

    #if MICROPY_PY_OS_DUPTERM
    mp_obj_t term_obj;
    #endif

    #if MICROPY_PY_LWIP_SLIP
    mp_obj_t lwip_slip_stream;
    #endif

    #if MICROPY_FSUSERMOUNT
    // for user-mountable block device (max fixed at compile time)
    struct _fs_user_mount_t *fs_user_mount[MICROPY_FATFS_VOLUMES];
    #endif

    //
    // END ROOT POINTER SECTION
    ////////////////////////////////////////////////////////////

    // pointer and sizes to store interned string data
    // (qstr_last_chunk can be root pointer but is also stored in qstr pool)
    byte *qstr_last_chunk;
    size_t qstr_last_alloc;
    size_t qstr_last_used;

    // Stack top at the start of program
    // Note: this entry is used to locate the end of the root pointer section.
    char *stack_top;

    #if MICROPY_STACK_CHECK
    mp_uint_t stack_limit;
    #endif

    mp_uint_t mp_optimise_value;

    // size of the emergency exception buf, if it's dynamically allocated
    #if MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF && MICROPY_EMERGENCY_EXCEPTION_BUF_SIZE == 0
    mp_int_t mp_emergency_exception_buf_size;
    #endif
} mp_state_vm_t;

// This structure combines the above 2 structures, and adds the local
// and global dicts.
// Note: if this structure changes then revisit all nlr asm code since they
// have the offset of nlr_top hard-coded.
typedef struct _mp_state_ctx_t {
    // these must come first for root pointer scanning in GC to work
    mp_obj_dict_t *dict_locals;
    mp_obj_dict_t *dict_globals;
    // this must come next for root pointer scanning in GC to work
    mp_state_vm_t vm;
    mp_state_mem_t mem;
} mp_state_ctx_t;

extern mp_state_ctx_t mp_state_ctx;

#define MP_STATE_CTX(x) (mp_state_ctx.x)
#define MP_STATE_VM(x) (mp_state_ctx.vm.x)
#define MP_STATE_MEM(x) (mp_state_ctx.mem.x)

#endif // __MICROPY_INCLUDED_PY_MPSTATE_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_MPZ_H__
#define __MICROPY_INCLUDED_PY_MPZ_H__

#include <stdint.h>

//#include "py/mpconfig.h"
//#include "py/misc.h"

// This mpz module implements arbitrary precision integers.
//
// The storage for each digit is defined by mpz_dig_t.  The actual number of
// bits in mpz_dig_t that are used is defined by MPZ_DIG_SIZE.  The machine must
// also provide a type that is twice as wide as mpz_dig_t, in both signed and
// unsigned versions.
//
// MPZ_DIG_SIZE can be between 4 and 8*sizeof(mpz_dig_t), but it makes most
// sense to have it as large as possible.  If MPZ_DIG_SIZE is not already
// defined then it is auto-detected below, depending on the machine.  The types
// are then set based on the value of MPZ_DIG_SIZE (although they can be freely
// changed so long as the constraints mentioned above are met).

#ifndef MPZ_DIG_SIZE
  #if defined(__x86_64__) || defined(_WIN64)
    // 64-bit machine, using 32-bit storage for digits
    #define MPZ_DIG_SIZE (32)
  #else
    // default: 32-bit machine, using 16-bit storage for digits
    #define MPZ_DIG_SIZE (16)
  #endif
#endif

#if MPZ_DIG_SIZE > 16
typedef uint32_t mpz_dig_t;
typedef uint64_t mpz_dbl_dig_t;
typedef int64_t mpz_dbl_dig_signed_t;
#elif MPZ_DIG_SIZE > 8
typedef uint16_t mpz_dig_t;
typedef uint32_t mpz_dbl_dig_t;
typedef int32_t mpz_dbl_dig_signed_t;
#elif MPZ_DIG_SIZE > 4
typedef uint8_t mpz_dig_t;
typedef uint16_t mpz_dbl_dig_t;
typedef int16_t mpz_dbl_dig_signed_t;
#else
typedef uint8_t mpz_dig_t;
typedef uint8_t mpz_dbl_dig_t;
typedef int8_t mpz_dbl_dig_signed_t;
#endif

#ifdef _WIN64
  #ifdef __MINGW32__
    #define MPZ_LONG_1 1LL
  #else
    #define MPZ_LONG_1 1i64
  #endif
#else
  #define MPZ_LONG_1 1L
#endif

// these define the maximum storage needed to hold an int or long long
#define MPZ_NUM_DIG_FOR_INT ((sizeof(mp_int_t) * 8 + MPZ_DIG_SIZE - 1) / MPZ_DIG_SIZE)
#define MPZ_NUM_DIG_FOR_LL ((sizeof(long long) * 8 + MPZ_DIG_SIZE - 1) / MPZ_DIG_SIZE)

typedef struct _mpz_t {
    mp_uint_t neg : 1;
    mp_uint_t fixed_dig : 1;
    mp_uint_t alloc : BITS_PER_WORD - 2;
    mp_uint_t len;
    mpz_dig_t *dig;
} mpz_t;

// convenience macro to declare an mpz with a digit array from the stack, initialised by an integer
#define MPZ_CONST_INT(z, val) mpz_t z; mpz_dig_t z ## _digits[MPZ_NUM_DIG_FOR_INT]; micropy_mpz_init_fixed_from_int(mp_state, &z, z_digits, MPZ_NUM_DIG_FOR_INT, val);

void micropy_mpz_init_zero(struct _mp_state_ctx_t *mp_state, mpz_t *z);
void micropy_mpz_init_from_int(struct _mp_state_ctx_t *mp_state, mpz_t *z, mp_int_t val);
void micropy_mpz_init_fixed_from_int(struct _mp_state_ctx_t *mp_state, mpz_t *z, mpz_dig_t *dig, mp_uint_t dig_alloc, mp_int_t val);
void micropy_mpz_deinit(struct _mp_state_ctx_t *mp_state, mpz_t *z);

void micropy_mpz_set(struct _mp_state_ctx_t *mp_state, mpz_t *dest, const mpz_t *src);
void micropy_mpz_set_from_int(struct _mp_state_ctx_t *mp_state, mpz_t *z, mp_int_t src);
void micropy_mpz_set_from_ll(struct _mp_state_ctx_t *mp_state, mpz_t *z, long long i, bool is_signed);
#if MICROPY_PY_BUILTINS_FLOAT
void micropy_mpz_set_from_float(struct _mp_state_ctx_t *mp_state, mpz_t *z, mp_float_t src);
#endif
mp_uint_t micropy_mpz_set_from_str(struct _mp_state_ctx_t *mp_state, mpz_t *z, const char *str, mp_uint_t len, bool neg, mp_uint_t base);

bool micropy_mpz_is_zero(struct _mp_state_ctx_t *mp_state, const mpz_t *z);
int micropy_mpz_cmp(struct _mp_state_ctx_t *mp_state, const mpz_t *lhs, const mpz_t *rhs);

void micropy_mpz_abs_inpl(struct _mp_state_ctx_t *mp_state, mpz_t *dest, const mpz_t *z);
void micropy_mpz_neg_inpl(struct _mp_state_ctx_t *mp_state, mpz_t *dest, const mpz_t *z);
void micropy_mpz_not_inpl(struct _mp_state_ctx_t *mp_state, mpz_t *dest, const mpz_t *z);
void micropy_mpz_shl_inpl(struct _mp_state_ctx_t *mp_state, mpz_t *dest, const mpz_t *lhs, mp_uint_t rhs);
void micropy_mpz_shr_inpl(struct _mp_state_ctx_t *mp_state, mpz_t *dest, const mpz_t *lhs, mp_uint_t rhs);
void micropy_mpz_add_inpl(struct _mp_state_ctx_t *mp_state, mpz_t *dest, const mpz_t *lhs, const mpz_t *rhs);
void micropy_mpz_sub_inpl(struct _mp_state_ctx_t *mp_state, mpz_t *dest, const mpz_t *lhs, const mpz_t *rhs);
void micropy_mpz_mul_inpl(struct _mp_state_ctx_t *mp_state, mpz_t *dest, const mpz_t *lhs, const mpz_t *rhs);
void micropy_mpz_pow_inpl(struct _mp_state_ctx_t *mp_state, mpz_t *dest, const mpz_t *lhs, const mpz_t *rhs);
void micropy_mpz_and_inpl(struct _mp_state_ctx_t *mp_state, mpz_t *dest, const mpz_t *lhs, const mpz_t *rhs);
void micropy_mpz_or_inpl(struct _mp_state_ctx_t *mp_state, mpz_t *dest, const mpz_t *lhs, const mpz_t *rhs);
void micropy_mpz_xor_inpl(struct _mp_state_ctx_t *mp_state, mpz_t *dest, const mpz_t *lhs, const mpz_t *rhs);
void micropy_mpz_divmod_inpl(struct _mp_state_ctx_t *mp_state, mpz_t *dest_quo, mpz_t *dest_rem, const mpz_t *lhs, const mpz_t *rhs);

mp_int_t micropy_mpz_hash(struct _mp_state_ctx_t *mp_state, const mpz_t *z);
bool micropy_mpz_as_int_checked(struct _mp_state_ctx_t *mp_state, const mpz_t *z, mp_int_t *value);
bool micropy_mpz_as_uint_checked(struct _mp_state_ctx_t *mp_state, const mpz_t *z, mp_uint_t *value);
void micropy_mpz_as_bytes(struct _mp_state_ctx_t *mp_state, const mpz_t *z, bool big_endian, mp_uint_t len, byte *buf);
#if MICROPY_PY_BUILTINS_FLOAT
mp_float_t micropy_mpz_as_float(struct _mp_state_ctx_t *mp_state, const mpz_t *z);
#endif
mp_uint_t micropy_mpz_as_str_size(struct _mp_state_ctx_t *mp_state, const mpz_t *i, mp_uint_t base, const char *prefix, char comma);
mp_uint_t micropy_mpz_as_str_inpl(struct _mp_state_ctx_t *mp_state, const mpz_t *z, mp_uint_t base, const char *prefix, char base_char, char comma, char *str);

#endif // __MICROPY_INCLUDED_PY_MPZ_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_RUNTIME0_H__
#define __MICROPY_INCLUDED_PY_RUNTIME0_H__

// These must fit in 8 bits; see scope.h
#define MP_SCOPE_FLAG_VARARGS      (0x01)
#define MP_SCOPE_FLAG_VARKEYWORDS  (0x02)
#define MP_SCOPE_FLAG_GENERATOR    (0x04)
#define MP_SCOPE_FLAG_DEFKWARGS    (0x08)

// types for native (viper) function signature
#define MP_NATIVE_TYPE_OBJ  (0x00)
#define MP_NATIVE_TYPE_BOOL (0x01)
#define MP_NATIVE_TYPE_INT  (0x02)
#define MP_NATIVE_TYPE_UINT (0x03)
#define MP_NATIVE_TYPE_PTR  (0x04)
#define MP_NATIVE_TYPE_PTR8 (0x05)
#define MP_NATIVE_TYPE_PTR16 (0x06)
#define MP_NATIVE_TYPE_PTR32 (0x07)

typedef enum {
    MP_UNARY_OP_BOOL, // __bool__
    MP_UNARY_OP_LEN, // __len__
    MP_UNARY_OP_HASH, // __hash__; must return a small int
    MP_UNARY_OP_POSITIVE,
    MP_UNARY_OP_NEGATIVE,
    MP_UNARY_OP_INVERT,
    MP_UNARY_OP_NOT,
} mp_unary_op_t;

typedef enum {
    MP_BINARY_OP_OR,
    MP_BINARY_OP_XOR,
    MP_BINARY_OP_AND,
    MP_BINARY_OP_LSHIFT,
    MP_BINARY_OP_RSHIFT,

    MP_BINARY_OP_ADD,
    MP_BINARY_OP_SUBTRACT,
    MP_BINARY_OP_MULTIPLY,
    MP_BINARY_OP_FLOOR_DIVIDE,
    MP_BINARY_OP_TRUE_DIVIDE,

    MP_BINARY_OP_MODULO,
    MP_BINARY_OP_POWER,
    MP_BINARY_OP_DIVMOD, // not emitted by the compiler but supported by the runtime
    MP_BINARY_OP_INPLACE_OR,
    MP_BINARY_OP_INPLACE_XOR,

    MP_BINARY_OP_INPLACE_AND,
    MP_BINARY_OP_INPLACE_LSHIFT,
    MP_BINARY_OP_INPLACE_RSHIFT,
    MP_BINARY_OP_INPLACE_ADD,
    MP_BINARY_OP_INPLACE_SUBTRACT,

    MP_BINARY_OP_INPLACE_MULTIPLY,
    MP_BINARY_OP_INPLACE_FLOOR_DIVIDE,
    MP_BINARY_OP_INPLACE_TRUE_DIVIDE,
    MP_BINARY_OP_INPLACE_MODULO,
    MP_BINARY_OP_INPLACE_POWER,

    // these should return a bool
    MP_BINARY_OP_LESS,
    MP_BINARY_OP_MORE,
    MP_BINARY_OP_EQUAL,
    MP_BINARY_OP_LESS_EQUAL,
    MP_BINARY_OP_MORE_EQUAL,

    MP_BINARY_OP_NOT_EQUAL,
    MP_BINARY_OP_IN,
    MP_BINARY_OP_IS,
    MP_BINARY_OP_EXCEPTION_MATCH,
    // these are not supported by the runtime and must be synthesised by the emitter
    MP_BINARY_OP_NOT_IN,
    MP_BINARY_OP_IS_NOT,
} mp_binary_op_t;

typedef enum {
    MP_F_CONVERT_OBJ_TO_NATIVE = 0,
    MP_F_CONVERT_NATIVE_TO_OBJ,
    MP_F_LOAD_NAME,
    MP_F_LOAD_GLOBAL,
    MP_F_LOAD_BUILD_CLASS,
    MP_F_LOAD_ATTR,
    MP_F_LOAD_METHOD,
    MP_F_STORE_NAME,
    MP_F_STORE_GLOBAL,
    MP_F_STORE_ATTR,
    MP_F_OBJ_SUBSCR,
    MP_F_OBJ_IS_TRUE,
    MP_F_UNARY_OP,
    MP_F_BINARY_OP,
    MP_F_BUILD_TUPLE,
    MP_F_BUILD_LIST,
    MP_F_LIST_APPEND,
    MP_F_BUILD_MAP,
    MP_F_STORE_MAP,
#if MICROPY_PY_BUILTINS_SET
    MP_F_BUILD_SET,
    MP_F_STORE_SET,
#endif
    MP_F_MAKE_FUNCTION_FROM_RAW_CODE,
    MP_F_NATIVE_CALL_FUNCTION_N_KW,
    MP_F_CALL_METHOD_N_KW,
    MP_F_CALL_METHOD_N_KW_VAR,
    MP_F_GETITER,
    MP_F_ITERNEXT,
    MP_F_NLR_PUSH,
    MP_F_NLR_POP,
    MP_F_NATIVE_RAISE,
    MP_F_IMPORT_NAME,
    MP_F_IMPORT_FROM,
    MP_F_IMPORT_ALL,
#if MICROPY_PY_BUILTINS_SLICE
    MP_F_NEW_SLICE,
#endif
    MP_F_UNPACK_SEQUENCE,
    MP_F_UNPACK_EX,
    MP_F_DELETE_NAME,
    MP_F_DELETE_GLOBAL,
    MP_F_NEW_CELL,
    MP_F_MAKE_CLOSURE_FROM_RAW_CODE,
    MP_F_SETUP_CODE_STATE,
    MP_F_NUMBER_OF,
} mp_fun_kind_t;

extern void *const mp_fun_table[MP_F_NUMBER_OF];

#endif // __MICROPY_INCLUDED_PY_RUNTIME0_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_LEXER_H__
#define __MICROPY_INCLUDED_PY_LEXER_H__

#include <stdint.h>

//#include "py/mpconfig.h"
//#include "py/qstr.h"

/* lexer.h -- simple tokeniser for Micro Python
 *
 * Uses (byte) length instead of null termination.
 * Tokens are the same - UTF-8 with (byte) length.
 */

typedef enum _mp_token_kind_t {
    MP_TOKEN_END,                   // 0

    MP_TOKEN_INVALID,
    MP_TOKEN_DEDENT_MISMATCH,
    MP_TOKEN_LONELY_STRING_OPEN,
    MP_TOKEN_BAD_LINE_CONTINUATION,

    MP_TOKEN_NEWLINE,               // 5
    MP_TOKEN_INDENT,                // 6
    MP_TOKEN_DEDENT,                // 7

    MP_TOKEN_NAME,                  // 8
    MP_TOKEN_INTEGER,
    MP_TOKEN_FLOAT_OR_IMAG,
    MP_TOKEN_STRING,
    MP_TOKEN_BYTES,

    MP_TOKEN_ELLIPSIS,

    MP_TOKEN_KW_FALSE,              // 14
    MP_TOKEN_KW_NONE,
    MP_TOKEN_KW_TRUE,
    MP_TOKEN_KW_AND,
    MP_TOKEN_KW_AS,
    MP_TOKEN_KW_ASSERT,
    #if MICROPY_PY_ASYNC_AWAIT
    MP_TOKEN_KW_ASYNC,
    MP_TOKEN_KW_AWAIT,
    #endif
    MP_TOKEN_KW_BREAK,
    MP_TOKEN_KW_CLASS,
    MP_TOKEN_KW_CONTINUE,
    MP_TOKEN_KW_DEF,                // 23
    MP_TOKEN_KW_DEL,
    MP_TOKEN_KW_ELIF,
    MP_TOKEN_KW_ELSE,
    MP_TOKEN_KW_EXCEPT,
    MP_TOKEN_KW_FINALLY,
    MP_TOKEN_KW_FOR,
    MP_TOKEN_KW_FROM,
    MP_TOKEN_KW_GLOBAL,
    MP_TOKEN_KW_IF,
    MP_TOKEN_KW_IMPORT,             // 33
    MP_TOKEN_KW_IN,
    MP_TOKEN_KW_IS,
    MP_TOKEN_KW_LAMBDA,
    MP_TOKEN_KW_NONLOCAL,
    MP_TOKEN_KW_NOT,
    MP_TOKEN_KW_OR,
    MP_TOKEN_KW_PASS,
    MP_TOKEN_KW_RAISE,
    MP_TOKEN_KW_RETURN,
    MP_TOKEN_KW_TRY,                // 43
    MP_TOKEN_KW_WHILE,
    MP_TOKEN_KW_WITH,
    MP_TOKEN_KW_YIELD,

    MP_TOKEN_OP_PLUS,               // 47
    MP_TOKEN_OP_MINUS,
    MP_TOKEN_OP_STAR,
    MP_TOKEN_OP_DBL_STAR,
    MP_TOKEN_OP_SLASH,
    MP_TOKEN_OP_DBL_SLASH,
    MP_TOKEN_OP_PERCENT,
    MP_TOKEN_OP_LESS,
    MP_TOKEN_OP_DBL_LESS,
    MP_TOKEN_OP_MORE,
    MP_TOKEN_OP_DBL_MORE,           // 57
    MP_TOKEN_OP_AMPERSAND,
    MP_TOKEN_OP_PIPE,
    MP_TOKEN_OP_CARET,
    MP_TOKEN_OP_TILDE,
    MP_TOKEN_OP_LESS_EQUAL,
    MP_TOKEN_OP_MORE_EQUAL,
    MP_TOKEN_OP_DBL_EQUAL,
    MP_TOKEN_OP_NOT_EQUAL,

    MP_TOKEN_DEL_PAREN_OPEN,        // 66
    MP_TOKEN_DEL_PAREN_CLOSE,
    MP_TOKEN_DEL_BRACKET_OPEN,
    MP_TOKEN_DEL_BRACKET_CLOSE,
    MP_TOKEN_DEL_BRACE_OPEN,
    MP_TOKEN_DEL_BRACE_CLOSE,
    MP_TOKEN_DEL_COMMA,
    MP_TOKEN_DEL_COLON,
    MP_TOKEN_DEL_PERIOD,
    MP_TOKEN_DEL_SEMICOLON,
    MP_TOKEN_DEL_AT,                // 76
    MP_TOKEN_DEL_EQUAL,
    MP_TOKEN_DEL_PLUS_EQUAL,
    MP_TOKEN_DEL_MINUS_EQUAL,
    MP_TOKEN_DEL_STAR_EQUAL,
    MP_TOKEN_DEL_SLASH_EQUAL,
    MP_TOKEN_DEL_DBL_SLASH_EQUAL,
    MP_TOKEN_DEL_PERCENT_EQUAL,
    MP_TOKEN_DEL_AMPERSAND_EQUAL,
    MP_TOKEN_DEL_PIPE_EQUAL,
    MP_TOKEN_DEL_CARET_EQUAL,       // 86
    MP_TOKEN_DEL_DBL_MORE_EQUAL,
    MP_TOKEN_DEL_DBL_LESS_EQUAL,
    MP_TOKEN_DEL_DBL_STAR_EQUAL,
    MP_TOKEN_DEL_MINUS_MORE,
} mp_token_kind_t;

// the next-byte function must return the next byte in the stream
// it must return MP_LEXER_EOF if end of stream
// it can be called again after returning MP_LEXER_EOF, and in that case must return MP_LEXER_EOF
#define MP_LEXER_EOF ((unichar)(-1))

typedef mp_uint_t (*mp_lexer_stream_next_byte_t)(struct _mp_state_ctx_t *mp_state, void*);
typedef void (*mp_lexer_stream_close_t)(struct _mp_state_ctx_t *mp_state, void*);

// this data structure is exposed for efficiency
// public members are: source_name, tok_line, tok_column, tok_kind, vstr
typedef struct _mp_lexer_t {
    qstr source_name;           // name of source
    void *stream_data;          // data for stream
    mp_lexer_stream_next_byte_t stream_next_byte;   // stream callback to get next byte
    mp_lexer_stream_close_t stream_close;           // stream callback to free

    unichar chr0, chr1, chr2;   // current cached characters from source

    mp_uint_t line;             // current source line
    mp_uint_t column;           // current source column

    mp_int_t emit_dent;             // non-zero when there are INDENT/DEDENT tokens to emit
    mp_int_t nested_bracket_level;  // >0 when there are nested brackets over multiple lines

    mp_uint_t alloc_indent_level;
    mp_uint_t num_indent_level;
    uint16_t *indent_level;

    mp_uint_t tok_line;         // token source line
    mp_uint_t tok_column;       // token source column
    mp_token_kind_t tok_kind;   // token kind
    vstr_t vstr;                // token data
} mp_lexer_t;

mp_lexer_t *micropy_lexer_new(struct _mp_state_ctx_t *mp_state, qstr src_name, void *stream_data, mp_lexer_stream_next_byte_t stream_next_byte, mp_lexer_stream_close_t stream_close);
mp_lexer_t *micropy_lexer_new_from_str_len(struct _mp_state_ctx_t *mp_state, qstr src_name, const char *str, mp_uint_t len, mp_uint_t free_len);

void micropy_lexer_free(struct _mp_state_ctx_t *mp_state, mp_lexer_t *lex);
void micropy_lexer_to_next(struct _mp_state_ctx_t *mp_state, mp_lexer_t *lex);
void micropy_lexer_show_token(struct _mp_state_ctx_t *mp_state, const mp_lexer_t *lex);

/******************************************************************/
// platform specific import function; must be implemented for a specific port
// TODO tidy up, rename, or put elsewhere

//mp_lexer_t *mp_import_open_file(qstr mod_name);

typedef enum {
    MP_IMPORT_STAT_NO_EXIST,
    MP_IMPORT_STAT_DIR,
    MP_IMPORT_STAT_FILE,
} mp_import_stat_t;

uint micropy_import_stat(struct _mp_state_ctx_t *mp_state, const char *path);
mp_lexer_t *micropy_lexer_new_from_file(struct _mp_state_ctx_t *mp_state, const char *filename);

#if MICROPY_HELPER_LEXER_UNIX
mp_lexer_t *micropy_lexer_new_from_fd(struct _mp_state_ctx_t *mp_state, qstr filename, int fd, bool close_fd);
#endif

#endif // __MICROPY_INCLUDED_PY_LEXER_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_PARSE_H__
#define __MICROPY_INCLUDED_PY_PARSE_H__

#include <stddef.h>
#include <stdint.h>

//#include "py/obj.h"

struct _mp_state_ctx_t;
struct _mp_lexer_t;

// a mp_parse_node_t is:
//  - 0000...0000: no node
//  - xxxx...xxx1: a small integer; bits 1 and above are the signed value, 2's complement
//  - xxxx...xx00: pointer to mp_parse_node_struct_t
//  - xx...xx0010: an identifier; bits 4 and above are the qstr
//  - xx...xx0110: a string; bits 4 and above are the qstr holding the value
//  - xx...xx1010: a string of bytes; bits 4 and above are the qstr holding the value
//  - xx...xx1110: a token; bits 4 and above are mp_token_kind_t

#define MP_PARSE_NODE_NULL      (0)
#define MP_PARSE_NODE_SMALL_INT (0x1)
#define MP_PARSE_NODE_ID        (0x02)
#define MP_PARSE_NODE_STRING    (0x06)
#define MP_PARSE_NODE_BYTES     (0x0a)
#define MP_PARSE_NODE_TOKEN     (0x0e)

typedef uintptr_t mp_parse_node_t; // must be pointer size

typedef struct _mp_parse_node_struct_t {
    uint32_t source_line;       // line number in source file
    uint32_t kind_num_nodes;    // parse node kind, and number of nodes
    mp_parse_node_t nodes[];    // nodes
} mp_parse_node_struct_t;

// macros for mp_parse_node_t usage
// some of these evaluate their argument more than once

#define MP_PARSE_NODE_IS_NULL(pn) ((pn) == MP_PARSE_NODE_NULL)
#define MP_PARSE_NODE_IS_LEAF(pn) ((pn) & 3)
#define MP_PARSE_NODE_IS_STRUCT(pn) ((pn) != MP_PARSE_NODE_NULL && ((pn) & 3) == 0)
#define MP_PARSE_NODE_IS_STRUCT_KIND(pn, k) ((pn) != MP_PARSE_NODE_NULL && ((pn) & 3) == 0 && MP_PARSE_NODE_STRUCT_KIND((mp_parse_node_struct_t*)(pn)) == (k))

#define MP_PARSE_NODE_IS_SMALL_INT(pn) (((pn) & 0x1) == MP_PARSE_NODE_SMALL_INT)
#define MP_PARSE_NODE_IS_ID(pn) (((pn) & 0x0f) == MP_PARSE_NODE_ID)
#define MP_PARSE_NODE_IS_TOKEN(pn) (((pn) & 0x0f) == MP_PARSE_NODE_TOKEN)
#define MP_PARSE_NODE_IS_TOKEN_KIND(pn, k) ((pn) == (MP_PARSE_NODE_TOKEN | ((k) << 4)))

#define MP_PARSE_NODE_LEAF_KIND(pn) ((pn) & 0x0f)
#define MP_PARSE_NODE_LEAF_ARG(pn) (((uintptr_t)(pn)) >> 4)
#define MP_PARSE_NODE_LEAF_SMALL_INT(pn) (((mp_int_t)(intptr_t)(pn)) >> 1)
#define MP_PARSE_NODE_STRUCT_KIND(pns) ((pns)->kind_num_nodes & 0xff)
#define MP_PARSE_NODE_STRUCT_NUM_NODES(pns) ((pns)->kind_num_nodes >> 8)

mp_parse_node_t micropy_parse_node_new_leaf(struct _mp_state_ctx_t *mp_state, size_t kind, mp_int_t arg);
bool micropy_parse_node_get_int_maybe(struct _mp_state_ctx_t *mp_state, mp_parse_node_t pn, mp_obj_t *o);
int micropy_parse_node_extract_list(struct _mp_state_ctx_t *mp_state, mp_parse_node_t *pn, size_t pn_kind, mp_parse_node_t **nodes);
void micropy_parse_node_print(struct _mp_state_ctx_t *mp_state, mp_parse_node_t pn, size_t indent);

typedef enum {
    MP_PARSE_SINGLE_INPUT,
    MP_PARSE_FILE_INPUT,
    MP_PARSE_EVAL_INPUT,
} mp_parse_input_kind_t;

typedef struct _mp_parse_t {
    mp_parse_node_t root;
    struct _mp_parse_chunk_t *chunk;
} mp_parse_tree_t;

// the parser will raise an exception if an error occurred
// the parser will free the lexer before it returns
mp_parse_tree_t micropy_parse(struct _mp_state_ctx_t *mp_state, struct _mp_lexer_t *lex, mp_parse_input_kind_t input_kind);
void micropy_parse_tree_clear(struct _mp_state_ctx_t *mp_state, mp_parse_tree_t *tree);

#endif // __MICROPY_INCLUDED_PY_PARSE_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_COMPILE_H__
#define __MICROPY_INCLUDED_PY_COMPILE_H__

//#include "py/lexer.h"
//#include "py/parse.h"
//#include "py/emitglue.h"

// These must fit in 8 bits; see scope.h
enum {
    MP_EMIT_OPT_NONE,
    MP_EMIT_OPT_BYTECODE,
    MP_EMIT_OPT_NATIVE_PYTHON,
    MP_EMIT_OPT_VIPER,
    MP_EMIT_OPT_ASM_THUMB,
};

// the compiler will raise an exception if an error occurred
// the compiler will clear the parse tree before it returns
mp_obj_t micropy_compile(struct _mp_state_ctx_t *mp_state, mp_parse_tree_t *parse_tree, qstr source_file, uint emit_opt, bool is_repl);

#if MICROPY_PERSISTENT_CODE_SAVE
// this has the same semantics as mp_compile
mp_raw_code_t *micropy_compile_to_raw_code(struct _mp_state_ctx_t *mp_state, mp_parse_tree_t *parse_tree, qstr source_file, uint emit_opt, bool is_repl);
#endif

// this is implemented in runtime.c
mp_obj_t micropy_parse_compile_execute(struct _mp_state_ctx_t *mp_state, mp_lexer_t *lex, mp_parse_input_kind_t parse_input_kind, mp_obj_dict_t *globals, mp_obj_dict_t *locals);

#endif // __MICROPY_INCLUDED_PY_COMPILE_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_EMITGLUE_H__
#define __MICROPY_INCLUDED_PY_EMITGLUE_H__

//#include "py/obj.h"

// These variables and functions glue the code emitters to the runtime.

typedef enum {
    MP_CODE_UNUSED,
    MP_CODE_RESERVED,
    MP_CODE_BYTECODE,
    MP_CODE_NATIVE_PY,
    MP_CODE_NATIVE_VIPER,
    MP_CODE_NATIVE_ASM,
} mp_raw_code_kind_t;

typedef struct _mp_raw_code_t {
    mp_raw_code_kind_t kind : 3;
    mp_uint_t scope_flags : 7;
    mp_uint_t n_pos_args : 11;
    union {
        struct {
            const byte *bytecode;
            const mp_uint_t *const_table;
            #if MICROPY_PERSISTENT_CODE_SAVE
            mp_uint_t bc_len;
            uint16_t n_obj;
            uint16_t n_raw_code;
            #endif
        } u_byte;
        struct {
            void *fun_data;
            const mp_uint_t *const_table;
            mp_uint_t type_sig; // for viper, compressed as 2-bit types; ret is MSB, then arg0, arg1, etc
        } u_native;
    } data;
} mp_raw_code_t;

mp_raw_code_t *micropy_emit_glue_new_raw_code(struct _mp_state_ctx_t *mp_state);

void micropy_emit_glue_assign_bytecode(struct _mp_state_ctx_t *mp_state, mp_raw_code_t *rc, const byte *code, mp_uint_t len,
    const mp_uint_t *const_table,
    #if MICROPY_PERSISTENT_CODE_SAVE
    uint16_t n_obj, uint16_t n_raw_code,
    #endif
    mp_uint_t scope_flags);
void micropy_emit_glue_assign_native(struct _mp_state_ctx_t *mp_state, mp_raw_code_t *rc, mp_raw_code_kind_t kind, void *fun_data, mp_uint_t fun_len, const mp_uint_t *const_table, mp_uint_t n_pos_args, mp_uint_t scope_flags, mp_uint_t type_sig);

mp_obj_t micropy_make_function_from_raw_code(struct _mp_state_ctx_t *mp_state, const mp_raw_code_t *rc, mp_obj_t def_args, mp_obj_t def_kw_args);
mp_obj_t micropy_make_closure_from_raw_code(struct _mp_state_ctx_t *mp_state, const mp_raw_code_t *rc, mp_uint_t n_closed_over, const mp_obj_t *args);

#if MICROPY_PERSISTENT_CODE_LOAD
typedef struct _mp_reader_t {
    void *data;
    mp_uint_t (*read_byte)(void *data);
    void (*close)(void *data);
} mp_reader_t;

mp_raw_code_t *micropy_raw_code_load(struct _mp_state_ctx_t *mp_state, mp_reader_t *reader);
mp_raw_code_t *micropy_raw_code_load_mem(struct _mp_state_ctx_t *mp_state, const byte *buf, size_t len);
mp_raw_code_t *micropy_raw_code_load_file(struct _mp_state_ctx_t *mp_state, const char *filename);
#endif

#if MICROPY_PERSISTENT_CODE_SAVE
void micropy_raw_code_save(struct _mp_state_ctx_t *mp_state, mp_raw_code_t *rc, mp_print_t *print);
void micropy_raw_code_save_file(struct _mp_state_ctx_t *mp_state, mp_raw_code_t *rc, const char *filename);
#endif

#endif // __MICROPY_INCLUDED_PY_EMITGLUE_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_RUNTIME_H__
#define __MICROPY_INCLUDED_PY_RUNTIME_H__

//#include "py/mpstate.h"
//#include "py/obj.h"

typedef enum {
    MP_VM_RETURN_NORMAL,
    MP_VM_RETURN_YIELD,
    MP_VM_RETURN_EXCEPTION,
} mp_vm_return_kind_t;

typedef enum {
    MP_ARG_BOOL      = 0x001,
    MP_ARG_INT       = 0x002,
    MP_ARG_OBJ       = 0x003,
    MP_ARG_KIND_MASK = 0x0ff,
    MP_ARG_REQUIRED  = 0x100,
    MP_ARG_KW_ONLY   = 0x200,
} mp_arg_flag_t;

typedef union _mp_arg_val_t {
    bool u_bool;
    mp_int_t u_int;
    mp_obj_t u_obj;
    mp_rom_obj_t u_rom_obj;
} mp_arg_val_t;

typedef struct _mp_arg_t {
    qstr qst;
    mp_uint_t flags;
    mp_arg_val_t defval;
} mp_arg_t;

// defined in objtype.c
extern const qstr mp_unary_op_method_name[];
extern const qstr mp_binary_op_method_name[];

void micropy_init(struct _mp_state_ctx_t *mp_state);
void micropy_deinit(struct _mp_state_ctx_t *mp_state);

// extra printing method specifically for mp_obj_t's which are integral type
int micropy_print_mp_int(struct _mp_state_ctx_t *mp_state, const mp_print_t *print, mp_obj_t x, int base, int base_char, int flags, char fill, int width, int prec);

void micropy_arg_check_num(struct _mp_state_ctx_t *mp_state, size_t n_args, size_t n_kw, size_t n_args_min, size_t n_args_max, bool takes_kw);
void micropy_arg_parse_all(struct _mp_state_ctx_t *mp_state, size_t n_pos, const mp_obj_t *pos, mp_map_t *kws, size_t n_allowed, const mp_arg_t *allowed, mp_arg_val_t *out_vals);
void micropy_arg_parse_all_kw_array(struct _mp_state_ctx_t *mp_state, size_t n_pos, size_t n_kw, const mp_obj_t *args, size_t n_allowed, const mp_arg_t *allowed, mp_arg_val_t *out_vals);
NORETURN void micropy_arg_error_terse_mismatch(struct _mp_state_ctx_t *mp_state);
NORETURN void micropy_arg_error_unimpl_kw(struct _mp_state_ctx_t *mp_state);

static inline mp_obj_dict_t *micropy_locals_get(struct _mp_state_ctx_t *mp_state) { return mp_state->dict_locals; }
static inline void micropy_locals_set(struct _mp_state_ctx_t *mp_state, mp_obj_dict_t *d) { mp_state->dict_locals = d; }
static inline mp_obj_dict_t *micropy_globals_get(struct _mp_state_ctx_t *mp_state) { return mp_state->dict_globals; }
static inline void micropy_globals_set(struct _mp_state_ctx_t *mp_state, mp_obj_dict_t *d) { mp_state->dict_globals = d; }

mp_obj_t micropy_load_name(struct _mp_state_ctx_t *mp_state, qstr qst);
mp_obj_t micropy_load_global(struct _mp_state_ctx_t *mp_state, qstr qst);
mp_obj_t micropy_load_build_class(struct _mp_state_ctx_t *mp_state);
void micropy_store_name(struct _mp_state_ctx_t *mp_state, qstr qst, mp_obj_t obj);
void micropy_store_global(struct _mp_state_ctx_t *mp_state, qstr qst, mp_obj_t obj);
void micropy_delete_name(struct _mp_state_ctx_t *mp_state, qstr qst);
void micropy_delete_global(struct _mp_state_ctx_t *mp_state, qstr qst);

mp_obj_t micropy_unary_op(struct _mp_state_ctx_t *mp_state, mp_uint_t op, mp_obj_t arg);
mp_obj_t micropy_binary_op(struct _mp_state_ctx_t *mp_state, mp_uint_t op, mp_obj_t lhs, mp_obj_t rhs);

mp_obj_t micropy_call_function_0(struct _mp_state_ctx_t *mp_state, mp_obj_t fun);
mp_obj_t micropy_call_function_1(struct _mp_state_ctx_t *mp_state, mp_obj_t fun, mp_obj_t arg);
mp_obj_t micropy_call_function_2(struct _mp_state_ctx_t *mp_state, mp_obj_t fun, mp_obj_t arg1, mp_obj_t arg2);
mp_obj_t micropy_call_function_n_kw(struct _mp_state_ctx_t *mp_state, mp_obj_t fun, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args);
mp_obj_t micropy_call_method_n_kw(struct _mp_state_ctx_t *mp_state, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args);
mp_obj_t micropy_call_method_n_kw_var(struct _mp_state_ctx_t *mp_state, bool have_self, mp_uint_t n_args_n_kw, const mp_obj_t *args);
// Call function and catch/dump exception - for Python callbacks from C code
void micropy_call_function_1_protected(struct _mp_state_ctx_t *mp_state, mp_obj_t fun, mp_obj_t arg);
void micropy_call_function_2_protected(struct _mp_state_ctx_t *mp_state, mp_obj_t fun, mp_obj_t arg1, mp_obj_t arg2);

typedef struct _mp_call_args_t {
    mp_obj_t fun;
    mp_uint_t n_args, n_kw, n_alloc;
    mp_obj_t *args;
} mp_call_args_t;

#if MICROPY_STACKLESS
// Takes arguments which are the most general mix of Python arg types, and
// prepares argument array suitable for passing to ->call() method of a
// function object (and mp_call_function_n_kw()).
// (Only needed in stackless mode.)
void micropy_call_prepare_args_n_kw_var(struct _mp_state_ctx_t *mp_state, bool have_self, mp_uint_t n_args_n_kw, const mp_obj_t *args, mp_call_args_t *out_args);
#endif

void micropy_unpack_sequence(struct _mp_state_ctx_t *mp_state, mp_obj_t seq, mp_uint_t num, mp_obj_t *items);
void micropy_unpack_ex(struct _mp_state_ctx_t *mp_state, mp_obj_t seq, mp_uint_t num, mp_obj_t *items);
mp_obj_t micropy_store_map(struct _mp_state_ctx_t *mp_state, mp_obj_t map, mp_obj_t key, mp_obj_t value);
mp_obj_t micropy_load_attr(struct _mp_state_ctx_t *mp_state, mp_obj_t base, qstr attr);
void micropy_convert_member_lookup(struct _mp_state_ctx_t *mp_state, mp_obj_t obj, const mp_obj_type_t *type, mp_obj_t member, mp_obj_t *dest);
void micropy_load_method(struct _mp_state_ctx_t *mp_state, mp_obj_t base, qstr attr, mp_obj_t *dest);
void micropy_load_method_maybe(struct _mp_state_ctx_t *mp_state, mp_obj_t base, qstr attr, mp_obj_t *dest);
void micropy_store_attr(struct _mp_state_ctx_t *mp_state, mp_obj_t base, qstr attr, mp_obj_t val);

mp_obj_t micropy_getiter(struct _mp_state_ctx_t *mp_state, mp_obj_t o);
mp_obj_t micropy_iternext_allow_raise(struct _mp_state_ctx_t *mp_state, mp_obj_t o); // may return MP_OBJ_STOP_ITERATION instead of raising StopIteration()
mp_obj_t micropy_iternext(struct _mp_state_ctx_t *mp_state, mp_obj_t o); // will always return MP_OBJ_STOP_ITERATION instead of raising StopIteration(...)
mp_vm_return_kind_t micropy_resume(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, mp_obj_t send_value, mp_obj_t throw_value, mp_obj_t *ret_val);

mp_obj_t micropy_make_raise_obj(struct _mp_state_ctx_t *mp_state, mp_obj_t o);

mp_obj_t micropy_import_name(struct _mp_state_ctx_t *mp_state, qstr name, mp_obj_t fromlist, mp_obj_t level);
mp_obj_t micropy_import_from(struct _mp_state_ctx_t *mp_state, mp_obj_t module, qstr name);
void micropy_import_all(struct _mp_state_ctx_t *mp_state, mp_obj_t module);

// Raise NotImplementedError with given message
NORETURN void micropy_not_implemented(struct _mp_state_ctx_t *mp_state, const char *msg);
NORETURN void micropy_exc_recursion_depth(struct _mp_state_ctx_t *mp_state);

// helper functions for native/viper code
mp_uint_t micropy_convert_obj_to_native(struct _mp_state_ctx_t *mp_state, mp_obj_t obj, mp_uint_t type);
mp_obj_t micropy_convert_native_to_obj(struct _mp_state_ctx_t *mp_state, mp_uint_t val, mp_uint_t type);
mp_obj_t micropy_native_call_function_n_kw(struct _mp_state_ctx_t *mp_state, mp_obj_t fun_in, mp_uint_t n_args_kw, const mp_obj_t *args);
void micropy_native_raise(struct _mp_state_ctx_t *mp_state, mp_obj_t o);

#define mp_sys_path (MP_OBJ_FROM_PTR(&(mp_state)->vm.mp_sys_path_obj))
#define mp_sys_argv (MP_OBJ_FROM_PTR(&(mp_state)->vm.mp_sys_argv_obj))

#if MICROPY_WARNINGS
void micropy_warning(struct _mp_state_ctx_t *mp_state, const char *msg, ...);
#else
#define micropy_warning(mp_state, msg, ...)
#endif

#endif // __MICROPY_INCLUDED_PY_RUNTIME_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_BUILTIN_H__
#define __MICROPY_INCLUDED_PY_BUILTIN_H__

//#include "py/obj.h"

mp_obj_t micropy_builtin___import__(struct _mp_state_ctx_t *mp_state, size_t n_args, const mp_obj_t *args);
mp_obj_t micropy_builtin_open(struct _mp_state_ctx_t *mp_state, size_t n_args, const mp_obj_t *args, mp_map_t *kwargs);
mp_obj_t micropy_micropython_mem_info(struct _mp_state_ctx_t *mp_state, size_t n_args, const mp_obj_t *args);

MP_DECLARE_CONST_FUN_OBJ(mp_builtin___build_class___obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin___import___obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin___repl_print___obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_abs_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_all_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_any_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_bin_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_callable_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_compile_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_chr_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_dir_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_divmod_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_eval_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_exec_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_execfile_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_getattr_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_setattr_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_globals_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_hasattr_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_hash_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_hex_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_id_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_isinstance_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_issubclass_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_iter_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_len_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_list_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_locals_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_max_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_min_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_next_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_oct_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_ord_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_pow_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_print_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_repr_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_round_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_sorted_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_sum_obj);
// Defined by a port, but declared here for simplicity
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_help_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_input_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_builtin_open_obj);

MP_DECLARE_CONST_FUN_OBJ(mp_namedtuple_obj);

MP_DECLARE_CONST_FUN_OBJ(mp_op_contains_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_op_getitem_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_op_setitem_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_op_delitem_obj);

extern const mp_obj_module_t mp_module___main__;
extern const mp_obj_module_t mp_module_builtins;
extern const mp_obj_module_t mp_module_array;
extern const mp_obj_module_t mp_module_collections;
extern const mp_obj_module_t mp_module_io;
extern const mp_obj_module_t mp_module_math;
extern const mp_obj_module_t mp_module_cmath;
extern const mp_obj_module_t mp_module_micropython;
extern const mp_obj_module_t mp_module_ustruct;
extern const mp_obj_module_t mp_module_sys;
extern const mp_obj_module_t mp_module_gc;

extern const mp_obj_dict_t mp_module_builtins_globals;

// extmod modules
extern const mp_obj_module_t mp_module_uerrno;
extern const mp_obj_module_t mp_module_uctypes;
extern const mp_obj_module_t mp_module_uzlib;
extern const mp_obj_module_t mp_module_ujson;
extern const mp_obj_module_t mp_module_ure;
extern const mp_obj_module_t mp_module_uheapq;
extern const mp_obj_module_t mp_module_uhashlib;
extern const mp_obj_module_t mp_module_ubinascii;
extern const mp_obj_module_t mp_module_ucrypto;
extern const mp_obj_module_t mp_module_utime;
extern const mp_obj_module_t mp_module_apollo;
extern const mp_obj_module_t mp_module_urandom;
extern const mp_obj_module_t mp_module_ussl;
extern const mp_obj_module_t mp_module_machine;
extern const mp_obj_module_t mp_module_lwip;
extern const mp_obj_module_t mp_module_websocket;
extern const mp_obj_module_t mp_module_webrepl;
extern const mp_obj_module_t mp_module_framebuf;

// extmod functions
MP_DECLARE_CONST_FUN_OBJ(pyb_mount_obj);

#endif // __MICROPY_INCLUDED_PY_BUILTIN_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_REPL_H__
#define __MICROPY_INCLUDED_PY_REPL_H__

//#include "py/mpconfig.h"
//#include "py/misc.h"
//#include "py/mpprint.h"

#if MICROPY_HELPER_REPL
bool micropy_repl_continue_with_input(struct _mp_state_ctx_t *mp_state, const char *input);
mp_uint_t micropy_repl_autocomplete(struct _mp_state_ctx_t *mp_state, const char *str, mp_uint_t len, const mp_print_t *print, const char **compl_str);
#endif

#endif // __MICROPY_INCLUDED_PY_REPL_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_GC_H__
#define __MICROPY_INCLUDED_PY_GC_H__

#include <stdint.h>

//#include "py/mpconfig.h"
//#include "py/misc.h"

void micropy_gc_init(struct _mp_state_ctx_t *mp_state, void *start, void *end);

// These lock/unlock functions can be nested.
// They can be used to prevent the GC from allocating/freeing.
void micropy_gc_lock(struct _mp_state_ctx_t *mp_state);
void micropy_gc_unlock(struct _mp_state_ctx_t *mp_state);
bool micropy_gc_is_locked(struct _mp_state_ctx_t *mp_state);

// A given port must implement gc_collect by using the other collect functions.
void micropy_gc_collect(struct _mp_state_ctx_t *mp_state);
void micropy_gc_collect_start(struct _mp_state_ctx_t *mp_state);
void micropy_gc_collect_root(struct _mp_state_ctx_t *mp_state, void **ptrs, size_t len);
void micropy_gc_collect_end(struct _mp_state_ctx_t *mp_state);

void *micropy_gc_alloc(struct _mp_state_ctx_t *mp_state, size_t n_bytes, bool has_finaliser);
void micropy_gc_free(struct _mp_state_ctx_t *mp_state, void *ptr); // does not call finaliser
size_t micropy_gc_nbytes(struct _mp_state_ctx_t *mp_state, const void *ptr);
void *micropy_gc_realloc(struct _mp_state_ctx_t *mp_state, void *ptr, size_t n_bytes, bool allow_move);

typedef struct _gc_info_t {
    size_t total;
    size_t used;
    size_t free;
    size_t num_1block;
    size_t num_2block;
    size_t max_block;
} gc_info_t;

void micropy_gc_info(struct _mp_state_ctx_t *mp_state, gc_info_t *info);
void micropy_gc_dump_info(struct _mp_state_ctx_t *mp_state);
void micropy_gc_dump_alloc_table(struct _mp_state_ctx_t *mp_state);

#endif // __MICROPY_INCLUDED_PY_GC_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Paul Sokolovsky
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_STACKCTRL_H__
#define __MICROPY_INCLUDED_PY_STACKCTRL_H__

//#include "py/mpconfig.h"

void micropy_stack_ctrl_init(struct _mp_state_ctx_t *mp_state);
void micropy_stack_set_top(struct _mp_state_ctx_t *mp_state, void *top);
mp_uint_t micropy_stack_usage(struct _mp_state_ctx_t *mp_state);

#if MICROPY_STACK_CHECK

void micropy_stack_set_limit(struct _mp_state_ctx_t *mp_state, mp_uint_t limit);
void micropy_stack_check(struct _mp_state_ctx_t *mp_state);
#define MP_STACK_CHECK() micropy_stack_check(mp_state)

#else

#define micropy_stack_set_limit(mp_state, limit)
#define MP_STACK_CHECK()

#endif

#endif // __MICROPY_INCLUDED_PY_STACKCTRL_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_STREAM_H__
#define __MICROPY_INCLUDED_PY_STREAM_H__

//#include "py/obj.h"

#define MP_STREAM_ERROR ((mp_uint_t)-1)

// Stream ioctl request codes
#define MP_STREAM_FLUSH (1)
#define MP_STREAM_SEEK  (2)
#define MP_STREAM_POLL  (3)
//#define MP_STREAM_CLOSE       (4)  // Not yet implemented
#define MP_STREAM_TIMEOUT       (5)  // Get/set timeout (single op)
#define MP_STREAM_GET_OPTS      (6)  // Get stream options
#define MP_STREAM_SET_OPTS      (7)  // Set stream options
#define MP_STREAM_GET_DATA_OPTS (8)  // Get data/message options
#define MP_STREAM_SET_DATA_OPTS (9)  // Set data/message options

// Argument structure for MP_STREAM_SEEK
struct mp_stream_seek_t {
    mp_off_t offset;
    int whence;
};

MP_DECLARE_CONST_FUN_OBJ(mp_stream_read_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_stream_read1_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_stream_readinto_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_stream_readall_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_stream_unbuffered_readline_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_stream_unbuffered_readlines_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_stream_write_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_stream_write1_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_stream_seek_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_stream_tell_obj);
MP_DECLARE_CONST_FUN_OBJ(mp_stream_ioctl_obj);

// these are for mp_get_stream_raise and can be or'd together
#define MP_STREAM_OP_READ (1)
#define MP_STREAM_OP_WRITE (2)
#define MP_STREAM_OP_IOCTL (4)

const mp_stream_p_t *micropy_get_stream_raise(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, int flags);
mp_obj_t micropy_stream_close(struct _mp_state_ctx_t *mp_state, mp_obj_t stream);

// Iterator which uses mp_stream_unbuffered_readline_obj
mp_obj_t micropy_stream_unbuffered_iter(struct _mp_state_ctx_t *mp_state, mp_obj_t self);

mp_obj_t micropy_stream_write(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, const void *buf, size_t len, byte flags);

// C-level helper functions
#define MP_STREAM_RW_READ  0
#define MP_STREAM_RW_WRITE 2
#define MP_STREAM_RW_ONCE  1
mp_uint_t micropy_stream_rw(struct _mp_state_ctx_t *mp_state, mp_obj_t stream, void *buf, mp_uint_t size, int *errcode, byte flags);
#define micropy_stream_write_exactly(mp_state, stream, buf, size, err) micropy_stream_rw(mp_state, stream, (byte*)buf, size, err, MP_STREAM_RW_WRITE)
#define micropy_stream_read_exactly(mp_state, stream, buf, size, err) micropy_stream_rw(mp_state, stream, buf, size, err, MP_STREAM_RW_READ)

#if MICROPY_STREAMS_NON_BLOCK
// TODO: This is POSIX-specific (but then POSIX is the only real thing,
// and anything else just emulates it, right?)
#define micropy_is_nonblocking_error(mp_state, errno) ((errno) == EAGAIN || (errno) == EWOULDBLOCK)
#else
#define micropy_is_nonblocking_error(mp_state, errno) (0)
#endif

#endif // __MICROPY_INCLUDED_PY_STREAM_H__
void micropy_stream_write_adaptor(struct _mp_state_ctx_t *mp_state, void *self, const char *buf, size_t len);
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_BINARY_H__
#define __MICROPY_INCLUDED_PY_BINARY_H__

//#include "py/obj.h"

// Use special typecode to differentiate repr() of bytearray vs array.array('B')
// (underlyingly they're same).
#define BYTEARRAY_TYPECODE 0

size_t micropy_binary_get_size(struct _mp_state_ctx_t *mp_state, char struct_type, char val_type, mp_uint_t *palign);
mp_obj_t micropy_binary_get_val_array(struct _mp_state_ctx_t *mp_state, char typecode, void *p, mp_uint_t index);
void micropy_binary_set_val_array(struct _mp_state_ctx_t *mp_state, char typecode, void *p, mp_uint_t index, mp_obj_t val_in);
void micropy_binary_set_val_array_from_int(struct _mp_state_ctx_t *mp_state, char typecode, void *p, mp_uint_t index, mp_int_t val);
mp_obj_t micropy_binary_get_val(struct _mp_state_ctx_t *mp_state, char struct_type, char val_type, byte **ptr);
void micropy_binary_set_val(struct _mp_state_ctx_t *mp_state, char struct_type, char val_type, mp_obj_t val_in, byte **ptr);
long long micropy_binary_get_int(struct _mp_state_ctx_t *mp_state, mp_uint_t size, bool is_signed, bool big_endian, const byte *src);
void micropy_binary_set_int(struct _mp_state_ctx_t *mp_state, mp_uint_t val_sz, bool big_endian, byte *dest, mp_uint_t val);

#endif // __MICROPY_INCLUDED_PY_BINARY_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_OBJINT_H__
#define __MICROPY_INCLUDED_PY_OBJINT_H__

//#include "py/mpz.h"
//#include "py/obj.h"

typedef struct _mp_obj_int_t {
    mp_obj_base_t base;
#if MICROPY_LONGINT_IMPL == MICROPY_LONGINT_IMPL_LONGLONG
    mp_longint_impl_t val;
#elif MICROPY_LONGINT_IMPL == MICROPY_LONGINT_IMPL_MPZ
    mpz_t mpz;
#endif
} mp_obj_int_t;

extern const mp_obj_int_t mp_maxsize_obj;

#if MICROPY_PY_BUILTINS_FLOAT
typedef enum {
    MP_FP_CLASS_FIT_SMALLINT,
    MP_FP_CLASS_FIT_LONGINT,
    MP_FP_CLASS_OVERFLOW
} mp_fp_as_int_class_t;

mp_fp_as_int_class_t micropy_classify_fp_as_int(struct _mp_state_ctx_t *mp_state, mp_float_t val);
#endif // MICROPY_PY_BUILTINS_FLOAT

void micropy_obj_int_print(struct _mp_state_ctx_t *mp_state, const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind);
char *micropy_obj_int_formatted(struct _mp_state_ctx_t *mp_state, char **buf, mp_uint_t *buf_size, mp_uint_t *fmt_size, mp_const_obj_t self_in,
                           int base, const char *prefix, char base_char, char comma);
char *micropy_obj_int_formatted_impl(struct _mp_state_ctx_t *mp_state, char **buf, mp_uint_t *buf_size, mp_uint_t *fmt_size, mp_const_obj_t self_in,
                                int base, const char *prefix, char base_char, char comma);
mp_int_t micropy_obj_int_hash(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in);
void micropy_obj_int_to_bytes_impl(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, bool big_endian, mp_uint_t len, byte *buf);
int micropy_obj_int_sign(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in);
mp_obj_t micropy_obj_int_abs(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in);
mp_obj_t micropy_obj_int_unary_op(struct _mp_state_ctx_t *mp_state, mp_uint_t op, mp_obj_t o_in);
mp_obj_t micropy_obj_int_binary_op(struct _mp_state_ctx_t *mp_state, mp_uint_t op, mp_obj_t lhs_in, mp_obj_t rhs_in);
mp_obj_t micropy_obj_int_binary_op_extra_cases(struct _mp_state_ctx_t *mp_state, mp_uint_t op, mp_obj_t lhs_in, mp_obj_t rhs_in);

#endif // __MICROPY_INCLUDED_PY_OBJINT_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_FORMATFLOAT_H__
#define __MICROPY_INCLUDED_PY_FORMATFLOAT_H__

//#include "py/mpconfig.h"

#if MICROPY_PY_BUILTINS_FLOAT
int micropy_format_float(struct _mp_state_ctx_t *mp_state, mp_float_t f, char *buf, size_t bufSize, char fmt, int prec, char sign);
#endif

#endif // __MICROPY_INCLUDED_PY_FORMATFLOAT_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_UNICODE_H__
#define __MICROPY_INCLUDED_PY_UNICODE_H__

//#include "py/mpconfig.h"
//#include "py/misc.h"

mp_uint_t utf8_ptr_to_index(const byte *s, const byte *ptr);

#endif // __MICROPY_INCLUDED_PY_UNICODE_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_PARSENUM_H__
#define __MICROPY_INCLUDED_PY_PARSENUM_H__

//#include "py/mpconfig.h"
//#include "py/lexer.h"
//#include "py/obj.h"

// these functions raise a SyntaxError if lex!=NULL, else a ValueError
mp_obj_t micropy_parse_num_integer(struct _mp_state_ctx_t *mp_state, const char *restrict str, size_t len, int base, mp_lexer_t *lex);
mp_obj_t micropy_parse_num_decimal(struct _mp_state_ctx_t *mp_state, const char *str, size_t len, bool allow_imag, bool force_complex, mp_lexer_t *lex);

#endif // __MICROPY_INCLUDED_PY_PARSENUM_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_SMALLINT_H__
#define __MICROPY_INCLUDED_PY_SMALLINT_H__

//#include "py/mpconfig.h"
//#include "py/misc.h"

// Functions for small integer arithmetic

#ifndef MP_SMALL_INT_MIN

// In SMALL_INT, next-to-highest bits is used as sign, so both must match for value in range
#if MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_A || MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_C

#define MP_SMALL_INT_MIN ((mp_int_t)(((mp_int_t)WORD_MSBIT_HIGH) >> 1))
#define MP_SMALL_INT_FITS(n) ((((n) ^ ((n) << 1)) & WORD_MSBIT_HIGH) == 0)
// Mask to truncate mp_int_t to positive value
#define MP_SMALL_INT_POSITIVE_MASK ~(WORD_MSBIT_HIGH | (WORD_MSBIT_HIGH >> 1))

#elif MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_B

#define MP_SMALL_INT_MIN ((mp_int_t)(((mp_int_t)WORD_MSBIT_HIGH) >> 2))
#define MP_SMALL_INT_FITS(n) ((((n) & MP_SMALL_INT_MIN) == 0) || (((n) & MP_SMALL_INT_MIN) == MP_SMALL_INT_MIN))
// Mask to truncate mp_int_t to positive value
#define MP_SMALL_INT_POSITIVE_MASK ~(WORD_MSBIT_HIGH | (WORD_MSBIT_HIGH >> 1) | (WORD_MSBIT_HIGH >> 2))

#elif MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_D

#define MP_SMALL_INT_MIN ((mp_int_t)(((mp_int_t)0xffffffff80000000) >> 1))
#define MP_SMALL_INT_FITS(n) ((((n) ^ ((n) << 1)) & 0xffffffff80000000) == 0)
// Mask to truncate mp_int_t to positive value
#define MP_SMALL_INT_POSITIVE_MASK ~(0xffffffff80000000 | (0xffffffff80000000 >> 1))

#endif

#endif

#define MP_SMALL_INT_MAX ((mp_int_t)(~(MP_SMALL_INT_MIN)))

bool micropy_small_int_mul_overflow(struct _mp_state_ctx_t *mp_state, mp_int_t x, mp_int_t y);
mp_int_t micropy_small_int_modulo(struct _mp_state_ctx_t *mp_state, mp_int_t dividend, mp_int_t divisor);
mp_int_t micropy_small_int_floor_divide(struct _mp_state_ctx_t *mp_state, mp_int_t num, mp_int_t denom);

#endif // __MICROPY_INCLUDED_PY_SMALLINT_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_SCOPE_H__
#define __MICROPY_INCLUDED_PY_SCOPE_H__

//#include "py/parse.h"
//#include "py/emitglue.h"

enum {
    ID_INFO_KIND_GLOBAL_IMPLICIT,
    ID_INFO_KIND_GLOBAL_EXPLICIT,
    ID_INFO_KIND_LOCAL, // in a function f, written and only referenced by f
    ID_INFO_KIND_CELL,  // in a function f, read/written by children of f
    ID_INFO_KIND_FREE,  // in a function f, belongs to the parent of f
};

enum {
    ID_FLAG_IS_PARAM = 0x01,
    ID_FLAG_IS_STAR_PARAM = 0x02,
    ID_FLAG_IS_DBL_STAR_PARAM = 0x04,
};

typedef struct _id_info_t {
    uint8_t kind;
    uint8_t flags;
    // when it's an ID_INFO_KIND_LOCAL this is the unique number of the local
    // whet it's an ID_INFO_KIND_CELL/FREE this is the unique number of the closed over variable
    uint16_t local_num;
    qstr qst;
} id_info_t;

// scope is a "block" in Python parlance
typedef enum { SCOPE_MODULE, SCOPE_FUNCTION, SCOPE_LAMBDA, SCOPE_LIST_COMP, SCOPE_DICT_COMP, SCOPE_SET_COMP, SCOPE_GEN_EXPR, SCOPE_CLASS } scope_kind_t;
typedef struct _scope_t {
    scope_kind_t kind;
    struct _scope_t *parent;
    struct _scope_t *next;
    mp_parse_node_t pn;
    qstr source_file;
    qstr simple_name;
    mp_raw_code_t *raw_code;
    uint8_t scope_flags;  // see runtime0.h
    uint8_t emit_options; // see compile.h
    uint16_t num_pos_args;
    uint16_t num_kwonly_args;
    uint16_t num_def_pos_args;
    uint16_t num_locals;
    uint16_t stack_size;     // maximum size of the locals stack
    uint16_t exc_stack_size; // maximum size of the exception stack
    uint16_t id_info_alloc;
    uint16_t id_info_len;
    id_info_t *id_info;
} scope_t;

scope_t *micropy_scope_new(struct _mp_state_ctx_t *mp_state, scope_kind_t kind, mp_parse_node_t pn, qstr source_file, mp_uint_t emit_options);
void micropy_scope_free(struct _mp_state_ctx_t *mp_state, scope_t *scope);
id_info_t *micropy_scope_find_or_add_id(struct _mp_state_ctx_t *mp_state, scope_t *scope, qstr qstr, bool *added);
id_info_t *micropy_scope_find(struct _mp_state_ctx_t *mp_state, scope_t *scope, qstr qstr);
id_info_t *micropy_scope_find_global(struct _mp_state_ctx_t *mp_state, scope_t *scope, qstr qstr);
id_info_t *micropy_scope_find_local_in_parent(struct _mp_state_ctx_t *mp_state, scope_t *scope, qstr qstr);
void micropy_scope_close_over_in_parents(struct _mp_state_ctx_t *mp_state, scope_t *scope, qstr qstr);

#endif // __MICROPY_INCLUDED_PY_SCOPE_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __MICROPY_INCLUDED_PY_EMIT_H__
#define __MICROPY_INCLUDED_PY_EMIT_H__

//#include "py/lexer.h"
//#include "py/scope.h"
//#include "py/runtime0.h"

/* Notes on passes:
 * We don't know exactly the opcodes in pass 1 because they depend on the
 * closing over of variables (LOAD_CLOSURE, BUILD_TUPLE, MAKE_CLOSURE), which
 * depends on determining the scope of variables in each function, and this
 * is not known until the end of pass 1.
 * As a consequence, we don't know the maximum stack size until the end of pass 2.
 * This is problematic for some emitters (x64) since they need to know the maximum
 * stack size to compile the entry to the function, and this affects code size.
 */

typedef enum {
    MP_PASS_SCOPE = 1,      // work out id's and their kind, and number of labels
    MP_PASS_STACK_SIZE = 2, // work out maximum stack size
    MP_PASS_CODE_SIZE = 3,  // work out code size and label offsets
    MP_PASS_EMIT = 4,       // emit code
} pass_kind_t;

#define MP_EMIT_STAR_FLAG_SINGLE (0x01)
#define MP_EMIT_STAR_FLAG_DOUBLE (0x02)

#define MP_EMIT_BREAK_FROM_FOR (0x8000)

#define MP_EMIT_NATIVE_TYPE_ENABLE (0)
#define MP_EMIT_NATIVE_TYPE_RETURN (1)
#define MP_EMIT_NATIVE_TYPE_ARG    (2)

typedef struct _emit_t emit_t;

typedef struct _mp_emit_method_table_id_ops_t {
    void (*fast)(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst, mp_uint_t local_num);
    void (*deref)(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst, mp_uint_t local_num);
    void (*name)(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst);
    void (*global)(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst);
} mp_emit_method_table_id_ops_t;

typedef struct _emit_method_table_t {
    void (*set_native_type)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t op, mp_uint_t arg1, qstr arg2);
    void (*start_pass)(struct _mp_state_ctx_t *mp_state, emit_t *emit, pass_kind_t pass, scope_t *scope);
    void (*end_pass)(struct _mp_state_ctx_t *mp_state, emit_t *emit);
    bool (*last_emit_was_return_value)(struct _mp_state_ctx_t *mp_state, emit_t *emit);
    void (*adjust_stack_size)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_int_t delta);
    void (*set_source_line)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t line);

    mp_emit_method_table_id_ops_t load_id;
    mp_emit_method_table_id_ops_t store_id;
    mp_emit_method_table_id_ops_t delete_id;

    void (*label_assign)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t l);
    void (*import_name)(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst);
    void (*import_from)(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst);
    void (*import_star)(struct _mp_state_ctx_t *mp_state, emit_t *emit);
    void (*load_const_tok)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_token_kind_t tok);
    void (*load_const_small_int)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_int_t arg);
    void (*load_const_str)(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst);
    void (*load_const_obj)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_obj_t obj);
    void (*load_null)(struct _mp_state_ctx_t *mp_state, emit_t *emit);
    void (*load_attr)(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst);
    void (*load_method)(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst);
    void (*load_build_class)(struct _mp_state_ctx_t *mp_state, emit_t *emit);
    void (*load_subscr)(struct _mp_state_ctx_t *mp_state, emit_t *emit);
    void (*store_attr)(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst);
    void (*store_subscr)(struct _mp_state_ctx_t *mp_state, emit_t *emit);
    void (*delete_attr)(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst);
    void (*delete_subscr)(struct _mp_state_ctx_t *mp_state, emit_t *emit);
    void (*dup_top)(struct _mp_state_ctx_t *mp_state, emit_t *emit);
    void (*dup_top_two)(struct _mp_state_ctx_t *mp_state, emit_t *emit);
    void (*pop_top)(struct _mp_state_ctx_t *mp_state, emit_t *emit);
    void (*rot_two)(struct _mp_state_ctx_t *mp_state, emit_t *emit);
    void (*rot_three)(struct _mp_state_ctx_t *mp_state, emit_t *emit);
    void (*jump)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t label);
    void (*pop_jump_if)(struct _mp_state_ctx_t *mp_state, emit_t *emit, bool cond, mp_uint_t label);
    void (*jump_if_or_pop)(struct _mp_state_ctx_t *mp_state, emit_t *emit, bool cond, mp_uint_t label);
    void (*break_loop)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t label, mp_uint_t except_depth);
    void (*continue_loop)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t label, mp_uint_t except_depth);
    void (*setup_with)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t label);
    void (*with_cleanup)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t label);
    void (*setup_except)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t label);
    void (*setup_finally)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t label);
    void (*end_finally)(struct _mp_state_ctx_t *mp_state, emit_t *emit);
    void (*get_iter)(struct _mp_state_ctx_t *mp_state, emit_t *emit);
    void (*for_iter)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t label);
    void (*for_iter_end)(struct _mp_state_ctx_t *mp_state, emit_t *emit);
    void (*pop_block)(struct _mp_state_ctx_t *mp_state, emit_t *emit);
    void (*pop_except)(struct _mp_state_ctx_t *mp_state, emit_t *emit);
    void (*unary_op)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_unary_op_t op);
    void (*binary_op)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_binary_op_t op);
    void (*build_tuple)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t n_args);
    void (*build_list)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t n_args);
    void (*list_append)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t list_stack_index);
    void (*build_map)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t n_args);
    void (*store_map)(struct _mp_state_ctx_t *mp_state, emit_t *emit);
    void (*map_add)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t map_stack_index);
    #if MICROPY_PY_BUILTINS_SET
    void (*build_set)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t n_args);
    void (*set_add)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t set_stack_index);
    #endif
    #if MICROPY_PY_BUILTINS_SLICE
    void (*build_slice)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t n_args);
    #endif
    void (*unpack_sequence)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t n_args);
    void (*unpack_ex)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t n_left, mp_uint_t n_right);
    void (*make_function)(struct _mp_state_ctx_t *mp_state, emit_t *emit, scope_t *scope, mp_uint_t n_pos_defaults, mp_uint_t n_kw_defaults);
    void (*make_closure)(struct _mp_state_ctx_t *mp_state, emit_t *emit, scope_t *scope, mp_uint_t n_closed_over, mp_uint_t n_pos_defaults, mp_uint_t n_kw_defaults);
    void (*call_function)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t n_positional, mp_uint_t n_keyword, mp_uint_t star_flags);
    void (*call_method)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t n_positional, mp_uint_t n_keyword, mp_uint_t star_flags);
    void (*return_value)(struct _mp_state_ctx_t *mp_state, emit_t *emit);
    void (*raise_varargs)(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t n_args);
    void (*yield_value)(struct _mp_state_ctx_t *mp_state, emit_t *emit);
    void (*yield_from)(struct _mp_state_ctx_t *mp_state, emit_t *emit);

    // these methods are used to control entry to/exit from an exception handler
    // they may or may not emit code
    void (*start_except_handler)(struct _mp_state_ctx_t *mp_state, emit_t *emit);
    void (*end_except_handler)(struct _mp_state_ctx_t *mp_state, emit_t *emit);
} emit_method_table_t;

void micropy_emit_common_get_id_for_load(struct _mp_state_ctx_t *mp_state, scope_t *scope, qstr qst);
void micropy_emit_common_get_id_for_modification(struct _mp_state_ctx_t *mp_state, scope_t *scope, qstr qst);
void micropy_emit_common_id_op(struct _mp_state_ctx_t *mp_state, emit_t *emit, const mp_emit_method_table_id_ops_t *emit_method_table, scope_t *scope, qstr qst);

extern const emit_method_table_t emit_cpython_method_table;
extern const emit_method_table_t emit_bc_method_table;
extern const emit_method_table_t emit_native_x64_method_table;
extern const emit_method_table_t emit_native_x86_method_table;
extern const emit_method_table_t emit_native_thumb_method_table;
extern const emit_method_table_t emit_native_arm_method_table;

extern const mp_emit_method_table_id_ops_t micropy_emit_bc_method_table_load_id_ops;
extern const mp_emit_method_table_id_ops_t micropy_emit_bc_method_table_store_id_ops;
extern const mp_emit_method_table_id_ops_t micropy_emit_bc_method_table_delete_id_ops;

emit_t *micropy_emit_cpython_new(struct _mp_state_ctx_t *mp_state);
emit_t *micropy_emit_bc_new(struct _mp_state_ctx_t *mp_state);
emit_t *micropy_emit_native_x64_new(struct _mp_state_ctx_t *mp_state, mp_obj_t *error_slot, mp_uint_t max_num_labels);
emit_t *micropy_emit_native_x86_new(struct _mp_state_ctx_t *mp_state, mp_obj_t *error_slot, mp_uint_t max_num_labels);
emit_t *micropy_emit_native_thumb_new(struct _mp_state_ctx_t *mp_state, mp_obj_t *error_slot, mp_uint_t max_num_labels);
emit_t *micropy_emit_native_arm_new(struct _mp_state_ctx_t *mp_state, mp_obj_t *error_slot, mp_uint_t max_num_labels);

void micropy_emit_cpython_set_max_num_labels(struct _mp_state_ctx_t *mp_state, emit_t* emit, mp_uint_t max_num_labels);
void micropy_emit_bc_set_max_num_labels(struct _mp_state_ctx_t *mp_state, emit_t* emit, mp_uint_t max_num_labels);

void micropy_emit_cpython_free(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_bc_free(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_native_x64_free(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_native_x86_free(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_native_thumb_free(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_native_arm_free(struct _mp_state_ctx_t *mp_state, emit_t *emit);

void micropy_emit_bc_start_pass(struct _mp_state_ctx_t *mp_state, emit_t *emit, pass_kind_t pass, scope_t *scope);
void micropy_emit_bc_end_pass(struct _mp_state_ctx_t *mp_state, emit_t *emit);
bool micropy_emit_bc_last_emit_was_return_value(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_bc_adjust_stack_size(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_int_t delta);
void micropy_emit_bc_set_source_line(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t line);

void micropy_emit_bc_load_fast(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst, mp_uint_t local_num);
void micropy_emit_bc_load_deref(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst, mp_uint_t local_num);
void micropy_emit_bc_load_name(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst);
void micropy_emit_bc_load_global(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst);
void micropy_emit_bc_store_fast(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst, mp_uint_t local_num);
void micropy_emit_bc_store_deref(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst, mp_uint_t local_num);
void micropy_emit_bc_store_name(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst);
void micropy_emit_bc_store_global(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst);
void micropy_emit_bc_delete_fast(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst, mp_uint_t local_num);
void micropy_emit_bc_delete_deref(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst, mp_uint_t local_num);
void micropy_emit_bc_delete_name(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst);
void micropy_emit_bc_delete_global(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst);

void micropy_emit_bc_label_assign(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t l);
void micropy_emit_bc_import_name(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst);
void micropy_emit_bc_import_from(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst);
void micropy_emit_bc_import_star(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_bc_load_const_tok(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_token_kind_t tok);
void micropy_emit_bc_load_const_small_int(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_int_t arg);
void micropy_emit_bc_load_const_str(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst);
void micropy_emit_bc_load_const_obj(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_obj_t obj);
void micropy_emit_bc_load_null(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_bc_load_attr(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst);
void micropy_emit_bc_load_method(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst);
void micropy_emit_bc_load_build_class(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_bc_load_subscr(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_bc_store_attr(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst);
void micropy_emit_bc_store_subscr(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_bc_delete_attr(struct _mp_state_ctx_t *mp_state, emit_t *emit, qstr qst);
void micropy_emit_bc_delete_subscr(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_bc_dup_top(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_bc_dup_top_two(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_bc_pop_top(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_bc_rot_two(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_bc_rot_three(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_bc_jump(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t label);
void micropy_emit_bc_pop_jump_if(struct _mp_state_ctx_t *mp_state, emit_t *emit, bool cond, mp_uint_t label);
void micropy_emit_bc_jump_if_or_pop(struct _mp_state_ctx_t *mp_state, emit_t *emit, bool cond, mp_uint_t label);
void micropy_emit_bc_unwind_jump(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t label, mp_uint_t except_depth);
#define micropy_emit_bc_break_loop micropy_emit_bc_unwind_jump
#define micropy_emit_bc_continue_loop micropy_emit_bc_unwind_jump
void micropy_emit_bc_setup_with(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t label);
void micropy_emit_bc_with_cleanup(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t label);
void micropy_emit_bc_setup_except(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t label);
void micropy_emit_bc_setup_finally(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t label);
void micropy_emit_bc_end_finally(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_bc_get_iter(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_bc_for_iter(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t label);
void micropy_emit_bc_for_iter_end(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_bc_pop_block(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_bc_pop_except(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_bc_unary_op(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_unary_op_t op);
void micropy_emit_bc_binary_op(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_binary_op_t op);
void micropy_emit_bc_build_tuple(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t n_args);
void micropy_emit_bc_build_list(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t n_args);
void micropy_emit_bc_list_append(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t list_stack_index);
void micropy_emit_bc_build_map(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t n_args);
void micropy_emit_bc_store_map(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_bc_map_add(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t map_stack_index);
#if MICROPY_PY_BUILTINS_SET
void micropy_emit_bc_build_set(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t n_args);
void micropy_emit_bc_set_add(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t set_stack_index);
#endif
#if MICROPY_PY_BUILTINS_SLICE
void micropy_emit_bc_build_slice(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t n_args);
#endif
void micropy_emit_bc_unpack_sequence(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t n_args);
void micropy_emit_bc_unpack_ex(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t n_left, mp_uint_t n_right);
void micropy_emit_bc_make_function(struct _mp_state_ctx_t *mp_state, emit_t *emit, scope_t *scope, mp_uint_t n_pos_defaults, mp_uint_t n_kw_defaults);
void micropy_emit_bc_make_closure(struct _mp_state_ctx_t *mp_state, emit_t *emit, scope_t *scope, mp_uint_t n_closed_over, mp_uint_t n_pos_defaults, mp_uint_t n_kw_defaults);
void micropy_emit_bc_call_function(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t n_positional, mp_uint_t n_keyword, mp_uint_t star_flags);
void micropy_emit_bc_call_method(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t n_positional, mp_uint_t n_keyword, mp_uint_t star_flags);
void micropy_emit_bc_return_value(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_bc_raise_varargs(struct _mp_state_ctx_t *mp_state, emit_t *emit, mp_uint_t n_args);
void micropy_emit_bc_yield_value(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_bc_yield_from(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_bc_start_except_handler(struct _mp_state_ctx_t *mp_state, emit_t *emit);
void micropy_emit_bc_end_except_handler(struct _mp_state_ctx_t *mp_state, emit_t *emit);

typedef struct _emit_inline_asm_t emit_inline_asm_t;

typedef struct _emit_inline_asm_method_table_t {
    void (*start_pass)(emit_inline_asm_t *emit, pass_kind_t pass, scope_t *scope, mp_obj_t *error_slot);
    void (*end_pass)(emit_inline_asm_t *emit, mp_uint_t type_sig);
    mp_uint_t (*count_params)(emit_inline_asm_t *emit, mp_uint_t n_params, mp_parse_node_t *pn_params);
    bool (*label)(emit_inline_asm_t *emit, mp_uint_t label_num, qstr label_id);
    void (*align)(emit_inline_asm_t *emit, mp_uint_t align);
    void (*data)(emit_inline_asm_t *emit, mp_uint_t bytesize, mp_uint_t val);
    void (*op)(emit_inline_asm_t *emit, qstr op, mp_uint_t n_args, mp_parse_node_t *pn_args);
} emit_inline_asm_method_table_t;

extern const emit_inline_asm_method_table_t emit_inline_thumb_method_table;

emit_inline_asm_t *emit_inline_thumb_new(mp_uint_t max_num_labels);
void micropy_emit_inline_thumb_free(struct _mp_state_ctx_t *mp_state, emit_inline_asm_t *emit);

#if MICROPY_WARNINGS
void micropy_emitter_warning(struct _mp_state_ctx_t *mp_state, pass_kind_t pass, const char *msg);
#else
#define micropy_emitter_warning(mp_state, pass, msg)
#endif

#endif // __MICROPY_INCLUDED_PY_EMIT_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_BC0_H__
#define __MICROPY_INCLUDED_PY_BC0_H__

// Micro Python byte-codes.
// The comment at the end of the line (if it exists) tells the arguments to the byte-code.

#define MP_BC_LOAD_CONST_FALSE   (0x10)
#define MP_BC_LOAD_CONST_NONE    (0x11)
#define MP_BC_LOAD_CONST_TRUE    (0x12)
#define MP_BC_LOAD_CONST_SMALL_INT   (0x14) // signed var-int
#define MP_BC_LOAD_CONST_STRING  (0x16) // qstr
#define MP_BC_LOAD_CONST_OBJ     (0x17) // ptr
#define MP_BC_LOAD_NULL          (0x18)

#define MP_BC_LOAD_FAST_N        (0x1a) // uint
#define MP_BC_LOAD_DEREF         (0x1b) // uint
#define MP_BC_LOAD_NAME          (0x1c) // qstr
#define MP_BC_LOAD_GLOBAL        (0x1d) // qstr
#define MP_BC_LOAD_ATTR          (0x1e) // qstr
#define MP_BC_LOAD_METHOD        (0x1f) // qstr
#define MP_BC_LOAD_BUILD_CLASS   (0x20)
#define MP_BC_LOAD_SUBSCR        (0x21)

#define MP_BC_STORE_FAST_N       (0x22) // uint
#define MP_BC_STORE_DEREF        (0x23) // uint
#define MP_BC_STORE_NAME         (0x24) // qstr
#define MP_BC_STORE_GLOBAL       (0x25) // qstr
#define MP_BC_STORE_ATTR         (0x26) // qstr
#define MP_BC_STORE_SUBSCR       (0x27)

#define MP_BC_DELETE_FAST        (0x28) // uint
#define MP_BC_DELETE_DEREF       (0x29) // uint
#define MP_BC_DELETE_NAME        (0x2a) // qstr
#define MP_BC_DELETE_GLOBAL      (0x2b) // qstr

#define MP_BC_DUP_TOP            (0x30)
#define MP_BC_DUP_TOP_TWO        (0x31)
#define MP_BC_POP_TOP            (0x32)
#define MP_BC_ROT_TWO            (0x33)
#define MP_BC_ROT_THREE          (0x34)

#define MP_BC_JUMP               (0x35) // rel byte code offset, 16-bit signed, in excess
#define MP_BC_POP_JUMP_IF_TRUE   (0x36) // rel byte code offset, 16-bit signed, in excess
#define MP_BC_POP_JUMP_IF_FALSE  (0x37) // rel byte code offset, 16-bit signed, in excess
#define MP_BC_JUMP_IF_TRUE_OR_POP    (0x38) // rel byte code offset, 16-bit signed, in excess
#define MP_BC_JUMP_IF_FALSE_OR_POP   (0x39) // rel byte code offset, 16-bit signed, in excess
#define MP_BC_SETUP_WITH         (0x3d) // rel byte code offset, 16-bit unsigned
#define MP_BC_WITH_CLEANUP       (0x3e)
#define MP_BC_SETUP_EXCEPT       (0x3f) // rel byte code offset, 16-bit unsigned
#define MP_BC_SETUP_FINALLY      (0x40) // rel byte code offset, 16-bit unsigned
#define MP_BC_END_FINALLY        (0x41)
#define MP_BC_GET_ITER           (0x42)
#define MP_BC_FOR_ITER           (0x43) // rel byte code offset, 16-bit unsigned
#define MP_BC_POP_BLOCK          (0x44)
#define MP_BC_POP_EXCEPT         (0x45)
#define MP_BC_UNWIND_JUMP        (0x46) // rel byte code offset, 16-bit signed, in excess; then a byte

#define MP_BC_BUILD_TUPLE        (0x50) // uint
#define MP_BC_BUILD_LIST         (0x51) // uint
#define MP_BC_LIST_APPEND        (0x52) // uint
#define MP_BC_BUILD_MAP          (0x53) // uint
#define MP_BC_STORE_MAP          (0x54)
#define MP_BC_MAP_ADD            (0x55) // uint
#define MP_BC_BUILD_SET          (0x56) // uint
#define MP_BC_SET_ADD            (0x57) // uint
#define MP_BC_BUILD_SLICE        (0x58) // uint
#define MP_BC_UNPACK_SEQUENCE    (0x59) // uint
#define MP_BC_UNPACK_EX          (0x5a) // uint

#define MP_BC_RETURN_VALUE       (0x5b)
#define MP_BC_RAISE_VARARGS      (0x5c) // byte
#define MP_BC_YIELD_VALUE        (0x5d)
#define MP_BC_YIELD_FROM         (0x5e)

#define MP_BC_MAKE_FUNCTION         (0x60) // uint
#define MP_BC_MAKE_FUNCTION_DEFARGS (0x61) // uint
#define MP_BC_MAKE_CLOSURE          (0x62) // uint
#define MP_BC_MAKE_CLOSURE_DEFARGS  (0x63) // uint
#define MP_BC_CALL_FUNCTION         (0x64) // uint
#define MP_BC_CALL_FUNCTION_VAR_KW  (0x65) // uint
#define MP_BC_CALL_METHOD           (0x66) // uint
#define MP_BC_CALL_METHOD_VAR_KW    (0x67) // uint

#define MP_BC_IMPORT_NAME        (0x68) // qstr
#define MP_BC_IMPORT_FROM        (0x69) // qstr
#define MP_BC_IMPORT_STAR        (0x6a)

#define MP_BC_LOAD_CONST_SMALL_INT_MULTI (0x70) // + N(64)
#define MP_BC_LOAD_FAST_MULTI            (0xb0) // + N(16)
#define MP_BC_STORE_FAST_MULTI           (0xc0) // + N(16)
#define MP_BC_UNARY_OP_MULTI             (0xd0) // + op(7)
#define MP_BC_BINARY_OP_MULTI            (0xd7) // + op(36)

#endif // __MICROPY_INCLUDED_PY_BC0_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_BC_H__
#define __MICROPY_INCLUDED_PY_BC_H__

//#include "py/runtime.h"
//#include "py/obj.h"

// bytecode layout:
//
//  n_state         : var uint
//  n_exc_stack     : var uint
//  scope_flags     : byte
//  n_pos_args      : byte          number of arguments this function takes
//  n_kwonly_args   : byte          number of keyword-only arguments this function takes
//  n_def_pos_args  : byte          number of default positional arguments
//
//  code_info_size  : var uint |    code_info_size counts bytes in this chunk
//  simple_name     : var qstr |
//  source_file     : var qstr |
//  <line number info>         |
//  <word alignment padding>   |    only needed if bytecode contains pointers
//
//  local_num0      : byte     |
//  ...             : byte     |
//  local_numN      : byte     |    N = num_cells
//  255             : byte     |    end of list sentinel
//  <bytecode>                 |
//
//
// constant table layout:
//
//  argname0        : obj (qstr)
//  ...             : obj (qstr)
//  argnameN        : obj (qstr)    N = num_pos_args + num_kwonly_args
//  const0          : obj
//  constN          : obj

// Exception stack entry
typedef struct _mp_exc_stack {
    const byte *handler;
    // bit 0 is saved currently_in_except_block value
    // bit 1 is whether the opcode was SETUP_WITH or SETUP_FINALLY
    mp_obj_t *val_sp;
    // Saved exception, valid if currently_in_except_block bit is 1
    mp_obj_base_t *prev_exc;
} mp_exc_stack_t;

typedef struct _mp_code_state {
    const byte *code_info;
    const byte *ip;
    const mp_uint_t *const_table;
    mp_obj_t *sp;
    // bit 0 is saved currently_in_except_block value
    mp_exc_stack_t *exc_sp;
    mp_obj_dict_t *old_globals;
    #if MICROPY_STACKLESS
    struct _mp_code_state *prev;
    #endif
    size_t n_state;
    // Variable-length
    mp_obj_t state[0];
    // Variable-length, never accessed by name, only as (void*)(state + n_state)
    //mp_exc_stack_t exc_state[0];
} mp_code_state;

mp_uint_t micropy_decode_uint(struct _mp_state_ctx_t *mp_state, const byte **ptr);

mp_vm_return_kind_t micropy_execute_bytecode(struct _mp_state_ctx_t *mp_state, mp_code_state *code_state, volatile mp_obj_t inject_exc);
mp_code_state *mp_obj_fun_bc_prepare_codestate(mp_obj_t func, size_t n_args, size_t n_kw, const mp_obj_t *args);
struct _mp_obj_fun_bc_t;
void micropy_setup_code_state(struct _mp_state_ctx_t *mp_state, mp_code_state *code_state, struct _mp_obj_fun_bc_t *self, size_t n_args, size_t n_kw, const mp_obj_t *args);
void micropy_bytecode_print(struct _mp_state_ctx_t *mp_state, const void *descr, const byte *code, mp_uint_t len, const mp_uint_t *const_table);
void micropy_bytecode_print2(struct _mp_state_ctx_t *mp_state, const byte *code, mp_uint_t len);
const byte *micropy_bytecode_print_str(struct _mp_state_ctx_t *mp_state, const byte *ip);
#define micropy_bytecode_print_inst(mp_state, code) micropy_bytecode_print2(mp_state, code, 1)

// Helper macros to access pointer with least significant bits holding flags
#define MP_TAGPTR_PTR(x) ((void*)((uintptr_t)(x) & ~((uintptr_t)3)))
#define MP_TAGPTR_TAG0(x) ((uintptr_t)(x) & 1)
#define MP_TAGPTR_TAG1(x) ((uintptr_t)(x) & 2)
#define MP_TAGPTR_MAKE(ptr, tag) ((void*)((uintptr_t)(ptr) | (tag)))

#if MICROPY_PERSISTENT_CODE_LOAD || MICROPY_PERSISTENT_CODE_SAVE

#define MP_OPCODE_BYTE (0)
#define MP_OPCODE_QSTR (1)
#define MP_OPCODE_VAR_UINT (2)
#define MP_OPCODE_OFFSET (3)

uint micropy_opcode_format(struct _mp_state_ctx_t *mp_state, const byte *ip, size_t *opcode_size);

#endif

#endif // __MICROPY_INCLUDED_PY_BC_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_OBJSTR_H__
#define __MICROPY_INCLUDED_PY_OBJSTR_H__

//#include "py/obj.h"

typedef struct _mp_obj_str_t {
    mp_obj_base_t base;
    mp_uint_t hash;
    // len == number of bytes used in data, alloc = len + 1 because (at the moment) we also append a null byte
    mp_uint_t len;
    const byte *data;
} mp_obj_str_t;

#define MP_DEFINE_STR_OBJ(obj_name, str) mp_obj_str_t obj_name = {{&mp_type_str}, 0, sizeof(str) - 1, (const byte*)str}

// use this macro to extract the string hash
#define GET_STR_HASH(str_obj_in, str_hash) \
    mp_uint_t str_hash; if (MP_OBJ_IS_QSTR(str_obj_in)) \
    { str_hash = micropy_qstr_hash(mp_state, MP_OBJ_QSTR_VALUE(str_obj_in)); } else { str_hash = ((mp_obj_str_t*)MP_OBJ_TO_PTR(str_obj_in))->hash; }

// use this macro to extract the string length
#define GET_STR_LEN(str_obj_in, str_len) \
    size_t str_len; if (MP_OBJ_IS_QSTR(str_obj_in)) \
    { str_len = micropy_qstr_len(mp_state, MP_OBJ_QSTR_VALUE(str_obj_in)); } else { str_len = ((mp_obj_str_t*)MP_OBJ_TO_PTR(str_obj_in))->len; }

// use this macro to extract the string data and length
#if MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_C
const byte *micropy_obj_str_get_data_no_check(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, size_t *len);
#define GET_STR_DATA_LEN(str_obj_in, str_data, str_len) \
    size_t str_len; const byte *str_data = micropy_obj_str_get_data_no_check(mp_state, str_obj_in, &str_len);
#else
#define GET_STR_DATA_LEN(str_obj_in, str_data, str_len) \
    const byte *str_data; size_t str_len; if (MP_OBJ_IS_QSTR(str_obj_in)) \
    { str_data = micropy_qstr_data(mp_state, MP_OBJ_QSTR_VALUE(str_obj_in), &str_len); } \
    else { str_len = ((mp_obj_str_t*)MP_OBJ_TO_PTR(str_obj_in))->len; str_data = ((mp_obj_str_t*)MP_OBJ_TO_PTR(str_obj_in))->data; }
#endif

mp_obj_t micropy_obj_str_make_new(struct _mp_state_ctx_t *mp_state, const mp_obj_type_t *type_in, size_t n_args, size_t n_kw, const mp_obj_t *args);
void micropy_str_print_json(struct _mp_state_ctx_t *mp_state, const mp_print_t *print, const byte *str_data, size_t str_len);
mp_obj_t micropy_obj_str_format(struct _mp_state_ctx_t *mp_state, size_t n_args, const mp_obj_t *args, mp_map_t *kwargs);
mp_obj_t micropy_obj_str_split(struct _mp_state_ctx_t *mp_state, size_t n_args, const mp_obj_t *args);
mp_obj_t micropy_obj_new_str_of_type(struct _mp_state_ctx_t *mp_state, const mp_obj_type_t *type, const byte* data, size_t len);

mp_obj_t micropy_obj_str_binary_op(struct _mp_state_ctx_t *mp_state, mp_uint_t op, mp_obj_t lhs_in, mp_obj_t rhs_in);
mp_int_t micropy_obj_str_get_buffer(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags);

const byte *micropy_str_index_to_ptr(struct _mp_state_ctx_t *mp_state, const mp_obj_type_t *type, const byte *self_data, size_t self_len,
                             mp_obj_t index, bool is_slice);
const byte *micropy_find_subbytes(struct _mp_state_ctx_t *mp_state, const byte *haystack, mp_uint_t hlen, const byte *needle, mp_uint_t nlen, mp_int_t direction);

MP_DECLARE_CONST_FUN_OBJ(str_encode_obj);
MP_DECLARE_CONST_FUN_OBJ(str_find_obj);
MP_DECLARE_CONST_FUN_OBJ(str_rfind_obj);
MP_DECLARE_CONST_FUN_OBJ(str_index_obj);
MP_DECLARE_CONST_FUN_OBJ(str_rindex_obj);
MP_DECLARE_CONST_FUN_OBJ(str_join_obj);
MP_DECLARE_CONST_FUN_OBJ(str_split_obj);
MP_DECLARE_CONST_FUN_OBJ(str_splitlines_obj);
MP_DECLARE_CONST_FUN_OBJ(str_rsplit_obj);
MP_DECLARE_CONST_FUN_OBJ(str_startswith_obj);
MP_DECLARE_CONST_FUN_OBJ(str_endswith_obj);
MP_DECLARE_CONST_FUN_OBJ(str_strip_obj);
MP_DECLARE_CONST_FUN_OBJ(str_lstrip_obj);
MP_DECLARE_CONST_FUN_OBJ(str_rstrip_obj);
MP_DECLARE_CONST_FUN_OBJ(str_format_obj);
MP_DECLARE_CONST_FUN_OBJ(str_replace_obj);
MP_DECLARE_CONST_FUN_OBJ(str_count_obj);
MP_DECLARE_CONST_FUN_OBJ(str_partition_obj);
MP_DECLARE_CONST_FUN_OBJ(str_rpartition_obj);
MP_DECLARE_CONST_FUN_OBJ(str_center_obj);
MP_DECLARE_CONST_FUN_OBJ(str_lower_obj);
MP_DECLARE_CONST_FUN_OBJ(str_upper_obj);
MP_DECLARE_CONST_FUN_OBJ(str_isspace_obj);
MP_DECLARE_CONST_FUN_OBJ(str_isalpha_obj);
MP_DECLARE_CONST_FUN_OBJ(str_isdigit_obj);
MP_DECLARE_CONST_FUN_OBJ(str_isupper_obj);
MP_DECLARE_CONST_FUN_OBJ(str_islower_obj);

#endif // __MICROPY_INCLUDED_PY_OBJSTR_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_OBJMODULE_H__
#define __MICROPY_INCLUDED_PY_OBJMODULE_H__

//#include "py/obj.h"

void micropy_module_init(struct _mp_state_ctx_t *mp_state);
void micropy_module_deinit(struct _mp_state_ctx_t *mp_state);
mp_obj_t micropy_module_get(struct _mp_state_ctx_t *mp_state, qstr module_name);
void micropy_module_register(struct _mp_state_ctx_t *mp_state, qstr qstr, mp_obj_t module);

#endif // __MICROPY_INCLUDED_PY_OBJMODULE_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_OBJGENERATOR_H__
#define __MICROPY_INCLUDED_PY_OBJGENERATOR_H__

//#include "py/obj.h"
//#include "py/runtime.h"

mp_vm_return_kind_t micropy_obj_gen_resume(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, mp_obj_t send_val, mp_obj_t throw_val, mp_obj_t *ret_val);

#endif // __MICROPY_INCLUDED_PY_OBJGENERATOR_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

enum {
    MP_FROZEN_NONE,
    MP_FROZEN_STR,
    MP_FROZEN_MPY,
};

int micropy_find_frozen_module(struct _mp_state_ctx_t *mp_state, const char *str, size_t len, void **data);
mp_import_stat_t micropy_frozen_stat(struct _mp_state_ctx_t *mp_state, const char *str);
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_OBJFUN_H__
#define __MICROPY_INCLUDED_PY_OBJFUN_H__

//#include "py/obj.h"

typedef struct _mp_obj_fun_bc_t {
    mp_obj_base_t base;
    mp_obj_dict_t *globals;         // the context within which this function was defined
    const byte *bytecode;           // bytecode for the function
    const mp_uint_t *const_table;   // constant table
    // the following extra_args array is allocated space to take (in order):
    //  - values of positional default args (if any)
    //  - a single slot for default kw args dict (if it has them)
    //  - a single slot for var args tuple (if it takes them)
    //  - a single slot for kw args dict (if it takes them)
    mp_obj_t extra_args[];
} mp_obj_fun_bc_t;

#endif // __MICROPY_INCLUDED_PY_OBJFUN_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_OBJTYPE_H__
#define __MICROPY_INCLUDED_PY_OBJTYPE_H__

//#include "py/obj.h"

// instance object
// creating an instance of a class makes one of these objects
typedef struct _mp_obj_instance_t {
    mp_obj_base_t base;
    mp_map_t members;
    mp_obj_t subobj[];
    // TODO maybe cache __getattr__ and __setattr__ for efficient lookup of them
} mp_obj_instance_t;

// this needs to be exposed for MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE to work
void micropy_obj_instance_attr(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, qstr attr, mp_obj_t *dest);

// these need to be exposed so mp_obj_is_callable can work correctly
bool micropy_obj_instance_is_callable(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in);
mp_obj_t micropy_obj_instance_call(struct _mp_state_ctx_t *mp_state, mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args);

#define micropy_obj_is_instance_type(mp_state, type) ((type)->make_new == micropy_obj_instance_make_new)
#define micropy_obj_is_native_type(mp_state, type) ((type)->make_new != micropy_obj_instance_make_new)
// this needs to be exposed for the above macros to work correctly
mp_obj_t micropy_obj_instance_make_new(struct _mp_state_ctx_t *mp_state, const mp_obj_type_t *self_in, size_t n_args, size_t n_kw, const mp_obj_t *args);

#endif // __MICROPY_INCLUDED_PY_OBJTYPE_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_PARSENUMBASE_H__
#define __MICROPY_INCLUDED_PY_PARSENUMBASE_H__

//#include "py/mpconfig.h"

size_t micropy_parse_num_base(struct _mp_state_ctx_t *mp_state, const char *str, size_t len, int *base);

#endif // __MICROPY_INCLUDED_PY_PARSENUMBASE_H__
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __MICROPY_INCLUDED_PY_MPERRNO_H__
#define __MICROPY_INCLUDED_PY_MPERRNO_H__

#if MICROPY_USE_INTERNAL_ERRNO

// MP_Exxx errno's are defined directly as numeric values
// (Linux constants are used as a reference)

#define MP_EPERM              (1) // Operation not permitted
#define MP_ENOENT             (2) // No such file or directory
#define MP_ESRCH              (3) // No such process
#define MP_EINTR              (4) // Interrupted system call
#define MP_EIO                (5) // I/O error
#define MP_ENXIO              (6) // No such device or address
#define MP_E2BIG              (7) // Argument list too long
#define MP_ENOEXEC            (8) // Exec format error
#define MP_EBADF              (9) // Bad file number
#define MP_ECHILD            (10) // No child processes
#define MP_EAGAIN            (11) // Try again
#define MP_ENOMEM            (12) // Out of memory
#define MP_EACCES            (13) // Permission denied
#define MP_EFAULT            (14) // Bad address
#define MP_ENOTBLK           (15) // Block device required
#define MP_EBUSY             (16) // Device or resource busy
#define MP_EEXIST            (17) // File exists
#define MP_EXDEV             (18) // Cross-device link
#define MP_ENODEV            (19) // No such device
#define MP_ENOTDIR           (20) // Not a directory
#define MP_EISDIR            (21) // Is a directory
#define MP_EINVAL            (22) // Invalid argument
#define MP_ENFILE            (23) // File table overflow
#define MP_EMFILE            (24) // Too many open files
#define MP_ENOTTY            (25) // Not a typewriter
#define MP_ETXTBSY           (26) // Text file busy
#define MP_EFBIG             (27) // File too large
#define MP_ENOSPC            (28) // No space left on device
#define MP_ESPIPE            (29) // Illegal seek
#define MP_EROFS             (30) // Read-only file system
#define MP_EMLINK            (31) // Too many links
#define MP_EPIPE             (32) // Broken pipe
#define MP_EDOM              (33) // Math argument out of domain of func
#define MP_ERANGE            (34) // Math result not representable
#define MP_EWOULDBLOCK  MP_EAGAIN // Operation would block
#define MP_EOPNOTSUPP        (95) // Operation not supported on transport endpoint
#define MP_EAFNOSUPPORT      (97) // Address family not supported by protocol
#define MP_EADDRINUSE        (98) // Address already in use
#define MP_ECONNABORTED     (103) // Software caused connection abort
#define MP_ECONNRESET       (104) // Connection reset by peer
#define MP_ENOBUFS          (105) // No buffer space available
#define MP_ENOTCONN         (107) // Transport endpoint is not connected
#define MP_ETIMEDOUT        (110) // Connection timed out
#define MP_ECONNREFUSED     (111) // Connection refused
#define MP_EHOSTUNREACH     (113) // No route to host
#define MP_EALREADY         (114) // Operation already in progress
#define MP_EINPROGRESS      (115) // Operation now in progress

#else

// MP_Exxx errno's are defined in terms of system supplied ones

#include <errno.h>

#define MP_EPERM            EPERM
#define MP_ENOENT           ENOENT
#define MP_ESRCH            ESRCH
#define MP_EINTR            EINTR
#define MP_EIO              EIO
#define MP_ENXIO            ENXIO
#define MP_E2BIG            E2BIG
#define MP_ENOEXEC          ENOEXEC
#define MP_EBADF            EBADF
#define MP_ECHILD           ECHILD
#define MP_EAGAIN           EAGAIN
#define MP_ENOMEM           ENOMEM
#define MP_EACCES           EACCES
#define MP_EFAULT           EFAULT
#define MP_ENOTBLK          ENOTBLK
#define MP_EBUSY            EBUSY
#define MP_EEXIST           EEXIST
#define MP_EXDEV            EXDEV
#define MP_ENODEV           ENODEV
#define MP_ENOTDIR          ENOTDIR
#define MP_EISDIR           EISDIR
#define MP_EINVAL           EINVAL
#define MP_ENFILE           ENFILE
#define MP_EMFILE           EMFILE
#define MP_ENOTTY           ENOTTY
#define MP_ETXTBSY          ETXTBSY
#define MP_EFBIG            EFBIG
#define MP_ENOSPC           ENOSPC
#define MP_ESPIPE           ESPIPE
#define MP_EROFS            EROFS
#define MP_EMLINK           EMLINK
#define MP_EPIPE            EPIPE
#define MP_EDOM             EDOM
#define MP_ERANGE           ERANGE
#define MP_EWOULDBLOCK      EAGAIN
#define MP_EOPNOTSUPP       EOPNOTSUPP
#define MP_EAFNOSUPPORT     EAFNOSUPPORT
#define MP_EADDRINUSE       EADDRINUSE
#define MP_ECONNABORTED     ECONNABORTED
#define MP_ECONNRESET       ECONNRESET
#define MP_ENOBUFS          ENOBUFS
#define MP_ENOTCONN         ENOTCONN
#define MP_ETIMEDOUT        ETIMEDOUT
#define MP_ECONNREFUSED     ECONNREFUSED
#define MP_EHOSTUNREACH     EHOSTUNREACH
#define MP_EALREADY         EALREADY
#define MP_EINPROGRESS      EINPROGRESS

#endif

#if MICROPY_PY_UERRNO
qstr micropy_errno_to_str(struct _mp_state_ctx_t *mp_state, mp_obj_t errno_val);
#endif

#endif // __MICROPY_INCLUDED_PY_MPERRNO_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_MPHAL_H__
#define __MICROPY_INCLUDED_PY_MPHAL_H__

//#include "py/mpconfig.h"

#ifdef MICROPY_MPHALPORT_H
#include MICROPY_MPHALPORT_H
#else
//#include <mphalport.h>
#endif

struct _mp_state_ctx_t;
#ifndef mp_hal_stdin_rx_chr
int micropy_hal_stdin_rx_chr(struct _mp_state_ctx_t *mp_state);
#endif

#ifndef mp_hal_stdout_tx_str
void micropy_hal_stdout_tx_str(struct _mp_state_ctx_t *mp_state, const char *str);
#endif

#ifndef mp_hal_stdout_tx_strn
void micropy_hal_stdout_tx_strn(struct _mp_state_ctx_t *mp_state, const char *str, size_t len);
#endif

#ifndef mp_hal_stdout_tx_strn_cooked
void micropy_hal_stdout_tx_strn_cooked(struct _mp_state_ctx_t *mp_state, const char *str, size_t len);
#endif

#ifndef mp_hal_delay_ms
void micropy_hal_delay_ms(struct _mp_state_ctx_t *mp_state, mp_uint_t ms);
#endif

#ifndef mp_hal_delay_us
void micropy_hal_delay_us(struct _mp_state_ctx_t *mp_state, mp_uint_t us);
#endif

#ifndef mp_hal_ticks_ms
mp_uint_t micropy_hal_ticks_ms(struct _mp_state_ctx_t *mp_state);
#endif

#ifndef mp_hal_ticks_us
mp_uint_t micropy_hal_ticks_us(struct _mp_state_ctx_t *mp_state);
#endif

// If port HAL didn't define its own pin API, use generic
// "virtual pin" API from the core.
#ifndef mp_hal_pin_obj_t
#define mp_hal_pin_obj_t mp_obj_t
#define micropy_hal_get_pin_obj(mp_state, pin) (pin)
#define micropy_hal_pin_read(mp_state, pin) micropy_virtual_pin_read(mp_state, pin)
#define micropy_hal_pin_write(mp_state, pin, v) micropy_virtual_pin_write(mp_state, pin, v)
#endif

#endif // __MICROPY_INCLUDED_PY_MPHAL_H__
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Paul Sokolovsky
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef __MICROPY_INCLUDED_PY_RINGBUF_H__
#define __MICROPY_INCLUDED_PY_RINGBUF_H__

typedef struct _ringbuf_t {
    uint8_t *buf;
    uint16_t size;
    uint16_t iget;
    uint16_t iput;
} ringbuf_t;

// Static initalization:
// byte buf_array[N];
// ringbuf_t buf = {buf_array, sizeof(buf_array)};

// Dynamic initialization. This creates root pointer!
#define micropy_ringbuf_alloc(mp_state, r, sz) \
{ \
    (r)->buf = micropy_m_new(mp_state, uint8_t, sz); \
    (r)->size = sz; \
    (r)->iget = (r)->iput = 0; \
}

static inline int micropy_ringbuf_get(struct _mp_state_ctx_t *mp_state, ringbuf_t *r) {
    if (r->iget == r->iput) {
        return -1;
    }
    uint8_t v = r->buf[r->iget++];
    if (r->iget >= r->size) {
        r->iget = 0;
    }
    return v;
}

static inline int micropy_ringbuf_put(struct _mp_state_ctx_t *mp_state, ringbuf_t *r, uint8_t v) {
    uint32_t iput_new = r->iput + 1;
    if (iput_new >= r->size) {
        iput_new = 0;
    }
    if (iput_new == r->iget) {
        return -1;
    }
    r->buf[r->iput] = v;
    r->iput = iput_new;
    return 0;
}

#endif // __MICROPY_INCLUDED_PY_RINGBUF_H__
/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Paul Sokolovsky
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef MICROPY_EXTMOD_MODUBINASCII
#define MICROPY_EXTMOD_MODUBINASCII

extern mp_obj_t mod_binascii_hexlify(size_t n_args, const mp_obj_t *args);
extern mp_obj_t mod_binascii_unhexlify(mp_obj_t data);
extern mp_obj_t mod_binascii_a2b_base64(mp_obj_t data);
extern mp_obj_t mod_binascii_b2a_base64(mp_obj_t data);

MP_DECLARE_CONST_FUN_OBJ(mod_binascii_hexlify_obj);
MP_DECLARE_CONST_FUN_OBJ(mod_binascii_unhexlify_obj);
MP_DECLARE_CONST_FUN_OBJ(mod_binascii_a2b_base64_obj);
MP_DECLARE_CONST_FUN_OBJ(mod_binascii_b2a_base64_obj);

#endif /* MICROPY_EXTMOD_MODUBINASCII */
mp_state_ctx_t *micropy_create(void *heap, size_t heap_size);
void micropy_destroy(mp_state_ctx_t *mp);
int micropy_exec_str(mp_state_ctx_t *mp, const char *str);
