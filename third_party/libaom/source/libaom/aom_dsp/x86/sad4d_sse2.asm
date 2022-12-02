;
; Copyright (c) 2016, Alliance for Open Media. All rights reserved
;
; This source code is subject to the terms of the BSD 2 Clause License and
; the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
; was not distributed with this source code in the LICENSE file, you can
; obtain it at www.aomedia.org/license/software. If the Alliance for Open
; Media Patent License 1.0 was not distributed with this source code in the
; PATENTS file, you can obtain it at www.aomedia.org/license/patent.
;

;

%include "third_party/x86inc/x86inc.asm"

SECTION .text

%macro AVG_4x2x4 2
  movh                  m2, [second_predq]
  movlhps               m2, m2
  pavgb                 %1, m2
  pavgb                 %2, m2
  lea                   second_predq, [second_predq+8]
%endmacro
; 'spill_src_stride' affect a lot how the code works.
;
; When 'spill_src_stride' is false, the 'src_strideq' resides in
; register, [srcq + src_strideq + offset] is allowed, so we can simply
; use such form to access src memory and don't bother to update 'srcq'
; at each line. We only update 'srcq' each two-lines using a compact
; LEA instruction like [srcq+src_strideq*2].
;
; When 'spill_src_stride' is true, the 'src_strideq' resides in memory.
; we cannot use above form to access memory, we have to update
; 'srcq' at each line break. As we process two parts (first,second)
; together in each macro function, the second part may also sit
; in the next line, which means we also need to possibly add
; one 'src_strideq' to 'srcq' before processing second part.

%macro HANDLE_SECOND_OFFSET 0
  %if spill_src_stride
    %define second_offset 0
    add srcq, src_strideq
  %else
    %define second_offset (src_strideq)
  %endif
%endmacro

; This is specically designed to handle when src_strideq is a
; memory position, under such case, we can not accomplish
; complex address calculation using LEA, and fall back to
; using simple ADD instruction at each line ending.
%macro ADVANCE_END_OF_TWO_LINES 0
  %if spill_src_stride
    add srcq, src_strideq
  %else
    lea                 srcq, [srcq+src_strideq*2]
  %endif

; note: ref_stride is never spilled when processing two lines
  lea                ref1q, [ref1q+ref_strideq*2]
  lea                ref2q, [ref2q+ref_strideq*2]
  lea                ref3q, [ref3q+ref_strideq*2]
  lea                ref4q, [ref4q+ref_strideq*2]
%endmacro

; PROCESS_4x2x4 first, do_avg
%macro PROCESS_4x2x4 2
  movd                  m0, [srcq]
  HANDLE_SECOND_OFFSET
%if %1 == 1
  movd                  m6, [ref1q]
  movd                  m4, [ref2q]
  movd                  m7, [ref3q]
  movd                  m5, [ref4q]

  movd                  m1, [srcq + second_offset]
  movd                  m2, [ref1q+ref_strideq]
  punpckldq             m0, m1
  punpckldq             m6, m2
  movd                  m1, [ref2q+ref_strideq]
  movd                  m2, [ref3q+ref_strideq]
  movd                  m3, [ref4q+ref_strideq]
  punpckldq             m4, m1
  punpckldq             m7, m2
  punpckldq             m5, m3
  movlhps               m0, m0
  movlhps               m6, m4
  movlhps               m7, m5
%if %2 == 1
  AVG_4x2x4             m6, m7
%endif
  psadbw                m6, m0
  psadbw                m7, m0
%else
  movd                  m1, [ref1q]
  movd                  m5, [ref1q+ref_strideq]
  movd                  m2, [ref2q]
  movd                  m4, [ref2q+ref_strideq]
  punpckldq             m1, m5
  punpckldq             m2, m4
  movd                  m3, [ref3q]
  movd                  m5, [ref3q+ref_strideq]
  punpckldq             m3, m5
  movd                  m4, [ref4q]
  movd                  m5, [ref4q+ref_strideq]
  punpckldq             m4, m5
  movd                  m5, [srcq + second_offset]
  punpckldq             m0, m5
  movlhps               m0, m0
  movlhps               m1, m2
  movlhps               m3, m4
