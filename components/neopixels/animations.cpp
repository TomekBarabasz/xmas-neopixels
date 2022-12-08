#include <animation.hpp>
#include <cstdint>
#include <led_strip.hpp>
#include <utils.hpp>
#include <math_utils.h>
#include <color.hpp>
#include <random.hpp>
#include <RandomWalkAnimation.hpp>

namespace Neopixel
{
struct Colortest : public Animation
{
    LedStrip *strip;
    uint16_t delay_ms;
    int current_position {0};
    int current_color {0};
    
    static constexpr int n_colors = 3;
    static constexpr RGB rgb[n_colors] = { {255,0,0}, {0, 255,0}, {0,0,255} };

    Colortest(LedStrip *strip_, int datasize, void *data) : strip(strip_)
    {
        delay_ms = decode<uint16_t>(data);
        ESP_LOGI("Colortest-app", "colortest animation : delay %d", delay_ms);
        const auto size = strip->getLength();
        strip->fillPixelsRGB(0,size,{0,0,0});
        strip->setPixelsRGB(current_position,1,rgb+current_color);
    }
    uint16_t get_delay_ms() override { return delay_ms;}
    void step() override
    {
        RGB black = {0,0,0};
        const auto size = strip->getLength();

        if (++current_color >= n_colors)
        {
            current_color = 0;
            strip->setPixelsRGB(current_position,1,&black);
            if (++current_position >= size) {
                current_position = 0;
            }
        }
        strip->setPixelsRGB(current_position,1,rgb+current_color);
    }
};

#if 0
struct Cylon : Animation
{
    LedStrip* strip;
    uint16_t delay_snake_ms, delay_bkg_ms;
    int8_t inc_snake, inc_backgroud;
    uint8_t fade;
    HSV background;

    Cylon(LedStrip *strip_, int datasize, void *data) : strip(strip_)
    {
        delay_snake_ms = decode<uint16_t>(data);
        delay_bkg_ms = decode<uint16_t>(data);
        inc_snake = decode<int8_t>(data);
        inc_backgroud = decode<int8_t>(data);
        fade = decode<uint8_t>(data);
        auto background_hsv = decode<uint32_t>(data);
        background = {uint16_t(background_hsv>>16), uint8_t((background_hsv>>8)&0xFF), uint8_t(background_hsv&0xFF)};

        ESP_LOGI(TAG, "Cylon animation : snake delay %d background delay %d snake inc %d background inc %d fade %d background hsv %x", delay_snake_ms, delay_bkg_ms, inc_snake, inc_backgroud, fade, background_hsv);
    }
    uint16_t get_delay_ms() override { return delay_ms;}
    void run() override
    {
        const auto size = strip->getLength();
        HSV hsv {0,255,255}, bkg_hsv = background;
        RGB bkg_rgb;
        int count = size;
        int i = 0;
        int dir = +1;
        int snake_delay = delay_snake_ms;
        int bkg_delay = delay_bkg_ms;
        const uint16_t delay = min(delay_snake_ms, delay_bkg_ms);

        bkg_rgb = bkg_hsv.toRGB();
        strip->fillPixelsRGB(0,size,bkg_rgb);

        for(;;)
        {
            while(count-->0)
            {
                auto buffer = strip->getBuffer();
                for (int j=0; j<size;++j) {
                    buffer[j] = buffer[j].mix(bkg_rgb, fade);
                }
                strip->fillPixelsRGB(i,1,hsv.toRGB());
                strip->refresh();
                
                vTaskDelay(pdMS_TO_TICKS(delay));
                snake_delay -= delay;
                if (snake_delay <=0) {
                    i += dir;
                    int hsv_h = hsv.h + inc_snake;
                    if (hsv_h>360) hsv.h = uint16_t(hsv_h - 360);
                    else if (hsv_h<0) hsv.h = uint16_t(hsv_h + 360);
                    else hsv.h = uint16_t(hsv_h);
                    snake_delay = delay_snake_ms;
                }
                bkg_delay -= delay;
                if (bkg_delay <= 0) {
                    int bkg_hsv_h = bkg_hsv.h + inc_backgroud;
                    if (bkg_hsv_h>360) bkg_hsv.h = uint16_t(bkg_hsv_h - 360);
                    else if (bkg_hsv_h<0) bkg_hsv.h = uint16_t(bkg_hsv_h + 360);
                    else bkg_hsv.h = uint16_t(bkg_hsv_h);
                    bkg_rgb = bkg_hsv.toRGB();
                    bkg_delay = delay_bkg_ms;
                }
            }
            if (dir>0) {
                dir = -1;
                --i;
            } else {
                dir = +1;
                ++i;
            }
            count = size;
        }
    }
};
#endif

struct Random : Animation
{
    LedStrip* strip;
    uint16_t delay_new_ms;
    uint16_t delay_fade_ms;
    uint16_t delay_ms;
    uint8_t fade;
    int ms_to_new, ms_to_fade;
    RandomGenerator* random;

