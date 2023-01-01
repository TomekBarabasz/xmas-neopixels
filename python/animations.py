import unittest,math
from itertools import accumulate,count
from bisect import bisect_left
from dataclasses import dataclass
from utils import loadConfigFile
from random import randrange,sample
from utils import *
from statistics import mean,median

class Base:
    pass

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
        c = findClosest( [1,2,3],1)
        self.assertEqual(c,[0])

        c = findClosest( [1,2,3],2)
        self.assertEqual(c,[1])

        c = findClosest( [1,2,3],3)
        self.assertEqual(c,[2])

        c = findClosest( [1,2,3],1.5)
        self.assertEqual(c,[0,1])

        c = findClosest( [1,2,3],2.5)
        self.assertEqual(c,[1,2])

        c = findClosest( [1,2,3],3.5)
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
        self.totalPixels = totalPixels(strips)
        self.pixels = [(0,0,0)] * self.totalPixels
        self.dt = 0
        self.fade_dt = 0
        self.params = params
        self.neighbours = makeNeighboursFromStrips(strips)
        self.hue = 0
        self.occupied_cells = [0] * self.totalPixels
        self.restart()
        
    @staticmethod
    def getParams():
        return 'delay_ms.H,hue:B,hue_inc:B,num_init_cells.H,size_init_cell.B'
    
    def restart(self):
        print("restarting")
        num_init_cells = self.params.num_init_cells
        num_iters = 0
        while(num_init_cells > 0 and num_iters < 3*self.params.num_init_cells):
            while True:
                ipos = random.randrange(0,self.totalPixels-1)
                if ipos not in self.neighbours: continue
                Ne = self.neighbours[ipos]
                break
            occ = [ self.occupied_cells[n] for n in Ne]
            self.pixels[ipos] = hsv_to_rgb(self.hue,255,255)
            if sum(occ) == 0:
                self.occupied_cells[ipos]=1
                ne = sample(Ne,k=self.params.size_init_cell) if self.params.size_init_cell < len(Ne) else Ne
                for n in ne:
                    self.occupied_cells[n] = 1
                    self.pixels[n] = hsv_to_rgb(self.hue,255,255)
                num_init_cells -= 1
            num_iters += 1            

    def step(self, dt):
        self.dt += dt
        if self.dt * 1000 > self.params.delay_ms:
            self.dt = 0
            num_alive_cells = 0
            new_occ = [0]*self.totalPixels
            for i,Ne in self.neighbours.items():
                occ_ne = [self.occupied_cells[n] for n in Ne]
                n_occ_ne = sum(occ_ne)
                if self.occupied_cells[i] == 0:
                    if n_occ_ne == 3:
                        new_occ[i] = 1
                        self.pixels[i] = hsv_to_rgb(self.hue,255,255)
                        num_alive_cells += 1
                        #print('age',self.occupied_cells[i],'hue',self.hue)
                else:
                    if n_occ_ne >=2 and n_occ_ne <=3:
                        new_occ[i] = self.occupied_cells[i] + 1
                        hue = self.hue + new_occ[i] * self.params.hue_inc
                        self.pixels[i] = hsv_to_rgb(hue,255,255)
                        num_alive_cells += 1
                        #print('age',self.occupied_cells[i],'hue',hue)
                    else:
                        new_occ[i] = 0
                        self.pixels[i] = (0,0,0)
            self.occupied_cells = new_occ
            if num_alive_cells == 0:
                self.restart()
            
        return self.pixels

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

class TextScrollAnimation:
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
  
    @staticmethod
    def getParams():
        return 'delay_ms.H'

    def step(self, dt):
        self.dt += dt
        if self.dt * 1000 > self.params.delay_ms:
            self.dt = 0
  
        return self.pixels

class TextScrollAnimation_TestCase(unittest.TestCase):
    def test_001(self):
        pass

class PlasmaAnimation:
    @dataclass
    class SineWave:
        origin : (int,int)
        phase : int
        phase_speed : int
        size : int
        amplitude : int
        move : int
        color : (int,int,int)
    
    def __init__(self, params, cfg):
        strips = cfg.strips
        self.totalPixels = totalPixels(strips)
        self.pixels = [(0,0,0)] * self.totalPixels
        self.dt = 0
        self.params = params
        self.positionMatrix = makePositionMatrix(strips)
        self.blobs = [(origin,size,mv,hsv)]
        self.sin_table = [math.sin(math.pi/2/256*x) for x in range(256)]
        self.width = len(strips)
        self.height = longestStrip(strips)
        self.waves = makeWaves(self.params)

    @staticmethod
    def makeWaves(params,width,height):
        Waves = []
        for i in count(1):
            if hasattr(params,f'wave{i}'):
                size,ampl,phase_speed, move_speed,color = getattr(params, f'wave{i}')
                w = SineWave( (randrange(0,width,),randrange(0,height)), randrange(0,256), phase_speed, size, ampl, move_speed, color )
                Waves.append(w)
            else:
                break
        return Waves
    
    def sin(self, x):
        n = x / 256
        x = x % 256
        if n & 1: x = 256-x
        #revert 1 3 5 7 i.e. sin_table[256-x]
        #invert 2 3  6 7 10 11 -1*sin_table[x]
        s = -1 if n//1 & 1 else 1
        return s * self.sin_table[x]
        #if n==0: return self.sin_table[x]
        #elif n==1: return self.sin_table[256-x]
        #elif n==2: return -self.sin_table[x]
        #elif n==3: return -self.sin_table[256-x]

    def drawWave(self, wave):
        for line in self.positionMatrix:
            for px,py,i in line:
                r = math.sqrt( (wave.origin[0] - px)**2 + (wave.origin[1]-py)**2 ) * wave.size
                ampl = wave.ampl * self.sin(r + wave.phase)
    
    def step(self, dt):
        self.dt += dt
        if self.dt * 1000 > self.params.delay_ms:
            self.dt = 0
            self.pixels = [(0,0,0)] * self.totalPixels
            for wave in self.Waves:
                self.drawWave(wave)
                wave.phase += wave.phase_speed
        return self.pixels

