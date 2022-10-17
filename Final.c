// Lcd pinout settings
   sbit LCD_RS at RB4_bit;
   sbit LCD_EN at RB5_bit;
   sbit LCD_D7 at RB3_bit;
   sbit LCD_D6 at RB2_bit;
   sbit LCD_D5 at RB1_bit;
   sbit LCD_D4 at RB0_bit;

// Pin direction
   sbit LCD_RS_Direction at TRISB4_bit;
   sbit LCD_EN_Direction at TRISB5_bit;
   sbit LCD_D7_Direction at TRISB3_bit;
   sbit LCD_D6_Direction at TRISB2_bit;
   sbit LCD_D5_Direction at TRISB1_bit;
   sbit LCD_D4_Direction at TRISB0_bit;

char i, rfid[13]; // Counter & Character array to store Tag ID
char error; // To save error state from software UART
unsigned int Mcntr; // Miliseconds counter for delay function
unsigned char c_err=0;  // Software counter to avoid starving in the software uart read function by calling the UART function
unsigned char myRxBuffer;  // To saved received byte
unsigned char myRxFlag;   // To check if we received anything
int Finger_cntr; // To avoid starvation waiting for the finger print to be read
unsigned char HL;//High Low for the compare PWM
unsigned int angle; // Servomotor angle
unsigned char Key_RFID [13] ={'8', '3', '7', '0', '0', '9', 'D', '2', '8', 'C', '6', '4'}; //Authorized RFID tag
unsigned char access_flag; // Flag to check access



void myDelay(unsigned int x);
void USART_init(void);



void interrupt(void){
if(PIR1&0x04){//CCP1 interrupt --> Highest Priority to ensure proper timing for the servomotor PWM
   if(HL){ //high
     CCPR1H= angle >>8;
     CCPR1L= angle;
     HL=0;//next time low
     CCP1CON=0x09;//next time Falling edge
     TMR1H=0;
     TMR1L=0;
   }
   else{  //low
     CCPR1H= (40000 - angle) >>8;
     CCPR1L= (40000 - angle);
     CCP1CON=0x08; //next time rising edge
     HL=1; //next time High
     TMR1H=0;
     TMR1L=0;

   }

 PIR1=PIR1&0xFB;  // Clear the flag
 }


if(INTCON&0x04){// TMR0 overflow interrupt, will get here every 1ms
    TMR0=248;
    Mcntr++;

    if (c_err>=1000){ // To avoid starvation for the Software UART read function
    Soft_UART_Break();
    c_err=0; }
    else{
    c_err++;
    }

  INTCON = INTCON & 0xFB;}  //clear T0IF

   if(PIR1&0x20){//ISR for the Receive interrupt
        myRxBuffer=RCREG;// read the received byte
        myRxFlag=1;

    PIR1=PIR1& 0xDF;//Clear RCIF
    }

    if(PIR1 & 0x02){   // ISR for TMR2 overflow, will get here every 2 ms
    Finger_cntr++;
    if(Finger_cntr == 5000){ // will get here every 10 seconds
    myRxFlag = 1; // To avoid starving the program waiting for the fingerprint, this will break the loop waiting to receive a byte from the fingerprint
    Finger_cntr = 0;
    }
    PIR1 = PIR1 & 0xFC; // Clear the flag

                  }


}

void main()
{

 Lcd_Init();                       // Initialize LCD
 Lcd_Cmd(_LCD_CLEAR);              // Clear display
 Lcd_Cmd(_LCD_CURSOR_OFF);         // Cursor off
 USART_init(); // Initialize hardware UART at 9600 bps
 INTCON = 0xE0; // GIE, PEIE, and T0IE enabled
 OPTION_REG = 0x87; ////Fosc/4 with 256 prescaler => increment every 0.5us*256=128us ==> overflow 8count*128us=1ms to overflow
 TMR0=248;
 T2CON = 0x07; // TMR2 will overflow ever 2ms, we will use it to interrupt the program when it is waiting for a fingerprint by making it look like
               // it received a byte. We will do this to avoid starvation (waiting for a fingerprint).
 PIE1= 0x27; // Enable RCIE, CCP1IE, TMR2IE, TMR1IE
 Mcntr = 0;
 TRISC=0x82;// RC7&RC1 Input, Rest is Output
 TRISB=0x00; // Port B is output and interfaced with the LCD
 myRxBuffer=0; // Empty buffer
 myRxFlag=0;
 TMR1H=0;
 TMR1L=0;
 HL=1; //start high
 CCP1CON=0x08;// Set output on match
 T1CON=0x01;//TMR1 On Fosc/4 (inc 0.5uS) with 0 prescaler (TMR1 overflow after 0xFFFF counts ==65535)==> 32.767ms
 CCPR1H=2000>>8;
 CCPR1L=2000;
 error = Soft_UART_Init(&PORTC, 1, 0, 9600, 0); // RC1 is a software UART receiver, & RC0 is a software UART transmitter, Baud rate set to 9600
 angle = 4000; // Initialize servo angle

 rfid[12] = '\0';               // String Terminating Character


 while(1)
 {
  access_flag = 1; // Reset access_flag everytime
  Lcd_Cmd(_LCD_CLEAR);
  Lcd_Out(1,1,"WELCOME!");
  Lcd_Out(2,1,"Scan your tag!");
  do{for(i=0;i<12;)         // To Read 12 characters
    {
         rfid[i] = Soft_UART_Read(&error);
         i++;
     }} while(error); // While reading is successful (error = 0) continue reading

     Lcd_Cmd(_LCD_CLEAR);


    for(i=0; i<12; i++) if(rfid[i]!=Key_RFID[i]) access_flag = 0; // Check if key is authorized
    if(access_flag) { // Authorized RFID tag
    Finger_cntr = 0; // Software counter to interrupt the program in case no fingerprint was read within 10 seconds.
    myRxBuffer = 0;  // To reset the finger_print access_flag
    Lcd_Out(2,1,"Scan you finger!");
    myRxFlag = 0; // To make sure the the receive flag is false before trying to read the fingerprint
    while(!myRxFlag);    // wait to receive signal from fingerprint
    myRxFlag=0;
    if(myRxBuffer)   {
     Lcd_Cmd(_LCD_CLEAR);
     Lcd_Out(2,1,"Permitted");
     angle = 2000; // Rotate servo motor to 180 degrees (OPEN DOOR!)
     myDelay(10000);
     angle = 4000; // Rotate servo back to 0 degrees to close door
     }
     else  {
    Lcd_Cmd(_LCD_CLEAR);
    Lcd_Out(2,1,"Time out! ");
    myDelay(1500);
            }


















    }


   else { // Non-authorized RFID tag
   Lcd_Out(2,1,"Denied");
   myDelay(1500);}
   }
   }



void myDelay(unsigned int x){
     Mcntr=0;
     while(Mcntr<x);
}
void USART_init(void){
  SPBRG=12;
  TXSTA=0x20;//low speed (9600bps), 8-bit, enable Tx, Asynchronous
  RCSTA=0x90;//Enable SP, 8bit data, continuous receive
  PIE1=PIE1|0x20;//RCIE
  INTCON=INTCON|0xC0;//GIE, PIE
}
