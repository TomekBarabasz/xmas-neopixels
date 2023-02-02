#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <tuple>
#include <ParticlesAnimation.hpp>
#include <led_strip.hpp>
#include <collections.hpp>
#include <random.hpp>
#include <utils.hpp>
#include <math_utils.hpp>
#include <string>
#include <map>
#include <cmath>

using namespace Neopixel;
using namespace ::testing;
using ::testing::Return;
using ParamsMap = std::map<std::string,int>;

namespace
{
struct MockLedStrip : public LedStrip 
{
    MOCK_METHOD( int,  getLength,    (), (const));
    MOCK_METHOD( RGB*, getBuffer,    ());
    MOCK_METHOD( void, setPixelsRGB, (int first, int num, const RGB*));
    MOCK_METHOD( void, fillPixelsRGB,(int first, int num, const RGB&));
    MOCK_METHOD( void, setPixelsHSV, (int first, int num, const HSV*));
    MOCK_METHOD( void, refresh,      (bool wait));
    MOCK_METHOD( void, copyFrontToBack,());
    MOCK_METHOD( bool, waitReady,    (uint32_t timeout_ms));
    MOCK_METHOD( void, release,      ());
};

struct MockRandomGenerator : public RandomGenerator
{
    MOCK_METHOD( uint32_t, make_random, ());
    MOCK_METHOD( void,     make_random_n, (uint32_t *values, int length));
    MOCK_METHOD( void,     release, ());
};

template <typename T>
void encode_prm(const char *name,const ParamsMap& prms, void*& data, T default_value)
{
    auto it = prms.find(name);
    T p = it != prms.end() ? uint16_t(it->second) : default_value;
    encode<T>(data, p);
}

std::tuple<uint8_t*,int> encodeParams(const ParamsMap& prms)
{
    static uint8_t params[12];
    void *p = params;
    encode_prm<uint16_t>("delay_ms",prms,p,20);
    encode_prm<uint16_t>("fade_delay_ms",prms,p,60);

    return {params, (uint8_t*)p-params};
}

template <size_t N>
ParticleAnimation* makeAnimation(Subset (&su)[N], const ParamsMap& prms, MockLedStrip& led_strip, MockRandomGenerator& random)
{
    Strips *pstrips = makeStrips(su);
    auto [params,len] = encodeParams(prms);
    EXPECT_CALL(led_strip, getLength()).Times(1).WillOnce(Return(pstrips->count));
    auto *pa = new ParticleAnimation(&led_strip,len,params,pstrips,&random);
    release(pstrips);
    return pa;
}
}

TEST(Particles, getPoint1D_posdir)
{
    Subset s {10,5,1};
    //indices : 10 11 12 13 14
    constexpr uint16_t nmax = 5;
    
    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(0,0),nmax,s);
    EXPECT_EQ(10,idx0);
    EXPECT_EQ(11,idx1);
    EXPECT_EQ(0,v);}
     
    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(1,0),nmax,s);
    EXPECT_EQ(11,idx0);
    EXPECT_EQ(12,idx1);
    EXPECT_EQ(0,v);}

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(1,127),nmax,s);
    EXPECT_EQ(11,idx0);
    EXPECT_EQ(12,idx1);
    EXPECT_EQ(127,v);}
}

TEST(Particles, getPoint1D_negdir)
{    
    Subset s {10,5,-1};
    //indices : 14 13 12 11 10
    constexpr uint16_t nmax = 5;
    
    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(0,0),nmax,s);
    EXPECT_EQ(14,idx0);
    EXPECT_EQ(13,idx1);
    EXPECT_EQ(0,v);}
     
    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(1,0),nmax,s);
    EXPECT_EQ(13,idx0);
    EXPECT_EQ(12,idx1);
    EXPECT_EQ(0,v);}

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(1,127),nmax,s);
    EXPECT_EQ(13,idx0);
    EXPECT_EQ(12,idx1);
    EXPECT_EQ(127,v);}
}

