//******************************************************************************
//            Wecker                            
//     Autor:         André Hering, Matthias Jahn                
//    Version:     0.1                            
//                                        
//******************************************************************************
#include <msp430f2272.h>
//******************************************************************************
// Beschreibung    : Globale Variablen werden initialisiert
//******************************************************************************
unsigned char datetime[7]; // 0. Sekunden, 1. Minute, 2. Stunden, 3. Tage,4. Wochentag(0=Sonntag, 6=Samstag) 5. Monate, 6. Jahre
unsigned char state; //Statusbyte, 0. Skundenänderung 1. Minutenänderung 2. Stundenänderung 3. Tagesänderung 4. Wochentagsänderung 5. Monatsänderung 6. Jahresänderung 7. Aktuelles Menü
unsigned char mainstate = BIT1;
unsigned char days_per_month[12] = {31,31,28,31,30,31,30,31,31,30,31,30}; // Tage pro Monat 0. Dezember...11. November
unsigned char Taster = 0; // '4' = S4 (rot), '3' = S3 (gelb), '2' = S2 (grün), '1' = S1 (blau)
unsigned char weckzeit[2]; // 1. Minute, 2. Stunden
char u_week_days[7] = {'S','M','D','M','D','F','S'};
char l_week_days[7] = {'o','o','i','i','o','r','a'};
unsigned int LCD_base_y[2] = {0x80, 0xc0};   // example for 2x16 display
// 1. line 0x00..0x0F + Bit7==0x80
// 2. line 0x40..0x4F + Bit7==0xc0
// BIT7 nessesary for command "Set DD RAM address"
unsigned int LCD_x,LCD_y,LCD_max_x;
//******************************************************************************
// Beschreibung    : Eine kleine Runde Bezeichner definieren
// Hier könnte man noch was machen, was die Statevariablen angeht, um da evt griffigere bezeichnungen für Hexwerte zu finden
//******************************************************************************
#define LCD_DATA_OUT         P1OUT
#define LCD_DATA_DIR         P1DIR
#define LCD_COMM_OUT         P3OUT
#define LCD_COMM_DIR         P3DIR
#define RS_BIT            ( 0x0001 )                    // RS Display
#define ENABLE_BIT               ( 0x0002 )                    // Enable Display
#define SELECT_COMMAND_REG       LCD_COMM_OUT &= ~RS_BIT          // set RS to low
#define SELECT_DATA_REG          LCD_COMM_OUT |=  RS_BIT          // set RS high
// "clocksignal" of LCD
#define DISPLAY_ENABLE_HIGH      LCD_COMM_OUT |=  ENABLE_BIT      // enable HIGH
#define DISPLAY_ENABLE_LOW       LCD_COMM_OUT &= ~ENABLE_BIT      // enable LOW
#define F_CPU 8000                        //Takt der CPU in Hz
void calc_feb(void){
//******************************************************************************
//Beschreibung    : Berechnet anhand des Jahres die Anzahl der Tage des Februars
//Input        : none
//Output    : none
//******************************************************************************
    if(0 == datetime[6] % 4){            // Durch 4 Teilbare Jahre Schaltjahr
        //if(0 == datetime[6] % 100){        // Durch 100 Teilbare Jahre kein Schaltjahr
        //    if(0 == datetime[6] % 400){    // Durch 400 Teilbare Jahre Schaltjahr
        //        days_per_month[2] = 29;
        //    }
        //    else{
        //        days_per_month[2] = 28;
        //    }
        //}
        //else{
        days_per_month[2] = 29;
        //}    
    }
    else{
        days_per_month[2] = 28;
    }
}
void delay_ms(unsigned int time2wait)
{
//******************************************************************************
//Beschreibung    : Berechnet über die Prozessorfrequenz die zu wartenden Takte
//Input        : Wartezeit in ms
//Output    : none
//******************************************************************************
    int i = 0;
    i = F_CPU * time2wait/1000;        //Berechnung der benötigten Takte
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
    
    delay_ms(15);                    //wait 15ms after Vcc
    LCD_DATA_DIR |= 0x000F;                // sets data lines as outputs
    LCD_COMM_DIR |= 0x0003;                // sets control lines as outputs    
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
    if (y>1) y=0;                //Startposition
    LCD_write_CMD(LCD_base_y[y]+x);    //Pos. anwählen
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
        str++;                //nächstes Zeichen wählen
    }
};
 
