#pragma once
#include <tuple>
#include "animation.hpp"

namespace Neopixel
{
struct LedStrip;
struct Strips;
struct Subset;
struct RandomGenerator;
struct Particle
{
    uint16_t omega_x,omega_y,ampl_x,ampl_y,phase_x,phase_y;
    uint16_t hue_min,hue_max;
    uint8_t hue_inc,hue_mode;
    static constexpr size_t params_size = 18;
    uint16_t ph_x,ph_y,time,hue;
};
class LissajousAnimation : public Animation
{
public:
    LissajousAnimation(LedStrip *strip_, int datasize, void *data,const Strips*, RandomGenerator*);
    ~LissajousAnimation();
    void step() override;
    uint16_t get_delay_ms() override { return delay_ms; }
    void decode_particle(Particle&,void*&);
    std::tuple<uint16_t,uint16_t,uint16_t> update_particle(Particle&);

protected:
    LedStrip* strip = {nullptr};
    const Strips* lines;
    RandomGenerator * rand;
    uint16_t delay_ms,fade_delay_ms;
    uint16_t ms_to_fade;
    uint8_t fading_factor;
    uint8_t particle_draw_mode;
    uint8_t n_particles;
    Particle *particles {nullptr};

    uint16_t totalPixels,nmax;
};
}
