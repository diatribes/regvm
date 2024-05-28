STORE R0, 10
STORE R1, 1
STORE R2, 32
STORE R3, 1
STORE R4, 50

LABEL 0

    SYNC
    CLS

    MOVE  I0, R0
    MOVE  I1, R1    
    ADD

    MOVE R0, O0
    MOVE  I0, R0
    STORE I1, 128
    CMPI
    DECEQ R1
    DECEQ R1

    MOVE  I0, R0
    MOVE  I0, R0
    STORE I1, 1
    CMPI
    INCEQ R1
    INCEQ R1

    MOVE  I0, R0
    MOVE  I0, R2
    MOVE  I1, R3    
    ADD

    MOVE R2, O0
    MOVE  I0, R2
    STORE I1, 127
    CMPI
    DECEQ R3
    DECEQ R3

    MOVE I0, R2
    STORE I1, 1
    CMPI
    INCEQ R3
    INCEQ R3

    MOVE I0, R0
    MOVE I1, R2
    STORE I2, 0xff000000
    PIXEL

    MOVE I0, R4
    STORE I1, 120
    STORE I2, 0xff000000

    PIXEL
    INC I0

    PIXEL
    INC I0

    PIXEL
    INC I0

    PIXEL
    INC I0

    PIXEL
    INC I0

    PIXEL
    INC I0

    PIXEL
    INC I0

    PIXEL
    INC I0

    PIXEL
    INC I0

    PIXEL

    STORE I0, 0
    BUTTON
    MOVE I0, O0
    STORE I1, 1
    CMPI
    DECEQ R4

    STORE I0, 1
    BUTTON
    MOVE I0, O0
    STORE I1, 1
    CMPI
    INCEQ R4

    MOVE I0, R2
    STORE I1, 120
    CMPI
    JMPLT 0

    MOVE I0, R3
    STORE I1, 1
    CMPI
    JMPLT 0

    MOVE I0, R4
    MOVE I1, R0
    CMPI
    DECEQ R3
    DECEQ R3
    JMPEQ 0

    INC I0
    CMPI
    DECEQ R3
    DECEQ R3
    JMPEQ 0

    INC I0
    CMPI
    DECEQ R3
    DECEQ R3
    JMPEQ 0

    INC I0
    CMPI
    DECEQ R3
    DECEQ R3
    JMPEQ 0

    INC I0
    CMPI
    DECEQ R3
    DECEQ R3
    JMPEQ 0

    INC I0
    CMPI
    DECEQ R3
    DECEQ R3
    JMPEQ 0

    INC I0
    CMPI
    DECEQ R3
    DECEQ R3
    JMPEQ 0

    INC I0
    CMPI
    DECEQ R3
    DECEQ R3
    JMPEQ 0

    INC I0
    CMPI
    DECEQ R3
    DECEQ R3
    JMPEQ 0

    INC I0
    CMPI
    DECEQ R3
    DECEQ R3
    JMPEQ 0

    INC I0
    CMPI
    DECEQ R3
    DECEQ R3
    JMPEQ 0

JMP 0
HALT