void LCD_clr()
{
    LCD_write_CMD(0x01);            //Clear-CMD
        LCD_write_Pos(0,0);
}
 
 
void int_to_ascii(unsigned char cmd){
//******************************************************************************
//Beschreibung        : Wandel entsprechend der Position cmd int aus datetime 
//            int in char um und gibt sie auch LCD aus
//Input            : int wert
//Output        : none
//Einschränkungen    : maximal 2 stellige int
//******************************************************************************
    unsigned char zehner;
    unsigned char einer;
    if(cmd != 4){
        zehner = 0x30 + datetime[cmd] / 10;    //Zehner stelle in ASCII
        einer = 0x30 + datetime[cmd] % 10;    //Einer stelle in ASCII
    }
    else{
        zehner = u_week_days[datetime[4]];    //1. Buchstabe wochentag
        einer = l_week_days[datetime[4]];    //2. Buchstabe wochentag
    }
    //0 1 2 3 4 5 6 7 8 9 A B C D E F
    //********************************
    //_ _ _ _ _ _ _ _ S S : M M : S S* 0
    //                 *
    //_ _ _ _ _ W T _ T T . M M . J J* 1
    //********************************
    switch(cmd){
        case 0:
            LCD_write_Pos(14,0);        
            break;                
        case 1:                    
            LCD_write_Pos(11,0);        
            break;                
        case 2:
            LCD_write_Pos(8,0);
            break;
        case 3:
            LCD_write_Pos(8,1);
            break;
        case 4:
            LCD_write_Pos(5,1);
            break;
        case 5:
            LCD_write_Pos(11,1);
            break;
        case 6:
            LCD_write_Pos(14,1);
            break;
    }
    LCD_write_char(zehner);
    LCD_write_char(einer);
}
void update_datetime(void){
//******************************************************************************
//Beschreibung  : Zählt eine Sekunde nach oben und aktualisier entsprechend alle anderen Werte von datetime
//input         : none
//output        : none
//******************************************************************************
    datetime[0] = (datetime[0] + 1) % 60;    // Sekunde erhöhen
    state |= BIT0;                // Statusbit 0 auf 1 gesetzt, veränderung der Sekunde
    if(0 == datetime[0]){            // Wenn sekunden 0 sind, dann wird Minuten erhöht
        datetime[1] = (datetime[1] + 1) %60;    // Minute erhöhen
        state |= BIT1;                // Statusbit 1 auf 1 setzen, veränderung der Minute
        if(0 == datetime[1]){                // Wenn Minuten 0 sind werden Stunden erhöht
            datetime[2] = (datetime[2] + 1) % 24;    // Stunden erhöhen
            state |= BIT2;                // Statusbit 2 auf 1 setzen, veränderung der Stunde
            if(0 == datetime[2]){                // Wenn Stunden 0 sind wird Tag und Wochentag erhöht
                datetime[3] = (datetime[3] +1);        // Tag wird um 1 erhöht
                state |= BIT3;                // Statusbit 3 auf 1 setzen, veränderung Tag
                datetime[4] = (datetime[4] + 1) % 7;     // Wochentag wird erhöht
                state |= BIT4;                // Statusbit 4 auf 1 setzen, veränderung Wochentag
                if (datetime[3] > days_per_month[datetime[5]]){    // Wenn Tage tage im Monat überschriten,
                    datetime[3] = 1;            // dann Tage auf 1 setzen und
                    datetime[5] = (datetime[5] + 1) % 12;    // Monat um 1 erhöhen
                    state |= BIT5;                // Statusbit 5 auf 1 setzen, veränderung Monat
                    if(1 == datetime[5]){            // Wenn neuer Monat Januer, dann
                        datetime[6]++;            // Jahr um 1 erhöhen
                        state |= BIT6;            // Statusbit 6 auf 1 setzten, jahr verändert
                        calc_feb();            // Februarlänge neu berechnen
                    }
                }
            }
        }
    }
}
void LCD_update(void){
//******************************************************************************
// Beschreibung    : aktualisiert das LC display je nach status des des Menüaufrufs und setzt das state byte auf unverändert
// input    : none
// output    : none
//******************************************************************************
    switch(state){
        case 0xFF:        //Alle Werte haben sich geändert und Hauptmenü
            for(int i=0;i<7;i++){
                int_to_ascii(i);
            }
            break;
        case 0xBF:        //Jahr konstant, Hauptmenü
            for(int i=0;i<6;i++){
                int_to_ascii(i);
            }
            break;
        case 0x9F:        //Jahr, Monat konstant, HM
            for(int i=0;i<5;i++){
                int_to_ascii(i);
            }
            break;
        case 0x87:        //Jahr, Monat, Tag Konstant, HM
            for(int i=0;i<3;i++){
                int_to_ascii(i);
            }
            break;
        case 0x83:        //Jahr, Monat, Tag, Stunde konstant , HM
            for(int i=0;i<2;i++){
                int_to_ascii(i);
            }
            break;
        case 0x81:        // nur Sekunde hat sich geändert, HM
            int_to_ascii(0);
            break;
    }
    state = 0x80;
}
void init_datetime(void){
//******************************************************************************
// Beschreibung    : Initialisiert das datetime array mit der aktuellen Uhrzeit im idealfall vm DCF77 Modul
// input    : none
// output    : none
//******************************************************************************
    datetime[0] = 00;
    datetime[1] = 28;
    datetime[2] = 13;
    datetime[3] = 14;
    datetime[4] = 2;
    datetime[5] = 5;
    datetime[6] = 13;
}
void init_main_menu(void){
//******************************************************************************
// Beschreibung    : Initialisiert das Hauptmenü
// input    : none
// output    : none
//******************************************************************************
    state = 0xFF;
    //LCD_clr();
        LCD_write_Pos(10,0);
    LCD_write_char(':');
    LCD_write_Pos(13,0);
    LCD_write_char(':');
    LCD_write_Pos(10,1);
    LCD_write_char('.');
    LCD_write_Pos(13,1);
    LCD_write_char('.');
    LCD_update();
}
void Weckzeit_Einstellung(void){
//******************************************************************************
// Beschreibung    : 
// input    : none
// output    : none
//******************************************************************************
        unsigned char weckindex = 0;
        unsigned char weckzeittmp[4] = {0, 0, 0, 0}; //hh:mm 
        LCD_write_Pos(8,0);
        
        while(mainstate == BIT2){
          switch(Taster){
            case 1:               // BLAUER - Taster
              LCD_clr();
              mainstate = BIT1; 
              init_main_menu();
            break;                  
                  
            case 2:               // GRÜNER - Taster      / Zeiteinheit +
              switch(weckindex){
                case 0:           // Stunde (Zehner)
                  if(weckzeittmp[0] == 2)
                    weckzeittmp[0] = 0;
                  else
                    weckzeittmp[0] += 1;
                  LCD_write_char(0x30 + weckzeittmp[0]);
                  LCD_write_Pos(8,0); 
                break;
                
                
                case 1:           // Stunde (Einer)
                  if(weckzeittmp[0] == 2){
                    if(weckzeittmp[1] == 4)
                      weckzeittmp[1] = 0;
                    else
                      weckzeittmp[1] += 1;
                  }  
                  else{  
                    if(weckzeittmp[1] == 9)
                      weckzeittmp[1] = 0;
                    else
                      weckzeittmp[1] += 1;
                  }
                  LCD_write_char(0x30 + weckzeittmp[1]);
                  LCD_write_Pos(9,0); 
                break;
                
                case 2:           // Minute (Zehner)
                  if(weckzeittmp[2] == 5)
                    weckzeittmp[2] = 0;
                  else
                    weckzeittmp[2] += 1;
                  LCD_write_char(0x30 + weckzeittmp[2]);
                  LCD_write_Pos(11,0); 
                break;
                
                case 3:           // Minute (Einer)
                  if(weckzeittmp[3] == 9)
                    weckzeittmp[3] = 0;
                  else
                    weckzeittmp[3] += 1;
                  LCD_write_char(0x30 + weckzeittmp[3]);
                  LCD_write_Pos(12,0); 
                break;                
              }    
            break;
                  
            case 3:               // GELBER - Taster      / Zeiteinheit -
              //..............................................................
            break;                  
                  
            case 4:               // ROTER - Taster       / Zeiteinheit wechseln
              if(LCD_x == 8){
                LCD_write_Pos(9,0);
                weckindex = 1;
              }
              else if(LCD_x == 9){
                LCD_write_Pos(11,0);
                weckindex = 2;
              }
              else if(LCD_x == 11){
                LCD_write_Pos(12,0);
                weckindex = 3;
              }              
              else if(LCD_x == 12){
                LCD_write_Pos(8,0);
                weckindex = 0;
              } 
              
            break;
          }
          Taster = 0;
          if(mainstate == BIT2)
            LPM3;
        }
}
 

