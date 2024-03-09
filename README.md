# _ConvertOS_
>ОС для выполнения простых операций по конвертации величин
>Транслятор ассемблера для загрузчика: GNU, syntax: AT&T
>Компилятор ядра: gcc

## Алгоритм запуска
- Склонируйте репозиторий:

'''

git clone git@github.com:oleffr/ConvertOS.git

'''

- Введите последовательно команды:

'''

as --32 -o bootsect.o bootsect.asm
ld -Ttext 0x7c00 --oformat binary -m elf_i386 -o bootsect.bin bootsect.o
g++ -ffreestanding -m32 -o kernel.o -c kernel.cpp
ld --oformat binary -Ttext 0x10000 -o kernel.bin --entry=kmain -m elf_i386 kernel.o
qemu –fda bootsect.bin –fdb kernel.bin

'''

## Описание ОС
Поддерживаемые ОС команды:
- info
> Выводит информацию об авторе и средствах разработки ОС (транслятор ассемблера,
компилятор), параметры загрузчика — выбранный пользователем цвет шрифта консоли
ОС.
- сlear
> Очистить все введенные пользователем команды и выведенные ОС результаты их
выполнения.
- shutdown
> Выключение компьютера
- nsconv число исх_система конечн_система
> Переводит указанное в параметре число из исходной системы счисления в конечную
систему. В случае целочисленного переполнения должна выводиться ошибка. В случае
ошибочного знака (некорректный символ для данной СС) выводится ошибка. Должны
поддерживаются системы счисления от 2 до 36. Буквенные символы принимаются в
нижнем регистре.
Примеры:
# nsconv ff 16 10
255
# nsconv 78 8 10
error: invalid symbol: 8
# nsconv 9999999999999999999 10 16
error: int overflow
- posixtime число
> Распечатывает дату и время заданные указанным числом, являющимся меткой времени
POSIX (время, прошедшее в секундах, начиная с 01.01.1970 00:00:00 (UTC)
# posixtime 1111111111
18.03.2005 01:58:31
# posixtime 1500000000
14.07.2017 02:40:00
- wintime число
> Распечатывает дату и время заданные указанным 64-битным числом, являющимся меткой
времени Windows (время, прошедшее в 100-наносекундных интервалах, начиная с
01.01.1601 00:00:00 UTC)
# wintime 123456789012345678
21.03.1992 19:15:01