TEST(Particles, getPoint1D_posdir_stepgr1)
{
    Subset s {10,5,1};
    //indices : 10 11 12 13 14
    constexpr uint16_t nmax = 4;
    /* pos -> idx
     * 0			1			2			3	nmax=4
     * idx
     * 0       1    |     2     |    3      4	n=5  d = (n-1) / (nmax-1) = (5-1)/(4-1)=4/3=1,33
     * 0       1    1.33        2.66        4
     */

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(0,0),nmax,s);
    EXPECT_EQ(10,idx0);
    EXPECT_EQ(11,idx1);
    EXPECT_EQ(0,v);}
     
    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(1,0),nmax,s);
    EXPECT_EQ(11,idx0);
    EXPECT_EQ(12,idx1);
    EXPECT_EQ(255/3,v);}

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(1,127),nmax,s); //1.498 = (1,127/256) = 1.498 * 1.333 = 1.997 = (1,254/256)
    EXPECT_EQ(11,idx0);
    EXPECT_EQ(12,idx1);
    EXPECT_EQ(254,v);}

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(2,0),nmax,s);   //pos 2.0 - > local pos 2.66 -> 256 * .66 -> 170
    EXPECT_EQ(12,idx0);
    EXPECT_EQ(13,idx1);
    EXPECT_EQ(170,v);}
}

TEST(Particles, getPoint1D_posdir_stepless1)
{
    Subset s {10,5,1};
    //indices : 10 11 12 13 14
    constexpr uint16_t nmax = 7;
    /* pos -> idx
     * 0	1   2   3   4   5   6       nmax=7
     * idx
     * 0   1     2      3       4	    n=5  d = (n-1) / (nmax-1) = (5-1)/(7-1)=4/6=0.666
     * 0    1    1.33        2.66        4
     */

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(0,0),nmax,s);
    EXPECT_EQ(10,idx0);
    EXPECT_EQ(11,idx1);
    EXPECT_EQ(0,v);}
     
    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(1,0),nmax,s);   //(1,0/256) * 0.66666 = 0.666666 = (0,170/256)
    EXPECT_EQ(10,idx0);
    EXPECT_EQ(11,idx1);
    EXPECT_EQ(170,v);}

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(1,127),nmax,s); //1.498 = (1,127/256) = 1.498 * 0.666 = 0.998 = (0,255/256)
    EXPECT_EQ(10,idx0);
    EXPECT_EQ(11,idx1);
    EXPECT_EQ(255,v);}

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(2,0),nmax,s);   //pos (2,0/256) * 0.66666  -> 1.33333  = (1,85/256)
    EXPECT_EQ(11,idx0);
    EXPECT_EQ(12,idx1);
    EXPECT_EQ(85,v);}
}

TEST(Particles, getPoint1D_negdir_stepgr1)
{
    Subset s {10,5,-1};
    //indices : 14 13 12 11 10
    constexpr uint16_t nmax = 4;
    /* pos -> idx
     * 0			1			2			3	nmax=4
     * idx
     * 0       1    |     2     |    3      4	n=5  d = (n-1) / (nmax-1) = (5-1)/(4-1)=4/3=1,33
     * 0       1    1.33        2.66        4
     */

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(0,0),nmax,s);
    EXPECT_EQ(14,idx0);
    EXPECT_EQ(13,idx1);
    EXPECT_EQ(0,v);}
     
    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(1,0),nmax,s);
    EXPECT_EQ(13,idx0);
    EXPECT_EQ(12,idx1);
    EXPECT_EQ(255/3,v);}

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(1,127),nmax,s); //1.498 = (1,127/256) = 1.498 * 1.333 = 1.997 = (1,254/256)
    EXPECT_EQ(13,idx0);
    EXPECT_EQ(12,idx1);
    EXPECT_EQ(254,v);}

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(2,0),nmax,s);   //pos 2.0 - > local pos 2.66 -> 256 * .66 -> 170
    EXPECT_EQ(12,idx0);
    EXPECT_EQ(11,idx1);
    EXPECT_EQ(170,v);}
}

TEST(Particles, getPoint1D_negdir_stepless1)
{
    Subset s {10,5,-1};
    //indices : 14 13 12 11 10
    constexpr uint16_t nmax = 7;
    /* pos -> idx
     * 0	1   2   3   4   5   6       nmax=7
     * idx
     * 0   1     2      3       4	    n=5  d = (n-1) / (nmax-1) = (5-1)/(7-1)=4/6=0.666
     * 0    1    1.33        2.66        4
     */

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(0,0),nmax,s);
    EXPECT_EQ(14,idx0);
    EXPECT_EQ(13,idx1);
    EXPECT_EQ(0,v);}
     
    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(1,0),nmax,s);   //(1,0/256) * 0.66666 = 0.666666 = (0,170/256)
    EXPECT_EQ(14,idx0);
    EXPECT_EQ(13,idx1);
    EXPECT_EQ(170,v);}

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(1,127),nmax,s); //1.498 = (1,127/256) = 1.498 * 0.666 = 0.998 = (0,255/256)
    EXPECT_EQ(14,idx0);
    EXPECT_EQ(13,idx1);
    EXPECT_EQ(255,v);}

    {auto [idx0,idx1,v] = Strips::getPoint1D(makeFixpoint88(2,0),nmax,s);   //pos (2,0/256) * 0.66666  -> 1.33333  = (1,85/256)
    EXPECT_EQ(13,idx0);
    EXPECT_EQ(12,idx1);
    EXPECT_EQ(85,v);}
}

