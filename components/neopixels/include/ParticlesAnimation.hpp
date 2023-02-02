#pragma once
#include <tuple>
#include "animation.hpp"
#include <value_animation.hpp>

namespace Neopixel
{
struct LedStrip;
struct Strips;
struct Subset;
struct RandomGenerator;

enum ParticleType : int8_t
{
    Lissajous = 0,
    Polar
};
struct Particle
{
    static Particle* load(void*& data, int& datasize);
    virtual ~Particle(){}
    virtual std::tuple<int16_t,int16_t,uint16_t> update(uint16_t ms,RandomGenerator*) = 0;
    uint16_t center_x,center_y;
};
struct LissajousParticle : Particle
{
    U16ValueAnimation *omega_x,*omega_y;
    U16ValueAnimation *ampl_x,*ampl_y;
    I16ValueAnimation *phase_x,*phase_y;
    U16ValueAnimation *hue;

    static LissajousParticle* load(void*&);   
    static size_t getBufferSize() { return 18; }
    ~LissajousParticle() override;
    std::tuple<int16_t,int16_t,uint16_t> update(uint16_t,RandomGenerator*) override;
    uint16_t time;
};
struct PolarParticle : Particle
{
    U16ValueAnimation *angle;
    U16ValueAnimation *radius;
    U16ValueAnimation *hue;

    static PolarParticle* load(void*&);    
    static size_t getBufferSize() { return 0; }
    ~PolarParticle() override;
    std::tuple<int16_t,int16_t,uint16_t> update(uint16_t,RandomGenerator*) override;
};

class ParticleAnimation : public Animation
{
public:
    ParticleAnimation(LedStrip *strip_, int datasize, void *data,const Strips*, RandomGenerator*);
    ~ParticleAnimation();
    void step() override;
    uint16_t get_delay_ms() override { return delay_ms; }

protected:
    LedStrip* strip = {nullptr};
    const Strips* lines;
    RandomGenerator * rand;
    uint16_t delay_ms,fade_delay_ms;
    uint16_t ms_to_fade;
    uint8_t fading_factor;
    uint8_t particle_draw_mode;
    uint8_t n_particles;
    Particle** particles;

    uint16_t totalPixels,nmax;
    uint16_t center_x,center_y;
};
}
