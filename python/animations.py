import unittest,random
from itertools import accumulate,count
from bisect import bisect_left
from dataclasses import dataclass
from utils import loadConfigFile
from random import randrange

class Base:
    pass

def hue_inc(h,inc):
    h += inc
    if h > 360 : return h-360
    elif h < 0 : return h+360
    else : return h

def scale8(rgb, scale):
    scale_fixed = scale + 1
    return [ (c * scale_fixed) >> 8 for c in rgb ]

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

def interpolate(hsv1,hsv2,alpha): 
    #alpha is 0-255
    oma = 255-alpha
    return (c[0]*oma + c[1]*alpha for c in zip(hsv1,hsv2))
 
def totalPixels(strip):
    s,c,d = strip[-1]
    return s+c 

def stripIndices(strips):
    return [ list(range(s,s+c)) if d==1 else list(range(s+c-1,s-1,-1)) for s,c,d in strips ]

def makeNeighboursFromStrips(strips):
    # neighbours = [ neighbour ]
    # neighbour = [ (left|None,up|,right|None,bottom|None) ]
    Neighbours = {}
    npxInVStrip = [ s[1] for s in strips ]
    max_pxInVStrip = max(npxInVStrip)
    IdxStrips = stripIndices(strips)
    Dv = [ (max_pxInVStrip-1) / (n_int -1)  for n_int in npxInVStrip ]
    Pv = [ [dv*n for n in range(len(s))] for s,dv in zip(IdxStrips,Dv) ]
    
    for ih in range(len(Pv)):
        pv = Pv[ih]
        for iv in range(len(pv)):
            v = pv[iv]
            current_idx = IdxStrips[ih][iv]
            neighbours = []
            if ih > 0:
                idx = RandomWalkAnimation.findClosest(Pv[ih-1], v)
                neighbours.extend( [IdxStrips[ih-1][i] for i in idx] )
            if iv > 0:
                neighbours.append(IdxStrips[ih][iv-1])
            if ih < len(Pv)-1:
                idx = RandomWalkAnimation.findClosest(Pv[ih+1], v)
                idxs = IdxStrips[ih+1]
                ##print('idxs=',idxs,'idx=',idx,'pv=',Pv[ih+1],'v=',v, end=' ')
                neighbours.extend( [idxs[i] for i in idx] )
            if iv < len(pv)-1:
                neighbours.append(IdxStrips[ih][iv+1])
            Neighbours[current_idx] = neighbours
    return Neighbours

class RandomWalkAnimation:
    def __init__(self, params, cfg):
        strips = cfg.strips
        self.totPixels = totalPixels(strips)
        self.pixels = [(0,0,0)] * self.totPixels
        self.brightness = [0] * self.totPixels
        self.dt = 0
        self.fade_dt = 0
        self.params = params
        self.neighbours = makeNeighboursFromStrips(strips)
        self.hue = 0
        self.ipos = random.randrange(0,self.totPixels-1)
        self.pixels[self.ipos] = hsv_to_rgb(self.hue,255,255)
        self.brightness[self.ipos] = 255
        
    @staticmethod
    def getParams():
        return 'delay_ms.H,fade_delay_ms.H,hue_inc:B,fade.B'
    
    @staticmethod
    def findClosest(vector,val):
        N = len(vector)
        for i in range(N):
            if vector[i] == val:
                return [i]
            elif vector[i] > val:
                return [i-1,i] if i > 0 else [i]
        return [N-1]
    
    
    def step(self, dt):
        self.dt += dt
        self.fade_dt += dt
        #print(f'dt {dt} self.dt {self.dt}')
        if self.dt * 1000 > self.params.delay_ms:
            self.hue = hue_inc(self.hue,self.params.hue_inc)
            ne = self.neighbours[self.ipos]
            prob = [255-self.brightness[i] for i in ne]
            tot_prob = sum(prob)
            s = random.randrange(0,tot_prob) if tot_prob > 0 else 0
            cumprob = list(accumulate(prob))
            i = bisect_left(cumprob,s)
            #print(f'idx {self.ipos} ne={ne} prob={prob} cumprob={cumprob} i={i} next idx {ne[i]}')
            #print(f'move {self.ipos} -> {ne[i]}')
            self.ipos = ne[i]
            self.pixels[self.ipos] = hsv_to_rgb(self.hue,255,255)
            self.brightness[self.ipos] = 255
            self.dt = 0
        if self.fade_dt * 1000 > self.params.fade_delay_ms:
            self.fade_dt = 0
            f = self.params.fade + 1
            for i in range(self.totPixels):
                if i == self.ipos: continue
                self.pixels[i] = scale8(self.pixels[i],self.params.fade)
                self.brightness[i] = (self.brightness[i] * f) >> 8
        return self.pixels

