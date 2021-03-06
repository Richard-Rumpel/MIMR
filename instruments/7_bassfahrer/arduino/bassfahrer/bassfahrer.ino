/*
 * MIMR - Bassfahrer
 * KAZOOSH! 2017
 */

#include <FastPin.h>
#include <SoftwareSerial.h>

// driver pins for meter and light
FastPin MeterPin1(  9, OUTPUT );
FastPin MeterPin2( 10, OUTPUT );
FastPin LightPin1( 11, OUTPUT );
FastPin LightPin2( 12, OUTPUT );

// PWM switching time (milliseconds)
#define PWM_SWITCHING_TIME 10*1000L // 10 ms
#define PWM_STEPS 20 // 20 steps * 10 ms = 200 ms cycle length

// serial connection to sensor controller
SoftwareSerial SensorSerial( 7, 8 );

// dummy byte indicating start of MIDI data
#define SYNC_BYTE 255

// Arduino -> RasPi send interval (microseconds)
#define SEND_INTERVAL 50*1000L // 50 ms = 20 S/s

// baud rate for USB communication with RasPi
#define BAUD_RATE 115200L


void setup()
{
  // start USB connection to RasPi
  Serial.begin( BAUD_RATE );

  // start serial connection to sensor controller
  SensorSerial.begin( 9600 );
}


void driveOutputs( int meter, int light )
{
  static unsigned long lastCycleTime = 0;
  static char pwmCounter = 0;

  // time to update?
  if ( micros() - lastCycleTime > PWM_SWITCHING_TIME )
  {
    // quantize duty cycles
    char digitalMeter = pwmCounter <= ( meter * PWM_STEPS / 256 ) ? HIGH : LOW;
    char digitalLight = pwmCounter <= ( light * PWM_STEPS / 256 ) ? HIGH : LOW;

    // drive meter pins
    MeterPin1.write( digitalMeter );
    MeterPin2.write( digitalMeter );

    // drive light pins
    LightPin1.write( digitalLight );
    LightPin2.write( digitalLight );

    // roll counter over
    pwmCounter = ( pwmCounter + 1 ) % PWM_STEPS;
  }
}


void loop()
{
  static unsigned char serialIn[1];
  static int sensorValue = 0;
  static unsigned long lastSendTime = 0;
  
  // check serial buffer for input
  int test = Serial.read();

  // start byte received?
  if( test == SYNC_BYTE )
  {
    // yes, read data bytes
    Serial.readBytes( serialIn, 1 );
  }

  // try to read latest data from sensor
  while ( SensorSerial.peek() >= 0 )
  {
    // update cached value (0..255)
    sensorValue = SensorSerial.read();
  }

  // visualize data: show sensor value with meter, MIDI data dims light
  driveOutputs( sensorValue, 256-2*serialIn[0] );                                                                              

  // time to update RasPi with current value?
  if ( micros() - lastSendTime > SEND_INTERVAL )
  {
    // remember time
    lastSendTime = micros();
    
    // write start character
    Serial.print( ":" );
    
    // write value as ASCII and append line break
    Serial.println( sensorValue );

    // wait for transmission to finish
    Serial.flush();
  }
}
