// Fast math optimization in Assembly (NASM-style syntax)
// UACE will auto-detect and compile this

section .text
global calculate_fast

calculate_fast:
    ; Fast integer multiplication using shift and add
    ; EAX = EBX * ECX algorithm using shifts
    
    xor eax, eax        ; Clear result
    xor ecx, ecx        ; Counter
    
multiply_loop:
    test ebx, ebx      ; Check if multiplier is zero
    jz multiply_done   ; If yes, done
    
    test ecx, 1        ; Check LSB of counter
    jz skip_add        ; If even, skip add
    
    add eax, ebx       ; Add multiplicand to result
    
skip_add:
    shr ebx, 1         ; Shift multiplier right (divide by 2)
    shl ecx, 1         ; Counter left (actually we test original)
    inc ecx            ; Increment counter
    jmp multiply_loop  ; Continue multiplication

multiply_done:
    ret                  ; Return with result in EAX

section .data
    ; Constants can go here

section .bss
    ; Uninitialized data goes here