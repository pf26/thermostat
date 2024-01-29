/*
* Project Name: OLED_I2C
* File: OLED.c
* Description: OLED 128X32 SSD1306  I2C library c file
* Author: Gavin Lyons.
* Complier: xc8 v2.00 compiler
* PIC: PIC16F1619 
* IDE:  MPLAB X v5.00
* Development board: Microchip Curiosity Board, PIC16F1619
* Created March 2019
* Description: See URL for full details.
* URL: https://github.com/gavinlyonsrepo/pic_16F1619_projects
*/
//#include "mcc_generated_files/mcc.h"

#include "OLED.h"
#include "font.h"
#include "font1632.h"
#include <i2c.h>

char oledX, oledY;

void I2C_Wait()
{
  while ((SSPSTAT & 0x04) || (SSPCON2 & 0x1F));
}

void I2C_Master_Init()
{
 // TRISC3 = 1;
  WPUB |= 0x50;	// Pullup B4 6
  TRISBbits.TRISB6 = 1;
  TRISBbits.TRISB4 = 1;
  SSPCON1 = 0x28;
  SSPCON2 = 0x00;
  SSPSTAT = 0x00;
  SSPADD = 12 - 1;	// ~100kHz
}

void I2C_Start()
{
  //---[ Initiate I2C Start Condition Sequence ]---
  I2C_Wait();
  SSPCON2bits.SEN = 1;
}

void I2C_Stop()
{
  //---[ Initiate I2C Stop Condition Sequence ]---
  I2C_Wait();
  SSPCON2bits.PEN = 1;
}

void I2C_Restart()
{
  //---[ Initiate I2C Restart Condition Sequence ]---
  I2C_Wait();
  SSPCON2bits.RSEN = 1;
}

void I2C_ACK(void)
{
  //---[ Send ACK - For Master Receiver Mode ]---
  I2C_Wait();
  SSPCON2bits.ACKDT = 0; // 0 -> ACK, 1 -> NACK
  SSPCON2bits.ACKEN = 1; // Send ACK Signal!
}

void I2C_NACK(void)
{
  //---[ Send NACK - For Master Receiver Mode ]---
  I2C_Wait();
  SSPCON2bits.ACKDT = 1; // 1 -> NACK, 0 -> ACK
  SSPCON2bits.ACKEN = 1; // Send NACK Signal!
}

unsigned char I2C_Write(unsigned char Data)
{
  //---[ Send Byte, Return The ACK/NACK ]---
  I2C_Wait();
  SSPBUF = Data;
  I2C_Wait();
  return SSPCON2bits.ACKSTAT;
}

unsigned char I2C_Read_Byte(void)
{
  //---[ Receive & Return A Byte ]---
  SSPCON2bits.RCEN = 1;        // Enable & Start Reception
  while(!PIR1bits.SSPIF);   // Wait Until Completion
  PIR1bits.SSPIF = 0;       // Clear The Interrupt Flag Bit
  return SSPBUF;   // Return The Received Byte
}


//Function to Send command byte to OLED, passed byte
void Oled_Command( uint8_t command )
{
  I2C_Start();
  I2C_Write(SSD1306_ADDR);
  I2C_Write(SSD1306_COMMAND); // Slave I2C Device Address + Write
  I2C_Write(command); // Data To Be Sent
  I2C_Stop();
}

void Oled_Command3( uint8_t c1, uint8_t c2, uint8_t c3 )
{
  I2C_Start();
  I2C_Write(SSD1306_ADDR);
  I2C_Write(SSD1306_COMMAND); // Slave I2C Device Address + Write
  I2C_Write(c1); // Data To Be Sent
  I2C_Write(c2); // Data To Be Sent
  I2C_Write(c3); // Data To Be Sent
  I2C_Stop();
}

//Function to Send data byte to OLED, passed byte
void Oled_Data( uint8_t value )
{
  I2C_Start();
  I2C_Write(SSD1306_ADDR);
  I2C_Write(SSD1306_DATA_CONTINUE); // Slave I2C Device Address + Write
  I2C_Write(value); // Data To Be Sent
  I2C_Stop();
}

//Function to init OLED
void Oled_Init(void)
{
  I2C_Master_Init();
  I2C_Stop();
  Oled_Command( SSD1306_DISPLAY_OFF);
  Oled_Command( SSD1306_SET_DISPLAY_CLOCK_DIV_RATIO);
  Oled_Command( 0x80);
  Oled_Command( SSD1306_SET_MULTIPLEX_RATIO );
  Oled_Command( SSD1306_LCDHEIGHT - 1 );
  Oled_Command( SSD1306_SET_DISPLAY_OFFSET );
  Oled_Command(0x00);
  Oled_Command( SSD1306_SET_START_LINE|0x00);  // Line: 0
  Oled_Command( SSD1306_CHARGE_PUMP );
  Oled_Command(0x14);
  Oled_Command( SSD1306_MEMORY_ADDR_MODE );
  Oled_Command(0x00);  //Hor Addressing Mode is Used (02 is Ver)
  Oled_Command( SSD1306_SET_SEGMENT_REMAP| 0x01);
  Oled_Command( SSD1306_COM_SCAN_DIR_DEC );
  Oled_Command( SSD1306_SET_COM_PINS );
  Oled_Command( 0x02 );
  Oled_Command( SSD1306_SET_CONTRAST_CONTROL );
  Oled_Command(0x8F);
  Oled_Command( SSD1306_SET_PRECHARGE_PERIOD );
  Oled_Command( 0xF1 );
  Oled_Command( SSD1306_SET_VCOM_DESELECT );
  Oled_Command( 0x40 );
  Oled_Command( SSD1306_DISPLAY_ALL_ON_RESUME );
  Oled_Command( SSD1306_NORMAL_DISPLAY );
  Oled_Command( SSD1306_DEACTIVATE_SCROLL );
  Oled_Command( SSD1306_DISPLAY_ON );
Oled_Clear();
}

