/**
 *  arToFDistanceMeasuringUart.ino
 * 
 * {Wiring:}
 * 
 *     GY-53           Nano
 *    ---------------------- 
 *     VCC     ----    VCC
 *     GND     ----    GND
 *     RX      ----    TX
 *     TX      ----    RX
 *   
 *  USB to TTL           Nano
 * --------------------------- 
 *     VCC     ----     X
 *     GND     ----    GND
 *     RX      ----    D5
 *     TX      ----    D6
 *          
 *
 *  {Compile:}
 *      > Arduino IDE 1.8.2
 * 
 * {Result} Work !!!
 *  
 */
#include <SoftwareSerial.h>

//#define DBGSERIAL
#ifdef DBGSERIAL
    SoftwareSerial serialDbg(5, 6); // RX, TX
#else

#define DFPLAYERMINI
#ifdef DFPLAYERMINI
  #include "DFRobotDFPlayerMini.h"
  SoftwareSerial serialDFPlayer(10, 11); // RX, TX
  DFRobotDFPlayerMini myDFPlayer;
#endif

#endif

#define GOGOSTOPSTOP_MAX_SEC 7
#define GOGOSTOPSTOP_MAX_CNT 5
#define STEP_CHG_THRESHOLD   70
#define JOKE_MAX_NUM         1

bool bSign = false;
unsigned short shCounter = 0;
unsigned char szRcv[ 8 ] = { 0 };

typedef struct
{
    uint16_t nDistance;
    uint8_t  nMode;
} tagTOFVL53L0X;

void setup() {
    randomSeed( analogRead(A0) );
    Serial.begin( 9600 ); // VL53L0X整合模組序列埠
    
#ifdef DFPLAYERMINI
    serialDFPlayer.begin( 9600 );
#endif

#ifdef DBGSERIAL
    serialDbg.begin( 115200 );
    serialDbg.println( "Start VL53L0X UART Test" );
#endif

    /*
        測量模式：
        (1) 長距離測量模式 (0xA5, 0x50, 0xF5)
            > 0 ~ 2.0M, T ~ 35ms, ± 4cm
        (2) 快速測量模式   (0xA5, 0x51, 0xF6)
            > 0 ~ 1.2M, T ~ 22ms, ± 3cm
        (3) 高精度測量模式 (0xA5, 0x52, 0xF7)
            > 0 ~ 1.2M, T ~ 200ms, ± 1cm
        (4) 一般測量模式   (0xA5, 0x53, 0xF8)
            > 0 ~ 1.2M, T ~ 35ms, ± 2cm
     */
    // 高精度測量模式 (0xA5, 0x52, 0xF7)    
    Serial.write( 0xA5 ); 
    Serial.write( 0x52 );
    Serial.write( 0xF7 );

#ifdef DFPLAYERMINI
    //
    //serialDbg.println();
    //serialDbg.println(F("DFRobot DFPlayer Mini Demo"));
    //serialDbg.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  
    if( !myDFPlayer.begin( serialDFPlayer ) )
    {   //Use softwareSerial to communicate with mp3.
        //serialDbg.println(F("Unable to begin:"));
        //serialDbg.println(F("1.Please recheck the connection!"));
        //serialDbg.println(F("2.Please insert the SD card!"));
        while(true);
    }
    //serialDbg.println(F("DFPlayer Mini online."));
    
    myDFPlayer.setTimeOut(500); //Set serial communictaion time out 500ms
    myDFPlayer.volume(15);  //Set volume value (0~30).
    myDFPlayer.EQ(DFPLAYER_EQ_NORMAL);
    myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
#endif
}

bool     g_bOnAccelatorPedal = false;
uint8_t  n                   = 0;
uint8_t  nCrc                = 0;
uint16_t nLastDistance       = 0;
uint16_t nStepOnCnt          = 0;   // 持續踩踏油門經過秒數(大約)
uint16_t nChgStepCnt         = 0;   // 切換油門剎車的次數
static unsigned long nlStepOnTimer  = millis();
tagTOFVL53L0X tof;