    Random(LedStrip *strip_, int datasize, void *data, const Strips*,RandomGenerator*rng) : strip(strip_),random(rng)
    {
        delay_new_ms = decode<uint16_t>(data);
        delay_fade_ms = decode<uint16_t>(data);
        fade = decode<uint8_t>(data);
        delay_ms = min(delay_new_ms, delay_fade_ms);
        ESP_LOGI("Random-app", "Random animation : delay_new_ms %d delay_fade_ms %d fade %d", delay_new_ms, delay_fade_ms, fade);
        ms_to_new = delay_new_ms;
        ms_to_fade = delay_fade_ms;
    }
    uint16_t get_delay_ms() override { return delay_ms; }
    void step() override
    {
        const auto size = strip->getLength();
        ms_to_new -= delay_ms;
        if (ms_to_new <= 0) 
        {
            uint32_t rnd = random->make_random();
            HSV hsv = {uint16_t(rnd % 360), 255, 255};
            strip->fillPixelsRGB((rnd>>8)%size, 1, hsv.toRGB());
            ms_to_new = delay_new_ms;
        }
        ms_to_fade -= delay_ms;
        if (ms_to_fade <=0)
        {
            fade_all(strip->getBuffer(), size, fade);
            ms_to_fade = delay_fade_ms;
        }
        strip->refresh();
    }
};

struct Fire : Animation
{
    LedStrip* strip;
    uint16_t delay_ms;
    // COOLING: How much does the air cool as it rises?
    // Less cooling = taller flames.  More cooling = shorter flames.
    // Default 50, suggested range 20-100 
    uint8_t cooling;

    // SPARKING: What chance (out of 255) is there that a new spark will be lit?
    // Higher chance = more roaring fire.  Lower chance = more flickery fire.
    // Default 120, suggested range 50-200.
    uint8_t sparking;

    uint8_t direction;
    uint16_t size;
    uint8_t *heat;
    RandomGenerator *random;

