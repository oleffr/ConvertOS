__asm("jmp kmain");
#define VIDEO_BUF_PTR (0xb8000)
#define IDT_TYPE_INTR (0x0E)
#define IDT_TYPE_TRAP (0x0F)
#define CURSOR_PORT (0x3D4)
#define VIDEO_WIDTH (80)
#define PIC1_PORT (0x20)
#define GDT_CS (0x8)

//Functions heading
typedef void (*intr_handler)();
void intr_init();
void intr_start();
void intr_enable();
void intr_disable();
void intr_reg_handler(int num, unsigned short segm_sel, unsigned short flags, intr_handler hndlr);

void keyb_process_keys();
void keyb_init();
void keyb_handler();

static inline unsigned char inb (unsigned short port);
static inline void outb (unsigned short port, unsigned char data);
void cursor_moveto(unsigned int strnum, unsigned int pos);
void screen_scroll ();
void keyboard_scan(char key_num);
void print_string(char* pointer);

void define_date(int format_count, int year, int sec_extra);
int strlen(char *str);
bool strcmp(char *str1, char *str2);
void memset(char* str);
void message_error(int flag_error);
void nsconv();
void start();
int atoi(char* num);


// Constants
char symbol_array[] = "0123456789abcdefghijklmnopqrstuvwxyz";
char input_str[41]={0};
int current_str = 0, current_cursor = 0, counter_str = 0, flag_nsconv = 0, flag_posixtime = 0, flag_wintime = 0;
char sys_colour = 0;
int date_array[6] = {0, 0, 0 , 0, 0, 0};


struct idt_entry{
    unsigned short base_lo; 
    unsigned short segm_sel; 
    unsigned char always0; 
    unsigned char flags;
    unsigned short base_hi;
} __attribute__((packed));


