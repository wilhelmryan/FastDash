//fastLED library for output
#define FASTLED_ALLOW_INTERRUPTS 1
#include "FastLED3.1/FastLED.h"
//Encoder library for input
#define ENCODER_OPTIMIZE_INTERRUPTS
#include "Encoder/Encoder.h"
//EEPROM for storage
#include "EEPROMEx/EEPROMex.h"
// memory information for leds
#define bytesperled 2


//encoder information
#define ENCODERPIN1 4
#define ENCODERPIN2 5
#define BUTTONPIN 6
#define ANALOGPIN A0

#define _DEBUG 0
#define LEDPIN 13

//make an instance of the encoder on the correct pins
Encoder myEnc (ENCODERPIN1,ENCODERPIN2);
long encoderPosition=0;
long prevEncoderPos=0;
//debounce for encoder button
unsigned long debouncetime=500;
unsigned long lastupdate=0;

//led information
//#define NUMLEDS 32
#define NUMLEDS 25
#define DATAPIN 2
#define TYPE WS2811
#define COLORORDER GRB
//storage container for LED information
CRGB leds[NUMLEDS];
//global brightness control
#define MAXIUMUMBRIGHTNESS 200

//output enable on level shifter
#define OEPIN 12

//gauge cluster information
// gauges go left to right, gas, tach, speedo, temp, odometer

//old code first

/*const int numberofsections=4;*/
const int numberofsections=5;

/*const int ledspersection[numberofsections]={4,12,12,4};*/
const int ledspersection[numberofsections]={3,9,9,3,1};
	
/*const int startPositions[numberofsections]={0,4,16,28};*/
const int startPositions[numberofsections]={0,3,12,21,24};
	
/*const int endPosittion[numberofsections]={3,15,27,31};*/
const int endPosittion[numberofsections]={2,11,20,23,24};

// are we storing new colors to the dash
bool programmingmode=false;
//are we programming which mode we're in?
bool programdisplaymode=false; 
//has what I want displayed been displayed?
bool displayed=false;

//which led are we programming
volatile int mode=0;
//what display mode are we in
int displaymode=0;
//what mode were we in
int prevmode;

int brightness;


void setup()
{
	//set the reference
	analogReference(EXTERNAL);
	
	//set our various pins to output
	
	//show that the teensy is active
	pinMode(LEDPIN,OUTPUT);
	digitalWrite(LEDPIN,HIGH);
	
	//shut down the OE pin so that we don't get random firing on the LEDS during powerup
	pinMode(OEPIN,OUTPUT);
	digitalWrite(OEPIN,HIGH);
	
	//debugging information
	#ifdef _DEBUG
	Serial.begin(9600);
	Serial.println("Dash Lights V1.0");
	if(!EEPROM.isReady())Serial.println("There is a problem with EEPROM, please consult manual.");
	else Serial.println("Beginning Dash sequence");
#endif // _DEBUG
	
	//start up delay
	delay(4000);
	//led information
	FastLED.addLeds<TYPE, DATAPIN, COLORORDER>(leds,NUMLEDS);
	FastLED.setBrightness(MAXIUMUMBRIGHTNESS);
	
	//initialize our pin as input
	pinMode(BUTTONPIN,INPUT);
	//turn on pullups
	//digitalWrite(BUTTONPIN,HIGH);
	//allow the button to remain high
	delay(1000);
	//check if we want to program new colors, we don't have an interrupt defined but disable anyways
	noInterrupts();
	//check if button is pressed during power-up
	if(digitalRead(BUTTONPIN)) programmingmode=false;
	//if it's not, continue as normal
	else if (digitalRead(BUTTONPIN)==LOW) {digitalWrite(OEPIN,LOW); delay(5); programmingmode=true;blink();prevmode=mode;}
	//allow interrupts again
	interrupts();
	
	//attach our button interrupt
	attachInterrupt(BUTTONPIN,interruptService,FALLING);
	
	//give the user a blink of black/white to let them know we're programming
	if(programmingmode){programdisplaymode=true;chooseDisplayMode();}
	else
	{
		digitalWrite(OEPIN,LOW);
		delay(5);
		startupDisplay();
	}
	
	//have shifting rainbow until the user programs colors in
	if(EEPROM.read(2047)==255)displaymode=99;
	//else set it to their chosen display mode
	else displaymode=EEPROM.read(2047);
	
	// get our current brightness level
	brightness=EEPROM.read(2046);
	myEnc.write(brightness*4);
}
void startupDisplay()
{
	//blank the display
	fill_solid(leds,NUMLEDS,CRGB::Black);
	
	//use greatest number of leds per section
	for(int i=0;i<9;i++)
	{
		for(int q=0;q<numberofsections;q++)
		{
			if(i<ledspersection[q])
			{
				leds[startPositions[q]+i]=CHSV(96-((96/ledspersection[q])*i),255,255);
			}
		}
		FastLED.show();
		delay(100);
	}
	
	delay(500);
	for(int i=0;i<50;i++)
	{
		for(int q=0;q<NUMLEDS;q++)
		{
			leds[q]-=CRGB(0,255/50,255/50);
			leds[q]+=CRGB(255/50,0,0);
		}
		FastLED.show();
		delay(20);
	}
	
	for(int i=0;i<50;i++)
	{
		fadeToBlackBy(leds,NUMLEDS,20);
		FastLED.show();
		delay(10);
	}
}

