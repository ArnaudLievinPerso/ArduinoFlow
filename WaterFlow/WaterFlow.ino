/*
Liquid flow rate sensor

Modified from -DIYhacking.com Arvind Sanjeev
with https://www.arduino.cc/reference/cs/language/functions/external-interrupts/attachinterrupt/ for XIAO MKR.

Modified 2025-05-10 to go to Seeed Studio XIAO nRF52840 Sense


 */


#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <Fonts/Picopixel.h>
// Added this row from the Seeed studio web site https://wiki.seeedstudio.com/XIAO_BLE/
#include <Adafruit_TinyUSB.h>


#ifndef PSTR
 #define PSTR // Make Arduino Due happy
#endif

#define DisplayPIN A0
#define SensorPIN 4

byte statusLed    = 13;

// Display
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(10,6, 1, 1, DisplayPIN,
  NEO_MATRIX_BOTTOM + NEO_MATRIX_LEFT + NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,
  NEO_GRB + NEO_KHZ800);
const uint16_t colors[] = {  matrix.Color(0, 0, 255), matrix.Color(255, 127, 64), matrix.Color(255, 0, 0) };
const int flowLimits[] = {20, 40};

// Waterflow sensor
const float calibrationFactor = 4.5;
volatile byte pulseCount;  
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;

unsigned long oldTime;

void interrupt();

void setup() {
  // Waterflow sensor
   
  // Initialize a serial connection for reporting values to the host
  Serial.begin(9600);
  
  // Set up the status LED line as an output
  pinMode(statusLed, OUTPUT);
  digitalWrite(statusLed, HIGH);  // We have an active-low LED attached
  
  pinMode(SensorPIN, INPUT);
  digitalWrite(SensorPIN, HIGH);

  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  oldTime           = 0;

  // The Hall-effect sensor is connected to pin 2 which uses interrupt 0.
  // Configured to trigger on a FALLING state change (transition from HIGH
  // state to LOW state)
  attachInterrupt(digitalPinToInterrupt(SensorPIN), pulseCounter, FALLING);

  // Display
  matrix.begin();
  matrix.setBrightness(5);
  matrix.setFont(&Picopixel);
}

float f = 0.; // flow in liters
int s = 0;    // how much to move to right
int d = 0;    // decimal part
int c = 0;    // color index

void loop() {
  
  // Waterflow sensor
  if((millis() - oldTime) > 1000)    // Only process counters once per second
  { 
    // Disable the interrupt while calculating flow rate and sending the value to
    // the host
    detachInterrupt(digitalPinToInterrupt(SensorPIN));
        
    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
    
    // Note the time this processing pass was executed. Note that because we've
    // disabled interrupts the millis() function won't actually be incrementing right
    // at this point, but it will still return the value it was set to just before
    // interrupts went away.
    oldTime = millis();
    
    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;
    
    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;
      
    unsigned int frac;
    
    // Print the flow rate for this second in litres / minute
    Serial.print("Flow rate: ");
    Serial.print(int(flowRate));  // Print the integer part of the variable
    Serial.print("L/min");
    Serial.print("\t"); 		  // Print tab space

    // Print the cumulative total of litres flowed since starting
    Serial.print("Output Liquid Quantity: ");        
    Serial.print(totalMilliLitres);
    Serial.println("mL"); 
    Serial.print("\t"); 		  // Print tab space
	  Serial.print(totalMilliLitres/1000);
	  Serial.print("L");
    

    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;
    
    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(digitalPinToInterrupt(SensorPIN), pulseCounter, FALLING);
  }  

  // Set the number to display
  f = totalMilliLitres/1000.;

  // Display
  matrix.clear();
  
  // display integer part as a number

  // Set cursor position pour emulate a right-aligned number
  if (f<10) {
    s=7;
  } else {
    if (f<100) {
      s=3;
    } else {
      s=0; 
      }
    };
  matrix.setCursor(s, 4);

  // Set the number
  if (f<flowLimits[0]) {
    c=0;
  } else {
    if (f<flowLimits[1]) {
      c=1;
    } else {
      c=2;
    }
  }
  matrix.setTextColor(colors[c]);
  matrix.print(int(f));

  // display decimal part as a bar
  d = int(f * 10) % 10;
 // if (d>0) {
    matrix.drawRect(0, 5, d+1, 5, colors[c]);
  //}
  
  matrix.show();
  //f += 0.1;
  delay(100);
}

/*
Insterrupt Service Routine
 */
void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}