TEST(Particles, getPoint2D_neqnmax)
{
    Subset su[] {{0,5,1},{5,5,-1},{10,5,1}};
    Strips *pstrips = makeStrips(su);
    //x=0 indices:  0  1  2  3  4
    //x=1 indices:  9  8  7  6  5
    //x=2 indices: 10 11 12 13 14
    constexpr uint16_t nmax = 5;
    
    {auto ip = pstrips->getPoint2D(makeFixpoint88(0,0),makeFixpoint88(0,0),nmax);
    EXPECT_EQ(0,ip.idx[0]);
    EXPECT_EQ(255,ip.value[0]);
    EXPECT_EQ(1,ip.n_points);}

    {auto ip = pstrips->getPoint2D(makeFixpoint88(1,0),makeFixpoint88(0,0),nmax);
    EXPECT_EQ(9,ip.idx[0]);
    EXPECT_EQ(255,ip.value[0]);
    EXPECT_EQ(1,ip.n_points);}
     
    
    {auto ip = pstrips->getPoint2D(makeFixpoint88(0,0),makeFixpoint88(1,0),nmax);
    EXPECT_EQ(1,ip.idx[0]);
    EXPECT_EQ(255,ip.value[0]);
    EXPECT_EQ(1,ip.n_points);}

    {auto ip = pstrips->getPoint2D(makeFixpoint88(1,0),makeFixpoint88(1,0),nmax);
    EXPECT_EQ(8,ip.idx[0]);
    EXPECT_EQ(255,ip.value[0]);
    EXPECT_EQ(1,ip.n_points);}

    delete pstrips;
}

TEST(Particles, getPoint2D_xmax)
{
    Subset su[] {{0,5,1},{5,5,-1},{10,5,1}};
    Strips *pstrips = makeStrips(su);
    //x=0 indices:  0  1  2  3  4
    //x=1 indices:  9  8  7  6  5
    //x=2 indices: 10 11 12 13 14
    constexpr uint16_t nmax = 5;
    
    {auto ip = pstrips->getPoint2D(makeFixpoint88(2,0),makeFixpoint88(0,0),nmax);
    EXPECT_EQ(10,ip.idx[0]);
    EXPECT_EQ(255,ip.value[0]);
    EXPECT_EQ(1,ip.n_points);}

    {auto ip = pstrips->getPoint2D(makeFixpoint88(2,0),makeFixpoint88(1,0),nmax);
    EXPECT_EQ(11,ip.idx[0]);
    EXPECT_EQ(255,ip.value[0]);
    EXPECT_EQ(1,ip.n_points);}
     
    
    {auto ip = pstrips->getPoint2D(makeFixpoint88(2,127),makeFixpoint88(2,0),nmax);
    EXPECT_EQ(12,ip.idx[0]);
    EXPECT_EQ(255,ip.value[0]);
    EXPECT_EQ(1,ip.n_points);}

    {auto ip = pstrips->getPoint2D(makeFixpoint88(2,127),makeFixpoint88(2,127),nmax);
    EXPECT_EQ(12,ip.idx[0]);
    EXPECT_EQ(128,ip.value[0]);
    EXPECT_EQ(13,ip.idx[1]);
    EXPECT_EQ(127,ip.value[1]);
    EXPECT_EQ(2,ip.n_points);}

    delete pstrips;
}

TEST(Particles, getPoint2D_yconst)
{
    Subset su[] {{0,5,1},{5,5,-1},{10,5,1}};
    Strips *pstrips = makeStrips(su);
    //x=0 indices:  0  1  2  3  4
    //x=1 indices:  9  8  7  6  5
    //x=2 indices: 10 11 12 13 14
    constexpr uint16_t nmax = 5;
    
   {auto ip = pstrips->getPoint2D(makeFixpoint88(1,55),makeFixpoint88(1,0),nmax);
    EXPECT_EQ(8,ip.idx[0]);
    EXPECT_EQ(200,ip.value[0]);
    EXPECT_EQ(11,ip.idx[1]);
    EXPECT_EQ(55,ip.value[1]);
    EXPECT_EQ(2,ip.n_points);}

    delete pstrips;
}

