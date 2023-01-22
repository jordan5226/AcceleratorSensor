/******************************************************************************
* IICLCDLargeNumber.h
* 
* 使用 {5V} 整合型 LCD @ I2C 模式，創建顯示大型數字的程式碼。
*
*  {Result} Work !!!
*
*******************************************************************************/
#include <Arduino.h>
#include <stdio.h>
#include <Wire.h>

// 每一個數字由六個 5x8 的區塊構成
// 這些區塊定義如下：
const unsigned char CGRAM_block[]={
    0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
    0X1F,0X1F,0X1F,0X00,0X00,0X00,0X00,0X00,
    0X00,0X00,0X00,0X00,0X00,0X1F,0X1F,0X1F,
    0X1F,0X1F,0X1F,0X00,0X00,0X1F,0X1F,0X1F,
    0X1F,0X1F,0X1F,0X1F,0X1F,0X1F,0X1F,0X1F,
    0X00,0X00,0X00,0X00,0X00,0X0E,0X0E,0X0E,
};

const unsigned char Numerics[10][6]={
    {4,1,4,4,2,4},	// 0
    {1,4,0,2,4,2},
    {3,3,4,4,2,2},
    {1,3,4,2,2,4},
    {4,2,4,0,0,4},
    {4,3,3,2,2,4},
    {4,3,3,4,2,4},
    {1,1,4,0,4,0},
    {4,3,4,4,2,4},
    {4,3,4,0,0,4}, // 9
};

const int nix[] = { 1, 5, 9, 13 };
const int pix[] = { 2, 6, 10, 14 };

const unsigned char IIC_ADDR_LCD1 = 0x3C;

// 函數宣告
void initLCD();
void clearLCD();
void displayCharOnLCD( int line, int column, const char *dp, unsigned char len );
void writeCGRAM( const unsigned char dp[], unsigned char charlen);
void displayCGRAM( unsigned char cgramaddr, int row, int col);
void displayNumeric( const char dp[], const int len );

/****************************************************************************************
* 整合型 LCD @ I2C 模式
****************************************************************************************/
void initLCD()
{
	Wire.beginTransmission(IIC_ADDR_LCD1);
	Wire.write( 0x00 ); 
	Wire.write( 0x38 ); 
	Wire.write( 0x0C ); 
	Wire.write( 0x01 ); 
	Wire.write( 0x06 ); 
	Wire.endTransmission();
}

void clearLCD()
{

	Wire.beginTransmission(IIC_ADDR_LCD1);

	Wire.write( 0x80 ); 
	Wire.write( 0x01 ); 	

	Wire.endTransmission();
}

void displayCharOnLCD( int line, int column, const char *dp, unsigned char len )
{
	unsigned char i;

	Wire.beginTransmission(IIC_ADDR_LCD1);

	Wire.write( 0x80 );
	Wire.write( 0x80 + ( line - 1 ) * 0x40 + ( column - 1 ) );
	Wire.write( 0x40 );

	for( i = 0; i < len; i++)
	{
		Wire.write( *dp++ );
	}

	Wire.endTransmission();
}

// 
// charlen: 要輸入多少的自訂字元
//
// start: 數值介於 1 到 8，總共 8 個自訂字元, 
//
void writeCGRAM( const unsigned char dp[], unsigned char charlen, int start )
{
    Wire.beginTransmission(IIC_ADDR_LCD1);
    // To assign CGRAM start address
    Wire.write(0x80);
    Wire.write( 0x40 + ( ( start - 1 ) << 3 ));
    // 寫入 CGRAM
	Wire.write(0x40);
	// CGRAM_block 的數目；每一個字元，需要 8-byte 資料
	for( int i = 0; i < charlen * 8; i++ )
	{
		Wire.write(*dp);
		dp = dp + 1;
	}
    Wire.endTransmission();
}


void displayCGRAM( unsigned char cgramaddr, int row, int col)
{
	// set cursor position to col, row
    Wire.beginTransmission(IIC_ADDR_LCD1);
    Wire.write(0x80);
    Wire.write( 0x80 + 0x40 * ( row - 1 ) + ( col - 1 ) );
    delayMicroseconds(100000);
    Wire.write(0x40);
    Wire.write( cgramaddr );
    Wire.endTransmission();
}

/****************************************************************************************
* 大型數字顯示函式
****************************************************************************************/
void displayNumeric( const char dp[], const int len )
{
    int p =0, n = 0;
    for( int i = 0; i < len; i++ )
    {
    	//Serial.println(i); Serial.print("0x");
    	//Serial.print(dp[i], HEX);
        switch(dp[i])
        {
            case 0x2e:  // .
                if(p<n) p = n;
                // line 1
                displayCGRAM( 0x00, 1, pix[p] );
                // line 2
                displayCGRAM( 0x05, 2, pix[p] );
                p++;                
            break;
            case 0x3A:  // :
                if(p<n) p = n;
                // line 1
                displayCGRAM( 0x05, 1, pix[p] );
                // line 2
                displayCGRAM( 0x05, 2, pix[p] );
                p++;
            break;
            case 0x20:  // space
            	// line 1
                for( int f = 0; f < 3; f++ )
                {
                    displayCGRAM( 0x00 , 1, nix[n] + f );
                }
                //Serial.print(", nix[n]="); Serial.print(nix[n]); Serial.println("");
                // line 2
                for( int f = 3; f < 6; f++ )
                {
                    displayCGRAM( 0x00, 2, nix[n] + f - 3 );
                }
                n++;
            break;
            case 0x30:  // 0
            case 0x31:  // 1
            case 0x32:  // 2
            case 0x33:  // 3
            case 0x34:  // 4
            case 0x35:  // 5
            case 0x36:  // 6
            case 0x37:  // 7
            case 0x38:  // 8
            case 0x39:  // 9
                // line 1
                for( int f = 0; f < 3; f++ )
                {
                    displayCGRAM( Numerics[ dp[i] - 0x30 ][f] , 1, nix[n] + f );
                }
                //Serial.print("nix[n]="); Serial.print(nix[n]); Serial.println("");
                // line 2
                for( int f = 3; f < 6; f++ )
                {
                    displayCGRAM( Numerics[ dp[i] - 0x30 ][f], 2, nix[n] + f - 3 );
                }
                n++;
            break;
        }
        //Serial.print("p, n = "); Serial.print(p); Serial.print(", "); Serial.println(n);
    }
}
