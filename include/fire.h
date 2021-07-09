#include <Arduino.h>
#define FASTLED_INTERNAL
#include <FastLED.h>
#include "ledgfx.h"

class ClassicFireEffect
{
protected:
    int     Size;
    int     Cooling;
    int     Sparks;
    int     SparkHeight;
    int     Sparking;
    bool    bReversed;
    bool    bMirrored;
    CRGBPalette16 palette;

    byte  * heat;

    // When diffusing the fire upwards, these control how much to blend in from the cells below (ie: downward neighbors)
    // You can tune these coefficients to control how quickly and smoothly the fire spreads.  

    static const byte BlendSelf = 2;
    static const byte BlendNeighbor1 = 3;
    static const byte BlendNeighbor2 = 2;
    static const byte BlendNeighbor3 = 1;
    static const byte BlendTotal = (BlendSelf + BlendNeighbor1 + BlendNeighbor2 + BlendNeighbor3);



public:
    
    // Lower sparking -> more flicker.  Higher sparking -> more consistent flame

    ClassicFireEffect(int size, int cooling = 80, int sparking = 50, int sparks = 3, int sparkHeight = 3, bool breversed = true, bool bmirrored = false) 
        : Size(size),
          Cooling(cooling),
          Sparks(sparks),
          SparkHeight(sparkHeight),
          Sparking(sparking),
          bReversed(breversed),
          bMirrored(bmirrored)
    {
        if (bMirrored)
            Size = Size / 2;

        heat = new byte[size] { 0 };
        setNormalFire();
    }

    virtual ~ClassicFireEffect()
    {
        delete [] heat;
    }

    virtual void setNormalFire() {
        // This palette is the basic 'black body radiation' colors,
        // which run from black to red to bright yellow to white.
        //palette = HeatColors_p;

        // Similar but without the white
        palette = CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::Yellow,  CRGB::LightYellow);
    }

    virtual void setBlueFire() {
        palette = CRGBPalette16( CRGB::Black, CRGB::Blue, CRGB::Aqua,  CRGB::White);
    }

    virtual void setGreenFire() {
        palette = CRGBPalette16( CRGB::Black, CRGB::Green, CRGB::White);
    }

    virtual void setRedFire() {
        palette = CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::White);
    }

    virtual void setSolidBlue() {
        palette = CRGBPalette16( CRGB::Blue, CRGB::Blue, CRGB::Blue);
    }

    virtual void setSolidGreen() {
        palette = CRGBPalette16( CRGB::Green, CRGB::Green, CRGB::Green);
    }

    virtual void setSolidRed() {
        palette = CRGBPalette16( CRGB::Red, CRGB::Red, CRGB::Red);
    }

    virtual void DrawFire()
    {
        // First cool each cell by a little bit
        for (int i = 0; i < Size; i++)
            heat[i] = max(0L, heat[i] - random(0, ((Cooling * 10) / Size) + 2));

        // Next drift heat up and diffuse it a little but
        for (int i = 0; i < Size; i++)
            heat[i] = (heat[i] * BlendSelf + 
                       heat[(i + 1) % Size] * BlendNeighbor1 + 
                       heat[(i + 2) % Size] * BlendNeighbor2 + 
                       heat[(i + 3) % Size] * BlendNeighbor3) 
                      / BlendTotal;

        // Randomly ignite new sparks down in the flame kernel
        for (int i = 0; i < Sparks; i++)
        {
            if (random(255) < Sparking)
            {
                int y = Size - 1 - random(SparkHeight);
                heat[y] = heat[y] + random(160, 255); // This randomly rolls over sometimes of course, and that's essential to the effect
            }
        }

        // Finally convert heat to a color
        for (int i = 0; i < Size; i++)
        {
            CRGB color = ColorFromPalette(palette, heat[i]);
            int j = bReversed ? (Size - 1 - i) : i;
            DrawPixels(j, 1, color);
            if (bMirrored)
            {
                int j2 = !bReversed ? (2 * Size - 1 - i) : Size + i;
                DrawPixels(j2, 1, color);
            }
        }
    }
};