%if %2 == 1
  AVG_4x2x4             m1, m3
%endif
  psadbw                m1, m0
  psadbw                m3, m0
  paddd                 m6, m1
  paddd                 m7, m3
%endif
%endmacro

; PROCESS_8x2x4 first, do_avg
%macro PROCESS_8x2x4 2
  movh                  m0, [srcq]
  HANDLE_SECOND_OFFSET
%if %1 == 1
  movh                  m4, [ref1q]
  movh                  m5, [ref2q]
  movh                  m6, [ref3q]
  movh                  m7, [ref4q]
  movhps                m0, [srcq + second_offset]
  movhps                m4, [ref1q+ref_strideq]
  movhps                m5, [ref2q+ref_strideq]
  movhps                m6, [ref3q+ref_strideq]
  movhps                m7, [ref4q+ref_strideq]
%if %2 == 1
  movu                  m3, [second_predq]
  pavgb                 m4, m3
  pavgb                 m5, m3
  pavgb                 m6, m3
  pavgb                 m7, m3
  lea                   second_predq, [second_predq+mmsize]
%endif
  psadbw                m4, m0
  psadbw                m5, m0
  psadbw                m6, m0
  psadbw                m7, m0
%else
  movh                  m1, [ref1q]
  movh                  m2, [ref2q]
  movhps                m0, [srcq + second_offset]
  movhps                m1, [ref1q+ref_strideq]
  movhps                m2, [ref2q+ref_strideq]
%if %2 == 1
  movu                  m3, [second_predq]
  pavgb                 m1, m3
  pavgb                 m2, m3
%endif
  psadbw                m1, m0
  psadbw                m2, m0
  paddd                 m4, m1
  paddd                 m5, m2

  movh                  m1, [ref3q]
  movhps                m1, [ref3q+ref_strideq]
  movh                  m2, [ref4q]
  movhps                m2, [ref4q+ref_strideq]
%if %2 == 1
  pavgb                 m1, m3
  pavgb                 m2, m3
  lea                   second_predq, [second_predq+mmsize]
%endif
  psadbw                m1, m0
  psadbw                m2, m0
  paddd                 m6, m1
  paddd                 m7, m2
%endif
%endmacro

; PROCESS_FIRST_MMSIZE do_avg
%macro PROCESS_FIRST_MMSIZE 1
  mova                  m0, [srcq]
  movu                  m4, [ref1q]
  movu                  m5, [ref2q]
  movu                  m6, [ref3q]
  movu                  m7, [ref4q]
%if %1 == 1
  movu                  m3, [second_predq]
  pavgb                 m4, m3
  pavgb                 m5, m3
  pavgb                 m6, m3
  pavgb                 m7, m3
  lea                   second_predq, [second_predq+mmsize]
%endif
  psadbw                m4, m0
  psadbw                m5, m0
  psadbw                m6, m0
  psadbw                m7, m0
%endmacro

; PROCESS_16x1x4 offset, do_avg
%macro PROCESS_16x1x4 2
  mova                  m0, [srcq + %1]
  movu                  m1, [ref1q + ref_offsetq + %1]
  movu                  m2, [ref2q + ref_offsetq + %1]
%if %2 == 1
  movu                  m3, [second_predq]
  pavgb                 m1, m3
  pavgb                 m2, m3
%endif
  psadbw                m1, m0
  psadbw                m2, m0
  paddd                 m4, m1
  paddd                 m5, m2

  movu                  m1, [ref3q + ref_offsetq + %1]
  movu                  m2, [ref4q + ref_offsetq + %1]
