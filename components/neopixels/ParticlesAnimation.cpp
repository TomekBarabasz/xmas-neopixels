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
#include <ParticlesAnimation.hpp>

namespace Neopixel
{
int getSizeByType(int type)
{
    switch(type)
    {
        case ParticleType::Lissajous : return LissajousParticle::getBufferSize();
        case ParticleType::Polar     : return PolarParticle::getBufferSize();
        default: return 0;
    }
}
Particle* Particle::load(void*& data, int& datasize)
{
    auto type = decode<uint8_t>(data);
    auto size = getSizeByType(type);
    if (datasize >= size)
    {
        Particle *p {nullptr};
        switch(type)
        {
            case 0: 
                p = LissajousParticle::load(data);
                break;
            case 1:
                p = PolarParticle::load(data);
                break;
        }
        datasize -= size;
        return p;
    }
    return nullptr;
}
ParticleAnimation::ParticleAnimation(LedStrip *strip, int datasize, void *data, const Strips* lines, RandomGenerator* rand) : 
    strip(strip),lines(lines),rand(rand)
{
    totalPixels = strip->getLength();
    ms_to_fade = 0;
    nmax = lines->getLongestLine();

    delay_ms = decode<uint16_t>(data);
    fade_delay_ms = decode<uint16_t>(data);
    fading_factor = decode<uint8_t>(data);
    particle_draw_mode = decode<uint8_t>(data);
    n_particles = decode<uint8_t>(data);    
    particles = new Particle*[n_particles];
    datasize -= 7;

    for (int i=0;i<n_particles;++i)
    {
        auto p = Particle::load(data,datasize);

        if (p != nullptr)
        {
            particles[i] = p;
        }
        else
        {
            n_particles = i;
            break;
        }
    }

    center_x = lines->count * 256/2;
    center_y = nmax * 256/2;
    
    ESP_LOGI("LissajousAnimation", "LissajousAnimation : delay %d fade_delay %d", delay_ms, fade_delay_ms);
}
ParticleAnimation::~ParticleAnimation()
{
    for (int i=0;i<n_particles;++i){
        delete particles[i];
    }
    delete[] particles;
}
LissajousParticle* LissajousParticle::load(void*& data)
{
    auto ptr = new LissajousParticle;
    auto & p = *ptr;
    p.omega_x = loadValueAnimation<uint16_t>(data);
    p.omega_y = loadValueAnimation<uint16_t>(data);
    p.ampl_x  = loadValueAnimation<uint16_t>(data);
    p.ampl_y  = loadValueAnimation<uint16_t>(data);
    p.phase_x = loadValueAnimation<int16_t>(data);
    p.phase_y = loadValueAnimation<int16_t>(data);
    p.hue     = loadValueAnimation<uint16_t>(data);

    p.time = 0;

    return ptr;
}
std::tuple<int16_t,int16_t,uint16_t>  LissajousParticle::update(uint16_t dt,RandomGenerator* rnd)
{
    auto omx = omega_x->nextValue(rnd);
    auto omy = omega_y->nextValue(rnd);
    auto phx = phase_x->nextValue(rnd);
    auto phy = phase_y->nextValue(rnd);
    int32_t ax  = ampl_x->nextValue(rnd);
    int32_t ay  = ampl_y->nextValue(rnd);

    /** @param theta input angle from 0-65535, 65535 is 2Pi
    ** @returns sin of theta, value between -32767 to 32767*/

    time += dt;

    int16_t x = static_cast<int16_t>((ax * sin_16b( (omx * time + phx) )) >> 15);
    int16_t y = static_cast<int16_t>((ay * cos_16b( (omy * time + phy) )) >> 15);  

    return {x,y,hue->nextValue(rnd)};
}
LissajousParticle::~LissajousParticle()
{ 
    delete hue;
    delete omega_x;
    delete omega_y;
    delete phase_x;
    delete phase_y;
    delete ampl_x;
    delete ampl_y;
}
PolarParticle* PolarParticle::load(void*& data)
{
    auto ptr = new PolarParticle;
    auto & p = *ptr;

    p.angle  = loadValueAnimation<uint16_t>(data);
    p.radius = loadValueAnimation<uint16_t>(data);
    p.hue    = loadValueAnimation<uint16_t>(data);

    return ptr;
}
std::tuple<int16_t,int16_t,uint16_t>  PolarParticle::update(uint16_t ms,RandomGenerator* rnd)
{
    auto a = angle->nextValue(rnd);
    int32_t r = radius->nextValue(rnd);

    int16_t x = static_cast<int16_t>((r * cos_16b(a)) >> 15);
    int16_t y = static_cast<int16_t>((r * sin_16b(a)) >> 15);

    return {x,y,hue->nextValue(rnd)};
}
PolarParticle::~PolarParticle()
{
    delete angle;
    delete radius;
    delete hue;
}
void ParticleAnimation::step()
{
    auto * pixels = strip->getBuffer();
    ms_to_fade += delay_ms;
    if (ms_to_fade > fade_delay_ms)
    {
        ms_to_fade = 0;
        fade_all(pixels, totalPixels, fading_factor);
    }

    int16_t ymax = makeFixpoint88(nmax,0);
    int16_t xmax = makeFixpoint88(lines->count,0);

    for (int i=0;i<n_particles;++i)
    {
        auto [x,y,hue] = particles[i]->update(1,rand);
        x += center_x;
        y += center_y;
        if (x < 0 || y < 0 || x > xmax || y > ymax) continue;
        InterpolatedPoint ip = lines->getPoint2D(x,y,nmax);
        HSV hsv {hue,255,0};
        for (int j=0;j<ip.n_points;++j)
        {
            hsv.v = ip.value[j];
            auto & px = pixels[ip.idx[j]];
            px = sat_add(px, hsv.toRGB());
        }
    }
    strip->refresh();
}
}