#pragma vector=TIMERA1_VECTOR
__interrupt void Timer_A1(void){
//******************************************************************************
//Beschreibung    : Time A3 ISR, Aktualisiert datetime, unterbricht im zweifel dan LPM3
//input        : none
//output    : none
//******************************************************************************
    switch(TAIV){                // TAIV auswerten
        case 2:                // CCR1 Erreicht
            update_datetime();    // Zeit aktualisieren
            break; 
        case 10:            // Überlauf erreicht
            update_datetime();    // Zeit aktualisieren
            break;            
    }
    LPM3_EXIT;                // LPM3 verlassen
}
#pragma vector=PORT2_VECTOR
__interrupt void P2_ISR(void){
//******************************************************************************
//Beschreibung    : Fragt die vier vorhanden Taster ab und gibt den aktiven Taster über eine Globale zurück
//input        : none
//output    : none
//******************************************************************************
        delay_ms(100);                          // x ms warten -> entprellen
        if(P2IFG & BIT0)                        // ROTER - Taster
        {
         Taster = 4; 
         P2IFG &= ~BIT0;
        }
        else if(P2IFG & BIT1)                   // GELBER - Taster
        {
         Taster = 3; 
         P2IFG &= ~BIT1;
        }
        else if(P2IFG & BIT2)                   // GRÜNER - Taster
        {
         Taster = 2; 
         P2IFG &= ~BIT2;
        }
        else if(P2IFG & BIT5)                   // BLAUER - Taster
        {
         Taster = 1; 
         P2IFG &= ~BIT5;
         if(mainstate == BIT1)                   // Menüwechsel
           mainstate = BIT2;                       
        }
        
        LPM3_EXIT;       
}         

