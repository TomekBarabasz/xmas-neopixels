import socket,argparse,re,struct,sys,keyboard
from time import sleep
from pathlib import Path
from collections import namedtuple
#192.168.1.16:123
#172.19.47.252:1234

RGB = namedtuple('RGB',['r','g','b'])
HSV = namedtuple('HSV',['h','s','v'])
def makeRGB(rgb):
    return RGB(rgb >> 16, (rgb >> 8)&0xFF, rgb & 0xFF)

def set_keepalive(sock):
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
    # Linux specific: after 10 idle minutes, start sending keepalives every 5 minutes. 
    # Drop connection after 10 failed keepalives
    if hasattr(socket, "TCP_KEEPIDLE") and hasattr(socket, "TCP_KEEPINTVL") and hasattr(socket, "TCP_KEEPCNT"):
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPIDLE,  60)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPINTVL, 60)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPCNT,   2) 

def connect(addr):
    host,port = addr.split(':')
    port = int(port)
    print(f'connecting {host} port {port} ...')    
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((host, port))
    set_keepalive(sock)
    print('connected')
    return sock

#set 0-100 255:255:0, 100-100 0:255:0 r
def makeSetCommand(Commands, refresh):
    tosend = struct.pack("<BBB",0,len(Commands),1 if refresh else 0)
    for cmd in Commands:
        start,cnt,color = cmd
        tosend += struct.pack("<BBBHH", color.r, color.g, color.b,start,cnt)
    return tosend

class ControllerOneLed:
    def __init__(self,sock,keyboard, **kwargs):
        self.currentPos = 0
        self.color = kwargs.get('color',RGB(255,0,0) )
        self.bck = kwargs.get('bgcolor',RGB(0,0,0) )
        self.sock = sock
        self.add_hotkeys(keyboard)

    def refresh(self):
        print("current pos",self.currentPos,end='\r')
        tosend = makeSetCommand( ((0,450,self.bck), (self.currentPos,1,self.color)),True)
        send = self.sock.send(tosend)
        if send == 0:   print('send failed')

    def add_hotkeys(self, keyboard):
        keyboard.add_hotkey('left',self.onLeft)
        keyboard.add_hotkey('right',self.onRight)

    def onLeft(self):
        if self.currentPos > 0:
            self.currentPos -= 1
            self.refresh()
    def onRight(self):
        if self.currentPos < 450:
            self.currentPos += 1
            self.refresh()
    def onUp(self):
        pass
    def onDown(self):
        pass
    def run(self):
        keyboard.wait('q')

VStrips = [(0,28, 0), (30,26, 1),(57,26, 0),(85,24, 1),(110,26, 0),(137,26, 1),(164,27, 0),(193,22, 1),(214,21, 0),(237,12, 1)]
class ControllerHorizontal:  
    def __init__(self,sock,keyboard,**kwargs):
        self.currentRow = 0
        self.color = kwargs.get('color',RGB(255,0,0) )
        self.bck = kwargs.get('bgcolor',RGB(0,0,0) )
        self.sock = sock
        self.add_hotkeys(keyboard)

    def add_hotkeys(self, keyboard):
        keyboard.add_hotkey('left',self.onLeft)
        keyboard.add_hotkey('right',self.onRight)
    
    def refresh(self):
        print("current pos",self.currentPos,end='\r')
        tosend = makeSetCommand( ((0,250,self.bck), (self.currentPos,1,self.color)),True)
        send = self.sock.send(tosend)
        if send == 0:   print('send failed')

    def onLeft(self):
        if self.currentPos > 0:
            self.currentPos -= 1
            self.refresh()

    def onRight(self):
        if self.currentPos < 249:
            self.currentPos += 1
            self.refresh()
    def run(self):
        keyboard.wait('q')

class ControllerVertical:
    def __init__(self,sock,keyboard,**kwargs):
        self.currentStrip = 0
        self.color = kwargs.get('color',RGB(255,0,0) )
        self.bck = kwargs.get('bgcolor',RGB(0,0,0) )
        self.bck = RGB(0,0,0)
        self.sock = sock
        self.add_hotkeys(keyboard)

    def add_hotkeys(self,keyboard):
        keyboard.add_hotkey('left',self.onLeft)
        keyboard.add_hotkey('right',self.onRight)

    def refresh(self):
        print("current strip",self.currentStrip,end='\r')
        start,cnt,dir = VStrips[self.currentStrip]
        tosend = makeSetCommand( ((0,250,self.bck), (start,cnt,self.color)),True)
        send = self.sock.send(tosend)
        if send == 0:   print('send failed')

    def onLeft(self):
        if self.currentStrip > 0:
            self.currentStrip -= 1
            self.refresh()

    def onRight(self):
        if self.currentStrip < len(VStrips)-1:
            self.currentStrip += 1
            self.refresh()
    
    def run(self):
        keyboard.wait('q')

def main(Ctrl, args):
    parser = argparse.ArgumentParser(description="Neopixel high level contoller")
    parser.add_argument("-addr","-a", type = str, default="172.19.47.252:1234", help="low level controller address:port")
    parser.add_argument("-color","-c", type = int, default=0xFFFFFF, help="foreground color")
    parser.add_argument("-bg",type = int, default=0, help="background color")
    Args = parser.parse_args(sys.argv[2:])
    sock = connect(Args.addr)
    #stop current animation
    tosend = struct.pack("<BHH", 1, 255, 0)
    send = sock.send(tosend)
    color=RGB(255,255,255)
    tosend = makeSetCommand( ((0,250,RGB(0,0,0)),(0,1,color)),True )
    send = sock.send(tosend)
    if send == 0:   print('send failed')
    prms={}
    if Args.color is not None:
        prms['color'] = makeRGB(Args.color)
    if Args.bg is not None:
        prms['bgcolor'] = makeRGB(Args.bg)
    ctrl = Ctrl(sock,keyboard,**prms)
    ctrl.run()

def test(args):
    pass

if __name__ == '__main__':
    cmd = sys.argv[1]
    Controllers = {'one' : ControllerOneLed, 'vert' : ControllerVertical, 'hor' : ControllerHorizontal}
    if cmd in Controllers:
        main(Controllers[cmd], sys.argv[2:])
    else:
        print('valid commands are',Controllers.keys())
