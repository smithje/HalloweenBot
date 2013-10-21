#include <Adafruit_NeoPixel.h>

#define TOP_LED_FIRST  0 // Change these if the first pixel is not
#define TOP_LED_SECOND 0 // at the top of the first and/or second ring.


// Pins
const byte GOGGLES_PIN = 0;
const byte BUTTON_PIN = 1;
const byte BUZZER_PIN = 2;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(32, GOGGLES_PIN, NEO_GRB + NEO_KHZ800);


byte maxEffectRepeats = getEffectRepeats();
byte effectCounter = 0;
unsigned long lastEffect = 0; // The last time an effect happened

// ROTATE vars
const byte ALT_ROTATE = 0;
unsigned int rotateDelay = 600;
byte showRemainder = 0;

const byte ALTERNATE = 1;

// ANGRY vars
const byte ANGRY = 10;
unsigned int angryDelay = 1000;

// SWAP vars
const byte SWAP = 2;
unsigned int swapDelay = 100;
byte swapStep = 0;
byte swapCenter = 0;

///////////////
/// Colors and Brightness
///////////////

byte
  iBrightness[16],    // Brightness map -- eye colors get scaled by these
  brightness   = 200, // Global brightness (0-255)
  effect = 2;

byte
  iColor[16][3];      // Background colors for eyes
const byte COLORS[][3] = {  // Rainbow, thanks wikipedia!
      {255, 0, 0},
      {255, 127, 0}, 
      {255, 255, 0},
      {0, 255, 0},
      {0, 0, 255},
      {0, 255, 255}, //Aqua
      {255, 0, 255}, //Purple
      {127, 255, 0} //Chartreuse
      
//      {143, 0, 255}
                     };


/////////////////////
// Button
/////////////////////

int
  buttonState  =   LOW, // Initial button state
  lastButtonState = LOW;

long lastDebounceTime = 0;
long debounceDelay = 50;





void setup() {
  pixels.begin();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  setRandomColor();
  effect = getRandomEffect();
}

void loop() {
  byte i;
  
  if (effectCounter >= maxEffectRepeats) {
    effect = getRandomEffect();
    effectCounter = 0;
    maxEffectRepeats = getEffectRepeats();
  }
  
  // Debounce!
  int reading = digitalRead(BUTTON_PIN);
  
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) { //It has changed
      buttonState = reading;
      
      if (buttonState == LOW) {
        effect = ANGRY;
        lastEffect = millis()+55;
      } else {
        effect = getRandomEffect();
        // Reset the swap step because this can get interrupted
        swapStep = 0;
      }
    }
  }
  
  lastButtonState = reading;
  // End debouncing
  

  if ((effect == ALT_ROTATE) || (effect == ALTERNATE)) {

    if ((millis() - lastEffect) > rotateDelay) {

        // Reset after we've gone around once
        if (showRemainder > 3) {
          showRemainder = 0;
          effectCounter++;
          if (effectCounter < maxEffectRepeats) {
            if (random(0,4) == 0) { // 25% chance of craziness!
              setRandomColors();
            } else {
              setRandomColor();
            }
          } else {
            return;
          }
        }
        
        lastEffect = millis();
        
        // Set everything dark on alternates
        if (effect == ALTERNATE && showRemainder % 2 == 1) {
           memset(iBrightness, 0, sizeof(iBrightness));
        } else {  
          for (i=0; i<16; i++) {
            if (i % 4 == showRemainder) {
              iBrightness[i] = brightness;
            } else {
              iBrightness[i] = 0;
            }
          }
        }
        
       drawEyes(pixels, iBrightness, iColor, true);
       showRemainder++;
    }

  } else if (effect == SWAP) {
    if ((millis() - lastEffect) > swapDelay) {
      lastEffect = millis();
      if (swapStep == 0) {
          swapCenter = random(0, 16);
          setRandomColor();
          // Set all to 0
          memset(iBrightness, 0, sizeof(iBrightness));
          // Set the center and the 3 pins on either side
          for (i=0; i<4; i++) {
            iBrightness[(swapCenter + i) & 0x0F] = brightness;
            iBrightness[(swapCenter - i) & 0x0F] = brightness;
          }
      // Skip steps to hang out at 0 for longer
      } else if (swapStep == 8) {
        // Swap the center
        swapOffset(0);
      } else if (swapStep == 9) {
        swapOffset(1);  
      } else if (swapStep == 10) {
        swapOffset(2);  
      } else if (swapStep == 11) {
        swapOffset(3);
      }  // Skip steps to hang out here longer
      
      drawEyes(pixels, iBrightness, iColor, false);
      swapStep++;
      if (swapStep > 19) {
        swapStep = 0;
        effectCounter++;
      }
    }
    
    


  } else if (effect == ANGRY) {
    // All red
    for (i=0; i<16; i++) {
     iColor[i][0] = 255; iColor[i][1] = 0 ; iColor[i][2] = 0;
     iBrightness[i] = brightness; 
    }   
    drawEyes(pixels, iBrightness, iColor, false);

    beep(BUZZER_PIN, 4000, 30);
    
    
    if ((millis() - lastEffect) > angryDelay) {
      lastEffect = millis();
      // Spin quickly around
      for (i=0; i<16; i++) {
        // Assume the color are all set already
        memset(iBrightness, 0, sizeof(iBrightness));
        iBrightness[i] = brightness;
        drawEyes(pixels, iBrightness, iColor, true);
        delay(20);
      }

    }
  }


}