void chooseDisplayMode()
{
	mode=0;
	displaymode=0;
	myEnc.write(0);
	displayed=false; 
	static int hue;
	
	do 
	{
		//check which mode we're in
		displaymode=abs(myEnc.read()/4);
		//make sure that if we need to update the colors, do so. this prevents flying through the colors on the random portion
		if(prevEncoderPos!=displaymode){displayed=false;prevEncoderPos=displaymode;}
		//wrap around and reset
		if(displaymode>3){displaymode=0;myEnc.write(0);}
			
		switch (displaymode)
		{
		case 0:
			for(int i=0;i<numberofsections;i++)
			{
				fill_gradient(leds,startPositions[i],CHSV(96,255,255),endPosittion[i],CHSV(0,255,255));
			}
			FastLED.show();
			break;
		case 1:
			for(int i=0;i<numberofsections;i++)
			{
				fill_solid(&leds[startPositions[i]],ledspersection[i],CHSV((255/numberofsections)*i,255,255));
			}
			FastLED.show();
			break;
		case 2: 
			if(!displayed)
			{
				for(int i=0;i<NUMLEDS;i++)
				{
					leds[i]=CHSV(random8(),255,255);
				}
				FastLED.show();
				displayed=true;
			}
			break;
		case 3:
			/*fill_rainbow(leds,NUMLEDS,hue,255/NUMLEDS);*/
			for(int i=0;i<numberofsections;i++)
			{
				fill_rainbow(&leds[startPositions[i]],ledspersection[i],hue,255/ledspersection[i]);
			}
			FastLED.show();
			delay(10);
			displaymode=99;
			break;
		//default case should not be possible, just here for debugging
		default:
			fill_solid(leds,NUMLEDS,CRGB::Grey);
			FastLED.show();
			break;
		}
		hue++;
	}while(programdisplaymode);
	
	//change the mode back to zero
	mode=0;
	//update our placeholder for the mode selection
	EEPROM.update(2047,displaymode);
	
	prevEncoderPos=0;
	fill_solid(leds,NUMLEDS,CRGB::Black);
}
void blink()
{
	for(int i=0;i<3;i++)
	{
		fill_solid(leds,NUMLEDS,CRGB::Grey);
		FastLED.show();
		delay(500);
		fill_solid(leds,NUMLEDS,CRGB::Black);
		FastLED.show();
		delay(500);
	}
}
void interruptService()
{
	//debounce
	if(millis()-lastupdate>debouncetime)
	{
		programdisplaymode=false;
		lastupdate=millis();
		mode++;
		if(!programmingmode)
		{
			if(displaymode==99){displaymode=0;return;}
			displaymode++;
			if(displaymode>3)displaymode=99;
			//if(displaymode==100)displaymode=0;
		}
	}
	
}
void updateregister(uint8_t location, int data)
{
	//make sure our EEPROM data is in range, fastLED automatically bring it in range for display, but not necessarily a byte for EEPROM
	if(data>255)data=data%255;
	//make sure we're not unnecessarily writing the EEPROM
	EEPROM.update(location,data);
}