    Fire(LedStrip *strip_, int datasize, void *data, const Strips* spatialConfig, RandomGenerator *random) : strip(strip_), random(random)
    {
        delay_ms = decode<uint16_t>(data);
        cooling = decode<uint8_t>(data);
        sparking = decode<uint8_t>(data);
        direction = decode<uint8_t>(data);

        size = strip->getLength();
        heat = new uint8_t[size];
        ESP_LOGI("Fire-animation", "Fire animation : delay %d cooling %d sparking %d direction %d", delay_ms, cooling, sparking, direction);
    }
    ~Fire()
    {
        delete[] heat;
    }
    uint16_t get_delay_ms() override { return delay_ms; }
    void step() override
    {
        const uint8_t cooling_factor = (cooling*10) / size + 2;

        uint32_t rnd = 0;
        // Step 1.  Cool down every cell
        for(int i=0;i<size;++i)
        {
            if (0==rnd) rnd = random->make_random();
            heat[i] = saturated_sub(heat[i], uint8_t((rnd&0xFF) % cooling_factor));
            rnd >>= 8;
        }

        // Step 2.  Heat from each cell drifts 'up' and diffuses a little
        for( int k= size - 1; k >= 2; k--) {
            heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
        }

        // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
        rnd = random->make_random();
        if( (rnd&0xFF) < sparking ) 
        {
            rnd >>= 8;
            const uint16_t pos = uint16_t(rnd&0xFFFF) % size;
            rnd >>= 16;
            const uint8_t const_add = 160;
            const uint8_t random_add = 255-const_add;
            heat[pos] = saturated_add( heat[pos], uint8_t(const_add + uint8_t(rnd&0xFF)%random_add));
        }

        // Step 4.  Map from heat cells to LED colors
        for( int j = 0; j < size; j++) 
        {
            const int pos = (direction==0 ? j : size-1-j);
            strip->fillPixelsRGB(pos,1,HeatColor( heat[j] ));
        }
        strip->refresh();
    }
    RGB HeatColor(uint8_t temperature)
    {
        // Scale 'heat' down from 0-255 to 0-191,
        // which can then be easily divided into three
        // equal 'thirds' of 64 units each.
        const uint8_t t192 = scale8_video( temperature, 191);

        // calculate a value that ramps up from
        // zero to 255 in each 'third' of the scale.
        const uint8_t heatramp = (t192 & 0x3F) << 2; // 0..63, scale up to 0..252

        // now figure out which third of the spectrum we're in:
        if( t192 & 0x80) {
            // we're in the hottest third
            return {255,255,heatramp};
        } else if( t192 & 0x40 ) {
            // we're in the middle third
            return {255,heatramp, 0};
        } else {
            // we're in the coolest third
            return {heatramp, 0, 0};
        }
    }
};

struct Wave : Animation
{
    LedStrip* strip;
    uint16_t delay_ms;
    uint8_t inc;
    uint8_t direction;
    uint16_t size;
    int start_hue;

    Wave(LedStrip *strip_, int datasize, void *data) : strip(strip_)
    {
        delay_ms  = decode_safe<uint16_t>(data, datasize, 500);
        inc       = decode_safe<uint8_t> (data, datasize, 1);
        direction = decode_safe<uint8_t> (data, datasize, 0);
        size = strip->getLength();
        ESP_LOGI("Wave-animation", "Wave animation : delay %d inc %d direction %d", delay_ms, inc, direction);
        start_hue = rainbow(0, inc);
        if (direction==0) start_hue=0;
    }
    uint16_t get_delay_ms() override { return delay_ms; }
    void step() override
    {
        auto * buffer = strip->getBuffer();
        if (direction==0)
        {
            //memmove(buffer+1,buffer,sizeof(RGB)*(size-1));
            for (int j=size-1;j>0;--j) {
                buffer[j] = buffer[j-1];
            }
            start_hue -= inc;
            if (start_hue < 0) start_hue += 360;
            HSV hsv = {uint16_t(start_hue), 255,255};
            strip->fillPixelsRGB(0,1,hsv.toRGB());
        }
        else
        {
            //memmove(buffer,buffer+1,sizeof(RGB)*(size-1));
            for (int j=0;j<size-1;++j) {
                buffer[j] = buffer[j+1];
            }
            start_hue += inc;
            if (start_hue > 360) start_hue -= 360;
            HSV hsv = {uint16_t(start_hue), 255,255};
            strip->fillPixelsRGB(size-1,1,hsv.toRGB());
        }
        strip->refresh();
    }
    uint16_t rainbow(uint16_t start_hue, uint8_t inc_hue)
    {   
        HSV hsv = {start_hue,255,255};
        for (int i=0;i<size;++i) {
            strip->fillPixelsRGB(i,1,hsv.toRGB());
            hsv.h += inc_hue;
        }
        return hsv.h;
    }
};

#if 0

struct RotatingRings2 : Animation
{  
    LedStrip* strip;
    uint16_t delay_ms;
    uint16_t ring_move;
    uint8_t ring_inc,base_inc;
    RGB base_color, ring_color;
    uint8_t fade;