class RandomWalkAnimation_TestCase(unittest.TestCase):
    def test_001(self):
        print('test test_001')
        #neighbours are indices!
        c = RandomWalkAnimation.findClosest( [1,2,3],1)
        self.assertEqual(c,[0])

        c = RandomWalkAnimation.findClosest( [1,2,3],2)
        self.assertEqual(c,[1])

        c = RandomWalkAnimation.findClosest( [1,2,3],3)
        self.assertEqual(c,[2])

        c = RandomWalkAnimation.findClosest( [1,2,3],1.5)
        self.assertEqual(c,[0,1])

        c = RandomWalkAnimation.findClosest( [1,2,3],2.5)
        self.assertEqual(c,[1,2])

        c = RandomWalkAnimation.findClosest( [1,2,3],3.5)
        self.assertEqual(c,[2])

    def test_002(self):
        print('test test_002')
        prms = Base()
        cfg = Base()
        cfg.strips = [(1,5,1),(6,10,0),(11,15,1)]
        a = RandomWalkAnimation(prms,cfg)

    def test_003(self):
        print('test test_003')
        prms = Base()
        cfg = Base()
        cfg.strips = [(1,7,1),(8,12,0),(13,17,1)]
        a = RandomWalkAnimation(prms,cfg)

    def xtest_004(self):
        print('test test_004')
        prms = Base()
        cfg = loadConfigFile('config.json')
        a = RandomWalkAnimation(prms,cfg)
        maxi = cfg.strips[-1][1]
        Nstrips = len(cfg.strips)
        Invalid = []
        for i in range(Nstrips-1):
            last  = max( [cfg.strips[i][0], cfg.strips[i][1]] )
            first = min( [cfg.strips[i+1][0], cfg.strips[i+1][1]] )
            Invalid.extend( range(last+1,first) )
        Invalid = set(Invalid)
        print('Invalid indices',Invalid)
        Ne = a.neighbours
        for i,ne in Ne.items():
            print('idx',i,'neighbours',ne)
            bad = Invalid.intersection(ne)
            if len(bad)>0:
                print('idx=',i,'bad=',bad)

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

class VerticalWaveAnimation:
    def __init__(self, params, cfg):
        strips = cfg.strips
        self.hlines = self._makeHorizontalLines(strips)
        self.totPixels = self.strips[-1][1] + 1
        self.pixels = [(0,0,0)] * self.totPixels
        self.lineHue = [0] * len(self.hlines)
        #uint16_t delay_ms;
        #uint8_t hue_step
        #uint8_t hue_inc;
        #uint8_t direction;
        self.params = params
        self.dt = 0
        
        h = 0
        for idx in range(len(self.hlines)):
            self.lineHue[idx] = h
            h = hue_inc(h,self.params.hue_step)

    def _makeHorizontalLines(self,strips):
        # horizontal lines = [ hline ]
        # hline = [ pixel ]
        # pixel = [ (idx,pct),(idx,pct) ]
        HLines=[]
        intervalsInVStrip = [s[1]-s[0]-1 for s in strips]
        max_pxInVStrip = max(pxInVStrip)
        Dv = [n_int / max_pxInVStrip for n_int in intervalsInVStrip]
        Accu = [0.0] * len(strips)
        #Pxi = [s0 if d =]
        for line_idx in range(max_pxInVStrip):
            pass

    
    def step(self, dt):
        self.dt += dt
        if self.dt * 1000 > self.params.delay_ms:
            self.dt = 0
            for hline in self.hlines:
                hue = self.lineHue[idx]
                hsv = hsv_to_rgb(hue,255,255)
                for pixel in hline:
                    p1i,p1a,p2i,p2a = pixel
                    if p1a > 0:
                        self.pixels[p1i] = interpolate(self.pixels[p1i],hsv,p1a)
                    if p2a > 0:
                        self.pixels[p1i] = interpolate(self.pixels[p2i],hsv,p2a)
                hue = hue_inc(hue, self.params.hue_inc)
                self.lineHue[idx] = hue
        return self.pixels

class GameOfLifeAnimation:
    def __init__(self, params, cfg):
        strips = cfg.strips
        self.totPixels = totalPixels(strips)
        self.pixels = [(0,0,0)] * self.totPixels
        self.dt = 0
        self.fade_dt = 0
        self.params = params
        self.neighbours = makeNeighboursFromStrips(strips)
        self.hue = 0
        self.ipos = random.randrange(0,self.totPixels-1)
        self.pixels[self.ipos] = hsv_to_rgb(self.hue,255,255)
        
    @staticmethod
    def getParams():
        return 'delay_ms.H,fade_delay_ms.H,hue_inc:B,fade.B'

    def step(self, dt):
        self.dt += dt
        if self.dt * 1000 > self.params.delay_ms:
            self.dt = 0
        return self.pixels