void loop()
{
	while (programmingmode)
	{
		programColors();
	}
	
	display();
}

void display()
{
	static int hue;
	switch (displaymode)
	{
		//gradient fill
		case 0:
		for(int i=0;i<numberofsections;i++)
		{
			TGradientDirectionCode dir;
			//update location here
			if(EEPROM.read(NUMLEDS*bytesperled+(i*6)+5))dir=SHORTEST_HUES;
			else dir=LONGEST_HUES;
			//make sure there isn't only one led in the section to fill gradient for
			if(ledspersection[i]!=1)fill_gradient(leds,startPositions[i],CHSV(EEPROM.read(NUMLEDS*bytesperled+(i*6)),EEPROM.read(NUMLEDS*bytesperled+(i*6)+1),255),endPosittion[i],CHSV(EEPROM.read(NUMLEDS*bytesperled+(i*6)+3),EEPROM.read(NUMLEDS*bytesperled+(i*6)+4),255),dir);
			else if(ledspersection[i]==1) leds[startPositions[i]]=CHSV(EEPROM.read(NUMLEDS*bytesperled+(i*6)),EEPROM.read(NUMLEDS*bytesperled+(i*6)+1),255);
		}
		break;
		
		//solid fill
		case 1:
		for(int i=0;i<numberofsections;i++)
		{
			fill_solid(&leds[startPositions[i]],ledspersection[i],CHSV(EEPROM.read((NUMLEDS*bytesperled)+(numberofsections*6)+(i*2)),EEPROM.read((NUMLEDS*bytesperled)+(numberofsections*6)+(i*2)+1),255));
		}
		break;
		
		//individual led fill
		case 2:
		for(int i=0;i<NUMLEDS;i++)
		{
			leds[i]=CHSV(EEPROM.read(i*2),EEPROM.read((i*2)+1),255);
		}
		break;
		//only displayed when unboxed or if used for showing off
		case 99:
		for(int i=0;i<numberofsections;i++)
		{
			fill_rainbow(&leds[startPositions[i]],ledspersection[i],hue,255/ledspersection[i]);
		}
		break;
		
		default:
			fill_solid(leds,NUMLEDS,CRGB::Grey);
	}
	
	int tempbrightness=analogRead(ANALOGPIN)/4;
	
// 	if(tempbrightness>=MAXIUMUMBRIGHTNESS) FastLED.show(MAXIUMUMBRIGHTNESS);
// 	else FastLED.show(tempbrightness);

if(tempbrightness>10)
{
	brightness=abs(myEnc.read())%255;

	if(brightness>MAXIUMUMBRIGHTNESS) FastLED.show(MAXIUMUMBRIGHTNESS);
	else FastLED.show(brightness);
	EEPROM.update(2046,brightness);
}
else FastLED.show(tempbrightness);
	
	hue++;
	delay(10);
	
}