void loop() {
    if( bSign )
    {
        for( n = 0 ; n < 7 ; ++n )
            nCrc += szRcv[ n ];

        if( nCrc == szRcv[ n ] )
        {
            tof.nDistance = ( szRcv[ 4 ] << 8 ) | szRcv[ 5 ];
            if( tof.nMode != szRcv[ 6 ] )
            {
                tof.nMode  = szRcv[ 6 ];
#if 0//DBGSERIAL
                serialDbg.print( F("Change mode to ") );
                serialDbg.println( tof.nMode );
#endif
            }
            
            /** software Debug serial */
#ifdef DBGSERIAL
            serialDbg.print( F("Distance: ") );
            serialDbg.println( tof.nDistance );
#endif
            //
            if( tof.nDistance >= 2500 )
                return;
            else if( nLastDistance > 0 )
            {
                if( ( tof.nDistance < nLastDistance ) && ( ( nLastDistance - tof.nDistance ) >= STEP_CHG_THRESHOLD ) )
                {   // 若腳放到了油門上，則播放警示音
#ifdef DFPLAYERMINI
                    myDFPlayer.pause();  // pause the mp3
                    myDFPlayer.playFolder( 1, 1 );
#endif
#ifdef DBGSERIAL
                    serialDbg.println( F("---------gas pedal!") );
#endif
                    ChangeStep( true );
                }
                else if( ( tof.nDistance > nLastDistance ) && ( ( tof.nDistance - nLastDistance ) >= STEP_CHG_THRESHOLD ) )
                {   // 若腳從油門踏板離開(放到剎車上)
#ifdef DBGSERIAL
                    serialDbg.println( F("---------breaking pedal!") );
#endif
                    ChangeStep( false );
                }
                else
                {   // 腳位置狀態不變
                    if( ( millis() - nlStepOnTimer ) > 1000 )
                    {
                        ++nStepOnCnt;
                        nlStepOnTimer = millis();
                    }
#ifdef DBGSERIAL
                    serialDbg.print( "+ nlStepOnTimer:" );
                    serialDbg.println( nlStepOnTimer );
                    serialDbg.print( "+ nStepOnCnt:" );
                    serialDbg.println( nStepOnCnt );
                    serialDbg.print( "OnAccelatorPedal: " );
                    serialDbg.println( g_bOnAccelatorPedal );
#endif
                    
                    if( g_bOnAccelatorPedal )
                    {   // 若腳持續放在油門踏板上
#ifdef DFPLAYERMINI
                        switch( nStepOnCnt )
                        {
                        case 10:    // 油門踩久了記得休息一下
                            {
                                myDFPlayer.pause();  // pause the mp3
                                myDFPlayer.playFolder( 2, 1 );
#ifdef DBGSERIAL
                                serialDbg.println( F("====> pedal 10 sec!") );
#endif
                            }
                            break;
                        case 30:    // 真是個腳踏實地的人
                            {
                                myDFPlayer.pause();  // pause the mp3
                                myDFPlayer.playFolder( 2, 2 );
#ifdef DBGSERIAL
                                serialDbg.println( F("====> pedal 30 sec!") );
#endif
                            }
                            break;
                        case 60:    // 踩油門這麼久了不如我講個笑話吧
                            {
                                int i = random( JOKE_MAX_NUM ) + 1;
                                myDFPlayer.pause();  // pause the mp3
                                myDFPlayer.playFolder( 3, i );
#ifdef DBGSERIAL
                                serialDbg.println( F("====> pedal 60 sec!") );
#endif
                            }
                            break;
                        case 90:    // 油門踩了這麼久，根本就是風見隼人
                            {
                                myDFPlayer.pause();  // pause the mp3
                                myDFPlayer.playFolder( 2, 4 );
#ifdef DBGSERIAL
                                serialDbg.println( F("====> pedal 90 sec!") );
#endif
                            }
                            break;
                        case 120:   // 你也太持久了吧
                            {
                                myDFPlayer.pause();  // pause the mp3
                                myDFPlayer.playFolder( 2, 5 );
                                nStepOnCnt = 0;
#ifdef DBGSERIAL
                                serialDbg.println( F("====> pedal 120 sec!") );
#endif
                            }
                            break;
                        }
#endif
                    }
                }

                if( nChgStepCnt >= GOGOSTOPSTOP_MAX_CNT )
                {   // 走走停停煩死了
                    nChgStepCnt = 0;
#ifdef DFPLAYERMINI
                    myDFPlayer.pause();  // pause the mp3
                    myDFPlayer.playFolder( 1, 2 );
#endif
#ifdef DBGSERIAL
                    serialDbg.println( F("###### go go stop stop!") );
#endif
                }
            }

            //
            nLastDistance = tof.nDistance;
            bSign = false;
        }
    } 
}

void serialEvent() {
    // 檢測VL53L0X整合模組訊號
    while( Serial.available() )
    {
        szRcv[ shCounter ] = (unsigned char)Serial.read();
        if( shCounter == 0 && szRcv[ 0 ] != 0x5A ) return;         
        shCounter++;
        if( shCounter == 8 )
        {
            shCounter = 0; 
            bSign = true;
        }
    }
}

void ChangeStep( bool bOnAccelatorPedal )
{
    if( nStepOnCnt < GOGOSTOPSTOP_MAX_SEC )
    {   // 持續踩油門不到GOGOSTOPSTOP_MAX_SEC秒
        ++nChgStepCnt;
    }
    else
    {
        nChgStepCnt = 0;
    }

    g_bOnAccelatorPedal = bOnAccelatorPedal;
    nStepOnCnt          = 0;
}

