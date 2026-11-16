AREA mycode, CODE, READONLY
ENTRY Reset

Reset
    LDR R0, =message
    LDR R0, [R0]

    ; Exit
    MOV R7, #1
    SWI 0

message DCB "Hello, World", 13, 10
ALIGN
AREA mydata, DATA, READWRITE
message2 DCB "PandaIDE Assembly", 0
END