int main(void){
//******************************************************************************
// Description  : main program
// Input        : none
// Output       : none
//******************************************************************************
    WDTCTL = WDTPW + WDTHOLD;    // Stop watchdog timer to prevent time out reset
        //P1DIR = 0xFF;                 // P1 als Output (1.0 .. 1.7)
        P2IES = BIT0+BIT1+BIT2+BIT5;    // ...
        P2IE =  BIT0+BIT1+BIT2+BIT5;    // ...
        
    LCD_init();            // Initialisiert das LC-Display
    init_datetime();        // Initialisiert das datetime array
    init_main_menu();               // schreibt das Hauptmenü auf das LC-Display
        CCTL1 = OUTMOD_4 + CCIE;    // CCR1 interrupt einschalten
        CCR1 = 0x8000;            // CCR1 auf Hälfte von FFFF setzen
    TACTL = TASSEL0 + TACLR + TAIE; // ACLK, clear TAR, interrupt enabled
    TACTL |= MC1;                   // Timer_A in continuous mode starten
         
    __enable_interrupt();           // Interrupt global einschalten
    
        while(1){
          switch(mainstate){
            case BIT1:                  // Hauptmenü
              LCD_update();        // Display aktualisieren
              LPM3;            // Energiesparmodus LPM3 starten
            break;
            
            case BIT2:                  // Weckzeiteinstellung
              LCD_clr();                // Komplette Anzeige löschen
              Taster = 0;               // Damit beim Wechsel vom Hauptmenü das Display nicht zurückgesetzt wird
 
              
              Weckzeit_Einstellung();
            break;
          }  
    }
    return 0;
}
