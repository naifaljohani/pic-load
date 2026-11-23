/**
  Generated Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This is the main file generated using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  Description:
    This header file provides implementations for driver APIs for all modules selected in the GUI.
    Generation Information :
        Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.81.8
        Device            :  PIC16F1779
        Driver Version    :  2.00
*/

/*
    (c) 2018 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
    SOFTWARE.
*/

#include "mcc_generated_files/mcc.h"
#include "LCD_I2C_lib.h"
/*
                         Main application
 */

static i2c_operations_t wr1RegCompleteHandler(void *ptr)
{
    I2C_SetBuffer(ptr,1);
    I2C_SetDataCompleteCallback(NULL,NULL);
    return I2C_CONTINUE;
}

void I2C_Write1ByteRegister_m(uint8_t address, uint8_t reg, uint8_t data)
{
    while(!I2C_Open(address)); // sit here until we get the bus..
    I2C_SetDataCompleteCallback(wr1RegCompleteHandler,&data);
    I2C_SetBuffer(&reg,1);
    I2C_SetAddressNackCallback(NULL,NULL); //NACK polling?
    I2C_MasterWrite();
    while(I2C_BUSY == I2C_Close()); // sit here until finished.
}



void LCD_Init(unsigned char I2C_Add) 
{
  i2c_add = I2C_Add;
  IO_Expander_Write(0x00);
  __delay_ms(30);
  LCD_CMD(0x03);
  __delay_ms(5);
  LCD_CMD(0x03);
  __delay_ms(5);
  LCD_CMD(0x03);
  __delay_ms(5);
  LCD_CMD(LCD_RETURN_HOME);
  __delay_ms(5);
  LCD_CMD(0x20 | (LCD_TYPE << 2));
  __delay_ms(50);
  LCD_CMD(LCD_TURN_ON);
  __delay_ms(50);
  LCD_CMD(LCD_CLEAR);
  __delay_ms(50);
  LCD_CMD(LCD_ENTRY_MODE_SET | LCD_RETURN_HOME);
  __delay_ms(50);
}

void IO_Expander_Write(unsigned char Data) 
{
  I2C_Write1ByteRegister_m( 39,  Data,  Data);
 // I2C_Write1ByteRegister_m( 56,  Data,  Data);
}

void LCD_Write_4Bit(unsigned char Nibble) 
{
  // Get The RS Value To LSB OF Data  
  Nibble |= RS;
  IO_Expander_Write(Nibble | 0x04);
  IO_Expander_Write(Nibble & 0xFB);
  __delay_us(50);
}

void LCD_CMD(unsigned char CMD) 
{
  RS = 0; // Command Register Select
  LCD_Write_4Bit(CMD & 0xF0);
  LCD_Write_4Bit((CMD << 4) & 0xF0);
}

void LCD_Write_Char(char Data)
{
  RS = 1;  // Data Register Select
  LCD_Write_4Bit(Data & 0xF0);
  LCD_Write_4Bit((Data << 4) & 0xF0);
}

void LCD_Write_String(char* Str)
{
    for(int i=0; Str[i]!='\0'; i++)
       LCD_Write_Char(Str[i]); 
}

void LCD_Set_Cursor(unsigned char ROW, unsigned char COL) 
{    
  switch(ROW) 
  {
    case 2:
      LCD_CMD(0xC0 + COL-1);
      break;
    case 3:
      LCD_CMD(0x94 + COL-1);
      break;
    case 4:
      LCD_CMD(0xD4 + COL-1);
      break;
    // Case 1  
    default:
      LCD_CMD(0x80 + COL-1);
  }
}

void Backlight() 
{
  BackLight_State = LCD_BACKLIGHT;
  IO_Expander_Write(LCD_BACKLIGHT);
}

void noBacklight() 
{
  BackLight_State = LCD_NOBACKLIGHT;
  IO_Expander_Write(0);
}

void LCD_SL()
{
  LCD_CMD(0x18);
  __delay_us(40);
}

