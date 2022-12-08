import sys,re
from animations import makeNeighboursFromStrips
from itertools import accumulate
from bisect import bisect_left

strips = [[0,28,1],
        [30,27,-1],
        [59,27,1],
        [88,25,-1],
        [115,27,1],
        [144,27,-1],
        [173,27,1],
        [201,22,-1],
        [224,22,1]]

def calcNextPos(pos,Ne,Br,Pr,acc_prob,sel):
    p = [255-b for b in Br]
    exp_pr = list(accumulate(p))
    assert(Pr == exp_pr)
    exp_acc_prob = sum(p)
    assert(exp_acc_prob == acc_prob)
    i = bisect_left(exp_pr,sel)
    return Ne[i]

exp_neighbours = makeNeighboursFromStrips(strips)
rx1 = re.compile(r'fromstrips: i (\d+) ne ([ \d]+)')
rx2 = re.compile(r'rwanim: step : pos (\d+) ne ([ \d]+)br ([ \d]+)pr ([ \d]+)acc prob (\d+) sel (\d+)')

#line='I (1902) rwanim: step : pos 189 ne 154 188 210 209 190  br 0 0 0 0 0  pr 255 510 765 1020 1275  acc prob 1275 sel 589'
#m=rx2.search(line)
#print(m.groups())
#exit()

filename = sys.argv[1]
output = open(filename)

next_pos = None
for line in output:
    m = rx1.search(line)
    if m is not None:
        idx = int( m.groups()[0] )
        ne  = list(map(int,m.groups()[1].split(' ')))
        nex = exp_neighbours[idx]
        d = set(ne).difference(nex)
        if len(d):
            print(f'idx {idx}\nexpected ne ',nex)
            print( 'loaded   ne ',ne)
    else:
        m = rx2.search(line)
        if m is not None:
            print( 'next pos', next_pos, 'step',m.groups() )
            pos = int( m.groups()[0] )
            if next_pos is not None:
                assert(pos == next_pos)
            ne  = list(map(int,m.groups()[1].strip().split(' ')))
            br  = list(map(int,m.groups()[2].strip().split(' ')))
            pr  = list(map(int,m.groups()[3].strip().split(' ')))
            acc_prob = int( m.groups()[4] )
            sel = int( m.groups()[5] )
            next_pos = calcNextPos(pos,ne,br,pr,acc_prob,sel)
