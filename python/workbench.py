import pygame,sys,argparse,unittest,json,re
from pathlib import Path
from dataclasses import dataclass
from types import SimpleNamespace
from datetime import datetime
from time import sleep

def loadConfigFile(filename):
    with open(filename, "r") as your_file:
        your_dict = json.load(your_file)
        your_file.seek(0)
        return json.load(your_file, object_hook= lambda x: SimpleNamespace(**x))

def got_quit_event(events):
    for event in events:
        if event.type == pygame.QUIT:
            return True
        elif event.type == pygame.KEYDOWN:
            if event.key == pygame.K_ESCAPE or event.unicode == 'q':
                return True
    return False

def hsv_to_rgb(h,s,v):
    h_ = h % 360
    region = h_ // 60
    #remainder = (h_ - (region * 60)) * (256/60)
    remainder = (h_ - (region * 60)) * (256//60)

    p = (v * (255 - s)) >> 8
    q = (v * (255 - ((s * remainder) >> 8))) >> 8
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8

    if region==0:   return (v,t,p)
    elif region==1: return (q,v,p)
    elif region==2: return (p,v,t)
    elif region==3: return (p,q,v)
    elif region==4: return (t,p,v)
    else:           return (v,p,q)

@dataclass
class RGB:
    r : int
    g : int
    b : int

def hue_inc(h,inc):
    h += inc
    if h > 360 : return h-360
    elif h < 0 : return h+360
    else : return h

class HorizontalWaveAnimation:
    def __init__(self, params, cfg):
        self.strips = cfg.strips
        self.totPixels = self.strips[-1][1] + 1
        self.stripHue = [0] * len(self.strips)
        self.pixels = [(0,0,0)] * self.totPixels
        #uint16_t delay_ms;
        #uint8_t hue_step
        #uint8_t hue_inc;
        #uint8_t direction;
        self.params = params
        self.dt = 0
        h = 0
        for idx in range(len(self.strips)):
            self.stripHue[idx] = h
            s,e,_ = self.strips[idx]
            hsv = hsv_to_rgb(h,255,255)
            for i in range(s,e+1):
                self.pixels[i] = hsv
            h = hue_inc(h,self.params.hue_step)

    def step(self, dt):
        self.dt += dt
        if self.dt * 1000 > self.params.delay_ms:
            self.dt = 0
            for idx in range(len(self.strips)):
                h = hue_inc(self.stripHue[idx], self.params.hue_inc)
                self.stripHue[idx] = h
                hsv = hsv_to_rgb(h, 255,255)
                s,e,_ = self.strips[idx]
                for i in range(s,e+1):
                    self.pixels[i] = hsv
        return self.pixels

def drawNeopixels(screen,height,colors,config,args):
    psize = config.pixel_size
    psx,psy = config.pixel_spacing_h, config.pixel_spacing_v
    ox = psx
    
    for ist,iend,dir in config.strips:
        start,end,step = (iend,ist-1,-1) if dir==1 else (ist,iend+1,1)
        np = abs(start-end)
        dy = psy if Args.uniform_spacing else int( (height - np * config.pixel_size) / (np + 1) + 0.5)
        oy = dy
        for ci in range(start,end,step):            
            try:
                pygame.draw.rect(screen,colors[ci],(ox,oy,psize,psize),0)
            except IndexError:
                print(f' strip ( {ist},{iend},{dir} )')
                print(f'idx {ci} ox={ox} oy={oy}')
                raise
            oy = oy + psize + dy
        ox = ox + psize + psx
    
def parseAnimationParameters(name_and_params):
    class Params:
        pass
    #format :name,param=val.[BHI][,param=val.[BHI]]
    rxPrm = re.compile( r'(.+)=([\dabcdefx-]+)(\.([BHI]))*' )
    np = name_and_params.split(',')
    name = np[0]
    params = Params()
    for prm in np[1:]:
        m = rxPrm.match(prm)
        if m is None:
            print('invalid parameter',prm)
        else:
            pn = m.groups()[0]
            v = m.groups()[1]
            v = int(v, 16 if v[0:2]=='0x' else 10)
            typ = m.groups()[3]
            setattr(params,pn, v)
    return name,params

def createAnimation(name_and_params,config):
    name,params = parseAnimationParameters(name_and_params)
    Animations = {'hwave' : HorizontalWaveAnimation}
    print('animation name',name)
    print('animation params',params)
    return Animations[name](params,config)

def main(Args):
    Config = loadConfigFile(Args.config)
    longest_strip = max( [s[1]-s[0] for s in Config.strips] ) + 1
    if Args.display is not None:
        width,height = tuple(map(int, Args.display.split(',')))
    else:
        width  = len(Config.strips) * Config.pixel_size + (len(Config.strips) +1) * Config.pixel_spacing_h
        height = longest_strip * Config.pixel_size + (longest_strip +1) * Config.pixel_spacing_v
    screen_size = width,height
    print('screen size is',screen_size)
    pygame.init()
    screen = pygame.display.set_mode(screen_size)
    background=0,0,0
    animation = createAnimation(Args.animation, Config)
    tm = datetime.utcnow()
    while True:
        events = pygame.event.get()
        if got_quit_event(events):
            break
        screen.fill(background)
        colors = animation.step( (datetime.utcnow()-tm).total_seconds() )
        drawNeopixels(screen,height,colors,Config,Args)
        pygame.display.flip()
        tm = datetime.utcnow()
    
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="neopixels animation workbench")
    parser.add_argument("config", type=Path, help='workbench config file')
    parser.add_argument("-test", action='store_true', help='execute unittests')
    parser.add_argument("-display","-d",type=str, help="screen size width,height")
    parser.add_argument("-animation","-a",type=str, help="animation name")
    parser.add_argument("-uniform_spacing", "-u", action='store_true', help='uniform pixel spacing')
    Args = parser.parse_args()
    if Args.test:
        unittest.main(argv=[__file__])
    else:
        main(Args)

