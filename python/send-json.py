import socket,argparse,re,struct,json
from time import sleep
from pathlib import Path
from utils import set_keepalive

LissajousParticleType = 0
PolarParticleType = 1

ValueAnimation_wrappedType  = 0
ValueAnimation_pingpongType = 1
ValueAnimation_sinusType    = 2
ValueAnimation_randomType   = 3
ValueAnimation_constType    = 4

def parseValueAnimation_const(format,desc):
    return struct.pack(format,ValueAnimation_constType, desc['value'])

def parseValueAnimation_wrapped(format,desc):
    return struct.pack(format,ValueAnimation_wrappedType, desc['min'], desc['max'], desc['inc'])

def parseValueAnimation_pingpong(format,desc):
    return struct.pack(format,ValueAnimation_pingpongType, desc['min'], desc['max'], desc['inc'])

def parseValueAnimation_sinus(format,desc):
    return struct.pack(format,ValueAnimation_sinusType, desc['min'], desc['max'], desc['omega'])

def parseValueAnimation_random(format,desc):
    return struct.pack(format,ValueAnimation_randomType, desc['min'],desc['max'])

def parseValueAnimation(type,desc):
    Parsers = {'ValueAnimation_wrapped_u16'  : ("<BHHh",parseValueAnimation_wrapped),
               'ValueAnimation_wrapped_i16'  : ("<Bhhh",parseValueAnimation_wrapped),

               'ValueAnimation_pingpong_u16' : ("<BHHh",parseValueAnimation_pingpong),
               'ValueAnimation_pingpong_i16' : ("<Bhhh",parseValueAnimation_pingpong),

               'ValueAnimation_sin_u16'      : ("<BHHh",parseValueAnimation_sinus),
               'ValueAnimation_sin_i16'      : ("<Bhhh",parseValueAnimation_sinus),

               'ValueAnimation_const_u16'    : ("<BH",parseValueAnimation_const),
               'ValueAnimation_const_i16'    : ("<Bh",parseValueAnimation_const),
               
               'ValueAnimation_random_u16'   : ("<BHH",parseValueAnimation_random),
               'ValueAnimation_random_i16'   : ("<Bhh",parseValueAnimation_random)
    }
    format,parser = Parsers[type]
    return parser(format,desc)

def parseParticleAnimation(descr):
    def parseLissajousParticle(descr):
        data = struct.pack("<BBHH",LissajousParticleType,descr['draw_mode'],descr['center_x'],descr['center_y'])
        for attr in ['omega_x','omega_y','ampl_x','ampl_y','phase_x','phase_y','hue']:
            vanim = descr[attr]
            data += parseValueAnimation(vanim['type'],vanim)
        return data
    
    def parsePolarParticle(descr):
        data = struct.pack("<BBHHH",PolarParticleType,descr['draw_mode'],descr['center_x'],descr['center_y'],descr['yscale'])
        for attr in ['angle','radius','hue']:
            vanim = descr[attr]
            data += parseValueAnimation(vanim['type'],vanim)
        return data
    
    data = struct.pack("<HHBB", *[descr[attr] for attr in ['delay_ms', 'fade_delay_ms','fading_factor','hue_shift']])
    data += struct.pack("<B", len(descr['particles']))
    for particle in descr['particles']:
        if particle['type'] == "LissajousParticle":
            data += parseLissajousParticle(particle)
        elif particle['type'] == "PolarParticle":
            data += parsePolarParticle(particle)
        else:
            print('unknown particle type',particle['type'])
    return data

def parseDigitalRainAnimation(description):
    pass

def parseRandomWalkAnimation(description):
    pass

def parseFireAnimation(description):
    pass

def parseJson(animation):
    Parsers = {'ParticleAnimation'    : (14,parseParticleAnimation),
               'DigitalRainAnimation' : (13,parseDigitalRainAnimation),
               'RandomWalkAnimation'  : (12,parseRandomWalkAnimation),
               'FireAnimation'        : (4,parseFireAnimation) }
    atype = animation['type']
    if atype not in Parsers:
        print('unknown animation type',atype)
        return None
    anim_id,parser = Parsers[ atype ]
    anim_params = parser(animation)
    tosend = struct.pack("<BHH", 1, anim_id, len(anim_params)) + anim_params
    return tosend

def make_output(Args):
    def fakesend(tosend):
        print('sending ',tosend)
    def tofile(tosend):
        output_file.write(tosend)
    def socket_send(tosend):
        n_send = sock.send(tosend)
        if n_send == 0:
            print('send failed')
        elif n_send == len(tosend):
            print('send ok')
    def toarray(tosend):
        string = 'unsigned char buffer[] = {'
        string += ','.join( map(str,tosend))
        string += '};'
        print(string)
    
    if Args.test:
        return fakesend
    elif Args.tofile is not None:
        output_file = open(Args.tofile,'wb')
        return tofile
    elif Args.toarray:
        return toarray
    else:
        host,port = Args.addr.split(':')
        port = int(port)
        print(f'connecting {host} port {port} ...')    
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((host, port))
        set_keepalive(sock)
        return socket_send

def main(Args):
    with open(Args.json) as jsonf:
        tosend = parseJson(json.load(jsonf))
    
    output = make_output(Args)
    output(tosend)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Neopixel json contoller")
    parser.add_argument("json",type=Path, help="json file to encode and send")
    parser.add_argument("-addr","-a", type = str, default="192.168.1.10:1234", help="low level controller address:port")
    parser.add_argument("-test", action='store_true',help="testing mode, not using sockets")
    parser.add_argument("-toarray", action='store_true',help="convert encoded message to c byte array")
    parser.add_argument("-tofile", type=Path,help="don't send, store encoded message in file")
    Args = parser.parse_args()
    main(Args)
