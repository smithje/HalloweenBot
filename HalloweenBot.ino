#include <Adafruit_NeoPixel.h>

#define TOP_LED_FIRST  0 // Change these if the first pixel is not
#define TOP_LED_SECOND 0 // at the top of the first and/or second ring.


// Pins
const byte GOGGLES_PIN = 0;
const byte BUTTON_PIN = 1;
const byte BUZZER_PIN = 2;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(32, GOGGLES_PIN, NEO_GRB + NEO_KHZ800);

byte maxEffectRepeats = getEffectRepeats();
byte effectRepeatCounter = 0;
unsigned long lastEffectTime = 0; 

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
byte swapCenterPixel = 0;


//SPIN vars
const byte SPIN = 3;

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
      {255, 0, 255} //Purple
//      {127, 255, 0} //Chartreuse
    };


/////////////////////
// Button
/////////////////////

int
  buttonState  =   HIGH, // Initial button state
  lastButtonState = HIGH;

long lastDebounceTime = 0;
long debounceDelay = 50;





void setup() {
  pixels.begin();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  setRandomColor();
}

void loop() {
  byte i;
  
  if (effectRepeatCounter >= maxEffectRepeats) {
    effect = getRandomEffect();
    effectRepeatCounter = 0;
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
        lastEffectTime = millis()+55;
      } else {
        effect = getRandomEffect();
        // Reset the swap step because this can get interrupted
        swapStep = 0;
      }
    }
  }
  
  lastButtonState = reading;
  // End debouncing
  
  /*
  ALT_ROTATE and ALTERNATE
  
  An alternating effect.
  
  There are actually two defined here, because they are so similar
  
  ALT_ROTATE lights up every 4th pixel and rotates the lit pixels
  
  ALTERNATE lights up every 4th pixel, then sets them all blank, then 
  lights up every 4th pixel again, offset by two pixels.
  
  */

  if ((effect == ALT_ROTATE) || (effect == ALTERNATE)) {

    if ((millis() - lastEffectTime) > rotateDelay) {

        // Reset after we've gone around once
        if (showRemainder > 3) {
          showRemainder = 0;
          effectRepeatCounter++;
          if (effectRepeatCounter < maxEffectRepeats) {
            // Set the colors
            chooseRandomColorer();
          } else {
            return;
          }
    }
        
        lastEffectTime = millis();
        
        // Set everything dark on alternates
        if (effect == ALTERNATE && showRemainder % 2 == 1) {
           setBrightnessToZero();
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

  /*
  SWAP
  The swap effect is supposed to look like eyes looking one way then moving to the opposite side
  
  We light up seven pixels, then move them in pairs across to the opposite side.
  
  */

  } else if (effect == SWAP) {
    if ((millis() - lastEffectTime) > swapDelay) {
      lastEffectTime = millis();
      if (swapStep == 0) {
          swapCenterPixel = random(0, 16);
          setRandomColor();
          // Set all to 0
          setBrightnessToZero();
          // Set the center and the 3 pins on either side
          for (i=0; i<4; i++) {
            iBrightness[(swapCenterPixel + i) & 0x0F] = brightness;
            iBrightness[(swapCenterPixel - i) & 0x0F] = brightness;
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
        effectRepeatCounter++;
      }
    }
    

  /*
  SPIN
  Quickly spin around
  */
  
  } else if (effect == SPIN) {
    setRandomColor();
    spin();
    effectRepeatCounter++;
    
    
 
  /*
  This is called when the button is pressed
  repeat while button is pressed:
    Turn everything red
    sound the buzzer
    spin()
    
  We can't spin and sound the buzzer at the same time, sorry.
  */
  
  } else if (effect == ANGRY) {
    // All red
    for (i=0; i<16; i++) {
     iColor[i][0] = 255; iColor[i][1] = 0 ; iColor[i][2] = 0;
     iBrightness[i] = brightness; 
    }   
    drawEyes(pixels, iBrightness, iColor, false);

    beep(BUZZER_PIN, 4000, 30);
    
    
    if ((millis() - lastEffectTime) > angryDelay) {
      lastEffectTime = millis();
      // Spin quickly around
      spin();

    }
  }


}


void setBrightnessToZero() {
  // Set all brightnesses to zero
  memset(iBrightness, 0, sizeof(iBrightness));
}

void spin() {
  // Spin around quickly.  Set the colors before calling this
  byte i;
  for (i=0; i<16; i++) {
    // set all brightness to 0
    setBrightnessToZero();
    iBrightness[i] = brightness;
    drawEyes(pixels, iBrightness, iColor, true);
    delay(20);
  }
}

void drawEyes(Adafruit_NeoPixel pixels, byte iBrightness[], byte iColor[][3], boolean reflectBrightness) {
    // Heavily based off of the Adafruit Goggles example
    byte i, r, g, b, a, reflected_i;
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
      
      // Optionally, reflect the brightness and color
      if (reflectBrightness == true) {
        reflected_i = (16-i) & 0x0F;
        a = iBrightness[reflected_i] + 1;
        r = iColor[reflected_i][0];
        g = iColor[reflected_i][1];
        b = iColor[reflected_i][2];
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
  // Set all pixels to the same random color
  byte i, colorIndex;
  colorIndex = getRandomColorIndex();
  for (i=0; i<16; i++) {
    iColor[i][0] = COLORS[colorIndex][0]; 
    iColor[i][1] = COLORS[colorIndex][1]; 
    iColor[i][2] = COLORS[colorIndex][2];
  }
}

void setFourRandomColors() {
    // Set every 4th pixel to the same random color
    byte i, j, colorIndex;
    for (i=0; i<4; i++) {
      colorIndex = getRandomColorIndex();
      for (j=0; j<4; j++) {
        iColor[i + j*4][0] = COLORS[colorIndex][0];
        iColor[i + j*4][1] = COLORS[colorIndex][1];
        iColor[i + j*4][2] = COLORS[colorIndex][2];
      }
    }
}

void setAllRandomColors() {
  // Set every pixel to a random color
  // Currently unused, due to lack of space
  byte i, j, colorIndex;
  for (i=0; i<16; i++) {
    colorIndex = getRandomColorIndex();
    iColor[i][0] = COLORS[colorIndex][0];
    iColor[i][1] = COLORS[colorIndex][1];
    iColor[i][2] = COLORS[colorIndex][2];
  }

}


void chooseRandomColorer() {
  // Choose a randomness level for use in setRandomColor
  byte randomNumber = (byte) random(0, 2);
  if (randomNumber == 0) {
    // 50% chance of craziness
    setFourRandomColors();
  } else {
    setRandomColor();
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
  swapPixel((swapCenterPixel + offset) & 0x0F, (swapCenterPixel + 8 + offset) & 0x0F);
  swapPixel((swapCenterPixel - offset) & 0x0F, (swapCenterPixel + 8 - offset) & 0x0F);
}
  
byte getRandomEffect() {
  // Get random, non-angry effect
  return (byte) random(0, 4);  // 0-4 are defined, increase this if you add another effect
}

byte getEffectRepeats() {
  return (byte) random(1, 5);
}