    RotatingRings2(LedStrip *strip_, int datasize, void *data) : strip(strip_)
    {
        delay_ms = decode<uint16_t>(data);
        ring_move = decode<uint16_t>(data);
        ring_inc = decode<uint8_t>(data);
        base_inc = decode<uint8_t>(data);
        uint32_t base_color_ = decode<uint32_t>(data);
        base_color = {uint8_t(base_color_>>16), uint8_t(base_color_>>8), uint8_t(base_color_&0xFF)};    //0x00RRGGBB
        uint32_t ring_color_ = decode<uint32_t>(data);
        ring_color = {uint8_t(ring_color_>>16), uint8_t(ring_color_>>8), uint8_t(ring_color_&0xFF)};
        fade = decode<uint8_t>(data);
        ESP_LOGI(TAG, "RotatingRings animation : delay ms %d ring_move %d ring inc %x base inc %x ring color %x base color %x", delay_ms, ring_move, ring_inc, base_inc, ring_color_, base_color_);
    }
    void run() override
    {
    #if 0
        const auto size = strip->getLength();
        auto buffer = strip->getBuffer();
        int current_ri = 0, dir = 1;
        int ms_to_move_ring = 0;
        strip->fillPixelsRGB(0,size,base_color);
        for(;;)
        {
            if (ms_to_move_ring <= 0) 
            {
                current_ri += dir;
                if (current_ri >= NumRings) {
                    current_ri = NumRings-2;
                    dir = -1;
                } else if (current_ri < 0) {
                    current_ri = 1;
                    dir = +1;
                }
                const Ring& ring = Rings[current_ri];
                strip->fillPixelsRGB(ring.first,ring.length,ring_color);
                ms_to_move_ring = move_delay_ms;
            }
            for(int ri=0;ri<NumRings;++ri) {
                if (ri!=current_ri) {
                    for (int i=0;i<r[1];++i) {
                        buffer[r[0] + i].mix(base_color, fade);
                    }
                }
            }
            strip->refresh();
            vTaskDelay(pdMS_TO_TICKS(fade_delay_ms));
            ms_to_move_ring -=fade_delay_ms;
        }
    #endif
    }
};

struct VerticalRings : Animation
{  
    static constexpr int NumRings = sizeof(Rings)/sizeof(Rings[0]);
    LedStrip* strip;
    uint16_t delay_ms;
    int8_t ring_move;
    int8_t ring_step;

    VerticalRings(LedStrip *strip_, int datasize, void *data) : strip(strip_)
    {
        delay_ms  = decode_safe<uint16_t>(data,datasize,100);
        ring_step = decode_safe<int16_t>(data,datasize,10);
        ring_move = decode_safe<int16_t>(data,datasize,10);
        ESP_LOGI(TAG, "VerticalRings animation : delay ms %d ring_move %d ring step %d", delay_ms, ring_move, ring_step);
    }
    void run() override
    {
        HSV hsv = {0,255,255};
        int hue[NumRings] = {};
        for (int r=0;r<NumRings;++r) {
            hue[r] = ring_step * r;
            if (hue[r]>360)     hue[r] -= 360;
            else if (hue[r]<0)  hue[r] += 360;
        }
        
        for(;;)
        {
            for (int r=0;r<NumRings;++r) {
                hsv = {uint16_t(hue[r]), 255,255};
                auto & ring = Rings[r];
                strip->fillPixelsRGB(ring.first,ring.count,hsv.toRGB());
                hue[r] += ring_move;
                if (hue[r]>360)     hue[r] -= 360;
                else if (hue[r]<0)  hue[r] += 360;
            }
            strip->refresh();
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }
    }
};

struct RotatingRings : Animation
{  
    static constexpr int NumRings = sizeof(Rings)/sizeof(Rings[0]);
    LedStrip* strip;
    uint16_t delay_ms;
    int16_t angular_speed;  //deg/sec
    int8_t num_hues;
    RGB tmp[250];

