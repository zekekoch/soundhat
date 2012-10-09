/*
This Example acquire analog signal from A0 of Arduino, and Serial out to Processing application to visualize.
Tested with preamplified audio data. Take a look at http://www.youtube.com/watch?v=drYWullBWcI

Analog signal is captured at 9.6 KHz, 64 spectrum bands each 150Hz which can be change from adcInit()
Load the this file to Arduio, run Processing application.

Original Fixed point FFT library is from ELM Chan, http://elm-chan.org/works/akilcd/report_e.html
Ported to the library and demo codes are from AMurchick http://arduino.cc/forum/index.php/topic,37751.0.html
Processing code is from boolscott http://boolscott.wordpress.com/2010/02/04/arduino-processing-analogue-bar-graph-2/
*/


#include <stdint.h>
#include <ffft.h>
#include <FastSPI_LED.h>

#define  IR_AUDIO  0 // ADC channel to capture
#define NUM_LEDS 90

// Sometimes chipsets wire in a backwards sort of way
struct CRGB { unsigned char b; unsigned char r; unsigned char g; };
// struct CRGB { unsigned char r; unsigned char g; unsigned char b; };
struct CRGB *leds;

#define PIN 13

volatile  byte  position = 0;
volatile  long  zero = 0;

int16_t capture[FFT_N];			/* Wave captureing buffer */
complex_t bfly_buff[FFT_N];		/* FFT buffer */
uint16_t spektrum[FFT_N/2];		/* Spectrum output buffer */

void setup()
{
  Serial.begin(9600);
  FastSPI_LED.setLeds(NUM_LEDS);
  FastSPI_LED.setChipset(CFastSPI_LED::SPI_LPD8806);  
  FastSPI_LED.setPin(PIN);
  
  FastSPI_LED.init();
  FastSPI_LED.start();

  leds = (struct CRGB*)FastSPI_LED.getRGBData(); 
  setAllColor(0,0,0);
  FastSPI_LED.show();

  adcInit();
  adcCalb();
}

void loop()
{
  // comment out to start running again
  return;
  if (position == FFT_N)
  {
    fft_input(capture, bfly_buff);
    fft_execute(bfly_buff);
    fft_output(bfly_buff, spektrum);

    int s;
    //fadeOld();

    for (byte i = 0; i < 64; i++){
      s = constrain(spektrum[i],25,255);
      if (s > 50)
      {
        if (i < 2)
          setAllColor(s,0,0);
        else if (i <32)
          setAllColor(0,s,0);
        else
          setAllColor(0,0,s);
      }
      //Serial.println(spektrum[i], DEC);
    }
    FastSPI_LED.show();

   position = 0;
  }
}

void setAllColor(byte r, byte g, byte b)
{
  for(int i = 0; i < NUM_LEDS; i++ ) 
  {
    leds[i].r = r;
    leds[i].g = g;
    leds[i].b = b;
  }      
}

void fadeOld()
{
  byte fadeDur = 16;
  for(int i = 0; i < NUM_LEDS; i++ ) 
  {
    if (leds[i].r > fadeDur)
      leds[i].r -= fadeDur;
    else
      leds[i].r = 0;
      
    if (leds[i].g > fadeDur)
      leds[i].g -= fadeDur;
    else
      leds[i].g = 0;

    if (leds[i].b > fadeDur)
      leds[i].b -= fadeDur;
    else
      leds[i].b = 0;
  }  
}


// free running ADC fills capture buffer
ISR(ADC_vect)
{
  if (position >= FFT_N)
    return;
  
  capture[position] = ADC + zero;
  if (capture[position] == -1 || capture[position] == 1)
    capture[position] = 0;

  position++;
}
void adcInit(){
  /*  REFS0 : VCC use as a ref, IR_AUDIO : channel selection, ADEN : ADC Enable, ADSC : ADC Start, ADATE : ADC Auto Trigger Enable, ADIE : ADC Interrupt Enable,  ADPS : ADC Prescaler  */
  // free running ADC mode, f = ( 16MHz / prescaler ) / 13 cycles per conversion 
  ADMUX = _BV(REFS0) | IR_AUDIO; // | _BV(ADLAR); 
//  ADCSRA = _BV(ADSC) | _BV(ADEN) | _BV(ADATE) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) //prescaler 64 : 19231 Hz - 300Hz per 64 divisions
  ADCSRA = _BV(ADSC) | _BV(ADEN) | _BV(ADATE) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0); // prescaler 128 : 9615 Hz - 150 Hz per 64 divisions, better for most music
  sei();
}
void adcCalb(){
  Serial.println("Start to calc zero");
  long midl = 0;
  // get 2 meashurment at 2 sec
  // on ADC input must be NO SIGNAL!!!
  for (byte i = 0; i < 2; i++)
  {
    position = 0;
    delay(100);
    midl += capture[0];
    delay(900);
  }
  zero = -midl/2;
  Serial.println("Done.");
}