void LCD_SR()
{
  LCD_CMD(0x1C);
  __delay_us(40);
}

void LCD_Clear()
{
  LCD_CMD(0x01); 
  __delay_us(40);
}

double average_adc(char chanel) {
    double sum = 0;
    for (uint8_t i = 0; i < 250; i++) {
        ADC_StartConversion();        
   while(!ADC_IsConversionDone());      
        sum += ADC_GetConversion(chanel); // Your ADC read function
      //  __delay_us(50);    // Optional: wait for signal to settle
    }
    return (double)(sum / 250);
}

/*
 * Small replacement for sprintf(buf, "%.Nf", value).
 * Writes 'value' with 'decimals' digits after the point into 'buf'
 * ('size' = buffer size including the terminating '\0').
 * If the number does not fit in the buffer, writes "OVF" instead.
 * Avoids linking the large printf / %f library code.
 */
void float_to_str(char *buf, uint8_t size, long double value, uint8_t decimals)
{
    char     tmp[12];        // digits, least significant first
    uint8_t  n = 0;          // number of digits in tmp
    uint8_t  pos = 0;        // write position in buf
    uint8_t  negative = 0;
    uint32_t scaled;
    long double scale = 1.0;

    for (uint8_t d = 0; d < decimals; d++)
        scale *= 10.0;

    if (value < 0) {
        negative = 1;
        value = -value;
    }

    value = value * scale + 0.5;            // scale and round to nearest

    if (value < 4.0e9) {                    // fits in uint32_t
        scaled = (uint32_t)value;

        // extract digits; keep at least decimals+1 so we get "0.xxx"
        do {
            tmp[n++] = (char)('0' + (scaled % 10));
            scaled /= 10;
        } while (scaled != 0 || n <= decimals);

        // sign + digits + decimal point must fit, leaving room for '\0'
        if ((uint8_t)(negative + n + (decimals ? 1 : 0)) < size) {
            if (negative)
                buf[pos++] = '-';
            while (n > 0) {
                if (n == decimals)
                    buf[pos++] = '.';
                buf[pos++] = tmp[--n];
            }
            buf[pos] = '\0';
            return;
        }
    }

    // does not fit
    buf[0] = 'O';
    buf[1] = 'V';
    buf[2] = 'F';
    buf[3] = '\0';
}


/////////////////////////////////////