    RotatingRings(LedStrip *strip_, int datasize, void *data) : strip(strip_)
    {
        delay_ms  = decode_safe<uint16_t>(data,datasize,100);
        angular_speed = decode_safe<int16_t>(data,datasize,10);
        num_hues = decode_safe<uint8_t>(data,datasize,90);
        ESP_LOGI(TAG, "RotatingRings animation : delay ms %d angular_speed %d num_hues %d", delay_ms, angular_speed, num_hues);
    }
    void run() override
    {
        int rot[NumRings] = {0};
        const int deg_per_tick = 256 * angular_speed * delay_ms / 1000;
        
        for (auto & ring : Rings) 
        {
            const int hinc = 360 * 256 / ring.count * (0==ring.dir ? 1 : -1);
            int h = (0==ring.dir ? 0 : 360*256);
            
            HSV hsv = {0,255,255};
            for (int i=ring.first; i<ring.first+ring.count;++i) {
                hsv.h = uint16_t( ((h>>8) / num_hues) * num_hues );
                strip->fillPixelsRGB(i,1,hsv.toRGB());
                h += hinc;
            }
        }
        strip->refresh();
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
        for(;;)
        {
            auto buffer = strip->getBuffer();
            for (int i=0;i<NumRings;++i) 
            {
                auto & ring = Rings[i];
                const int step = 360 * 256 / ring.count;
                rot[i] += deg_per_tick;
                if (rot[i] > step) 
                {
                    const int n = rot[i] / step;
                    rot[i] -= n*step;

                    if (0==ring.dir) {
                        rotLeft(ring.first, ring.count, n, buffer, tmp);
                    }
                    else {
                        rotRight(ring.first, ring.count, n, buffer, tmp);
                    }
                }
            }        
            strip->refresh();
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }
    }
};

struct RotatingStrips : Animation
{  
    LedStrip* strip;
    uint16_t delay_ms;
    int16_t angular_speed;  //deg/sec
    int8_t hue_quant;
    RGB tmp[250];

    RotatingStrips(LedStrip *strip_, int datasize, void *data) : strip(strip_)
    {
        delay_ms  = decode_safe<uint16_t>(data,datasize,100);
        angular_speed = decode_safe<int16_t>(data,datasize,10);
        hue_quant = decode_safe<uint8_t>(data,datasize,90);
        ESP_LOGI(TAG, "RotatingRings strips : delay ms %d angular_speed %d hue_quant %d", delay_ms, angular_speed, hue_quant);
    }
    void run() override
    {
        const int deg_per_tick = 256 * angular_speed * delay_ms / 1000;
        const int hinc = 360 * 256 / NumStrips;

        uint16_t hue[NumStrips];
        int rot = 0;
        int h = 0;

        for (int i=0;i<NumStrips;++i) 
        {
            hue[i] = uint16_t( ((h>>8) / hue_quant) * hue_quant );
            h += hinc;
        }
        for(;;)
        {
            int i0   = (rot / hinc) / 256;
            uint8_t fade = (rot % hinc) / 256;
            if (i0 >= NumStrips) i0 -= NumStrips;
            else if (i0 < 0) i0 += NumStrips;
            int i1 = i0 + 1;
            if (i1 >= NumStrips) i1 -= NumStrips;
            else if (i1 < 0) i1 += NumStrips;

            for (int i=0;i<NumStrips;++i) 
            {
                auto & vstrip = Strips[i];
                h = ( (hue[i1] - hue[i0]) * fade) / 256;
                HSV hsv = { uint16_t(h) ,255,255};
                strip->fillPixelsRGB(vstrip.first,vstrip.count,hsv.toRGB());
                i0 += 1;
                if (i0 >= NumStrips) i0 -= NumStrips;
                i1 += 1;
                if (i1 >= NumStrips) i1 -= NumStrips;
            }
            rot += deg_per_tick;
            if (rot > 256*256)       rot -= 256*256;
            else if (rot < -256*256) rot += 256*256;

            strip->refresh();
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }
    }
};

/* FallingStars parameters:
** H: <refresh delay>
** B: <new star chance>
** B: <star length>
** B: <star speed>  leds/<refresh_delay>
** B: <background change speed> <steps/sec>, full change = 255 steps
** B: <bacground change mode : 0 cyclic 1 pingpong>
** B: <num bacground colors>
** I: <background color>...
*/
struct FallingStars : Animation
{  
    LedStrip* strip;
    uint16_t delay_ms;
    uint8_t new_star_chance, star_length, star_speed, bkg_change_rate;
    uint8_t bkg_change_mode, num_bkg_colors;
    RGB *bkg_colors;

