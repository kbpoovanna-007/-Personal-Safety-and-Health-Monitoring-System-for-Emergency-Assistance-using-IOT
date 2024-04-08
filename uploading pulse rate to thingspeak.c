#include <SoftwareSerial.h>
#define DEBUG true 
SoftwareSerial esp8266(9,10);
#include <stdlib.h>
#include "ThingSpeak.h"

#define SSID // "SSID-WiFiname" 
#define PASS "98765432" // "password" 
#define IP ""; // enter your IP address 
int blinkPin = 7; // pin to blink led at each beat 
int fadePin = 0; 
int pulsePin = 12; 
int fadeRate = 0;

// Volatile Variables, used in the interrupt service routine!

volatile int BPM; 
volatile int IBI = 600; // int that holds raw Analog in 0. updated every 2m5 
volatile int Signal; // holds the incoming raw data // int that holds the time interval between beats! Must be seeded! 
volatile boolean Pulse = false; // "True" when heartbeat is detected. "False" when not a "live beat". 
volatile boolean QS = false; // becomes true when Arduino finds a beat.

// Regards Serial Output -- Set This Up to your needs

static boolean serialVisual = true; 
volatile unsigned long sampleCounter = 22; 
volatile int P = 512;
volatile int rate[10]; // array to hold last ten IBI values // used to determine pulse timing 
volatile unsigned long lastBeatTime = 0; // used to find IBI // used to find peak in pulse wave 
volatile int T = 512; // used to find trough in pulse wave // used to find instant moment of heart beat 
volatile int amp = 100; // used to hold amplitude of pulse waveform 
volatile boolean firstBeat = true; 
volatile int thresh;
volatile boolean secondBeat;

void setup () 
{
  Serial.begin(9600); 
  esp8266.begin(9600); 
  Serial.println ("AT");
  Serial.print ("Connecting...");
}

esp8266.println ("AT"); 
delay (5000); 
if (esp8266.find ("OK")) 
{
  connectWiFi (); 
  interruptsetup ();
}


void loop()
{
  start:
  error = 0;
  lcd.setCursor(0, 0);
  Serial.print("BPM = ");
  Serial.print(BPM);
  ThingSpeak.setField(1, BPM);
  ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  delay(100);
  updatebeat();

  if (error == 1)
  {
    goto start; // go to label "start"
  }

  delay(1000);
}



void updatebeat()
{
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += IP;
  Serial.println(cmd);
  esp8266.println(cmd);
  delay(2000);
  if (esp8266.find("Error"))
  {
    return;
  }

  cmd += msg;
  cmd += BPM;
  cmd += "\r\n";

  cmd += "&field1=";
  Serial.print("AT+CIPSEND=");
  esp8266.print("AT+CIPSEND=");
  Serial.println(cmd.length());
  esp8266.println(cmd.length());
  if (esp8266.find(">"))
  {
    Serial.print(cmd);
    esp8266.print(cmd);
  }
  else
  {
    Serial.println("AT+CIPCLOSE");
    esp8266.println("AT+CIPCLOSE");
    error = 1;
  }
}


boolean connectWiFi()
{
  Serial.println("AT+CWMODE=1");
  esp8266.println("AT+CWMODE=1");
  delay(2000);
  String cmd = "AT+CWJAP=\"";
  cmd += SSID;
  cmd += "\",";
  cmd += PASS;
  cmd += "\"";
  Serial.println(cmd);
  esp8266.println(cmd);
  delay(5000);
  if (esp8266.find("OK"))
  {
    Serial.println("OK");
    return true;
  }
  else
  {
    return false;
  }
}



void interruptsetup()
{
  TCCR2A = 0x82;
  OCR2A = 0x7C;
}

ISR(TIMER2_COMPA_vect)
{
  TCCR2B = 0x86; // DISABLE PWM ON DIGITAL PINS 3 AND 11, AND GO INTO CTC MODE 
  TIMSK2 = 0x82; // SET THE TOP OF THE COUNT TO 124 FOR 588HZ SAMPLE RATE 
  sei(); // MAKE SURE GLOBAL INTERRUPTS ARE ENABLED 

  // triggered when Timer2 counts to 124 
  cli(); // disable interrupts while we do this 
  Signal = analogRead(pulsePin); // read the Pulse Sensor 
  sampleCounter += 2; // keep track of the time in ms 
  lastBeatTime = sampleCounter; // monitor the time since the Last beat to avoid noise 

  int N = sampleCounter;

  // find the peak and trough of the pulse wave
  if (Signal < thresh && N > (IBI / 5) * 3)
  {
    T = Signal; // avoid dichrotic noise by waiting 3/5 of Last IBI 
    if (Signal < T)
    { 
      Pulse = true;
    }
  }
  
  if (Signal > thresh && Signal < P)
  {
    Pulse = false;
  }
  
  if (N > 250)
  {
    P = Signal; // P is the peak
    if ((Signal > thresh) && (Pulse == false) && (N > (IBI / 5) * 3))
    {
      Pulse = true;
      IBI = sampleCounter - lastBeatTime; // set the Pulse flag when there is a pulse 
      lastBeatTime = sampleCounter; // keep track of time for next pulse
    }
  }
  
  if (secondBeat)
  {
    secondBeat = false; // clear secondBeat flag
    for (int i = 0; i <= 9; i++)
    {
      rate[i] = IBI; // seed the running total to get a realistic BPM at startup
    }
  }
  
  if (firstBeat)
  {
    firstBeat = false; // clear firstBeat flag
    secondBeat = true; // set the second beat flag
    sei(); // enable interrupts again
  }
  
  word runningTotal = 0; // clear the runningTotal variable
  for (int i = 0; i <= 8; i++)
  {
    rate[i] = rate[i+1]; // shift data in the rate array and drop the oldest IBI value
    runningTotal += rate[i]; // add up the 9 oldest IBI values
  }
  
  rate[9] = IBI; // add the Latest IBI to the rate array
  runningTotal += rate[9]; // add the Latest IBI to runningTotal
  runningTotal /= 10; // average the last 10 IBI values
  BPM = 60000 / runningTotal; // how many beats can fit into a minute? that's BPM!
  QS = true; // set Quantified Self flag
  
  if (Signal < thresh && Pulse == true)
  {
    Pulse = false;
    thresh = amp / 2 + T; // when the values are going down, the beat is over
    digitalWrite(blinkPin, LOW); // turn off pin 13 LED
    amp = P - T; // get amplitude of the pulse wave
    P = thresh; // reset these for next time
    T = thresh;
  }
  
  if (N > 2500)
  {
    P = 512;
    thresh = 512; // set thresh default
    T = 512; // set T default
    lastBeatTime = sampleCounter;
    secondBeat = false; // bring the LastBeatTime up to date
    firstBeat = true; // set these to avoid noise when we get the heartbeat back
    sei();
  }
}
