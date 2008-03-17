
; ---------------------------------------------------------------------------
; Copyright (c) 2002, Dr Brian Gladman, Worcester, UK.   All rights reserved.
;
; LICENSE TERMS
;
; The free distribution and use of this software in both source and binary
; form is allowed (with or without changes) provided that:
;
;   1. distributions of this source code include the above copyright
;      notice, this list of conditions and the following disclaimer;
;
;   2. distributions in binary form include the above copyright
;      notice, this list of conditions and the following disclaimer
;      in the documentation and/or other associated materials;
;
;   3. the copyright holder's name is not used to endorse products
;      built using this software without specific written permission.
;
; ALTERNATIVELY, provided that this notice is retained in full, this product
; may be distributed under the terms of the GNU General Public License (GPL),
; in which case the provisions of the GPL apply INSTEAD OF those given above.
;
; DISCLAIMER
;
; This software is provided 'as is' with no explicit or implied warranties
; in respect of its properties, including, but not limited to, correctness
; and/or fitness for purpose.
; ---------------------------------------------------------------------------
; Issue 28/01/2004

; An AES (Rijndael) implementation for the Pentium MMX family using the NASM
; assembler <http://sourceforge.net/projects/nasm>.  This version implements
; the standard AES block length (128 bits, 16 bytes) with the same interface
; as that used in my C/C++ implementation.   This code does not preserve the
; eax, ecx or edx registers or the artihmetic status flags. However, the ebx,
; esi, edi, and ebp registers are preserved across calls.    Only encryption
; and decryption are implemented here, the key schedule code being that from
; compiling aes.c with USE_ASM defined.  This code uses VC++ register saving
; conentions; if it is used with another compiler, its conventions for using
; and saving registers will need to be checked (and calling conventions). If
; the parent application uses floating point instructions, then FPU_USED has
; to be defined below.  Define AES_DLL for the Dynamic Link Library version.
; The NASM command line for the VC++ custom build step is:
;
;    nasm -O2 -f win32 -o "$(TargetDir)\$(InputName).obj" "$(InputPath)"

    section .text use32

; aes_rval aes_encrypt(const unsigned char in_blk[],
;                           unsigned char out_blk[], const aes_ctx cx[1]);
; aes_rval aes_decrypt(const unsigned char in_blk[],
;                           unsigned char out_blk[], const aes_ctx cx[1]);
;
; comment in/out the following lines to obtain the desired subroutines. These
; selections MUST match those in the C header file aes.h

%define AES_128     ; define if AES with 128 bit keys is needed
%define AES_192     ; define if AES with 192 bit keys is needed
%define AES_256     ; define if AES with 256 bit keys is needed
%define AES_VAR     ; define if a variable key size is needed
%define ENCRYPTION  ; define if encryption is needed
%define DECRYPTION  ; define if decryption is needed

%ifdef AES_VAR
%define KS_LENGTH       60
%elifdef AES_256
%define KS_LENGTH       60
%elifdef AES_192
%define KS_LENGTH       52
%else
%define KS_LENGTH       44
%endif

; define this if floating point instructions are being used

%define    FPU_USED

; The DLL interface must use the _stdcall convention in which the number
; of bytes of parameter space is added after an @ to the routine's name.
; We must also remove our parameters from the stack before return (see
; the do_ret macro). Define AES_DLL for the Dynamic Link Library version.

;%define AES_DLL

tlen:   equ  1024   ; length of each of 4 'xor' arrays (256 32-bit words)

; offsets to parameters with one register pushed onto stack

in_blk: equ     4   ; input byte array address parameter
out_blk:equ     8   ; output byte array address parameter
ctx:    equ    12   ; AES context structure
stk_spc:equ    16   ; stack space

; offsets in context structure

ksch:   equ     0   ; encryption key schedule base address
nrnd:   equ   240   ; number of rounds

; This macro performs a forward encryption cycle.  It is entered with
; the previous round column values in eax, ebx, ecx and edi and exits
; with the final values in the same registers