    int bkg_i0 = 0, bkg_i1=1;
    int bkg_fade = 0;
    int bkg_dir = 1;

    struct Star
    {
    };
    FallingStars(LedStrip *strip_, int datasize, void *data) : strip(strip_)
    {
        delay_ms  = decode_safe<uint16_t>(data,datasize,100);
        new_star_chance = decode_safe<uint8_t>(data,datasize,125);
        star_length = decode_safe<uint8_t>(data,datasize,5);
        star_speed = decode_safe<uint8_t>(data,datasize,1);
        bkg_change_rate = decode_safe<uint8_t>(data,datasize,50);
        bkg_change_mode = decode_safe<uint8_t>(data,datasize,0);
        num_bkg_colors = decode_safe<uint8_t>(data,datasize,2);
        bkg_colors = new RGB[num_bkg_colors];
        for (int i=0;i<num_bkg_colors;++i)
        {
            uint32_t ihsv = decode_safe<uint32_t>(data,datasize,0);
            HSV hsv = {uint16_t(ihsv>>16), uint8_t((ihsv>>8)&0xFF), uint8_t(ihsv&0xFF)};
            bkg_colors[i] = hsv.toRGB();
        }

        ESP_LOGI(TAG, "FallingStars animation : delay ms %d ... ", delay_ms);
    }
    ~FallingStars()
    { 
        delete[] bkg_colors;
    }
    void doBackground()
    {
        if (1==num_bkg_colors) return;

        //0 1 2 3  (4 bkg colors)
        bkg_fade += bkg_change_rate;
        if (bkg_fade > 255) 
        {
            bkg_fade -= 255;
            bkg_i0 += bkg_dir;
            bkg_i1 += bkg_dir;
            //(0,1) (1,2) ->
            if (bkg_dir > 0 && bkg_i0 >= num_bkg_colors-1) 
            {
                if (0==bkg_change_mode) {
                    bkg_i0 = 0;
                    bkg_i1 = 1;
                } else {
                    bkg_i0 = num_bkg_colors-1;
                    bkg_i1 = num_bkg_colors-2;
                    bkg_dir = -1;
                }
            }
            else if (bkg_dir < 0 && bkg_i0==1) 
            {
                if (0==bkg_change_mode) {
                    bkg_i0 = 0;
                    bkg_i1 = 1;
                } else {
                    bkg_i0 = num_bkg_colors-1;
                    bkg_i1 = num_bkg_colors-2;
                    bkg_dir = -1;
                }
            }
        }
    }
    void doStars()
    {

    }
    void run() override
    {
        strip->fillPixelsRGB(0, strip->getLength(), *bkg_colors);
        for(;;)
        {
            doBackground();
            doStars();
            strip->refresh();
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }
    }
};

/* VerticalWave parameters:
** H:<refresh delay>
** B:<hue inc>
** B:<direction> 0: up 1:down
*/
struct VerticalWave : Animation
{  
    LedStrip* strip;
    uint16_t delay_ms;
    uint8_t hue_inc;
    uint8_t direction;