def clamp(v,vmin,vmax):
    return min( max(v,vmin), vmax)

class DigitalRainAnimation:
    @dataclass
    class Line:
        state : int #1 - drawing 0 - waiting        
        pos : int
        length : int # tail length
        delay : int # number of ticks delay before new
    
    def __init__(self, params, cfg):
        strips = cfg.strips
        longestLine = max( [s[1] for s in strips])
        self.totPixels = totalPixels(strips)
        self.pixels = [(0,0,0)] * self.totPixels
        self.dt = 0
        self.params = params
        self.hue = params.hue if hasattr(params,'hue') else 120
        self.head_len = params.head_len if hasattr(params,'head_len') else 3
        tlmin = params.tail_len_min if hasattr(params,'tail_len_min') else longestLine//3
        tlmax = params.tail_len_max if hasattr(params,'tail_len_max') else longestLine
        self.tail_len = (tlmin,tlmax)
        self.lineIndices = [list(reversed(l)) for l in stripIndices(strips)]
        self.new_line_delay = (0,longestLine)
        nLines = len(strips)
        self.Lines = [DigitalRainAnimation.Line(0,len(self.lineIndices[i])-1, randrange(*self.tail_len), randrange(*self.new_line_delay)) for i in range(nLines)]
    
    @staticmethod
    def getParams():
        return 'delay_ms.H,hue.H,head_len.B,tail_len_min.B,tail_len_max.B'

    def restartLine(self,idx,line):
        line.state = 0        
        line.pos = len(self.lineIndices[idx])-1
        line.length = randrange(*self.tail_len)
        line.delay = randrange(*self.new_line_delay)

    def moveLine(self,idx,line):
        line.pos -= 1
        
    def drawLine(self,idx,line):
        hlen = self.head_len
        Li = self.lineIndices[idx]
        N = len(Li)
        hmin = max(line.pos,0)
        hmax = clamp(line.pos + hlen,0,N)
        for i in range(hmin,hmax):
            self.pixels[ Li[i] ] = (255,255,255)
        tstart = line.pos + hlen
        tend = tstart + line.length
        tmin = clamp(tstart,0,N)
        tmax = clamp(tend,  0,N)
        toffset = tmin - tstart
        if tmax > 0:
            dv = 255 / line.length
            Dv = [int(255-dv*i) for i in range(line.length)]
            for i,j in zip(range(tmin,tmax),count(toffset)):
                self.pixels[ Li[i] ] = hsv_to_rgb(self.hue, 255, Dv[j])
            if tmax < N:
                self.pixels[ Li[tmax] ] = (0,0,0)
            return True
        else:
            return False

    def step(self, dt):
        self.dt += dt
        if self.dt * 1000 > self.params.delay_ms:
            self.dt = 0
            for line,idx in zip(self.Lines,count(0)):
                if line.state == 0:
                    if line.delay == 0:
                        line.state = 1
                    else:
                        line.delay -= 1
                        #print(f'line {idx} delay {line.delay}')
                else:
                    self.moveLine(idx,line)
                    if not self.drawLine(idx,line):
                        #print('restarting line',idx)
                        self.restartLine(idx,line)
        return self.pixels

class DigitalRainAnimation_TestCase(unittest.TestCase):
    def xtest_001(self):
        strips = [[0,28,1],[30,27,-1],[59,27,1],[88,25,-1],[115,27,1],[144,27,-1],[173,27,1],[201,22,-1],[224,22,1]]
        Lines = stripIndices(strips)
        Lines = [list(reversed(l)) for l in Lines]
        for line in Lines:
            print(line)
    
    def xtest_002(self):
        length = 12
        dv = 255 / length
        Dv = [255-dv*i for i in range(length)]
        print(Dv)

    def test_clamp(self):
        self.assertEqual( clamp(-1,0,5), 0)
        self.assertEqual( clamp( 0,0,5), 0)
        self.assertEqual( clamp( 1,0,5), 1)
        self.assertEqual( clamp( 5,0,5), 5)
        self.assertEqual( clamp( 6,0,5), 5)

    def test_draw_head(self):
        print('test_draw_head')
        N = 10
        for pos in range(N,-10,-1):
            hmin = max(pos,0)
            hmax = min(max(pos+3,0),N)
            if hmin==0 and hmax==0:
                break
            print( hmin,hmax )