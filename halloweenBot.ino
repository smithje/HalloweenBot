#include <Adafruit_NeoPixel.h>

#define PIN            0

#define TOP_LED_FIRST  0 // Change these if the first pixel is not
#define TOP_LED_SECOND 0 // at the top of the first and/or second ring.


// Pins
const int GOGGLES_PIN = 0;
const int BUTTON_PIN = 1;
const int BUZZER_PIN = 2;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(32, GOGGLES_PIN, NEO_GRB + NEO_KHZ800);


const byte MAX_EFFECT_REPEATS = 4;
byte effect_counter = 0;
long last_effect = 0;

// ROTATE vars
const byte ALT_ROTATE = 0;
long rotate_delay = 600;
byte show_remainder = 0;

const byte ALTERNATE = 1;

// ANGRY vars
const byte ANGRY = 10;
long angry_delay = 1000;
unsigned int color_counter = 0;

// SWAP vars
const byte SWAP = 2;
long swap_delay = 100;
byte swap_step = 0;
byte swap_center = 0;

///////////////
/// Colors and Brightness
///////////////

byte
  iBrightness[16],    // Brightness map -- eye colors get scaled by these
  brightness   = 50, // Global brightness (0-255)
  effect = 2;

byte
  iColor[16][3];      // Background colors for eyes
const byte COLORS[][3] = {  // Rainbow, thanks wikipedia!
      {255, 0, 0},
      {255, 127, 0}, 
      {255, 255, 0},
      {0, 255, 0},
      {0, 0, 255},
      {75, 0, 130},
      {143, 0, 255}
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
  pinMode(BUTTON_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  setColors(color_counter);
}

void loop() {
  byte i;
  
  if (effect_counter >= MAX_EFFECT_REPEATS) {
    effect = getRandomEffect();
    effect_counter = 0;
  }
  
  // Debounce!
  int reading = digitalRead(BUTTON_PIN);
  
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) { //It has changed
      buttonState = reading;
      
      if (buttonState == HIGH) {
        effect = ANGRY;
        last_effect = millis()+55;
      } else {
        effect = getRandomEffect();
        // Reset the swap step because this can get interrupted
        swap_step = 0;
      }
    }
  }
  
  lastButtonState = reading;
  // End debouncing
  

  if ((effect == ALT_ROTATE) || (effect == ALTERNATE)) {

    if ((millis() - last_effect) > rotate_delay) {
        last_effect = millis();
        // Reset after we've gone around once
        if (show_remainder > 3) {
          color_counter = getRandomColorIndex();
          setColors(color_counter);
          show_remainder = 0;
          effect_counter++;
        }
        
        // Set everything dark on alternates
        if (effect == ALTERNATE && show_remainder % 2 == 1) {
           memset(iBrightness, 0, sizeof(iBrightness));
        } else {  
          for (i=0; i<16; i++) {
            if (i % 4 == show_remainder) {
              iBrightness[i] = brightness;
            } else {
              iBrightness[i] = 0;
            }
          }
        }
        
       drawEyes(pixels, iBrightness, iColor, true);
       show_remainder++;
    }

  } else if (effect == SWAP) {
    if ((millis() - last_effect) > swap_delay) {
      last_effect = millis();
      if (swap_step == 0) {
        swap_center = random(0, 16);
        color_counter = getRandomColorIndex();
        setColors(color_counter);
        // Set all to 0
        memset(iBrightness, 0, sizeof(iBrightness));
        // Set the center and the 3 pins on either side
        for (i=0; i<4; i++) {
          iBrightness[(swap_center + i) & 0x0F] = brightness;
          iBrightness[(swap_center - i) & 0x0F] = brightness;
        }
      // Skip steps to hang out at 0 for longer
      } else if (swap_step == 6) {
        // Swap the center
        swapPixel(swap_center, (swap_center + 8) & 0x0F);
      } else if (swap_step == 7) {
        swapPixel((swap_center + 1) & 0x0F, (swap_center + 9) & 0x0F);
        swapPixel((swap_center - 1) & 0x0F, (swap_center + 7) & 0x0F);    
      } else if (swap_step == 8) {
        swapPixel((swap_center + 2) & 0x0F, (swap_center + 10) & 0x0F);
        swapPixel((swap_center - 2) & 0x0F, (swap_center + 6) & 0x0F);  
      } else if (swap_step == 9) {
        swapPixel((swap_center + 3) & 0x0F, (swap_center + 11) & 0x0F);
        swapPixel((swap_center - 3) & 0x0F, (swap_center + 5) & 0x0F);  
      }  // Skip step 6 to hang out here longer
      
      drawEyes(pixels, iBrightness, iColor, false);
      swap_step++;
      if (swap_step > 15) {
        swap_step = 0;
        effect_counter++;
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
    
    
    if ((millis() - last_effect) > angry_delay) {
      last_effect = millis();
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
       // pgm_read_byte(&gamma8[r]), // Gamma correct and set pixel
       // pgm_read_byte(&gamma8[g]),
       // pgm_read_byte(&gamma8[b]));
  
      // Second eye uses the same colors, but reflected horizontally.
      r = iColor[15 - i][0];
      g = iColor[15 - i][1];
      b = iColor[15 - i][2];
      
      // Optionally, reflect the brightness
      if (reflectBrightness == true) {
        a = iBrightness[15-i] + 1;
      }
      if(a) {
        r = (r * a) >> 8;
        g = (g * a) >> 8;
        b = (b * a) >> 8;
      }
      pixels.setPixelColor(16 + ((i + TOP_LED_SECOND) & 15), r, g, b);
       // pgm_read_byte(&gamma8[r]),
       // pgm_read_byte(&gamma8[g]),
       // pgm_read_byte(&gamma8[b]));
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

void setColors(unsigned int color_counter) {
  byte r, g, b, i;
  r = COLORS[color_counter][0];
  g = COLORS[color_counter][1];
  b = COLORS[color_counter][2];
  for (i=0; i<16; i++) {
    iColor[i][0] = r; 
    iColor[i][1] = g; 
    iColor[i][2] = b;
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

byte getRandomEffect() {
  // Get random, non-angry effect
  return random(0, 4);  // 0-3 are defined, increase this if you add another effect
}
