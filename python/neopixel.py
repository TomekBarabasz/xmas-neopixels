import socket,argparse,re,struct
from time import sleep
from pathlib import Path
#rxSet = re.compile( r'\s*(\d+)-(\d+) (\d+)\:(\d+)\:(\d+)\s*(\w)?\s*' )
rxSet = re.compile( r'\s*(\d+)-(\d+) ([\dabcdefx]+)\:([\dabcdefx]+)\:([\dabcdefx]+)\s*(\w)?\s*' )

def toint(s): return int(s[1:],16) if s[0]=='x' else int(s)

#set 0-100 255:255:0, 100-100 0:255:0 r
def parse_set_command(cmd):
    try:
        subc = ' '.join(cmd).split(',')
        refresh = 0
        parsed = [rxSet.match(s).groups() for s in subc]
        print(parsed)
        refresh = any( [p[5]=='r' for p in parsed ])
        tosend = struct.pack("<BBB",0,len(subc),refresh)
        for p in parsed:
            pi = [toint(x) for x in p[0:-1]]
            tosend += struct.pack("<BBBHH", pi[2],pi[3],pi[4],pi[0],pi[1])
        return tosend
    except:
        return None

#start animation_id B:value H:value I:value
def parse_start_command(cmd,AnimPresets={}):
    if cmd[0] in AnimPresets:
        return AnimPresets[ cmd[0] ]
    anim_id = int(cmd[0])
    if len(cmd) > 1:
        prms = [ (s[0],toint(s[2:])) for s in cmd[1:] ]
        fmt,vals = zip(*prms)
        prms = struct.pack("<"+''.join(fmt),*vals)
        tosend = struct.pack("<BHH", 1, anim_id, len(prms)) + prms
    else:
        tosend = struct.pack("<BHH", 1, anim_id, 0)
    return tosend

def LoadAnimationPresets(filename):
    Presets = {}
    with filename.open() as fn:
        for line in fn:
            if line[0]=='#': continue
            split = line.strip(' \t\n').split(' ')
            print(split)
            Presets[ split[0] ] = parse_start_command(split[1:])
    return Presets

def parse_configure_command(cmd):
    return None

def parse_save_command(cmd):
    return None

def set_keepalive(sock):
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
    # Linux specific: after 10 idle minutes, start sending keepalives every 5 minutes. 
    # Drop connection after 10 failed keepalives
    if hasattr(socket, "TCP_KEEPIDLE") and hasattr(socket, "TCP_KEEPINTVL") and hasattr(socket, "TCP_KEEPCNT"):
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPIDLE,  60)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPINTVL, 60)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPCNT,   2)    

def pingpong(sock, cmd):
    def makeSetCommand(setprm,refresh):
        tosend = struct.pack("<BBB",0,len(setprm),refresh)
        for s in setprm:
            tosend += struct.pack("<BBBHH", *(s))
        return tosend
    s = cmd.split(' ')
    numLeds=int(s[1])
    nloop = int(s[2])
    delay = float(s[3])
    for _ in range(nloop):
        for i in range(1,numLeds):
            setprm = [(255,0,0,i,1),(0,0,0,i-1,1)]
            sent = sock.send( makeSetCommand(setprm, 2) )
            if sent==0:
                print('send failed, exiting')
                return
            sleep(delay)
        for i in range(numLeds-2,-1,-1):
            setprm = [(255,0,0,i,1),(0,0,0,i+1,1)]
            sent = sock.send( makeSetCommand(setprm, 2) )
            if sent==0:
                print('send failed, exiting')
                return
            sleep(delay)

def fakesend(bytes):
    print('sending ',bytes)
    return len(bytes)

def main(Args):
    if not Args.test:
        host,port = Args.addr.split(':')
        port = int(port)
        print(f'connecting {host} port {port} ...')    
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((host, port))
        set_keepalive(sock)
    print('available commands are:')
    print('set <from>-<to> r:g:b [refresh]')
    print('start <animation_id> [b:value|s:value|w:value]...')
    print('configure ')
    print('quit')
    AnimPresets = LoadAnimationPresets(Args.preset) if Args.preset is not None else {}
    Commands = {'set'       : (parse_set_command,()),
                'start'     : (parse_start_command,(AnimPresets,)),
                'configure' : (parse_configure_command,()),
                'pingpong'  : (pingpong,(socket,)),
                'save'      : (parse_save_command,())
    } 
    while(True):
        cmd = input(":")
        if cmd=='quit':
            break
        else:
            split = cmd.split(' ')
            keyword = split[0]
            if keyword in Commands:
                fcn,params = Commands[keyword]
                tosend = fcn(split[1:],*params,)
            else:
                print('invalid command')
                continue
        if tosend is not None:
            send = sock.send(tosend) if not Args.test else fakesend(tosend)
            if send == 0:
                print('send failed')
            elif send == len(tosend):
                print('send ok')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Neopixel high level contoller")
    parser.add_argument("-addr","-a", type = str, default="172.19.47.252:1234", help="low level controller address:port")
    parser.add_argument("-preset","-p",type=Path,help="animation presets file")
    parser.add_argument("-test", action='store_true',help="testing mode, not using sockets")
    Args = parser.parse_args()
    main(Args)