%if %2 == 1
  pavgb                 m1, m3
  pavgb                 m2, m3
  lea                   second_predq, [second_predq+mmsize]
%endif
  psadbw                m1, m0
  psadbw                m2, m0
  paddd                 m6, m1
  paddd                 m7, m2
%endmacro

; void aom_sadNxNx4d_sse2(uint8_t *src,    int src_stride,
;                         uint8_t *ref[4], int ref_stride,
;                         uint32_t res[4]);
; Macro Arguments:
;   1: Width
;   2: Height
;   3: If 0, then normal sad, else avg
;   4: If 0, then normal sad, else skip rows
%macro SADNXN4D 2-4 0,0

%define spill_src_stride 0
%define spill_ref_stride 0
%define spill_cnt 0

; Whether a shared offset should be used instead of adding strides to
; each reference array. With this option, only one line will be processed
; per loop iteration.
%define use_ref_offset (%1 >= mmsize)

; Remove loops in the 4x4 and 8x4 case
%define use_loop (use_ref_offset || %2 > 4)

%if %4 == 1  ; skip rows
%if ARCH_X86_64
%if use_ref_offset
cglobal sad_skip_%1x%2x4d, 5, 10, 8, src, src_stride, ref1, ref_stride, res, \
                                     ref2, ref3, ref4, cnt, ref_offset
%elif use_loop
cglobal sad_skip_%1x%2x4d, 5, 9, 8, src, src_stride, ref1, ref_stride, res, \
                                    ref2, ref3, ref4, cnt
%else
cglobal sad_skip_%1x%2x4d, 5, 8, 8, src, src_stride, ref1, ref_stride, res, \
                                    ref2, ref3, ref4
%endif
%else
%if use_ref_offset
cglobal sad_skip_%1x%2x4d, 4, 7, 8, src, ref_offset, ref1, cnt, ref2, ref3, \
                                    ref4
%define spill_src_stride 1
%define spill_ref_stride 1
%elif use_loop
cglobal sad_skip_%1x%2x4d, 4, 7, 8, src, cnt, ref1, ref_stride, ref2, \
                                    ref3, ref4
%define spill_src_stride 1
%else
cglobal sad_skip_%1x%2x4d, 4, 7, 8, src, src_stride, ref1, ref_stride, ref2, \
                                    ref3, ref4
%endif
%endif
%elif %3 == 0  ; normal sad
%if ARCH_X86_64
%if use_ref_offset
cglobal sad%1x%2x4d, 5, 10, 8, src, src_stride, ref1, ref_stride, res, ref2, \
                               ref3, ref4, cnt, ref_offset
%elif use_loop
cglobal sad%1x%2x4d, 5, 9, 8, src, src_stride, ref1, ref_stride, res, ref2, \
                              ref3, ref4, cnt
%else
cglobal sad%1x%2x4d, 5, 8, 8, src, src_stride, ref1, ref_stride, res, ref2, \
                              ref3, ref4
%endif
%else
%if use_ref_offset
cglobal sad%1x%2x4d, 4, 7, 8, src, ref_offset, ref1, cnt, ref2, ref3, ref4
  %define spill_src_stride 1
  %define spill_ref_stride 1
%elif use_loop
cglobal sad%1x%2x4d, 4, 7, 8, src, cnt, ref1, ref_stride, ref2, ref3, ref4
  %define spill_src_stride 1
%else
cglobal sad%1x%2x4d, 4, 7, 8, src, src_stride, ref1, ref_stride, ref2, ref3, \
                              ref4
%endif
%endif
%else ; avg
%if ARCH_X86_64
%if use_ref_offset
cglobal sad%1x%2x4d_avg, 6, 11, 8, src, src_stride, ref1, ref_stride, \
                                   second_pred, res, ref2, ref3, ref4, cnt, \
                                   ref_offset
%elif use_loop
cglobal sad%1x%2x4d_avg, 6, 10, 8, src, src_stride, ref1, ref_stride, \
                                   second_pred, res, ref2, ref3, ref4, cnt