%macro fwd_rnd 1-2 _t_fn

    movzx   esi,al
    movzx   edi,ah
    shr     eax,16
    movd    mm4,[4*esi+%2]
    movd    mm7,[4*edi+%2+tlen]
    movzx   esi,al
    movzx   edi,ah
    movd    mm6,[4*esi+%2+2*tlen]
    movd    mm5,[4*edi+%2+3*tlen]

    movzx   esi,bl
    movzx   edi,bh
    shr     ebx,16
    movd    mm0,[4*esi+%2]
    movd    mm1,[4*edi+%2+tlen]
    movzx   esi,bl
    movzx   edi,bh
    movd    mm2,[4*esi+%2+2*tlen]
    movd    mm3,[4*edi+%2+3*tlen]
    pxor    mm5,mm0
    pxor    mm4,mm1
    pxor    mm7,mm2
    pxor    mm6,mm3

    movzx   esi,cl
    movzx   edi,ch
    shr     ecx,16
    movd    mm0,[4*esi+%2]
    movd    mm1,[4*edi+%2+tlen]
    movzx   esi,cl
    movzx   edi,ch
    movd    mm2,[4*esi+%2+2*tlen]
    movd    mm3,[4*edi+%2+3*tlen]
    pxor    mm6,mm0
    pxor    mm5,mm1
    pxor    mm4,mm2
    pxor    mm7,mm3

    movzx   esi,dl
    movzx   edi,dh
    shr     edx,16
    movd    mm0,[4*esi+%2]
    movd    mm1,[4*edi+%2+tlen]
    movzx   esi,dl
    movzx   edi,dh
    movd    mm2,[4*esi+%2+2*tlen]
    movd    mm3,[4*edi+%2+3*tlen]
    pxor    mm7,mm0
    pxor    mm6,mm1
    pxor    mm5,mm2
    pxor    mm4,mm3

    movd    eax,mm4
    movd    ebx,mm5
    movd    ecx,mm6
    movd    edx,mm7
    xor     eax,[%1]
    xor     ebx,[%1+ 4]
    xor     ecx,[%1+ 8]
    xor     edx,[%1+12]

%endmacro

; This macro performs an inverse encryption cycle. It is entered with
; the previous round column values in eax, ebx, ecx and edx and exits
; with the final values in the same registers

%macro inv_rnd 1-2 _t_in

    movzx   esi,al
    movzx   edi,ah
    shr     eax,16
    movd    mm4,[4*esi+%2]
    movd    mm5,[4*edi+%2+tlen]
    movzx   esi,al
    movzx   edi,ah
    movd    mm6,[4*esi+%2+2*tlen]
    movd    mm7,[4*edi+%2+3*tlen]

    movzx   esi,bl
    movzx   edi,bh
    shr     ebx,16
    movd    mm0,[4*esi+%2]
    movd    mm1,[4*edi+%2+tlen]
    movzx   esi,bl
    movzx   edi,bh
    movd    mm2,[4*esi+%2+2*tlen]
    movd    mm3,[4*edi+%2+3*tlen]
    pxor    mm5,mm0
    pxor    mm6,mm1
    pxor    mm7,mm2
    pxor    mm4,mm3

    movzx   esi,cl
    movzx   edi,ch
    shr     ecx,16
    movd    mm0,[4*esi+%2]
    movd    mm1,[4*edi+%2+tlen]
    movzx   esi,cl
    movzx   edi,ch
    movd    mm2,[4*esi+%2+2*tlen]
    movd    mm3,[4*edi+%2+3*tlen]
    pxor    mm6,mm0
    pxor    mm7,mm1
    pxor    mm4,mm2
    pxor    mm5,mm3

    movzx   esi,dl
    movzx   edi,dh
    shr     edx,16
    movd    mm0,[4*esi+%2]
    movd    mm1,[4*edi+%2+tlen]
    movzx   esi,dl
    movzx   edi,dh
    movd    mm2,[4*esi+%2+2*tlen]
    movd    mm3,[4*edi+%2+3*tlen]
    pxor    mm7,mm0
    pxor    mm4,mm1
    pxor    mm5,mm2
    pxor    mm6,mm3

    movd    eax,mm4
    movd    ebx,mm5
    movd    ecx,mm6
    movd    edx,mm7
    xor     eax,[%1]
    xor     ebx,[%1+ 4]
    xor     ecx,[%1+ 8]
    xor     edx,[%1+12]

%endmacro

; Standard return code.  The DLL has to implement the _stdcall
; calling interface on return. In this case we have to take our
; parameters (3 4-byte pointers) off the stack. Also we need an
; emms instruction is needed to reset the FPU if flaoting point
; instuctions are needed

%macro  do_ret  0-1 12

%ifdef  FPU_USED
    emms
%endif

%ifdef AES_DLL
    ret %1
%else
    ret
%endif

%endmacro

%macro  do_name 1-2 12
%ifndef AES_DLL
    global  %1
%1:
%else
    global  %1@%2
    export  %1@%2
%1@%2:
%endif
%endmacro

; AES (Rijndael) Encryption Subroutine

