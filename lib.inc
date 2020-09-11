section .data
    newLine: db 0xA

section .bss
    container: db ?
    number: db ?

section .text
 
 
; Принимает код возврата и завершает текущий процесс
exit: 
    xor rax, rax,
    ret 

; Принимает указатель на нуль-терминированную строку, возвращает её длину
string_length:
    xor rax, rax
.counter:
    cmp byte [rdi+rax], 0
    je .end
    inc rax
    jmp .counter
.end:
    ret

; Принимает указатель на нуль-терминированную строку, выводит её в stdout
print_string:
    xor rax, rax
    call string_length
    mov rdx, rax
    mov rax, 1
    mov rsi, rdi
    mov rdi, 1
    syscall
    ret

; Принимает код символа и выводит его в stdout
print_char:
    mov [container], rdi
    mov rdi, container
    call print_string
    ret

; Переводит строку (выводит символ с кодом 0xA)
print_newline:
    xor rax, rax
    mov rdi, 10
    call print_char
    ret

; Выводит беззнаковое 8-байтовое число в десятичном формате 
; Совет: выделите место в стеке и храните там результаты деления
; Не забудьте перевести цифры в их ASCII коды.
print_uint:
    push rbx
    xor rax, rax
    mov rax, rdi
    mov rbx, 10
    xor r8, r8
    .loop:
        xor rdx,rdx
        div rbx
        add rdx, '0'
        push rdx
        inc r8
        cmp rax, 0
        jg .loop
    xor rax, rax
    .strbuild:
        pop rdx
        mov [number + rax], rdx
        inc rax
        cmp rax,r8
        jne .strbuild
    mov rdi, number
    call print_string
    pop rbx
    ret

; Выводит знаковое 8-байтовое число в десятичном формате 
print_int:
    xor rax, rax
    mov rax, rdi
    test rax, rax
    jns .printdig
    mov rdi, '-'
    push rax
    call print_char
    pop rax
    neg rax
    mov rdi, rax
    .printdig:
    call print_uint
    ret

; Принимает два указателя на нуль-терминированные строки, возвращает 1 если они равны, 0 иначе
string_equals:
    xor rax, rax
    xor r8, r8
    xor r9, r9
    .loop:
        mov r8b, byte [rdi]
        mov r9b, byte [rsi]
        cmp r8b, r9b
        jne .no
        inc rdi
        inc rsi
        cmp r8b, 0
        je .end
        jmp .loop
    .no:
        mov rax, 0
        ret
    .end:
        mov rax, 1
        ret


; Читает один символ из stdin и возвращает его. Возвращает 0 если достигнут конец потока
read_char:
    push rdi
    push rdx
    xor rax, rax
    xor rdi, rdi
    mov rdx, 1
    mov rsi, rsp
    syscall
    test rax, rax
    je .return
    xor rax, rax
    mov al, [rsp]
.return:
    pop rdx
    pop rdi
    ret 

; Принимает: адрес начала буфера, размер буфера
; Читает в буфер слово из stdin, пропуская пробельные символы в начале, .
; Пробельные символы это пробел 0x20, табуляция 0x9 и перевод строки 0x10.
; Останавливается и возвращает 0 если слово слишком большое для буфера
; При успехе возвращает адрес буфера. 
; Эта функция должна дописывать к слову нуль-терминатор

read_word:
    push rbx
    mov r8, rsi
    mov r9, rdi
    xor r10, r10
    push rdi
    xor rdi, rdi
    mov rdx, 1
    .next:
        inc r10
        xor rax, rax
        mov rsi, r9
        syscall
        .stop:
        cmp r10, r8
        je .end
        cmp rax, 0
        je .end
        cmp byte [r9], 0x21
        jb .next
    .again:
        dec r8
        cmp r8, 1
        je .overflow
        inc r9
        xor rax, rax
        mov rsi, r9
        syscall
        cmp byte [r9], 0x21
        jb .end
        cmp rax, 0
        je .end
        jmp .again
    .end:
        mov byte [r9], 0
        pop rax
        mov rdx, r9
        sub rdx, rax
        pop rbx
        ret
    .overflow:
        pop rax
        mov rax, 0
        mov rdx, 0
        pop rbx
        ret

 

; Принимает указатель на строку, пытается
; прочитать из её начала беззнаковое число.
; Возвращает в rax: число, rdx : его длину в символах
; rdx = 0 если число прочитать не удалось
parse_uint:
    xor rax, rax
    xor rdx, rdx
    xor rsi, rsi
    xor r9, r9
    mov r8, 10
    .loop:
        mov sil, [rdi+r9]
        cmp sil, '0'
        jl .return
        cmp sil, '9'
        jg .return
        inc r9
        sub sil, '0'
        mul r8
        add rax, rsi
        jmp .loop
    .return:
        mov rdx, r9
        ret




; Принимает указатель на строку, пытается
; прочитать из её начала знаковое число.
; Если есть знак, пробелы между ним и числом не разрешены.
; Возвращает в rax: число, rdx : его длину в символах (включая знак, если он был) 
; rdx = 0 если число прочитать не удалось
parse_int:
    xor rax, rax
    cmp byte [rdi], '-'
    je .minus
    call parse_uint
    ret
    .minus:
        inc rdi
        call parse_uint
        neg rax
        inc rdx
        ret


; Принимает указатель на строку, указатель на буфер и длину буфера
; Копирует строку в буфер
; Возвращает длину строки если она умещается в буфер, иначе 0
string_copy:
    xor rax, rax
    xor r9, r9
    xor rcx, rcx
    call string_length
    push rax
    push rsi
.loop:
    cmp rcx, rdx
    je .error
    mov r8, [rdi+rcx]
    mov [rsi+rcx], r8
    cmp rax, 0
    je .end
    dec rax
    inc rcx
    jmp .loop
.end
    pop rsi
    pop rax
    mov byte [rsi+rax], 0
    ret
.error:
    pop rsi
    pop rax
    mov rax, 0
    ret