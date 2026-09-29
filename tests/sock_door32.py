import os, socket, subprocess, sys, time, select
binary, workdir = sys.argv[1], sys.argv[2]
a, b = socket.socketpair()
b.set_inheritable(True)
drop = os.path.join(workdir, "door32.sys")
open(drop, "w").write("2\n%d\n38400\nANetBBS\n1\nTest Caller\nTester\n100\n60\n1\n1\n" % b.fileno())
p = subprocess.Popen([binary, "-D", drop], pass_fds=(b.fileno(),), cwd=workdir,
                     stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
b.close()
def rd(t):
    out=b''; end=time.time()+t
    while time.time()<end:
        r,_,_=select.select([a],[],[],0.1)
        if r:
            d=a.recv(65536)
            if not d: break
            out+=d
    return out
out=rd(2.5); a.sendall(b'x'); out+=rd(2.5)
try: rc=p.wait(3)
except subprocess.TimeoutExpired: p.kill(); rc="HUNG"
hi=sorted(set(c for c in out if c>=0x80))
bare=sum(1 for i,c in enumerate(out) if c==10 and (i==0 or out[i-1]!=13))
print("rc:",rc,"bytes:",len(out),"bare LF:",bare)
print("high bytes:",[hex(c) for c in hi])
print("contains UTF-8 lead 0xe2:", 0xe2 in out)
print(out.decode('cp437').replace('\x1b','^['))
print("stderr:", p.stderr.read()[:300])