TEST(Particles, getPoint2D_bilinear)
{
    Subset su[] {{0,5,1},{5,5,-1},{10,5,1}};
    Strips *pstrips = makeStrips(su);
    //x=0 indices:  0  1  2  3  4
    //x=1 indices:  9  8  7  6  5
    //x=2 indices: 10 11 12 13 14
    constexpr uint16_t nmax = 5;
    
   {auto ip = pstrips->getPoint2D(makeFixpoint88(1,55),makeFixpoint88(1,100),nmax);
    EXPECT_EQ(8,ip.idx[0]);
    EXPECT_EQ(200*155/256,ip.value[0]);

    EXPECT_EQ(7,ip.idx[1]);
    EXPECT_EQ(155,ip.value[1]);

    EXPECT_EQ(11,ip.idx[2]);
    EXPECT_EQ(200,ip.value[2]);

    EXPECT_EQ(12,ip.idx[3]);
    EXPECT_EQ(55*100/256,ip.value[3]);

    EXPECT_EQ(4,ip.n_points);}

    delete pstrips;
}

template <typename T>
void* encode_params(void* buffer, T arg)
{
    encode(buffer,arg);
    return buffer;
}

template <typename T,typename... Args>
void* encode_params(void* buffer, T arg, Args... args)
{
    encode(buffer,arg);
    return encode_params(buffer,args...);
}

TEST(Particles,encode_params)
{
    int16_t buffer[4];
    encode_params(buffer, (uint16_t)101,(uint16_t)102,(uint16_t)103,(uint16_t)104);
    EXPECT_EQ(101, buffer[0]);
    EXPECT_EQ(102, buffer[1]);
    EXPECT_EQ(103, buffer[2]);
    EXPECT_EQ(104, buffer[3]);
}

TEST(Particles,wrappedAnimation)
{
    int8_t buffer[10];
    void*p = buffer;

    encode_params(buffer,ValueAnimationType::inc_wrapped, (uint16_t)100,(uint16_t)102,(int16_t)1);
    auto *a = loadValueAnimation<uint16_t>(p);

    EXPECT_EQ(100,a->nextValue(nullptr));
    EXPECT_EQ(101,a->nextValue(nullptr));
    EXPECT_EQ(102,a->nextValue(nullptr));

    EXPECT_EQ(100,a->nextValue(nullptr));
    EXPECT_EQ(101,a->nextValue(nullptr));
    EXPECT_EQ(102,a->nextValue(nullptr));
    
    delete a;
}

TEST(Particles,wrappedAnimation_neg)
{
    int8_t buffer[10];
    void*p = buffer;

    encode_params(buffer,ValueAnimationType::inc_wrapped, (uint16_t)100,(uint16_t)102,(int16_t)-1);
    auto *a = loadValueAnimation<uint16_t>(p);

    EXPECT_EQ(100,a->nextValue(nullptr));
    EXPECT_EQ(102,a->nextValue(nullptr));
    EXPECT_EQ(101,a->nextValue(nullptr));

    EXPECT_EQ(100,a->nextValue(nullptr));
    EXPECT_EQ(102,a->nextValue(nullptr));
    EXPECT_EQ(101,a->nextValue(nullptr));
    
    delete a;
}

TEST(Particles,pingpongAnimation)
{
    int8_t buffer[10];
    void*p = buffer;

    encode_params(buffer,ValueAnimationType::inc_pingpong, (uint16_t)100,(uint16_t)102,(int16_t)1);
    auto *a = loadValueAnimation<uint16_t>(p);

    EXPECT_EQ(100,a->nextValue(nullptr));
    EXPECT_EQ(101,a->nextValue(nullptr));
    EXPECT_EQ(102,a->nextValue(nullptr));
    EXPECT_EQ(101,a->nextValue(nullptr));
    EXPECT_EQ(100,a->nextValue(nullptr));
    EXPECT_EQ(101,a->nextValue(nullptr));
    EXPECT_EQ(102,a->nextValue(nullptr));
    
    delete a;
}