void programColors()
{
	//placeholder for moving forwards or backwards
	static bool forward;
	//storage for led information before it's saved
	static byte huevalues[NUMLEDS];
	//storage for led saturation before it's saved
	static byte saturationvalues[NUMLEDS];
	//direction for gradient fill
	static bool shorthue;
	static byte huedirection[numberofsections*bytesperled];
	
	int saturation;

	//get new encoder value
 	encoderPosition=myEnc.read()/4;
	 if(prevEncoderPos!=encoderPosition)
	 {
		 if(encoderPosition>0)
		 {
			 if(prevEncoderPos>encoderPosition){ forward=false; shorthue=true;}
			 else { forward=true; shorthue=false;}
		 }
		 if(encoderPosition<0)
		 {
			 if(prevEncoderPos>encoderPosition){ forward=false; shorthue=false;}
			 else {forward=true; shorthue=true;}
			 //encoderPosition=abs(encoderPosition);
		 }
		  prevEncoderPos=encoderPosition;
	 }
	 
	 //bring it in range
	if(encoderPosition>255){encoderPosition=0; myEnc.write(0);prevEncoderPos=0;}
	byte huetouse=abs(encoderPosition);

	switch (displaymode)
	{
	//gradient fill from start of section to end of section
	case 0:
		//bring value in range
		if(mode>numberofsections*2)mode=0;
		//get current saturation value
		saturation=analogRead(ANALOGPIN)/4;
		//update the color
		huevalues[mode]=huetouse;
		//update saturation
		saturationvalues[mode]=saturation;
		huedirection[mode]=shorthue;
		break;
	//solid fill per section
	case 1:
	//see code for case 0 for notes
		if(mode>numberofsections)mode=0;
		saturation=analogRead(ANALOGPIN)/4;
		huevalues[mode]=huetouse;
		saturationvalues[mode]=saturation;
		break;
	case 2:
	//see code for case 0 for notes
		if(mode>NUMLEDS)mode=0;
		saturation=analogRead(ANALOGPIN)/4;
		huevalues[mode]=huetouse;
		saturationvalues[mode]=saturation;
	}
	
	switch (displaymode)
	{
		//gradient fill from start to end of section with one direction code 
		case 0:
			for(int i=0;i<numberofsections;i++)
			{
				TGradientDirectionCode dir;
				if(huedirection[(i*2)+1])dir=SHORTEST_HUES;
				else dir=LONGEST_HUES;
				//if there's more than 1 led, fill it with a gradient
				if(ledspersection[i]!=1) fill_gradient(leds,startPositions[i],CHSV(huevalues[i*2],saturationvalues[(i*2)],255),endPosittion[i],CHSV(huevalues[(i*2)+1],saturationvalues[(i*2)+1],255),dir);
				//if there's only one, fill it with the first hue and saturation value
				else if (ledspersection[i]==1) leds[startPositions[i]]=CHSV(huevalues[i*2],saturationvalues[i*2],255);
			}
			break;
		//solid fill per section
		case 1:
			for(int i=0;i<numberofsections;i++)
			{
				if(i<=mode)fill_solid(&leds[startPositions[i]],ledspersection[i],CHSV(huevalues[i],saturationvalues[i],255));
				else fill_solid(&leds[startPositions[i]],ledspersection[i],CRGB::Black);
			}
			break;
		//individual led fill
		case 2:
			for(int i=0;i<NUMLEDS;i++)
			{
				if(i<=mode)leds[i]=CHSV(huevalues[i],saturationvalues[i],255);
				else leds[i]=CRGB::Black;
			}
			break;
		
	}
	FastLED.show();
	delay(10);
	
	if(prevmode!=mode)
	{
		
		switch (displaymode)
		{
			//section gradient
			case 0: 
				//update EEPROM HUE for each individual section
				EEPROM.update((NUMLEDS*bytesperled)+(prevmode*3),huevalues[prevmode]);
				//update EEPROM Saturation for each individual section
				EEPROM.update((NUMLEDS*bytesperled)+(prevmode*3)+1,saturationvalues[prevmode]);
				//update EEPROM for hue direction
				EEPROM.update((NUMLEDS*bytesperled)+(prevmode*3)+2,huedirection[prevmode]);
				//add 'value' update here if necessary (((NUMLEDS*bytesperled)-1)+(prevmode)*bytesperled)+2
				
				//old code
				//EEPROM.update(90+prevmode,huedirection[prevmode]);
				break;
				
			//solid fill per section
			case 1: 
				EEPROM.update(NUMLEDS*bytesperled+(numberofsections*6)+(prevmode*2),huevalues[prevmode]);
				EEPROM.update(NUMLEDS*bytesperled+(numberofsections*6)+(prevmode*2)+1,saturationvalues[prevmode]);
// 				EEPROM.update(80+(prevmode*2),huevalues[prevmode]);
// 				EEPROM.update(80+(prevmode*2)+1,saturationvalues[prevmode]);
				break;
			//individual led 
			case 2: 
				EEPROM.update(prevmode*2,huevalues[prevmode]);
				EEPROM.update((prevmode*2)+1,saturationvalues[prevmode]);
				break;
			default: 
				break;
		}
		prevmode=mode;
	}
}