%ifdef  ENCRYPTION

    extern  _t_fn
    extern  _t_fl

    do_name _aes_encrypt

    sub     esp,stk_spc
    mov     [esp+12],ebp
    mov     [esp+ 8],ebx
    mov     [esp+ 4],esi
    mov     [esp   ],edi
    mov     esi,[esp+in_blk+stk_spc]    ; input pointer
    mov     ebp,[esp+ctx+stk_spc]       ; context pointer

; input four columns and xor in first round key

    mov     eax,[esi   ]
    mov     ebx,[esi+ 4]
    xor     eax,[ebp   ]
    xor     ebx,[ebp+ 4]
    mov     ecx,[esi+ 8]
    mov     edx,[esi+12]
    xor     ecx,[ebp+ 8]
    xor     edx,[ebp+12]

; determine the number of rounds

    mov     edi,[ebp+4*KS_LENGTH]
    cmp     edi,10
    je      .3
    lea     ebp,[ebp+32]
    cmp     edi,12
    je      .2
    lea     ebp,[ebp+32]
    cmp     edi,14
    je      .1
    mov     eax,-1
    jmp     .4

.1: fwd_rnd ebp- 48         ; 14 rounds for 256-bit key
    fwd_rnd ebp- 32
.2: fwd_rnd ebp- 16         ; 12 rounds for 192-bit key
    fwd_rnd ebp
.3: fwd_rnd ebp+ 16         ; 10 rounds for 128-bit key
    fwd_rnd ebp+ 32
    fwd_rnd ebp+ 48
    fwd_rnd ebp+ 64
    fwd_rnd ebp+ 80
    fwd_rnd ebp+ 96
    fwd_rnd ebp+112
    fwd_rnd ebp+128
    fwd_rnd ebp+144
    fwd_rnd ebp+160,_t_fl   ; last round uses a different table

; move final values to the output array

    mov     ebp,[esp+out_blk+stk_spc]
    mov     [ebp+12],edx
    mov     [ebp+ 8],ecx
    mov     [ebp+ 4],ebx
    mov     [ebp   ],eax
    xor     eax,eax
.4: mov     ebp,[esp+12]
    mov     ebx,[esp+ 8]
    mov     esi,[esp+ 4]
    mov     edi,[esp   ]
    lea     esp,[esp+stk_spc]
    do_ret

%endif

; AES (Rijndael) Decryption Subroutine

%ifdef  DECRYPTION

    extern  _t_in
    extern  _t_il

    do_name _aes_decrypt

    sub     esp,stk_spc
    mov     [esp+12],ebp
    mov     [esp+ 8],ebx
    mov     [esp+ 4],esi
    mov     [esp],edi
    mov     esi,[esp+in_blk+stk_spc]    ; input pointer
    mov     ebp,[esp+ctx+stk_spc]       ; context pointer

; input four columns

    mov     eax,[esi]
    mov     ebx,[esi+4]
    mov     ecx,[esi+8]
    mov     edx,[esi+12]

; determine the number of rounds

    mov     edi,[ebp+4*KS_LENGTH]

; xor in initial keys

    lea     esi,[4*edi]
    lea     esi,[ebp+4*esi]
    xor     eax,[esi]
    xor     ebx,[esi+4]
    xor     ecx,[esi+8]
    xor     edx,[esi+12]
    cmp     edi,10
    je      .3
    cmp     edi,12
    je      .2
    cmp     edi,14
    je      .1
    mov     eax,-1
    jmp     .4

.1: inv_rnd ebp+208         ; 14 rounds for 256-bit key
    inv_rnd ebp+192
.2: inv_rnd ebp+176         ; 12 rounds for 192-bit key
    inv_rnd ebp+160
.3: inv_rnd ebp+144         ; 10 rounds for 128-bit key
    inv_rnd ebp+128
    inv_rnd ebp+112
    inv_rnd ebp+ 96
    inv_rnd ebp+ 80
    inv_rnd ebp+ 64
    inv_rnd ebp+ 48
    inv_rnd ebp+ 32
    inv_rnd ebp+ 16
    inv_rnd ebp,_t_il       ; last round uses a different table

; move final values to the output array

    mov     ebp,[esp+out_blk+stk_spc]
    mov     [ebp+12],edx
    mov     [ebp+8],ecx
    mov     [ebp+4],ebx
    mov     [ebp],eax
    xor     eax,eax
.4: mov     ebp,[esp+12]
    mov     ebx,[esp+ 8]
    mov     esi,[esp+ 4]
    mov     edi,[esp]
    lea     esp,[esp+stk_spc]
    do_ret

%endif

    end