    VerticalWave(LedStrip *strip_, int datasize, void *data) : strip(strip_)
    {
        delay_ms  = decode_safe<uint16_t>(data,datasize,100);
        hue_inc = decode_safe<uint8_t>(data,datasize,10);
        direction = decode_safe<uint8_t>(data,datasize,0);
        ESP_LOGI(TAG, "VerticalWave animation: delay ms %d hue_inc %d direction %d", delay_ms, hue_inc, direction);
    }
    void run() override
    {
        for (auto & s : Strips)
        {
            HSV hsv = {uint16_t(0==s.dir ? 0 : s.count*hue_inc),255,255};
            int inc = (0==s.dir ? hue_inc : -hue_inc);
            for (int i = s.first; i < s.first+s.count; ++i) {
                strip->fillPixelsRGB(i,1,hsv.toRGB());
                hsv.h += inc;
            }
        }
        strip->refresh();
        for(;;)
        {
            auto * buffer = strip->getBuffer();
            RGB rgb;
            for (auto & s : Strips)
            {
                if (direction == s.dir) {
                    rotLeft(s.first, s.count, 1, buffer, &rgb);
                }else {
                    rotRight(s.first, s.count, 1, buffer, &rgb);
                }
            }
            strip->refresh();
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }
    }
};

/* VerticalWave parameters:
** H:<refresh delay>
** B:<hue inc>
** B:<direction> 0: up 1:down
*/
struct HorizontalWave : Animation
{  
    LedStrip* strip;
    uint16_t delay_ms;
    uint8_t hue_step,hue_inc;
    uint8_t direction;
    int16_t stripHue[NumStrips];

    HorizontalWave(LedStrip *strip_, int datasize, void *data) : strip(strip_)
    {
        delay_ms  = decode_safe<uint16_t>(data,datasize,100);
        hue_step = decode_safe<uint8_t>(data,datasize,10);
        hue_inc = decode_safe<uint8_t>(data,datasize,1);
        direction = decode_safe<uint8_t>(data,datasize,0);
        ESP_LOGI(TAG, "VerticalWave animation: delay ms %d hue_inc %d direction %d", delay_ms, hue_inc, direction);
    }
    void run() override
    {
        stripHue[0]=0;
        for (int i=1;i<NumStrips;++i){
            stripHue[i] = stripHue[i-1] + hue_step;
        }
        strip->fillPixelsRGB(0,strip->getLength(),{0,0,0});
        for(;;)
        {
            HSV hsv = {0,255,255};
            auto *hue = stripHue;
            for (auto & s : Strips)
            {
                hsv.v = *hue;
                strip->fillPixelsRGB(s.first,s.count,hsv.toRGB());
                *hue += hue_inc;
                if (*hue > 360) *hue-=360;
                else if (*hue < 0) *hue += 360;
                ++hue;
            }
            strip->refresh();
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }
    }
};

#endif

Animation* Animation::create(LedStrip*strip,int animation_id, void* data, const Strips* strips,RandomGenerator*random)
{
    const uint16_t animation_data_size = decode<uint16_t>(data);
    switch(animation_id)
    {
        default:
        case 0: return new Colortest(strip, animation_data_size, data);
        //case 1: return new Cylon(strip, animation_data_size, data);
        //case 2: return new Reel100(strip, animation_data_size, data);
        case 3: return new Random(strip, animation_data_size, data, strips, random);
        case 4: return new Fire(strip, animation_data_size, data, strips, random);
        case 5: return new Wave(strip, animation_data_size, data);
        //case 6: return new VerticalRings(strip, animation_data_size, data);
        //case 7: return new RotatingRings(strip, animation_data_size, data);
        //case 8: return new RotatingStrips(strip, animation_data_size, data);
        //case 9: return new FallingStars(strip, animation_data_size, data);
        //case 10: return new VerticalWave(strip, animation_data_size, data);
        //case 11: return new HorizontalWave(strip, animation_data_size, data);
        case 12: return new RandomWalkAnimation(strip, animation_data_size, data, strips, random);
    }
}

}
