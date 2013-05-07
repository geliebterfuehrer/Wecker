//******************************************************************************
//			Wecker							
// 	Autor: 		Andr� Hering, Matthias Jahn				
//	Version: 	0.1							
//										
//******************************************************************************


#include <msp430x22x2.h>

unsigned char datetime[6]; // 0. Sekunden, 1. Minute, 2. Stunden, 3. Tage, 4. Monate, 5. Jahre
#define F_CPU 800000

unsigned int LCD_base_y[2] = {0x80, 0xc0};   // example for 2x16 display
// 1. line 0x00..0x0F + Bit7==0x80
// 2. line 0x40..0x4F + Bit7==0xc0
// BIT7 nessesary for command "Set DD RAM address"

unsigned int LCD_x,LCD_y,LCD_max_x;

#define LCD_DATA_OUT              P1OUT
#define LCD_DATA_DIR              P1DIR
#define LCD_COMM_OUT              P3OUT-
#define LCD_COMM_DIR              P3DIR
#define RS_BIT               ( 0x0001 )            // RS Display
#define ENABLE_BIT           ( 0x0002 )            // Enable Display
#define SELECT_COMMAND_REG   LCD_COMM_OUT &= ~RS_BIT      // set RS to low
#define SELECT_DATA_REG      LCD_COMM_OUT |=  RS_BIT      // set RS high
// "clocksignal" of LCD
#define DISPLAY_ENABLE_HIGH  LCD_COMM_OUT |=  ENABLE_BIT  // enable HIGH
#define DISPLAY_ENABLE_LOW   LCD_COMM_OUT &= ~ENABLE_BIT  // enable LOW


#define F_CPU 800000		//Takt der CPU in Hz



void LCD_Init ( )
{
	char LCD_init_CMDs[6] = {0x33, 0x32, 0x28, 0x0E, 0x01, 0x06};
	int i = 0;
	
	delay_ms(15);					//wait 15ms after Vcc
	LCD_DATA_DIR |= 0x000F;				// sets data lines as outputs
	LCD_COMM_DIR |= 0x0003;				// sets control lines as outputs	
	DISPLAY_ENABLE_LOW;	
	delay_ms(15);
	LCD_write_CMD(LCD_init_CMDs[0]);
	delay_ms(15);
	while (i < 5) 
	{
		i++;
		LCD_write_CMD(LCD_init_CMDs[i]);
	};
};


void LCD_write_byte(unsigned char data)
{
	// send high nibble along BIT0..BIT3
	DISPLAY_ENABLE_HIGH;
	LCD_DATA_OUT = data>>4;
	delay_ms(100);
	DISPLAY_ENABLE_LOW;
	delay_ms(100);
	
	// send low nibble along BIT0..BIT3
	DISPLAY_ENABLE_HIGH;
	LCD_DATA_OUT = data;
	delay_ms(100);
	DISPLAY_ENABLE_LOW;
	delay_ms(100);
};


void LCD_write_CMD(unsigned char data)
{
	SELECT_COMMAND_REG;
	LCD_write_byte(data);
};


void LCD_write_Data(unsigned char data)
{
	SELECT_DATA_REG;
	LCD_write_byte(data);
};


void LCD_write_char(unsigned char c)
{
	if (LCD_x => 16) 
	{
		LCD_y++;
		LCD_write_Pos(0,LCD_y);
	}
	LCD_write_Data(c);
	LCD_x++;
};

void LCD_write_chars(unsigned char *str)
{
	while (*str != 0x0)
	{
		LCD_write_char(*str);
		str++;				//nächstes Zeichen wählen
	}
};


void LCD_write_Pos(unsigned char x, unsigned char y)
{
	y = 0;					//Startposition
	LCD_write_CMD();			//Pos. anwählen
};


void LCD_clr()
{
	LCD_write_CMD(0xC1);			//Clear-CMD
	LCD_x = 0;
	LCD_y = 0;
}


void delay_ms(unsigned int time2wait)
{
//******************************************************************************
//Description   : Berechnet über die Prozessorfrequenz die zu wartenden Takte
//Input         : Wartezeit in ms
//Output        :none
//******************************************************************************
	int i = 0;
	i = F_CPU * time2wait/1000;		//Berechnung der benötigten Takte
	for (int j = 0; j < i; j++);
};


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
			LCD_write_Pos(6,0);
			break;
		case 1:
			LCD_write_Pos(3,0);
			break;
		case 2:
			LCD_write_Pos(0,0);
			break;
		case 3:
			LCD_write_Pos(0,1);
			break;
		case 4:
			LCD_write_Pos(3,1);
			break;
		case 5:
			LCD_write_Pos(6,1);
			break;
	}
LCD_write_char(zehner);
LCD_write_char(einer);
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
		      // F�r Tage Monate und Jahre entsprechnd weiter machen
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
		delay_ms(1000);
	}
}
