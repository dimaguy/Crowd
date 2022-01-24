%define has_errcode(v) (v == 8 || (v >= 10 && v <= 14) || v == 17 || v == 21)
%assign vector 0
%rep 256
isr%+vector:
  %if !has_errcode(vector)
  push   0
  %endif
  push  vector
  jmp   generic_isr
%assign vector vector + 1
%endrep

global isrs
isrs:
%assign vector 0
%rep 256
  dq   isr%+vector
%assign vector vector + 1
%endrep

extern interrupt_handler

generic_isr:
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    cld
    call interrupt_handler
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