TEST(Particles,pingpongAnimation_neg)
{
    int8_t buffer[10];
    void*p = buffer;

    encode_params(buffer,ValueAnimationType::inc_pingpong, (uint16_t)100,(uint16_t)102,(int16_t)-1);
    auto *a = loadValueAnimation<uint16_t>(p);

    EXPECT_EQ(100,a->nextValue(nullptr));
    EXPECT_EQ(101,a->nextValue(nullptr));
    EXPECT_EQ(102,a->nextValue(nullptr));
    EXPECT_EQ(101,a->nextValue(nullptr));
    EXPECT_EQ(100,a->nextValue(nullptr));
    EXPECT_EQ(101,a->nextValue(nullptr));
    EXPECT_EQ(102,a->nextValue(nullptr));
    
    delete a;
}

TEST(Particles,randomAnimation)
{
    MockRandomGenerator random;
    int8_t buffer[10];
    void*p = buffer;

    encode_params(buffer,ValueAnimationType::random, (uint16_t)100,(uint16_t)102);
    auto *a = loadValueAnimation<uint16_t>(p);

    EXPECT_CALL(random, make_random()).Times(1).WillOnce(Return(0));
    EXPECT_EQ(100,a->nextValue(&random));

    EXPECT_CALL(random, make_random()).Times(1).WillOnce(Return(1));
    EXPECT_EQ(101,a->nextValue(&random));

    EXPECT_CALL(random, make_random()).Times(1).WillOnce(Return(2));
    EXPECT_EQ(102,a->nextValue(&random));

    EXPECT_CALL(random, make_random()).Times(1).WillOnce(Return(3));
    EXPECT_EQ(100,a->nextValue(&random));
    
    delete a;
}

void* encode_const_lissajous(void* pbuff,std::initializer_list<uint16_t> values)
{
    pbuff = encode_params(pbuff, (uint8_t)ParticleType::Lissajous);
    auto it = values.begin();
    pbuff = encode_params(pbuff, ValueAnimationType::constant,*it++); //omega_x
    pbuff = encode_params(pbuff, ValueAnimationType::constant,*it++); //omega_y
    pbuff = encode_params(pbuff, ValueAnimationType::constant,*it++); //ampl_x
    pbuff = encode_params(pbuff, ValueAnimationType::constant,*it++); //ampl_y
    pbuff = encode_params(pbuff, ValueAnimationType::constant,*it++); //phase_x
    pbuff = encode_params(pbuff, ValueAnimationType::constant,*it++); //phase_y
    pbuff = encode_params(pbuff, ValueAnimationType::constant,*it);
    return pbuff;
}

TEST(Particles, Lissajous)
{
    int8_t buffer[128];

    auto pbuff = encode_const_lissajous(buffer, {101,102,103,104,105,106,107});
    void *p = buffer;
    int datasize = (int8_t*)pbuff - (int8_t*)buffer;
    auto *a = Particle::load(p,datasize);
    auto & lp = *reinterpret_cast<LissajousParticle*>(a);

    EXPECT_EQ(101,lp.omega_x->nextValue(nullptr));
    EXPECT_EQ(102,lp.omega_y->nextValue(nullptr));
    EXPECT_EQ(103,lp.ampl_x->nextValue(nullptr));
    EXPECT_EQ(104,lp.ampl_y->nextValue(nullptr));
    EXPECT_EQ(105,lp.phase_x->nextValue(nullptr));
    EXPECT_EQ(106,lp.phase_y->nextValue(nullptr));
    EXPECT_EQ(107,lp.hue->nextValue(nullptr));

    delete a;
}

TEST(Particles, create)
{
    MockLedStrip led_strip;
    MockRandomGenerator random;
    Subset su[] = {{0,10,1},{15,10,-1},{30,10,1}};
    Strips *pstrips = makeStrips(su);
    int8_t buffer[128];

    uint16_t delay_ms=5;
    uint16_t fade_delay=6;
    uint8_t fading_factor=7;
    uint8_t particle_draw_mode=8;
    uint8_t n_particles=2;
    void *pbuff = buffer;
    pbuff = encode_params(pbuff, delay_ms, fade_delay,fading_factor,particle_draw_mode,n_particles);
    pbuff = encode_const_lissajous(pbuff, {101,102,103,104,105,106,107});
    pbuff = encode_const_lissajous(pbuff, {111,112,113,114,115,116,117});

    EXPECT_CALL(led_strip, getLength()).Times(1).WillOnce(Return(pstrips->count));
    auto *anim = new ParticleAnimation(&led_strip,(int8_t*)pbuff-(int8_t*)buffer,buffer,pstrips,&random);

    delete pstrips;
    delete anim;
}