void drawEyes(Adafruit_NeoPixel pixels, byte iBrightness[], byte iColor[][3], boolean reflectBrightness) {
    // Heavily based off of the Adafruit Goggles example
    byte i, r, g, b, a;
    // Merge iColor with iBrightness, issue to NeoPixels
    for(i=0; i<16; i++) {
      a = iBrightness[i] + 1;
      // First eye
      r = iColor[i][0];            // Initial background RGB color
      g = iColor[i][1];
      b = iColor[i][2];
      if(a) {
        r = (r * a) >> 8;          // Scale by brightness map
        g = (g * a) >> 8;
        b = (b * a) >> 8;
      }
      pixels.setPixelColor(((i + TOP_LED_FIRST) & 15), r, g, b);
     
     
      // Second eye 
      r = iColor[i][0];            // Initial background RGB color
      g = iColor[i][1];
      b = iColor[i][2];
      
      // Optionally, reflect the brightness
      if (reflectBrightness == true) {
        a = iBrightness[(16-i) & 0x0F] + 1;
      }
      if(a) {
        r = (r * a) >> 8;
        g = (g * a) >> 8;
        b = (b * a) >> 8;
      }
      pixels.setPixelColor(16 + ((i + TOP_LED_SECOND) & 15), r, g, b);
    }
    pixels.show();
}

// See http://learn.adafruit.com/trinket-gemma-mini-theramin-music-maker/code
void beep (unsigned char speakerPin, int frequencyInHertz, long timeInMilliseconds) 
{	 // http://web.media.mit.edu/~leah/LilyPad/07_sound_code.html
          int x;	 
          long delayAmount = (long)(1000000/frequencyInHertz);
          long loopTime = (long)((timeInMilliseconds*1000)/(delayAmount*2));
          for (x=0;x<loopTime;x++)	 
          {	 
              digitalWrite(speakerPin,HIGH);
              delayMicroseconds(delayAmount);
              digitalWrite(speakerPin,LOW);
              delayMicroseconds(delayAmount);
          }	 
}

void setRandomColor() {
  // Set all to one random color
  byte r, g, b, i, colorIndex;
  colorIndex = getRandomColorIndex();
  r = COLORS[colorIndex][0];
  g = COLORS[colorIndex][1];
  b = COLORS[colorIndex][2];
  for (i=0; i<16; i++) {
    iColor[i][0] = r; 
    iColor[i][1] = g; 
    iColor[i][2] = b;
  }
}

void setRandomColors() {
  // set each pixel to a random color
  byte i, colorIndex;
  for (i=0; i<16; i++) {
    colorIndex = getRandomColorIndex();
    iColor[i][0] = COLORS[colorIndex][0];
    iColor[i][1] = COLORS[colorIndex][1];
    iColor[i][2] = COLORS[colorIndex][2];
  }
}

long getRandomColorIndex() {
  return random(0, sizeof(COLORS)/3);
}

void swapPixel(byte fromPixel, byte toPixel) {
  // Set fromPixel brightness to 0 
  // and toPixel brightness to brightness
  iBrightness[fromPixel] = 0;
  iBrightness[toPixel] = brightness;
}

void swapOffset(byte offset) {
  // We swap on either side of the center, offset by the offset argument 
  swapPixel((swapCenter + offset) & 0x0F, (swapCenter + 8 + offset) & 0x0F);
  swapPixel((swapCenter - offset) & 0x0F, (swapCenter + 8 - offset) & 0x0F);
}
  
byte getRandomEffect() {
  // Get random, non-angry effect
  return (byte) random(0, 3);  // 0-3 are defined, increase this if you add another effect
}

byte getEffectRepeats() {
  return (byte) random(1, 5);
}