%else
cglobal sad%1x%2x4d_avg, 6, 9, 8, src, src_stride, ref1, ref_stride, \
                                   second_pred, res, ref2, ref3, ref4
%endif
%else
%if use_ref_offset
cglobal sad%1x%2x4d_avg, 5, 7, 8, src, ref4, ref1, ref_offset, second_pred, ref2, ref3
  %define spill_src_stride 1
  %define spill_ref_stride 1
  %define spill_cnt 1
%elif use_loop
cglobal sad%1x%2x4d_avg, 5, 7, 8, src, ref4, ref1, ref_stride, second_pred, ref2, ref3
  %define spill_src_stride 1
  %define spill_cnt 1
%else
cglobal sad%1x%2x4d_avg, 5, 7, 8, src, ref4, ref1, ref_stride, second_pred, ref2, ref3
  %define spill_src_stride 1
%endif
%endif
%endif

%if spill_src_stride
  %define src_strideq r1mp
  %define src_strided r1mp
%endif
%if spill_ref_stride
  %define ref_strideq r3mp
  %define ref_strided r3mp
%endif

%if spill_cnt
  SUB                  rsp, 4
  %define cntd word [rsp]
%endif

%if %4 == 1
  sal          src_strided, 1
  sal          ref_strided, 1
%endif
  movsxdifnidn src_strideq, src_strided
  movsxdifnidn ref_strideq, ref_strided

  mov                ref2q, [ref1q+gprsize*1]
  mov                ref3q, [ref1q+gprsize*2]
  mov                ref4q, [ref1q+gprsize*3]
  mov                ref1q, [ref1q+gprsize*0]

; Is the loop for this wxh in another function?
; If so, we jump into that function for the loop and returning
%define external_loop (use_ref_offset && %1 > mmsize && %1 != %2)

%if use_ref_offset
  PROCESS_FIRST_MMSIZE %3
%if %1 > mmsize
  mov          ref_offsetq, 0
  mov                 cntd, %2 >> %4
; Jump part way into the loop for the square version of this width
%if %3 == 1
  jmp mangle(private_prefix %+ _sad%1x%1x4d_avg %+ SUFFIX).midloop
%elif %4 == 1
  jmp mangle(private_prefix %+ _sad_skip_%1x%1x4d %+ SUFFIX).midloop
%else
  jmp mangle(private_prefix %+ _sad%1x%1x4d %+ SUFFIX).midloop
%endif
%else
  mov          ref_offsetq, ref_strideq
  add                 srcq, src_strideq
  mov                 cntd, (%2 >> %4) - 1
%endif
%if external_loop == 0
.loop:
; Unrolled horizontal loop
%assign h_offset 0
%rep %1/mmsize
  PROCESS_16x1x4 h_offset, %3
%if h_offset == 0
; The first row of the first column is done outside the loop and jumps here
.midloop:
%endif
%assign h_offset h_offset+mmsize
%endrep

  add                 srcq, src_strideq
  add          ref_offsetq, ref_strideq
  sub                 cntd, 1
  jnz .loop
%endif
%else
  PROCESS_%1x2x4 1, %3
  ADVANCE_END_OF_TWO_LINES
%if use_loop
  mov                 cntd, (%2/2 >> %4) - 1
.loop:
%endif
  PROCESS_%1x2x4 0, %3
%if use_loop
  ADVANCE_END_OF_TWO_LINES
  sub                 cntd, 1
  jnz .loop
%endif
%endif

%if spill_cnt
; Undo stack allocation for cnt
  ADD                  rsp, 4
%endif

%if external_loop == 0
%if %3 == 0
  %define resultq r4
  %define resultmp r4mp
%else
  %define resultq r5
  %define resultmp r5mp
%endif

; Undo modifications on parameters on the stack
%if %4 == 1
%if spill_src_stride
  shr          src_strided, 1
