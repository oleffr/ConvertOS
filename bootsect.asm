.code16
.att_syntax
.global _start
_start:
    mov %cs, %ax 
    mov %ax, %ds
    mov %ax, %ss
    movw $_start, %bx 
    mov $0x0e, %ah ; В ah номер функции BIOS: 0x0e - вывод символа на активную видео страницу
    call clear

read:
    mov $0x1000, %ax
    mov %ax, %es
    mov $0x00, %bx  ; es:bx(0х1000:0х0000) - адрес буфера, в который считываются данные из ядра
    mov $0x02, %ah ; Функция 0x02 прерывания 0х13 (считывание заданного количества секторов с диска в память)
    mov $1, %dl ; Номер диска (носителя)
    mov $0x00, %dh ; Номер головки
    mov $0x01, %cl ; Номер сектора
    mov $0x00, %ch ; Номер цилиндра
    mov $40, %al
    int $0x13 ; Прерывание на дисковый ввод/вывод
    mov $0x00, %ebx
    mov $0x00, %ecx 
    mov $1, %cl 
    ;cl - текущий выбранный цвет
	;ch - текущая строка цвета для печати

menu:
    call clear
    mov $0xb8000, %edi ;Начало области памяти, выделенной под видеоадаптер
    mov $0x00, %edx
    mov $color_message, %esi 
    call colour_define
    call enter 
    mov $1, %ch
    mov $gray_message, %esi
    call colour_define
    call enter
    mov $white_message, %esi
    call colour_define
    call enter
    mov $yellow_message, %esi
    call colour_define
    call enter
    mov $blue_message, %esi
    call colour_define
    call enter
    mov $red_message, %esi
    call colour_define
    call enter
    mov $green_message, %esi
    call colour_define

keyboard:
    mov $0x00, %ah  ;ожидание нажатия и считывание нажатой пользователем клавиши
    int $0x16 ;обрабатываем нажатия клавиатуры
    cmp $0x48, %ah ;если это клавиша вверх, то выполняется следующая функция
    je up 
    cmp $0x50, %ah ;если это клавиша вниз, то выполняется следующая функция
    je down
    cmp $0x1C, %ah ;если это enter, то происходит загрузка ядра
    je load_kernel
    jmp keyboard ;если была нажата другая клавиша, то снова считываем нажатие клавиши

clear:
    mov $3, %ax 
    int $0x10
    ret
down:
    cmp $6, %cl ;Если текущий цвет последний(т.е. зеленый = 6), то следующий цвет будет начальным(т.е. серый = 1)
    je of_below
    inc %cl ;Иначе инкрементируем текущий цвет
    jmp menu
up:
    cmp $1, %cl ;Если текущий цвет начальный(т.е.серый = 1), то следующий цвет будет последним(т.е. зеленый = 6)
    je of_above
    dec %cl ;Иначе декрементируем текущий цвет
    jmp menu
of_below:
    mov $1, %cl 
    jmp menu
of_above:
    mov $6, %cl
    jmp menu

enter:
    inc %edx
    mov %edx, %ebx
    imul $160, %ebx
    mov $0xb8000, %edi
    add  %ebx, %edi
    ret

color_message:
    .asciz "Choose the colour of ConvertOs"

yellow_message:
    .asciz "Yellow colour"
show_yellow:
    mov $0x0e, %ah
    jmp puts

blue_message:
    .asciz "Blue colour"
show_blue:
    mov $0x01, %ah
    jmp puts

gray_message:
    .asciz "Gray colour"
show_gray:
    mov $0x07, %ah
    jmp puts

white_message:
    .asciz "White colour"
show_white:
    mov $0x0f, %ah
    jmp puts

green_message:
    .asciz "Green colour"
show_green:
    mov $0x02, %ah
    jmp puts

red_message:
    .asciz "Red colour"
show_red:
    mov $0x04, %ah
    jmp puts


colour_define:
    mov $0x08, %ah
    cmp %cl, %ch
    je puts
    cmp $3, %cl
    je show_yellow
    cmp $4, %cl
    je show_blue
    cmp $1, %cl
    je show_gray
    cmp $2, %cl
    je show_white
    cmp $6, %cl
    je show_green
    cmp $5, %cl
    je show_red
    jmp puts

puts:
    movb 0(%esi), %al
    test %al, %al
    jz return
    movb %al, 0(%edi)
    movb %ah, 1(%edi) 
    add $2, %edi
    add $1, %esi
    jmp puts

return:
    inc %ch
    ret

load_kernel:
    call clear
    mov %cl, 0x600
    cli
    lgdt gdt_info
    inb $0x92, %al
    orb $2, %al
    outb %al, $0x92
    movl %cr0, %eax
    orb $1, %al
    movl %eax, %cr0
    ljmp $0x8, $protected_mode

gdt:
    .byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    .byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00
    .byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00

gdt_info:
    .word gdt_info - gdt
    .word gdt, 0 

.code32
protected_mode:
    movw $0x10, %ax
    movw %ax, %es
    movw %ax, %ds
    movw %ax, %ss
    call 0x10000
    .zero (512 -(. - _start) - 2)
    .byte 0x55, 0xAA