//function to write a string to OLED, passed string
void Oled_WriteString(char rom *characters)
{
  while (*characters) Oled_WriteCharacter(*characters++);
}

//function to write a character, passed character
void Oled_WriteCharacter(char character)
{
char i;
  I2C_Start();
  I2C_Write(SSD1306_ADDR);
  I2C_Write(SSD1306_DATA_CONTINUE); // Slave I2C Device Address + Write
  for (i=0; i<5; i++) {
		  I2C_Write(ASCII[character - 0x20][i]); // Data To Be Sent
		}
  I2C_Write(0x00);
  I2C_Stop();
}

//function to write a character, passed character
void Oled_WriteLargeDigit(char character)
{
char i;
  I2C_Start();
  I2C_Write(SSD1306_ADDR);
  I2C_Write(SSD1306_DATA_CONTINUE); // Slave I2C Device Address + Write
  for (i=0; i<5; i++) {
	//	Oled_Data(ASCII[character - 0x20][i] );
	  I2C_Write(ssd1306xled_font16x32_digits[i]); // Data To Be Sent
	}
  I2C_Write(0x00);
  I2C_Stop();
}

void Oled_SetContrast( uint8_t contrast )
{
  Oled_Command(SSD1306_SET_CONTRAST_CONTROL );
  Oled_Command(contrast);
}

//function to clear OLED by writing to it.
void Oled_Clear(void)
{
  int i =0;
    // OLED 128* 32 pixels = total pixels -> (4096 / 1 byte) = 512 passes.
    // SSD1306_CLEAR_SIZE  = 512 for 128* 32 or 1024  for 128*64
  I2C_Start();
  I2C_Write(SSD1306_ADDR);
  I2C_Write(SSD1306_DATA_CONTINUE); // Slave I2C Device Address + Write
    for (i; i<SSD1306_CLEAR_SIZE ; i++) 
    {
  	I2C_Write(0x00);
    //  Oled_Data(0x00); // clear oled screen  
    }
  I2C_Write(0x00);
  I2C_Stop();

oledY = 0;
oledX = 0;
}

//function to clear OLED  line passed page number
void Oled_ClearLine(uint8_t page_num)
{
char i =0;
    Oled_SelectPage(page_num);
  // Clear line of 128 pixels of current page
    for (i; i<128 ; i++) 
    {
      Oled_Data(0x00); // clear oled screen  
    }
}

// Function to select [page] i.e. row or line number
// 128*32 has page 0-3
// 128* 64 has page 0-7
// Passed page num / byte
// page 0   8 
// page 1   16
// page 7   64
void Oled_SelectPage(uint8_t page_num)
{
 uint8_t Result =0xB0 | page_num; // Mask or with zero lets everything thru
 Oled_Command(Result); 
 Oled_Command(SSD1306_SET_LOWER_COLUMN);
 Oled_Command(SSD1306_SET_HIGHER_COLUMN); 
}

void Oled_setCursor(uint8_t x, uint8_t y) {
 	Oled_Command3(0xb0 | (y & 0x07), (((x & 0xf0) >> 4) | 0x10), (x & 0x0f));
	oledX = x;
	oledY = y;
}

void Oled_write32(uint8_t c)	{
	// If "font16x32.h" contains only 10 symbols (0..9) rotated 90deg. right, then:
unsigned int ci,c2;
uint8_t i;
char j;
  	c2 = c - 44;  	// Only symbols ,-./0...9:  are available 
	c2<<=6;		// x64
    oledY = 0;	// Can only write from the top position..
    Oled_setCursor(oledX, oledY);   //  then set oledY=0.
	j=3;
	while (j >=0)
	{ 		// Send from top to bottom 2*16 bytes
  		I2C_Start();
  		I2C_Write(SSD1306_ADDR);
  		I2C_Write(SSD1306_DATA_CONTINUE);
		ci =  16*(3-j) + c2;
		//	ssd1306_send_start(SSD1306_DATA);
		for (i = 0; i < 16; i++) { 	// Send from top to bottom 16 vertical bytes
			I2C_Write(ssd1306xled_font16x32_digits[ci]);
			ci++; 
		}
  		I2C_Write(0x00);
  		I2C_Stop();
		Oled_setCursor(oledX, 4-j);   		// Prepare position of next row of 16 bytes
		j--;
	}
  }