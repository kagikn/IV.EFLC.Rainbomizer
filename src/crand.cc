#include "CHud.hh"
#include "injector/hooking.hpp"
#include "Patterns/Patterns.hh"
#include "Utils.hh"
#include <algorithm>
#include <ctime>
#include <cmath>
#include "CTimer.hh"

void (__fastcall *FUN_453920) (void *);
void (__thiscall *CVehicle__OriginalSetRandomColour) (void *, uint8_t *,
                                                      uint8_t *, uint8_t *,
                                                      uint8_t *, int);
int (*sscanf_a6e120) (char *src, char *format, ...);

struct HSL
{
    float h;
    float s;
    float l;
};

/*******************************************************/
CARGB
HSLToRGB (HSL input)
{
    auto f = [input] (float n) {
        float k = fmod ((n + (input.h / 30)), 12);
        float a = input.s * std::min (input.l, 1 - input.l);
        return input.l
               - a * std::max (-1.0f, std::min (k - 3, std::min (9 - k, 1.0f)));
    };

    return CARGB (255, f (0) * 255, f (8) * 255, f (4) * 255);
}

/*******************************************************/
HSL
RGBToHSL (CARGB input)
{
    HSL   out;
    float R = input.r / 255.0;
    float G = input.g / 255.0;
    float B = input.b / 255.0;

    float Xmax = std::max ({R, G, B});
    float Xmin = std::min ({R, G, B});
    float C    = Xmax - Xmin;

    out.l = (Xmax + Xmin) / 2;
    if (C == 0)
        out.h = 0;
    else if (fabs (Xmax - R) < 0.01)
        out.h = 60 * (0 + ((G - B) / C));
    else if (fabs (Xmax - G) < 0.01)
        out.h = 60 * (2 + ((B - R) / C));
    else if (fabs (Xmax - B) < 0.01)
        out.h = 60 * (4 + ((R - G) / C));

    if (out.l == 0 || out.l == 1)
        out.s = 0;
    else
        out.s = C / (1 - (fabs (2 * Xmax - C - 1)));

    return out;
}

/*******************************************************/
class ColourRandomizer
{
    static int m_nTotalColours;

    /*******************************************************/
    static void __fastcall HookRender (void *thisCritical)
    {
        const int MAX_COLOURS = 73;
        const float CYCLE_TIME = 3;

        static HSL  colours[MAX_COLOURS];
        static bool coloursInitialised = false;

        if (CHud::aColourNameHashes[0] > 0)
            {
                if (!coloursInitialised)
                    {
                        for (int i = 0; i < MAX_COLOURS; i++)
                            {
                                colours[i]   = RGBToHSL (CHud::aColours[i]);
                                colours[i].s = 1;
                            }

                        coloursInitialised = true;
                    }
                for (int i = 0; i < MAX_COLOURS; i++)
                    {
                        CHud::aColours[i] = HSLToRGB (colours[i]);
                        colours[i].h
                            = fmod (colours[i].h
                                        + CTimer::ms_fFrameTime * 360 / 10.0,
                                    360);
                    }
            }

        return FUN_453920 (thisCritical);
    }

    /*******************************************************/
    static void __fastcall RandomizeCarCols (void *info, void *edx,
                                             uint8_t *prim, uint8_t *seco,
                                             uint8_t *tert, uint8_t *quat,
                                             int incr)
    {
        CVehicle__OriginalSetRandomColour (info, prim, seco, tert, quat, incr);

        if (m_nTotalColours != 0)
            {
                *prim = RandomInt (m_nTotalColours);
                *seco = RandomInt (m_nTotalColours);
                *tert = RandomInt (m_nTotalColours);
                *quat = RandomInt (m_nTotalColours);
            }
    }

    /*******************************************************/
    static void
    RandomizeColourTable (char *line, char *format, int *r, int *g, int *b,
                          char *Sprefix, char *Sname)
    {
        sscanf_a6e120 (line, format, r, g, b, Sprefix, Sname);

        CARGB colour = HSLToRGB (
            {m_nTotalColours / 133.0f * 360, 1.0, RandomInt (100) / 100.0f});

        *r = colour.r;
        *g = colour.g;
        *b = colour.b;

        m_nTotalColours++;
    }

public:
    /*******************************************************/
    ColourRandomizer ()
    {
        InitialiseAllComponents();
        
        RegisterHook ("0f ? ? 09 00 00 8d 8c 24 ? 01 00 00", 59, sscanf_a6e120,
                      ColourRandomizer::RandomizeColourTable);

        RegisterHook ("50 8d 8e e4 0f 00 00 51 8b ? e8", 10,
                      CVehicle__OriginalSetRandomColour,
                      ColourRandomizer::RandomizeCarCols);

        RegisterHook ("8d b7 e0 0f 00 00 b9 ? ? ? ? e8 ", 11, FUN_453920,
                      ColourRandomizer::HookRender);
    }

} _cols;

int ColourRandomizer::m_nTotalColours = 0;
