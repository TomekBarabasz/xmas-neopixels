import pygame,sys,argparse,unittest,re
from pathlib import Path
from datetime import datetime
from time import sleep
from animations import RandomWalkAnimation, RandomWalkAnimation_TestCase, HorizontalWaveAnimation, DigitalRainAnimation, DigitalRainAnimation_TestCase
from utils import loadConfigFile

def got_quit_event(events):
    for event in events:
        if event.type == pygame.QUIT:
            return True
        elif event.type == pygame.KEYDOWN:
            if event.key == pygame.K_ESCAPE or event.unicode == 'q':
                return True
    return False

def drawNeopixels(screen,height,colors,config,args):
    psize = config.pixel_size
    psx,psy = config.pixel_spacing_h, config.pixel_spacing_v
    ox = psx
    
    for ist,np,step in config.strips:
        start,end,step = (ist,ist+np,1) if step==1 else (ist+np-1,ist-1,-1)
        dy = psy if Args.uniform_spacing else int( (height - np * config.pixel_size) / (np + 1) + 0.5)
        oy = dy
        for ci in range(start,end,step):            
            try:
                pygame.draw.rect(screen,colors[ci],(ox,oy,psize,psize),0)
            except IndexError:
                print(f' strip ( {ist},{np},{step} )')
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
    if len(np)==1:
        return name,None
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
    Animations = {  'hwave' : HorizontalWaveAnimation,
                    'rwalk' : RandomWalkAnimation,
                    'drain' : DigitalRainAnimation
    }
    if params is None:
        print("please provide animation parametrers :", Animations[name].getParams())
        return None
    return Animations[name](params,config)

def main(Args):
    Config = loadConfigFile(Args.config)
    animation = createAnimation(Args.animation, Config)
    if animation is None:
        return
    longest_strip = max( [s[1] for s in Config.strips] ) + 1
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
    
#-a=hwave,delay_ms=50,hue_step=20,hue_inc=20
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="neopixels animation workbench")
    parser.add_argument("config", type=Path, help='workbench config file')
    parser.add_argument("-test", action='store_true', help='execute unittests')
    parser.add_argument("-display","-d",type=str, help="screen size width,height")
    parser.add_argument("-animation","-a",type=str, help="animation name and params, ex.: a=hwave,delay_ms=50,hue_step=20,hue_inc=20")
    parser.add_argument("-uniform_spacing", "-u", action='store_true', help='uniform pixel spacing')
    Args = parser.parse_args()
    if Args.test:
        unittest.main(argv=[__file__])
    else:
        main(Args)