void main(void)
{
    // initialize the device
    SYSTEM_Initialize();

    // When using interrupts, you need to set the Global and Peripheral Interrupt Enable bits
    // Use the following macros to:

    // Enable the Global Interrupts
    //INTERRUPT_GlobalInterruptEnable();

    // Enable the Peripheral Interrupts
    //INTERRUPT_PeripheralInterruptEnable();

    // Disable the Global Interrupts
    //INTERRUPT_GlobalInterruptDisable();

    // Disable the Peripheral Interrupts
    //INTERRUPT_PeripheralInterruptDisable();
    I2C_Initialize();  
    ADC_Initialize();
    FVR_Initialize();
    
    

    DAC1_Initialize();
    uint16_t count= 500;
    DAC1_Load10bitInputData(count); 
       
            
    
            
            
            
            
            
            
            
    uint8_t Address = 39;
    
    uint8_t data_1 = 6;
    uint8_t data_2 = 5;
    
    
long    double offset  = 0;
long    double K  = 0.001000977;
//long    double K  = 0.00400391;
   //  double K  = 0.004150537;
    //    double K  = 0.004151515;
       // double K  = 0.004201515;
       //   double K  = 0.006001515;  
long    double myFloat = 0;     //current 
    char str[10];

long    double myFloa_2 = 0;   /// voltage 
    char str_2[10];
    
long     double myFloa_3 = 0;   /// Power
    char str_3[10];
    
long     double myFloa_4 = 0;   /// resistance
    char str_4[10];
    
long    double  convertedValue_voltage =0 ;
long    double  convertedValue_current  =0;
    
    // Cast to int to remove decimal part
    float_to_str(str, sizeof(str), myFloat, 3);
    float_to_str(str_2, sizeof(str_2), myFloa_2, 3);
     
    LCD_Init(0x4E);    // Initialize LCD module with I2C address = 0x4E
    
    LCD_Set_Cursor(1, 19);
    LCD_Write_String("A");
    LCD_Set_Cursor(1, 1);
    LCD_Write_String(str);
    
    
    LCD_Set_Cursor(2, 19);
    LCD_Write_String("V");
    LCD_Set_Cursor(2, 1);
    LCD_Write_String(str_2);
    
    
    LCD_Set_Cursor(3, 19);
    LCD_Write_String("W");
    LCD_Set_Cursor(3, 1);
    LCD_Write_String(str_3);
    
    
    LCD_Set_Cursor(4, 17);
    LCD_Write_String("naif");
    LCD_Set_Cursor(4, 1);
    LCD_Write_String(str_4);
    
   OPTION_REGbits.nWPUEN = 0 ; 
   TRISD  = 0b11110000;
   ANSELD = 0 ;
   WPUD   = 0b11110000;
  
   LATDbits.LATD0 = 1 ;
   LATDbits.LATD1 = 1 ;
   LATDbits.LATD2 = 1 ;
   LATDbits.LATD3 = 1 ;
   
   
   
   
   
   
   
    while (1)
    {
     LATDbits.LATD0 = 0 ;
     LATDbits.LATD1 = 1 ;
     if(PORTDbits.RD7 == 0){
       __delay_ms(30);  
      count = count + 1;
         
     }   
     LATDbits.LATD0 = 1 ;
     LATDbits.LATD1 = 0 ;
    if(PORTDbits.RD7 == 0){
       __delay_ms(30);  
       count = count - 1; 
         
     }   
        
    DAC1_Load10bitInputData(count); 
    
    
   
    
    
        
    
    
    
    
    
    
//     ADC_StartConversion();
//
//     while(!ADC_IsConversionDone());   
//     convertedValue_current = ADC_GetConversion(5);
  
   // convertedValue_current =  ( average_adc(5) *10);
  
    convertedValue_current =   average_adc(5);
     offset = (convertedValue_current * K)* 0.05 ;
     myFloat = ((convertedValue_current * K) + offset );
   
   myFloat = ((convertedValue_current * K)  );
   
    float_to_str(str, sizeof(str), myFloat, 5);
    LCD_Set_Cursor(1, 1);
    LCD_Write_String(str);
    
    
    
   // __delay_ms(30);
    
//    
//    ADC_StartConversion();
//
//   while(!ADC_IsConversionDone());
//
//    convertedValue_voltage = ADC_GetConversion(6);
     convertedValue_voltage =   average_adc(6);
     offset = (convertedValue_voltage * K)* 0.35 ;
//      myFloa_2  = (convertedValue_voltage * K ) * 50.44);
      myFloa_2  = ((convertedValue_voltage * K ) + offset ) * 50; 
      
      
     // myFloa_2 = (myFloa_2 + 0.028) * 50 ;
      float_to_str(str_2, sizeof(str_2), myFloa_2, 3);
     
     LCD_Set_Cursor(2, 1);
      LCD_Write_String(str_2);
     
     
     myFloa_3  = myFloa_2 * myFloat ;
      float_to_str(str_3, sizeof(str_3), myFloa_3, 3);
      LCD_Set_Cursor(3, 1);
      LCD_Write_String(str_3);
  
     
     if (myFloat > 0 ){
      myFloa_4  = (myFloa_2 / myFloat) ;
      float_to_str(str_4, sizeof(str_4), myFloa_4, 3);
      LCD_Set_Cursor(4, 1);
      LCD_Write_String(str_4);
       
     } else{
       LCD_Set_Cursor(4, 1);
       
      LCD_Write_String("NAN     "); 
      
     
      }
   //  __delay_ms(30);
   
    }
}
/**
 End of File
*/