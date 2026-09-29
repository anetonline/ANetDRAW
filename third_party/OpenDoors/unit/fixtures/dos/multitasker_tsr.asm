; Deterministic DOS multitasker-detection fixture for Turbo C unit tests.

.8086
.model tiny
.code
org 100h

start:
        mov     ax, 3521h
        int     21h
        mov     word ptr cs:old_int21, bx
        mov     word ptr cs:old_int21+2, es

        mov     ax, 352fh
        int     21h
        mov     word ptr cs:old_int2f, bx
        mov     word ptr cs:old_int2f+2, es

        mov     ax, 3560h
        int     21h
        mov     word ptr cs:old_int60, bx
        mov     word ptr cs:old_int60+2, es

        push    cs
        pop     ds
        mov     dx, offset int21_handler
        mov     ax, 2521h
        int     21h
        mov     dx, offset int2f_handler
        mov     ax, 252fh
        int     21h
        mov     dx, offset int60_handler
        mov     ax, 2560h
        int     21h

        mov     dx, offset resident_end
        add     dx, 15
        mov     cl, 4
        shr     dx, cl
        mov     ax, 3100h
        int     21h

int21_handler proc far
        cmp     ah, 30h
        je      dos_version
        cmp     ax, 2b01h
        jne     chain_int21
        cmp     cx, 4445h
        jne     chain_int21
        cmp     dx, 5351h
        jne     chain_int21
        mov     al, 0ffh
        cmp     byte ptr cs:fixture_mode, 2
        jne     int21_done
        xor     al, al
int21_done:
        iret

dos_version:
        mov     al, 9
        cmp     byte ptr cs:fixture_mode, 1
        jne     int21_done
        mov     al, 0ah
        iret

chain_int21:
        jmp     dword ptr cs:old_int21
int21_handler endp

int2f_handler proc far
        cmp     ax, 1600h
        jne     chain_int2f
        xor     al, al
        cmp     byte ptr cs:fixture_mode, 3
        jne     int2f_done
        mov     al, 1
int2f_done:
        iret

chain_int2f:
        jmp     dword ptr cs:old_int2f
int2f_handler endp

int60_handler proc far
        mov     byte ptr cs:fixture_mode, al
        iret
int60_handler endp

fixture_mode db 0
old_int21 dd 0
old_int2f dd 0
old_int60 dd 0

resident_end label byte

end start
