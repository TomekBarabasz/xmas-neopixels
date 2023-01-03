import json
from types import SimpleNamespace
from random import randrange

def loadConfigFile(filename):
    with open(filename, "r") as your_file:
        your_dict = json.load(your_file)
        your_file.seek(0)
        return json.load(your_file, object_hook= lambda x: SimpleNamespace(**x))

def clamp(v,vmin,vmax):
    return min( max(v,vmin), vmax)

def hue_inc(h,inc):
    h += inc
    if h > 360 : return h-360
    elif h < 0 : return h+360
    else : return h

def scale8(rgb, scale):
    scale_fixed = scale + 1
    return [ (c * scale_fixed) >> 8 for c in rgb ]

def scale8_video(value, scale):
    #return (((int)i * (int)scale) >> 8) + ((i && scale) ? 1:0);
    return (int(value * scale) >> 8) + (1 if value > 0 and scale > 0 else 0)

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

def longestStrip(strips):
    return max([ s[1] for s in strips ])

def findClosest(vector,val):
    N = len(vector)
    for i in range(N):
        if vector[i] == val:
            return [i]
        elif vector[i] > val:
            return [i-1,i] if i > 0 else [i]
    return [N-1]

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
                idx = findClosest(Pv[ih-1], v)
                neighbours.extend( [IdxStrips[ih-1][i] for i in idx] )
            if iv > 0:
                neighbours.append(IdxStrips[ih][iv-1])
            if ih < len(Pv)-1:
                idx = findClosest(Pv[ih+1], v)
                idxs = IdxStrips[ih+1]
                ##print('idxs=',idxs,'idx=',idx,'pv=',Pv[ih+1],'v=',v, end=' ')
                neighbours.extend( [idxs[i] for i in idx] )
            if iv < len(pv)-1:
                neighbours.append(IdxStrips[ih][iv+1])
            Neighbours[current_idx] = neighbours
    return Neighbours

def makePositionMatrix(strips):
    npxInVStrip = [ s[1] for s in strips ]
    max_pxInVStrip = max(npxInVStrip)
    IdxStrips = stripIndices(strips)
    Dv = [ (max_pxInVStrip-1) / (n_int -1)  for n_int in npxInVStrip ]
    Pv = []
    for i in range(len(IdxStrips)):
        lineIdx = IdxStrips[i]
        dv = Dv[i]
        pv = [(i,dv*j, lineIdx[j]) for j in range(len(lineIdx))]
        Pv.append(pv)
    return Pv

def random2D(width,height):
    return randrange(width),randrange(height)

def random3D(max1,max2,max3):
    return randrange(max1),randrange(max2),randrange(max3)
