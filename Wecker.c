//******************************************************************************
//			Wecker							
// 	Autor: 		André Hering, Matthias Jahn				
//	Version: 	0.1							
//										
//******************************************************************************

#include <msp430x22x2.h>

unsigned int _base_y[2] = {0x80, 0xc0};   // example for 2x16 display
          // 1. line 0x00..0x0F + Bit7==0x80
          // 2. line 0x40..0x4F + Bit7==0xc0
          // BIT7 nessesary for command "Set DD RAM address"
            
unsigned int _lcd_x,_lcd_y,_lcd_maxx;
unsigned char datetime[6]; // 0. Sekunden, 1. Minute, 2. Stunden, 3. Tage, 4. Monate, 5. Jahre
unsigned char menu_state = 1

#define LCD_DATA_OUT              P1OUT
#define LCD_DATA_DIR              P1DIR
#define LCD_COMM_OUT              P3OUT
#define LCD_COMM_DIR              P3DIR
#define RS_BIT               ( 0x0001 )            // RS Display
#define ENABLE_BIT           ( 0x0002 )            // Enable Display
#define SELECT_COMMAND_REG   LCD_COMM_OUT &= ~RS_BIT      // set RS to low
#define SELECT_DATA_REG      LCD_COMM_OUT |=  RS_BIT      // set RS high
// "clocksignal" of LCD
#define DISPLAY_ENABLE_HIGH  LCD_COMM_OUT |=  ENABLE_BIT  // enable HIGH
#define DISPLAY_ENABLE_LOW   LCD_COMM_OUT &= ~ENABLE_BIT  // enable LOW


void lcd_delay(void){
//******************************************************************************
// Beschreibung : Wartezeit generieren, 400 Loop duchläufe
// Input        : none
// Output       : none
//******************************************************************************
	unsigned int count;
	for (count=0; count<400; count++);// delay loop
};

void Send_byte_to_LCD (unsigned char data){
// *****************************************************************************
// Description  : Sends byte to lcd in 4bit-mode
// Input        : data - data to send 
//              : reg - specifies the lcd-register
// Output       : none  
// Date valid at HIGH-LOW edge of enable-signal !!
// High nibble (BIT7-BIT4) are transfered first
// *****************************************************************************
	// send high nibble along BIT0..BIT3
	DISPLAY_ENABLE_HIGH;
	LCD_DATA_OUT = data>>4;
	lcd_delay();
	DISPLAY_ENABLE_LOW;
	lcd_delay();
     
  
	// send low nibble along BIT0..BIT3
	DISPLAY_ENABLE_HIGH;
	LCD_DATA_OUT = data;
	lcd_delay();
	DISPLAY_ENABLE_LOW;
	lcd_delay();
};

void _CommandToLCD(unsigned char data){
//******************************************************************************
// Description  : send instruction to LCD
// Input        : instruction code to send
// Output       : none
//******************************************************************************
	SELECT_COMMAND_REG;
	Send_byte_to_LCD(data);
};

void _DataToLCD(unsigned char data){
//******************************************************************************
// Description  : send data to LCD
// Input        : data to send
// Output       : none
//******************************************************************************
	SELECT_DATA_REG;
	Send_byte_to_LCD(data);
};

void lcd_init(void){
// *****************************************************************************
// Description  : initialize the LCD controller
//              : needs about 30ms to start
// Input        : lcd_columns   - count of columns
// Output       : none
//******************************************************************************
	char initmuster [6]={0x33,0x32,0x28,0x0E,0x01,0x06};
	int i=0;
	// wait for more than 15ms after Vcc rises to 4.5V
	while(i<70){
		i++;
		lcd_delay();
	};

	LCD_DATA_DIR |= 0x000F;         // sets data lines as outputs
 	LCD_COMM_DIR |= 0x0003;         // sets control lines as outputs
   
	i=0;
	DISPLAY_ENABLE_LOW;
	lcd_delay();
	_CommandToLCD(initmuster[i]);      // function set (Interface 8bit)
	lcd_delay();
	while(i<5){
		i++;
		_CommandToLCD(initmuster[i]);      // function set (Interface 8bit) 
	};
};

void lcd_gotoxy(unsigned char x, unsigned char y){
//******************************************************************************
// Description  : set the LCD display position x=0..39 y=0..3
// Input        : x - X-position
//              : y - Y-position
// Output       : none
//******************************************************************************
	if (y>1) y=0;
	_CommandToLCD(_base_y[y]+x); // function set + 8bit bus
	_lcd_x=x;
	_lcd_y=y;
};


void lcd_clear(void){
//******************************************************************************
// Description  : clears the LCD
// Input        : none
// Output       : none
//******************************************************************************
	_CommandToLCD(0xc1);	 	// cursor off
		                    	// function display clear
	_lcd_x=_lcd_y=0;
};

void lcd_putchar(unsigned char c){
//******************************************************************************
// Description  : write a char to the LCD
// Input        : c - character
// Output       : none
//******************************************************************************
	if (_lcd_x>=16){
		_lcd_y++;
		lcd_gotoxy(0,_lcd_y);
	};
	_DataToLCD(c);            // send data
	_lcd_x++;
};

void lcd_puts(unsigned char *str){
//******************************************************************************
// Description  : write the string str to the LCD
// Input        : *str  pointer to string
// Output       : none
//******************************************************************************
	while(*str!=0x0){
		lcd_putchar(*str);
		str++;
	}
}

void int_to_ascii(unsigned char cmd){
//******************************************************************************
//Description   :wandelt int in char gibt das uf display aus entsprechnd der position
//Input         :int wert
//Output        :none
//Restrictions  :maximal 2 stellige int
//******************************************************************************
	unsigned char zehner = 0x30 + datetime[cmd] / 10;
	unsigned char einer = 0x30 + datetime[cmd] % 10;
	switch(cmd){
		case 0:
			lcd_gotoxy(6,0);
			break;
		case 1:
			lcd_gotoxy(3,0);
			break;
		case 2:
			lcd_gotoxy(0,0);
			break;
		case 3:
			lcd_gotoxy(0,1);
			break;
		case 4:
			lcd_gotoxy(3,1);
			break;
		case 5:
			lcd_gotoxy(6,1);
			break;
	}
lcd_putchar(zehner);
lcd_putchar(einer);
}

void update_datetime(){
//******************************************************************************
//Beschreibung  : Zählt eine Sekunde nach oben
//input         : none
//output        : none
//******************************************************************************
	datetime[0] = (datetime[0] + 1) % 60;
	if (menu_state) int_to_ascii(0);
	if (0 == datetime[0]){
		datetime[1] = (datetime[1] + 1) % 60;
		if (menu_state) int_to_ascii(1);
		if (0 == datetime[1]){
			datetime[2] = (datetime[2] +1 ) % 24;
			if (menu_state) int_to_ascii(2);
		      // Für Tage Monate und Jahre entsprechnd weiter machen
		}
	}  
}

void main( void ){
//******************************************************************************
// Description  : main program
// Input        : none
// Output       : none
//******************************************************************************
	WDTCTL = WDTPW + WDTHOLD;	// Stop watchdog timer to prevent time out reset
	lcd_init();
	datetime[0] = 32;
	datetime[1] = 25;
	datetime[2] = 12;
	datetime[3] = 7;
	datetime[4] = 5;
	datetime[5] = 13;
	//unsigned char text[32]="Datum:  Zeit:  ";
	//lcd_puts(text);
	for(int i=0;i<6;i++){
		int_to_ascii(i);
	}
	while (1) {
		update_datetime();
		lcd_delay();
	}
}
