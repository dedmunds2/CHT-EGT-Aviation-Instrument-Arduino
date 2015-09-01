/*Program to display EGT and CHT on an Adafruit QVGA TFT display
This program assumes the use of my own design K-type thermocouple shield.
This shield utilises the ADG528A multiplexer and the AD595 thermocouple amplifier.
The shield is designed to allow addressing through PD2, PD3 and either PD4 or PD5, jumper
selectable on the shield.  This program uses PD2, PD3 and PD4.  Note the value of 
DDRB and PortD.
David Edmunds 
edit 2nd September 2015, bug fix to stop overwriting PD5 on last address update in EGT loop
*/

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

// For the Adafruit shield, these are the default.
#define TFT_DC 9
#define TFT_CS 10

#define ANALOG_IN_PIN  A0

#define redLine 47  //value to draw the red warning line on the screen

#define maxBarLength 190  //maximum of displayed temperature bar


// all temps in degrees centigrade

#define maxEGTdisplay 750  //this is the maximum temperature that can be displayed
// maxCHT  is 190, this does not have to be rescaled as by coincidence it is the same as the bar length

int CHT[3][5];  //arrays to hold the CHT and EGT values, need 2 dimensions to compare consecutive values, so as to reduce flicker
int EGT[3][5];  //array index corresponds th the cylinder being measured.
int maxCHT;  //ints to hold the index of the maximum value in the array
int maxEGT;



Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);   // Use hardware SPI 

//_______________________________________________________________

void setup() {
 
   DDRD = DDRD|B00011100;     //set PD2,3,4 as output to address the multiplexer.  Assumes closed link closest to the header
  
   delay(1000);  //allow time for the screen to reset.
     
   tft.begin();
  
   tft.fillScreen(ILI9341_BLACK);
  
   tft.setTextSize(2);  //size for labels
   
   setLabels();  //write the labels to the screen

   tft.fillRect(5, redLine, 300, 3, ILI9341_RED);   //set the redline to mark maximum allowable values

  
   tft.setTextColor(ILI9341_WHITE);  //settings to diplay the temperatures
   tft.setTextSize(1);  //size for CHT and EGT values
}

//_______________________________________________________________

void loop(void) {
   
   PORTD = PORTD & B11100011;  //set and reset the address of CHT1
   
   //get and display CHT values
   maxCHT = 1;

   for(int i = 1; i<=4; i++)
   {
      CHT[1][i] = CHT[2][i];   //store last CHT value
      CHT[2][i] = calcTemp(gettemp());   //get the A/D value and convert to degrees C
      if (CHT[2][i] > (CHT[2][maxCHT]+1))   //find the highest CHT, and add a bit of hysterisis
         maxCHT = i;

       PORTD = PORTD + 4;  //get next address for the multiplexer
   }

// and display it, with the maximum CHT shown in red
     
   for(int i = 1; i<=4; i++)
   {
      if (i == maxCHT)
         tft.fillRect((15+(i-1)*35), (210-CHT[2][i]), 35, CHT[2][i], ILI9341_RED);
      else
         tft.fillRect((15+(i-1)*35), (210-CHT[2][i]), 35, CHT[2][i], ILI9341_BLUE);
         
         tft.fillRect((15+(i-1)*35), 50, 35, (160-CHT[2][i]), ILI9341_BLACK);  //overwrite rects when the CHT is falling
 

      if (CHT[2][i] != CHT[1][i])   //don't overwrite values unless they have changed, to avoid jitter
      {
        tft.fillRect((25+(i-1)*35), 24, 35, 15, ILI9341_BLACK);  //overwrite text
        tft.setCursor(25+(i-1)*37,28); 
        tft.print(CHT[2][i]);
      }
    }  
  //_____________________________________________________
  //repeat for EGT
 
    maxEGT = 1;

    for(int i = 1; i<=4; i++)
    {
       EGT[1][i] = EGT[2][i];
       EGT[2][i] = calcTemp(gettemp());   //get the A/D value and convert to degrees C
       if (EGT[2][i] > (EGT[2][maxEGT]+1))   //find the highest EGT and allow a little hystersis
          maxEGT = i;
       
       if (i < 4)   
         PORTD = PORTD + 4;  //set address for the multiplexer, and don't overwrite PD5 on last pass through this loop
     }
   // and display it, with the maximum EGT shown in red

     
   for(int i = 1;i<=4; i++)
   {
      if (i == maxEGT)
         tft.fillRect((165+(i-1)*35), (210-scaleEGT(EGT[2][i])), 35, scaleEGT(EGT[2][i]), ILI9341_RED);  //scale EGT value to fit the bar on the screen
      else
         tft.fillRect((165+(i-1)*35), (210-scaleEGT(EGT[2][i])), 35, scaleEGT(EGT[2][i]), ILI9341_BLUE);
  
       tft.fillRect((165+(i-1)*35), 50, 35, (160-scaleEGT(EGT[2][i])), ILI9341_BLACK);  //overwrite rects when the CHT is falling
 
      if (EGT[2][i] != EGT[1][i])  //don't overwrite values unless they have changed, to avoid jitter       
      {         
        tft.fillRect((170+(i-1)*35), 24, 35, 15, ILI9341_BLACK);  //overwrite text
        tft.setCursor(172+(i-1)*37,28); 
        tft.print(EGT[2][i]);
      }
    }
}

//_______________________________________________________________

void setLabels(void) //function to write the labels to the TFT screen
{ 

   tft.setTextColor(ILI9341_GREEN);
   tft.setRotation(1);  //print horizontally
   tft.setCursor(60,5);
   tft.println("CHT");
   tft.setCursor(200,5);
   tft.println("EGT");
  
   for(int n = 35; n < 141; n+=35)
   {
      tft.setCursor(n,220);  //write cylinder numbers at the bottom of the screen
      tft.println(n/35);
   }

   for(int n = 180; n < 321; n+=35)
   {
      tft.setCursor(n,220);
      tft.println((n-145)/35);
   }  
}

//_______________________________________________________________

int gettemp()  //read temperatures, the average is to remove jitter, address for each CHT and EGT set in loop
               //and remove the largest and smallest reading before calculating the average.
{
   long temp = 0;
   int temps[101];
   
   int biggest = 1;
   int smallest = 1;
 
   for (int j=1;j<=100;j++)
   { 
     temps[j] = analogRead(ANALOG_IN_PIN);
     if (temps[j] > temps[biggest])
       biggest = j;
       
     if (temps[j] < temps[smallest])
       smallest = j;
   }  
   for (int j=1;j<=100;j++)
   { 
    if(!((j == biggest) || (j ==smallest)))
      temp = temp +temps[j];
   }
   return (int) temp/98;  
}

//_______________________________________________________________

int calcTemp(int ADValue)  //scale A/D value to degrees C
{
  float x;
  x = ADValue*1.055;
  return (int) x;
}  

//_______________________________________________________________

int scaleEGT(int EGTvalue)  // calculates a value to display on the TFT screen, CHT value is the same as the max bar length by coincidence
{
 float x;
  x = (float) EGTvalue/maxEGTdisplay;
  x = x * maxBarLength;

  return (int) x;
}

