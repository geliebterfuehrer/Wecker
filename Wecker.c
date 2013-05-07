//******************************************************************************
//			Wecker							
// 	Autor: 		André Hering, Matthias Jahn				
//	Version: 	0.1							
//										
//******************************************************************************


#include <msp430f2272.h>

unsigned char datetime[6]; // 0. Sekunden, 1. Minute, 2. Stunden, 3. Tage, 4. Monate, 5. Jahre
unsigned char menu_state = 1;
#define F_CPU 800000

unsigned int LCD_base_y[2] = {0x80, 0xc0};   // example for 2x16 display
// 1. line 0x00..0x0F + Bit7==0x80
// 2. line 0x40..0x4F + Bit7==0xc0
// BIT7 nessesary for command "Set DD RAM address"

unsigned int LCD_x,LCD_y,LCD_max_x;

#define LCD_DATA_OUT P1OUT
#define LCD_DATA_DIR P1DIR
#define LCD_COMM_OUT P3OUT
#define LCD_COMM_DIR P3DIR
#define RS_BIT               ( 0x0001 )            // RS Display
#define ENABLE_BIT           ( 0x0002 )            // Enable Display
#define SELECT_COMMAND_REG   LCD_COMM_OUT &= ~RS_BIT      // set RS to low
#define SELECT_DATA_REG      LCD_COMM_OUT |=  RS_BIT      // set RS high
// "clocksignal" of LCD
#define DISPLAY_ENABLE_HIGH  LCD_COMM_OUT |=  ENABLE_BIT  // enable HIGH
#define DISPLAY_ENABLE_LOW   LCD_COMM_OUT &= ~ENABLE_BIT  // enable LOW


#define F_CPU 800000		//Takt der CPU in Hz

void delay_ms(unsigned int time2wait)
{
//******************************************************************************
//Beschreibung	: Berechnet über die Prozessorfrequenz die zu wartenden Takte
//Input		: Wartezeit in ms
//Output	: none
//******************************************************************************
	int i = 0;
	i = F_CPU * time2wait/1000;		//Berechnung der benötigten Takte
	for (int j = 0; j < i; j++);
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

void LCD_init ( )
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

void LCD_write_Data(unsigned char data)
{
	SELECT_DATA_REG;
	LCD_write_byte(data);
};

void LCD_write_Pos(unsigned char x, unsigned char y){
	if (y>1) y=0;				//Startposition
	LCD_write_CMD(LCD_base_y[y]+x);	//Pos. anwählen
	LCD_x=x;
	LCD_y=y;
	
};

void LCD_write_char(unsigned char c)
{
	if (LCD_x >= 16) 
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



void LCD_clr()
{
	LCD_write_CMD(0xC1);			//Clear-CMD
	LCD_x = 0;
	LCD_y = 0;
}





void int_to_ascii(unsigned char cmd){
//******************************************************************************
//Beschreibung		: Wandel entsprechend der Position cmd int aus datetime 
//			int in char um und gibt sie auch LCD aus
//Input			: int wert
//Output		: none
//Einschränkungen	: maximal 2 stellige int
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
	datetime[0] = (datetime[0] + 1) % 60;	//Sekunden nach oben zählen
	if (menu_state) int_to_ascii(0);	//Anzeigen, wenn im Hauptmenü
	if (0 == datetime[0]){
		datetime[1] = (datetime[1] + 1) % 60;	//Minuten hoch zählen
		if (menu_state) int_to_ascii(1);	//Anzeigen, wenn im Hauptmenü
		if (0 == datetime[1]){
			datetime[2] = (datetime[2] + 1) % 24;	//Stunden nach oben zählen
			if (menu_state) int_to_ascii(2);	//Anzeigen, wenn im Hauptmenü
			if (0 == datetime[2]){
				datetime[3]++;			//Tag erhöhen
				switch(datetime[4]){		//je nach Monat
					case 1:		//Januar
						if (32 <= datetime[3]){		//Januar -> 
							datetime[3] = 1;	// Februar
							datetime[4] = 2;
							if (menue_state) int_to_ascii(4);	//Monat anzeigen, enn im Hauptmenü
						}
						break;
					case 2:		//Februar
						if (0 == datetime[5] % 4){	//Schaltjahr
							if (30 == datetime[3]){		//Januar -> 
								datetime[3] = 1;	// Februar
								datetime[4] = 3;
								if (menue_state) int_to_ascii(4);	//Monat anzeigen, wenn im Hauptmenü
							}
						}
						else{
							if (29 == datetime[3]){		//Januar -> 
								datetime[3] = 1;	// Februar
								datetime[4] = 3;
								if (menue_state) int_to_ascii(4);	//Monat anzeigen, wenn im Hauptmenü
							}
						}
						break;
					case 3:		//März
						if (32 == datetime[3]){		//März -> 
							datetime[3] = 1;	// April
							datetime[4] = 4;
							if (menue_state) int_to_ascii(4);	//Monat anzeigen, wenn im Hauptmenü
						}
						break;
					case 4:		//April
						if (31 == datetime[3]){
							datetime[3] = 1;	// April ->
							datetime[4] = 5;	//Mai
							if (menue_state) int_to_ascii(4);	//Monat anzeigen, wenn im Hauptmenü
						}
						break;
					case 5:		//Mai
						if (32 == datetime[3]){
							datetime[3] = 1;	// Mai ->
							datetime[4] = 6;	//Juni
							if (menue_state) int_to_ascii(4);	//Monat anzeigen, wenn im Hauptmenü
						}
						break;
					case 6:		//Juni
						if (31 == datetime[3]){
							datetime[3] = 1;	// Juni ->
							datetime[4] = 7;	//Juli
							if (menue_state) int_to_ascii(4);	//Monat anzeigen, wenn im Hauptmenü
						}
						break;
					case 7:		//Juli
						if (32 == datetime[3]){
							datetime[3] = 1;	// Juli ->
							datetime[4] = 8;	//August
							if (menue_state) int_to_ascii(4);	//Monat anzeigen, wenn im Hauptmenü
						}
						break;
					case 8:		//August
						if (32 == datetime[3]){
							datetime[3] = 1;	// August ->
							datetime[4] = 9;	//September
							if (menue_state) int_to_ascii(4);	//Monat anzeigen, wenn im Hauptmenü
						}
						break;
					case 9:		//September
						if (31 == datetime[3]){
							datetime[3] = 1;	// September ->
							datetime[4] = 10;	//Oktober
							if (menue_state) int_to_ascii(4);	//Monat anzeigen, wenn im Hauptmenü
						}
						break;
					case 10:	//Oktober
						if (32 == datetime[3]){
							datetime[3] = 1;	// Oktober ->
							datetime[4] = 11;	//November
							if (menue_state) int_to_ascii(4);	//Monat anzeigen, wenn im Hauptmenü
						}
						break;
					case 11:	//November
						if (31 == datetime[3]){
							datetime[3] = 1;	// November ->
							datetime[4] = 12;	//Dezember
							if (menue_state) int_to_ascii(4);	//Monat anzeigen, wenn im Hauptmenü
						}
						break;
					case 12:	//Dezember
						if (31 == datetime[3]){
							datetime[3] = 1;	// Dezember ->
							datetime[4] = 1;	//Januar
							datetime[5]++;		//Neus Jahr
							if (menu_state){
								int_to_ascii(4);
								int_to_ascii(5);
							}
						}
						break;
					if (menu_state) int_to_ascii(3);	//Tag Anzeigen, wenn im Hauptmenü
				}
				
			}
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
	LCD_init();
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