%endif
%if spill_ref_stride
  shr          ref_strided, 1
%endif
%endif

%if %1 > 4
  pslldq                m5, 4
  pslldq                m7, 4
  por                   m4, m5
  por                   m6, m7
  mova                  m5, m4
  mova                  m7, m6
  punpcklqdq            m4, m6
  punpckhqdq            m5, m7
  paddd                 m4, m5
%if %4 == 1
  pslld                 m4, 1
%endif
  movifnidn             resultq, resultmp
  movu                [resultq], m4
  RET
%else
  pshufd            m6, m6, 0x08
  pshufd            m7, m7, 0x08
%if %4 == 1
  pslld                 m6, 1
  pslld                 m7, 1
%endif
  movifnidn             resultq, resultmp
  movq              [resultq+0], m6
  movq              [resultq+8], m7
  RET
%endif
%endif ; external_loop == 0
%endmacro

INIT_XMM sse2
SADNXN4D 128, 128
SADNXN4D 128,  64
SADNXN4D  64, 128
SADNXN4D  64,  64
SADNXN4D  64,  32
SADNXN4D  32,  64
SADNXN4D  32,  32
SADNXN4D  32,  16
SADNXN4D  16,  32
SADNXN4D  16,  16
SADNXN4D  16,   8
SADNXN4D   8,  16
SADNXN4D   8,   8
SADNXN4D   8,   4
SADNXN4D   4,   8
SADNXN4D   4,   4
%if CONFIG_REALTIME_ONLY==0
SADNXN4D   4,  16
SADNXN4D  16,   4
SADNXN4D   8,  32
SADNXN4D  32,   8
SADNXN4D  16,  64
SADNXN4D  64,  16
%endif
%if CONFIG_REALTIME_ONLY==0
SADNXN4D 128, 128, 1
SADNXN4D 128,  64, 1
SADNXN4D  64, 128, 1
SADNXN4D  64,  64, 1
SADNXN4D  64,  32, 1
SADNXN4D  32,  64, 1
SADNXN4D  32,  32, 1
SADNXN4D  32,  16, 1
SADNXN4D  16,  32, 1
SADNXN4D  16,  16, 1
SADNXN4D  16,   8, 1
SADNXN4D   8,  16, 1
SADNXN4D   8,   8, 1
SADNXN4D   8,   4, 1
SADNXN4D   4,   8, 1
SADNXN4D   4,   4, 1
SADNXN4D   4,  16, 1
SADNXN4D  16,   4, 1
SADNXN4D   8,  32, 1
SADNXN4D  32,   8, 1
SADNXN4D  16,  64, 1
SADNXN4D  64,  16, 1
%endif
SADNXN4D 128, 128, 0, 1
SADNXN4D 128,  64, 0, 1
SADNXN4D  64, 128, 0, 1
SADNXN4D  64,  64, 0, 1
SADNXN4D  64,  32, 0, 1
SADNXN4D  32,  64, 0, 1
SADNXN4D  32,  32, 0, 1
SADNXN4D  32,  16, 0, 1
SADNXN4D  16,  32, 0, 1
SADNXN4D  16,  16, 0, 1
SADNXN4D  16,   8, 0, 1
SADNXN4D   8,  16, 0, 1
SADNXN4D   8,   8, 0, 1
SADNXN4D   4,   8, 0, 1
%if CONFIG_REALTIME_ONLY==0
SADNXN4D   4,  16, 0, 1
SADNXN4D   8,  32, 0, 1
SADNXN4D  32,   8, 0, 1
SADNXN4D  16,  64, 0, 1
SADNXN4D  64,  16, 0, 1
%endif

; Different assembly is needed when the height gets subsampled to 2
; SADNXN4D 16,  4, 0, 1
; SADNXN4D  8,  4, 0, 1
; SADNXN4D  4,  4, 0, 1