#define x88(a,b) makeFixpoint88(a,b)
#define x88u(a,b) makeFixpoint88u(a,b)

TEST(Particles, Polar)
{
    int8_t buffer[128];

    void* pbuff = buffer;
    pbuff = encode_params(pbuff, ParticleType::Polar);
    pbuff = encode_params(pbuff, ValueAnimationType::inc_wrapped, x88u(0,0), (uint16_t)65535, x88(1,0));
    pbuff = encode_params(pbuff, ValueAnimationType::constant, x88u(1,0));
    pbuff = encode_params(pbuff, ValueAnimationType::constant, (uint16_t)100);

    void *p = buffer;
    int datasize = (int8_t*)pbuff - (int8_t*)buffer;
    auto *a = Particle::load(p,datasize);
    auto & lp = *reinterpret_cast<PolarParticle*>(a);

    EXPECT_EQ(x88u(0,0),lp.angle->nextValue(nullptr));
    EXPECT_EQ(x88u(1,0),lp.radius->nextValue(nullptr));
    EXPECT_EQ(100,lp.hue->nextValue(nullptr));

    delete a;
}

TEST(Particles, Lissajous_update)
{
    int8_t buffer[128];

    auto pbuff = encode_const_lissajous(buffer, {uint16_t(65535/360.0*30), uint16_t(65535/360.0*40),
                                                 makeFixpoint88u( 10,0),makeFixpoint88u( 20,0),
                                                 makeFixpoint88u(  0,0),makeFixpoint88u(  0,0),
                                                            50}); //ox,oy,ax,ay,phx,phy,hue
    void *p = buffer;
    int datasize = (int8_t*)pbuff - (int8_t*)buffer;
    auto *a = Particle::load(p,datasize);
    auto & lp = *reinterpret_cast<LissajousParticle*>(a);
    constexpr float PI = 3.1415926f;
    
    for (int i=0;i<=12;++i)
    {
        auto [x,y,h] = lp.update(i > 0 ? 1 : 0,nullptr);
        int16_t xe = (int16_t)(10.0f * std::sin( i*30.0f/180.0f * PI) * 256.0f);
        int16_t ye = (int16_t)(20.0f * std::cos( i*40.0f/180.0f * PI) * 256.0f);

        //printf("i=%d x=%d xe=%d err x %d y=%d ye=%d err y %d\n", i, x, xe, abs(x-xe), y, ye, abs(y-ye));
        EXPECT_LT( abs(xe - x), 15);
        EXPECT_LT( abs(ye - y), 32);
        EXPECT_EQ(50, h);
    }

    delete a;
}

TEST(Particles, Polar_update)
{
    int8_t buffer[128];

    void* pbuff = buffer;
    pbuff = encode_params(pbuff, ParticleType::Polar);
    pbuff = encode_params(pbuff, ValueAnimationType::inc_wrapped, x88u(0,0), (uint16_t)65535, int16_t(65535/360.0*30));
    pbuff = encode_params(pbuff, ValueAnimationType::inc_pingpong, x88u(1,0), x88u(13,0), x88u(1,0));
    pbuff = encode_params(pbuff, ValueAnimationType::constant, (uint16_t)100);

    void *p = buffer;
    int datasize = (int8_t*)pbuff - (int8_t*)buffer;
    auto *a = Particle::load(p,datasize);
    auto & lp = *reinterpret_cast<PolarParticle*>(a);
    constexpr float PI = 3.1415926f;

    for (int i=0;i<=12;++i)
    {
        auto [x,y,h] = lp.update(i > 0 ? 1 : 0,nullptr);

        const float radius = 1.0f + i;
        int16_t xe = (int16_t)(radius * std::cos( i*30.0f/180.0f * PI) * 256.0f);
        int16_t ye = (int16_t)(radius * std::sin( i*30.0f/180.0f * PI) * 256.0f);

        //printf("i=%d x=%d xe=%d err x %d y=%d ye=%d err y %d\n", i, x, xe, abs(x-xe), y, ye, abs(y-ye));

        EXPECT_LT( abs(xe - x), 17);
        EXPECT_LT( abs(ye - y), 32);

        EXPECT_EQ(100, h);
    }

    delete a;
}