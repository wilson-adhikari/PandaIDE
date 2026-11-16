.MODEL small
.STACK 100h
.DATA
message db "Hello, World$, 0
.CODE
start:
mov ax, @data
mov ds, ax
mov ah, 09h
lea dx, [message]
int 21h
mov ax, 4C00h
int 21h
end start