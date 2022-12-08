import unittest,random
from itertools import accumulate
from bisect import bisect_left

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

def makeNeighboursFromStrips(strips):
    # neighbours = [ neighbour ]
    # neighbour = [ (left|None,up|,right|None,bottom|None) ]
    Neighbours = {}
    npxInVStrip = [ s[1] for s in strips ]
    max_pxInVStrip = max(npxInVStrip)
    IdxStrips = [ list(range(s,s+c)) if d==1 else list(range(s+c-1,s-1,-1)) for s,c,d in strips ]
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

    def test_004(self):
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
        self.totPixels = strips[-1][1] + 1
        self.pixels = [(0,0,0)] * self.totPixels
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

    def step(self, dt):
        self.dt += dt
        if self.dt * 1000 > self.params.delay_ms:
            self.dt = 0
    