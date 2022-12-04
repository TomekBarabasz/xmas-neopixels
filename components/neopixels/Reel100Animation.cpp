#include <Reel100Animation.hpp>
#include <utils.hpp>
#include <led_strip.hpp>
#include <color.hpp>
#include <random.hpp>

namespace Neopixel
{
using animFcn = void (Reel100::*)();
animFcn anims[2] = { &Reel100::rainbow, &Reel100::rainbowWithGlitter };
Reel100::Reel100(LedStrip *strip_, int data_size, void *data) : strip(strip_)
{
    hue_update_delay_ms = decode<uint16_t>(data);
    anim_update_delay_s= decode<uint16_t>(data);
    hue_inc = decode<uint8_t>(data);
    glitter_chance = decode<uint8_t>(data);
    size = strip->getLength();
    anim_time = anim_update_delay_s * 1000;
    ESP_LOGI(TAG, "Reel100 animation : hue_update_delay_ms %d anim_update_delay_s %d hue_inc %d glitter_chance %d", hue_update_delay_ms, anim_update_delay_s, hue_inc, glitter_chance);
}
void Reel100::step()
{
    (this->*anims[anim_idx])();
    strip->refresh();
    anim_time -= hue_update_delay_ms;
    if (anim_time <= 0) { 
        anim_time = anim_update_delay_s * 1000;
        anim_idx = (anim_idx+1) % (sizeof(anims)/sizeof(anims[0]));
    }
}
void Reel100::rainbow()
{   
    HSV hsv = {hue,255,240};
    for (int i=0;i<size;++i) {
        strip->fillPixelsRGB(i,1,hsv.toRGB());
        hsv.h += hue_inc;
    }
    hue = hsv.h;
}
void Reel100::addGlitter(uint8_t chance)
{
    const uint32_t rnd = make_random();
    if ((rnd & 0xFF) < chance) {
        strip->fillPixelsRGB((rnd>>8) % size, 1, {255,255,255});
    }
}
void Reel100::rainbowWithGlitter() 
{
    rainbow();
    addGlitter(glitter_chance);
}
void Reel100::confetti() 
{
    // random colored speckles that blink in and fade smoothly
    fade_all(strip->getBuffer(), size, fade);
    const uint32_t rnd = make_random();
    HSV hsv = {uint16_t(hue + (rnd & 64)), 200, 255};
    strip->getBuffer()[(rnd >> 8) % size] +=  hsv.toRGB();
}
#if 0
    void sinelon()
    {
        // a colored dot sweeping back and forth, with fading trails
        fade_all(strip->getBuffer(), size, fade);

        int pos = beatsin16( 13, 0, NUM_LEDS-1 );
        leds[pos] += CHSV( gHue, 255, 192);
    }

    void bpm()
    {
        // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
        uint8_t BeatsPerMinute = 62;
        CRGBPalette16 palette = PartyColors_p;
        uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
        for( int i = 0; i < NUM_LEDS; i++) { //9948
            leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
        }
    }

    void juggle() 
    {
        // eight colored dots, weaving in and out of sync with each other
        fadeToBlackBy( leds, NUM_LEDS, 20);
        byte dothue = 0;
        for( int i = 0; i < 8; i++) {
            leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
            dothue += 32;
        }
    }
#endif
};
}