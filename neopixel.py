import socket,argparse,re,struct
from time import sleep

rxSet = re.compile( r'\s*(\d+)-(\d+) (\d+)\:(\d+)\:(\d+)\s*(\w)?\s*' )

#set 0-99 255:255:0, 100,199 0:255:0 r
def parse_set_command(cmd):
    try:
        subc = cmd.split(',')
        refresh = 0
        parsed = [rxSet.match(s).groups() for s in subc]
        refresh = any( [p[5]=='r' for p in parsed ])
        tosend = struct.pack("<BBB",0,len(subc),refresh)
        for p in parsed:
            pi = [int(x) for x in p[0:-1]]
            tosend += struct.pack("<BBBHH", pi[2],pi[3],pi[4],pi[0],pi[1])
        return tosend
    except:
        return None

#start animation_id B:value H:value I:value
def parse_start_command(cmd):
    subc = cmd.strip().split(' ')
    anim_id = int(subc[0])
    if len(subc) > 1:
        prms = [ (s[0],int(s[2:])) for s in subc[1:] ]
        fmt,vals = zip(*prms)
        prms = struct.pack("<"+''.join(fmt),*vals)
        tosend = struct.pack("<BHH", 1, anim_id, len(prms)) + prms
    else:
        tosend = struct.pack("<BHH", 1, anim_id, 0)
    return tosend

def parse_configure_command(cmd):
    return None

def set_keepalive(sock):
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
    # Linux specific: after 10 idle minutes, start sending keepalives every 5 minutes. 
    # Drop connection after 10 failed keepalives
    if hasattr(socket, "TCP_KEEPIDLE") and hasattr(socket, "TCP_KEEPINTVL") and hasattr(socket, "TCP_KEEPCNT"):
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPIDLE,  60)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPINTVL, 60)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPCNT,   2)    

def makeSetCommand(setprm,refresh):
    tosend = struct.pack("<BBB",0,len(setprm),refresh)
    for s in setprm:
         tosend += struct.pack("<BBBHH", *(s))
    return tosend

def pingpong(sock, nloop):
    numLeds=150
    for _ in range(nloop):
        for i in range(1,numLeds):
            setprm = [(255,0,0,i,1),(0,0,0,i-1,1)]
            sent = sock.send( makeSetCommand(setprm, 2) )
            if sent==0:
                print('send failed, exiting')
                return
            sleep(0.1)
        for i in range(numLeds-2,-1,-1):
            setprm = [(255,0,0,i,1),(0,0,0,i+1,1)]
            sent = sock.send( makeSetCommand(setprm, 2) )
            if sent==0:
                print('send failed, exiting')
                return
            sleep(0.1)

def main(Args):
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
    while(True):
        cmd = input(":")
        if cmd=='quit':
            break
        elif cmd.startswith('set'):
           tosend = parse_set_command(cmd[3:])
        elif cmd.startswith('start'):
            tosend = parse_start_command(cmd[5:])
        elif cmd.startswith('configure'):
            tosend = parse_start_command(cmd[9:])
        elif cmd.startswith('pingpong'):
            nloop = int(cmd.split(' ')[1])
            pingpong(sock,nloop)
            continue
        if tosend is not None:
            send = sock.send(tosend)
            if send == 0:
                print('send failed')
            elif send == len(tosend):
                print('send ok')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Neopixel high level contoller")
    parser.add_argument("-addr","-a", type = str, default="172.19.47.252:1234", help="low level controller address:port")
    Args = parser.parse_args()
    main(Args)