struct idt_ptr{
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

struct idt_entry g_idt[256];
struct idt_ptr g_idtp;


static inline unsigned char inb (unsigned short port){ 
 unsigned char data;
 asm volatile ("inb %w1, %b0" : "=a" (data) : "Nd" (port));
 return data;
}
static inline void outb (unsigned short port, unsigned char data){
 asm volatile ("outb %b0, %w1" : : "a" (data), "Nd" (port));
}
static inline void outw (unsigned int port, unsigned int data){
	asm volatile ("outw %w0, %w1" : : "a" (data), "Nd" (port));
}

void cursor_moveto(unsigned int strnum, unsigned int pos){
    unsigned short new_pos = (strnum * VIDEO_WIDTH) + pos;
    outb(CURSOR_PORT, 0x0F);
    outb(CURSOR_PORT + 1, (unsigned char)(new_pos & 0xFF));
    outb(CURSOR_PORT, 0x0E);
    outb(CURSOR_PORT + 1, (unsigned char)( (new_pos >> 8) & 0xFF));
}

//Interrupts
void intr_reg_handler(int num, unsigned short segm_sel, unsigned short flags, intr_handler hndlr){
    unsigned int hndlr_addr = (unsigned int) hndlr;
    g_idt[num].base_lo = (unsigned short) (hndlr_addr & 0xFFFF);
    g_idt[num].segm_sel = segm_sel;
    g_idt[num].always0 = 0;
    g_idt[num].flags = flags;
    g_idt[num].base_hi = (unsigned short) (hndlr_addr >> 16);
}
void default_intr_handler(){
asm("pusha");
asm("popa; leave; iret");
}

void intr_start(){
    int idt_count = sizeof(g_idt) / sizeof(g_idt[0]);
    g_idtp.base = (unsigned int) (&g_idt[0]);
    g_idtp.limit = (sizeof (struct idt_entry) * idt_count) - 1;
    asm("lidt %0" : : "m" (g_idtp) );
}

void intr_enable(){
    asm("sti");
}

void intr_disable(){
    asm("cli");
}

void keyb_process_keys(){ 
    if (inb(0x64) & 0x01) {
    unsigned char key_num;
    unsigned char state;
    key_num = inb(0x60);
    if (key_num < 128) keyboard_scan(key_num);
    }
}

void keyb_init(){
    intr_reg_handler(0x09, GDT_CS, 0x80 | IDT_TYPE_INTR, keyb_handler); 
    outb(PIC1_PORT + 1, 0xFF ^ 0x02);
}
void intr_init(){
    int i;
    int idt_count = sizeof(g_idt) / sizeof(g_idt[0]);
    for(i = 0; i < idt_count; i++)intr_reg_handler(i, GDT_CS, 0x80 | IDT_TYPE_INTR,default_intr_handler);
}

void keyb_handler(){
    asm("pusha");
    keyb_process_keys();
    outb(PIC1_PORT, 0x20);
    asm("popa; leave; iret");
}

//Output
void print_string(char* pointer){
    char* ptr_video = (char*)VIDEO_BUF_PTR;
    ptr_video += 2*current_str*VIDEO_WIDTH;
    while (*pointer){
        ptr_video[0] = (char) *pointer;
        ptr_video[1] = sys_colour;
        ptr_video += 2;
        pointer++;
    }
    screen_scroll ();
}

void screen_scroll(){
    if (current_str > 23){
        char *ptr_video = (char*)VIDEO_BUF_PTR;
        int i = 0;
        while (i < VIDEO_WIDTH*25){*(ptr_video + i*2) = '\0'; i++;}
        current_str=0; current_cursor=0;
        cursor_moveto(current_str, current_cursor);
        start();
    }
    else{current_str++;}
}

//Library functions
int atoi(char* str) {
    int i = 0, num = 0;
    for (i = 0; str[i] >= '0' && str[i] <= '9'; i++){num = 10 * num + (str[i] - '0');}
    if ((str[i] != ' ') && (str[i] != '\0' )){return -1;}
    return num;
}

void memset(char* str){for(int i=0;i<40;i++){str[i]='\0';}}

int strlen(char* str){
    int length = 0;
    for(int i=0;i<40&&str[i]!='\0'; i++){length++;}
    return length;
}

bool strcmp(char* str1, char *str2){
    if(strlen(str1) != strlen(str2)){return false;}
    for(int i = 0; i < strlen(str1); i++){if(str1[i] != str2[i]){return false;}}
    return true; 
}


//Date
void define_month(int days, int years) {
    int month = 1;
    int dif = days - 31;
    if (dif <= 0) {
        date_array[0] = days;
        date_array[1] = month;
        return; //January
    };
    month++;
    days -= 31;

    if (years % 4 == 0){
        dif = days - 29;
        if (dif <= 0) {
            date_array[0] = days;
            date_array[1] = month;
            return; //February 29 days
        };
        month++;
        days -=29;
    }
    else{
        dif = days - 28;
        if (dif <= 0) {
            date_array[0] = days;
            date_array[1] = month;
            return; //February 28 days
        };
        month++;
        days -=28;
    }

    dif = days - 31;
    if (dif <= 0) {
        date_array[0] = days;
        date_array[1] = month;
        return; //March
    };
    month++;
    days -= 31;

    dif = days - 30;
    if (dif <= 0) {
        date_array[0] = days;
        date_array[1] = month;
        return; //April
    };
    month++;
    days -= 30;

    dif = days - 31;
    if (dif <= 0) {
        date_array[0] = days;
        date_array[1] = month;
        return; //May
    };
    month++;
    days -= 31;

    dif = days - 30;
    if (dif <= 0) {
        date_array[0] = days;
        date_array[1] = month;
        return; //June
    };
    month++;
    days -= 30;

    dif = days - 31;
    if (dif <= 0) {
        date_array[0] = days;
        date_array[1] = month;
        return; //July
    };
    month++;
    days -= 31;

    dif = days - 31;
    if (dif <= 0) {
        date_array[0] = days;
        date_array[1] = month;
        return; //August
    };
    month++;
    days -= 31;

    dif = days - 30;
    if (dif <= 0) {
        date_array[0] = days;
        date_array[1] = month;
        return; //September
    };
    month++;
    days -= 30;

    dif = days - 31;
    if (dif <= 0) {
        date_array[0] = days;
        date_array[1] = month;
        return; //October
    };
    month++;
    days -= 31;

    dif = days - 30;
    if (dif <= 0) {
        date_array[0] = days;
        date_array[1] = month;
        return; //November
    };
    month++;
    days -= 30;

    dif = days - 31;
    if (dif <= 0) {
        date_array[0] = days;
        date_array[1] = month;
        return; //December
    };
    month++;
    days -= 31;
}

void date_to_string(){
    int num_count=0, i=0;
    int constant = 0, j = 0;
    char date_string[20] = { 0 };

    while(i<6){
        if (i==2){constant = 4, j = 3;}
        else{constant = 2, j = 1;}

        while(j>0){
                date_string[num_count + j] = date_array[i] % 10 + '0';
                date_array[i] = date_array[i]/10;
                j--;
        }
        date_string[num_count] = date_array[i] % 10 + '0';
        num_count += constant;

        if (num_count==10){date_string[num_count] = ' ';}
        else if (num_count == 2||num_count==5){date_string[num_count] = '.';}
        else if (num_count ==  13|| num_count == 16){date_string[num_count] = ':';}

        num_count++;
        i++;
    }

    print_string(date_string);
    current_cursor=0;
    cursor_moveto(current_str, current_cursor);
    date_array[0] = 0;
    date_array[1] = 0;
    date_array[2] = 0;
    date_array[3] = 0;
    date_array[4] = 0;
    date_array[5] = 0;
}

void define_date(int format_count, int year, int sec_extra) {
    int sec=0;
    
    if(format_count==0){
    date_array[2] = year;
    date_array[1] = 1;
    date_array[0] = 1;
    date_to_string();
    return;
    }

    if(year==1601){sec = (format_count % 6)+sec_extra;format_count /= 6;}
    else{sec = format_count % 60;format_count /= 60;}
    
    int min = format_count % 60;
    format_count /= 60;
    int hour = format_count % 24;
    format_count /= 24;

    int dif = 0, i=0, years = year, days = format_count;
    while(i<999){
        dif = days - 365;
        if (dif < 0) {break;}

        if ((years % 4==0) && (dif > 0)){days -= 366;}
        else if(dif>=0){days -= 365;}

        years++;
        i++;
    }
    date_array[0] = days; date_array[2] = years;

    define_month(date_array[0], date_array[2]);

    date_array[0] = date_array[0]+4;
    date_array[3] = hour;
    date_array[4] = min;
    date_array[5] = sec;
    date_to_string();
}

//Parsing
void time_func(int i) {
    char input_command[40] = {0};
    int num = 0, num_count = 0;
    while (i < 40) {
        if ((input_str[i] == '\0')||(input_str[i] == '\n')||(input_str[i] == ' ')) {input_command[num_count] = '\0'; break;}
        input_command[num_count] = input_str[i];
        i++; num_count++;
    }

    if(input_command[0]=='\0'){message_error(2); memset(input_str); return;}

    for(int i=0;i<40;i++){
    if(input_command[i]=='\0'){break;}
    if(!((input_command[i]>='0')&&(input_command[i]<='9'))){message_error(2); memset(input_str); return;}
    }

    if(flag_posixtime==1){
        num=atoi(input_command); 
        if (num == -1) {message_error(1); memset(input_str); return;}

        char uint32[40]={"4294967295"};
        if(input_command[10]!='\0'){message_error(3); memset(input_str); return;}
        else{
            int i = 0;
            bool flag = false;
            while (i<11){
                if((input_command[i]=='\0')||(i<10)){flag = true; break;}
                if(input_command[i]<uint32[i]){flag = true; break;}
                if(uint32[i]=='\0'&&input_command[i]=='\0'){flag = true; break;}
                i++;
            }
            if (flag==false){message_error(3); memset(input_str); return;}
        }
    }

    if (flag_posixtime == 1){num=atoi(input_command); define_date(num, 1970,0);}

    if (flag_wintime == 1){
        int i=0, sec_extra=0;
        while(i<40){if(input_command[i]=='\0'){break;}i++;}

        int j = i-7;
        while(j<=i){input_command[j]='\0'; j++;}

        if(input_command[i-8]!='\0'){sec_extra=input_command[i-8]-'0';}
        input_command[i-8]='\0';
        int num=atoi(input_command);
        define_date(num, 1601,sec_extra);
    }
    flag_wintime = 0;
    flag_posixtime = 0;
}

bool check_nsconv(char* to_convert, int sys_from, int sys_to){
    if(to_convert[0]=='\0'){message_error(2);return false;}

    if (!((2<sys_from<36)&&(2<sys_to<36))){message_error(5);return false;}

    char uint32[40]={"4294967295"};

    if(to_convert[10]!='\0'){message_error(3);return false;}
    else{
        int i = 0;
        while (i<11){
            if((to_convert[i]=='\0')||(i<10)){return true;}
            if(to_convert[i]<uint32[i]){return true;}
            if(uint32[i]=='\0'&&to_convert[i]=='\0'){return true;}
            i++;
        }
        message_error(3);return false;
    }

    int i = 0;
    while((i<40)&&(to_convert[i]!='\0')){
        for(int j=0;j<37;j++){
            if((to_convert[i]==symbol_array[j])&&(j>=sys_from)){message_error(4);return false;}
        }
        i++;
    }
    return true;
}

void nsconv(int i) {
    flag_nsconv=0;
    char input_command[40]={0};
    char to_convert[40]={0};
    int sys_from=0, sys_to=0, num_count=0;
    int flag=1;
    
    while (i < 40){
        if ((input_str[i] == '\0')||(input_str[i] == ' ')) {
            if (flag == 1){to_convert[num_count] = '\0';}
            else{input_command[num_count] = '\0';}

            if (flag == 2){sys_from = atoi(input_command);}
            if (flag == 3){sys_to = atoi(input_command);}

            memset(input_command); num_count = -1; flag++;
            if (flag == 4){break;}
        }

        if (flag == 1){to_convert[num_count] = input_str[i];}
        else{input_command[num_count] = input_str[i];}

        i++; num_count++;
    }

    if(check_nsconv(to_convert, sys_from, sys_to)==false){memset(input_str); return;}

    char result[101] = {0};
    int counter = 0, j, length=0, num1=0, num2=99;
    length = strlen(to_convert);
    while (counter < length){
        int num2 = -1; j = 0;
        while (j < 37){if (symbol_array[j] == to_convert[counter]){num2 = j; break;}; j++;}
        num1 = num1 * sys_from + num2;
        counter++;
    }
    j = 0; length = 0;
    while(num1!=0){
        j = num1 % sys_to;
        result[num2] = symbol_array[j];
        num2--;
        length++;
        num1 = num1 / sys_to;
    }
    j = 0;
    num2++;
    while(true){result[j++] = result[num2++];if (num2 == 100){break;}}
    result[j + 1] = '\0';
    current_cursor=0;
    cursor_moveto(current_str, current_cursor);
    print_string(result);
}

void start() {
    char input_command[40] = {0};
    int i = 0, num_count = 0, num1 = 0, num2 = 0, num3 = 0;
    while (i < 40) {
        if (input_str[i] == ' ' || input_str[i] == '\0') {
            input_command[i] = '\0';
            if(strcmp(input_command, "info")){
                print_string("ConvertOS:");
                print_string("Assembler translator for loader: GNU, syntax: AT&T; Kernel Compiler: gcc");
                print_string("By Olga Efremova");

                if(sys_colour==0x0F){print_string("ConvertOS Colour - white");}
                else if(sys_colour==0x01){print_string("ConvertOS Colour - blue");}
                else if(sys_colour==0x0E){print_string("ConvertOS Colour - yellow");}
                else if(sys_colour==0x02){print_string("ConvertOS Colour - green");}
                else if(sys_colour==0x04){print_string("ConvertOS Colour - red");}
                else if(sys_colour==0x07){print_string("ConvertOS Colour - gray");}
                cursor_moveto(current_str, current_cursor);
            }
            else if(strcmp(input_command, "clear")){
                char *ptr_video = (char*)VIDEO_BUF_PTR;
                int i = 0;
                while (i < VIDEO_WIDTH*25){*(ptr_video + i*2) = '\0'; i++;}
                current_str=0; current_cursor=0;
                cursor_moveto(current_str, current_cursor);
            }
            else if(strcmp(input_command, "posixtime")){flag_posixtime = 1;}
            else if(strcmp(input_command, "nsconv")){flag_nsconv = 1;}
            else if(strcmp(input_command, "wintime")){flag_wintime = 1;}
            else if(strcmp(input_command, "shutdown")){outw(0x604, 0x2000);}
            else message_error(2);
            break;
        }
        input_command[i] = input_str[i];
        i++;
    }
    i++;
    memset(input_command);

    if (flag_nsconv == 1){nsconv(i);}
    else if (flag_wintime == 1 || flag_posixtime == 1){time_func(i);}
}

//Keyboard
void keyboard_scan(char key_num){    
    if (key_num == 15){
        //Tab
        char* ptr_video = (char*) VIDEO_BUF_PTR;
        current_cursor+=4;
        ptr_video+=current_str*VIDEO_WIDTH + current_cursor;
        cursor_moveto(current_str, current_cursor);
        return;
    }
    else if(key_num>=2&&key_num<=13||key_num>=16&&key_num<=27||key_num>=30&&key_num<=41||key_num>=43&&key_num<=53||key_num==57){
        //Numbers+Letters+Space+Special
        char symbol=0;
        char symbol_codes[] = "001234567890-=0\tqwertyuiop[]00asdfghjkl;\'00\\zxcvbnm,./000 ";
        symbol=symbol_codes[(int)key_num];
        if(current_cursor!=40){
            char* ptr_video = (char*) VIDEO_BUF_PTR;
            ptr_video += 2*(current_str*VIDEO_WIDTH + current_cursor);
            ptr_video[0] = symbol;
            ptr_video[1] = sys_colour;
            if(current_cursor<40){
                input_str[counter_str] = symbol;
                current_cursor++;
                counter_str++;
                cursor_moveto(current_str, current_cursor);
            }
        }
    }
    else if (key_num == 28){
        //Enter
        input_str[counter_str]='\0';
        screen_scroll ();
        start();
        current_cursor=0;
        cursor_moveto(current_str, current_cursor);
        memset(input_str);
        counter_str=0;
        
        char* ptr_video = (char*) VIDEO_BUF_PTR;
        ptr_video += 2*(current_str*VIDEO_WIDTH + current_cursor);
        ptr_video[0] = '-';
        ptr_video[1] = sys_colour;
        ptr_video[2] = ' ';
        ptr_video[3] = sys_colour;
        current_cursor+=2;
        cursor_moveto(current_str, current_cursor);

        flag_nsconv=0; flag_wintime=0; flag_posixtime = 0;
        return;
    }
    else if (key_num == 14) {
        //Backspace
        char* ptr_video = (char*) VIDEO_BUF_PTR;
        ptr_video += 2*(current_str*VIDEO_WIDTH + current_cursor-1);
        ptr_video[0] = '\0';
        if(current_cursor>2){current_cursor--;}
        cursor_moveto(current_str, current_cursor);
        if(counter_str>0){counter_str--;}
        input_str[counter_str]='\0';
        return;
    }
}

void message_error(int flag_error){
    flag_nsconv=0; flag_wintime=0; flag_posixtime = 0;
    memset(input_str);
    char* message_error={0};

    if(flag_error==1){message_error = "Incorrect number input - Error";}
    else if(flag_error==2){message_error = "Incorrect input - Error";}
    else if(flag_error==3){message_error = "Overflow - Error";}
    else if(flag_error==4){message_error = "Entered number is not supported by the target system - Error";}
    else if(flag_error==5){message_error = "Incorrect number input - Error";}


    print_string(message_error);
    current_cursor=0;
    cursor_moveto(current_str, current_cursor);
}

extern "C" int kmain() {
    char load_colour= (*(char*)(0x600));
    if(load_colour==1){sys_colour=0x07;} //gray
    else if(load_colour==4){sys_colour=0x01;} //blue
    else if(load_colour==2){sys_colour=0x0F;} //white
    else if(load_colour==5){sys_colour=0x04;} //red
    else if(load_colour==3){sys_colour=0x0E;} //yellow
    else if(load_colour==6){sys_colour=0x02;} //green
    
    unsigned char* ptr_video = (unsigned char*) VIDEO_BUF_PTR;
    for(int j=1;j<4002;j+=2){
    ptr_video[j] = sys_colour;   
    }

    print_string("Welcome! It is 'ConvertOS'");

    intr_init();
    keyb_init();
    intr_start();
    intr_enable();
    cursor_moveto(current_str, current_cursor);

    ptr_video = (unsigned char*) VIDEO_BUF_PTR;
    ptr_video += 2*(current_str*VIDEO_WIDTH + current_cursor);
    ptr_video[0] = '-';
    ptr_video[1] = sys_colour;
    ptr_video[2] = ' ';
    ptr_video[3] = sys_colour;
    current_cursor+=2;
    cursor_moveto(current_str, current_cursor);

    while(1){asm("hlt");}
    return 0;
}


