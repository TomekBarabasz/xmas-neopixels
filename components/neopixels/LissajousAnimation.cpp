#include <cstdint>
#include <color.hpp>
#include <led_strip.hpp>
#include <collections.hpp>
#include <utils.hpp>
#include <math_utils.hpp>
#include <random.hpp>
#include <cstring>
#ifndef UNIT_TEST
 #include <esp_log.h>
#else
 #define ESP_LOGI(tag,format,...)
#endif
#include <LissajousAnimation.hpp>

namespace Neopixel
{

LissajousAnimation::LissajousAnimation(LedStrip *strip, int datasize, void *data, const Strips* lines, RandomGenerator* rand) : 
    strip(strip),lines(lines),rand(rand)
{
    delay_ms = decode<uint16_t>(data);
    fade_delay_ms = decode<uint16_t>(data);
    fading_factor = decode<uint8_t>(data);
    particle_draw_mode = decode<uint8_t>(data);
    n_particles = decode<uint8_t>(data);
    particles = new Particle[n_particles];
    datasize -= 7;
    for (int i=0;i<n_particles;++i)
    {
        if (datasize >= Particle::params_size)
        {
            decode_particle(particles[i],data);
            datasize -= Particle::params_size;
        }
        else
        {
            n_particles = i;
            break;
        }
    }
    totalPixels = strip->getLength();
    ms_to_fade = 0;
    nmax = lines->getLongestLine()
    ESP_LOGI("LissajousAnimation", "LissajousAnimation : delay %d fade_delay %d", delay_ms, fade_delay_ms);
}
LissajousAnimation::~LissajousAnimation()
{
    delete[] particles;
}
void LissajousAnimation::decode_particle(Particle& p,void*& data)
{
    p.omega_x = decode<uint16_t>(data);
    p.omega_y = decode<uint16_t>(data);
    p.ampl_x  = decode<uint16_t>(data);
    p.ampl_y  = decode<uint16_t>(data);
    p.phase_x = decode<uint16_t>(data);
    p.phase_y = decode<uint16_t>(data);
    p.hue_min = decode<uint16_t>(data);
    p.hue_max = decode<uint16_t>(data);
    p.hue_inc = decode<uint8_t>(data);
    p.hue_mode = decode<uint8_t>(data);
}
std::tuple<uint16_t,uint16_t,uint16_t>  LissajousAnimation::update_particle(Particle& p)
{
    //x and y must be properly clamped!
    return {0,0,0};
}
void LissajousAnimation::step()
{
    auto * pixels = strip->getBuffer();
    ms_to_fade += delay_ms;
    if (ms_to_fade > fade_delay_ms)
    {
        ms_to_fade = 0;
        fade_all(pixels, totalPixels, fading_factor);
    }
    
    for (int i=0;i<n_particles;++i)
    {
        auto [x,y,hue] = update_particle(particles[i]);
        InterpolatedPoint ip = lines->getPoint2D(x,y,nmax);
        HSV hsv {hue,255,0};
        for (int j=0;j<4;++j)
        {
            hsv.v = ip.value[j];
            auto & px = pixels[ip.idx[j]];
            px = sat_add(px, hsv.toRGB());
        }
    }
    strip->refresh();
}
}
