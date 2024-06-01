LABEL 0

    STORE R0, 0

    SYNC

    LABEL 1

        STORE R1, 0

        INC R0

        MOVE  I0, R0
        STORE I1, 127
        CMP
        JMPEQ 0

        LABEL 2

            INC R1

            MOVE I0, R0
            MOVE I1, R1
            XOR

            MOVE I0, O0 
            MOVE I1, T0
            MUL
            STORE I0, 0xff000000
            MOVE  I1, O0
            ADD

            MOVE  I0, R0
            MOVE  I1, R1
            MOVE  I2, O0
            PIXEL

            MOVE  I0, R1
            STORE I1, 127
            CMP
            JMPEQ 1

        JMP 2