class PlasmaAnimation_TestCase(unittest.TestCase):
    def test_001(self):
        #strips = [[0,28,1],[30,27,-1],[59,27,1],[88,25,-1],[115,27,1],[144,27,-1],[173,27,1],[201,22,-1],[224,22,1]]
        strips = [[0,5,1],[7,4,-1],[12,6,1]]
        Pm =makePositionMatrix(strips)
        for p in Pm:
            print(p)

class WorleyNoiseAnimation:      
    def __init__(self, params, cfg):
        strips = cfg.strips
        self.totalPixels = totalPixels(strips)
        self.pixels = [(0,0,0)] * self.totalPixels
        self.dt = 0
        self.params = params
        self.positionMatrix = makePositionMatrix(strips)
        self.xmax = len(self.positionMatrix)
        self.ymax = int(max( row[-1][1] for row in self.positionMatrix ))
        n_features = self.params.num_features
        self.features = [ random2D(self.xmax,self.ymax) for _ in range(n_features) ]
        hue_min = params.hue_min if hasattr(params,'hue_min') else 0
        hue_max = params.hue_max if hasattr(params,'hue_max') else 360
        self.colors = [randrange(hue_min, hue_max) for _ in range(n_features)]
        self.tmp = [(0,0)] * self.totalPixels

    @staticmethod
    def getParams():
        return 'delay_ms.H,num_featuers:B,hue_min:H,hue_max:H,move_speed:B'

    def step(self, dt):
        self.dt += dt
        if self.dt * 1000 > self.params.delay_ms:
            self.dt = 0
            # recreate pixels array
            # move features
            speed = self.params.move_speed
            for i in range(len(self.features)):
                x,y = self.features[i]
                dx,dy = random2D(2*speed+1,2*speed+1)
                x = x + dx - speed
                x = clamp(x,0,self.xmax)
                y = y + dy - speed
                y = clamp(y,0,self.ymax)
                self.features[i] = (x,y)
                #print(f'feature {i} pos {x} {y} dx {dx} dy {dy}')
            
            n_features = self.params.num_features
            longest = [0] * n_features
            for line in self.positionMatrix:
                for px,py,idx in line:
                    dmin = 1e6
                    fim = None
                    for fi in range(n_features):
                        fx,fy = self.features[fi]
                        dx = (px-fx)
                        dy = (py-fy)
                        d = dx*dx + dy*dy
                        
                        if d < dmin : 
                            dmin = d
                            fim = fi
                            if d > longest[fi] : longest[fi] = d
                    self.tmp[idx] = (dmin,fim)
            
            #longest = [math.sqrt(l) for l in longest]
            feature_vals = [[]] * n_features
            for idx in range(self.totalPixels):
                dmin,fidx = self.tmp[idx]
                #val = 255 - int(math.sqrt(dmin) / longest[fidx] * 255)
                val = dmin/longest[fidx]
                feature_vals[fidx].append( val )
                val = 255 * val
                val = clamp(255 - int(val), 0, 255)
                self.pixels[idx] = hsv_to_rgb(self.colors[fidx],255,val)
            
            for i in range(n_features):
                break
                vals = feature_vals[i]
                _min = round(min(vals),3)
                _max = round(max(vals),3)
                _mean = round(mean(vals),3)
                _median = round(median(vals),3)
                print(f'feature {i} min {_min} max {_max} mean {_mean} median {_median}')

        return self.pixels

class WorleyNoise_TestCase(unittest.TestCase):
    def test_001(self):
        pass

class LavaAnimation:
    def __init__(self, params, cfg):
        strips = cfg.strips
        self.totalPixels = totalPixels(strips)
        self.pixels = [(0,0,0)] * self.totalPixels
        self.dt = 0
        self.params = params
        self.positionMatrix = makePositionMatrix(strips)
    
    def step(self, dt):
        self.dt += dt
        if self.dt * 1000 > self.params.delay_ms:
            self.dt = 0
            # recreate pixels array
            # self.pixels = [(0,0,0)] * self.totalPixels

        return self.pixels

class _2DTrajectory:
    def __init__(self, params, cfg):
        strips = cfg.strips
        self.totalPixels = totalPixels(strips)
        self.pixels = [(0,0,0)] * self.totalPixels
        self.dt = 0
        self.params = params
        self.positionMatrix = makePositionMatrix(strips)
    
    def step(self, dt):
        self.dt += dt
        if self.dt * 1000 > self.params.delay_ms:
            self.dt = 0
            # recreate pixels array
            # self.pixels = [(0,0,0)] * self.totalPixels

        return self.pixels
