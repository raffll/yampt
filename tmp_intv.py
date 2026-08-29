import struct

def subrecs(data, body_start, body_end):
    sp = body_start
    out = []
    while sp + 8 <= body_end:
        st = data[sp:sp+4]
        ss = struct.unpack_from("<I", data, sp+4)[0]
        sd = data[sp+8:sp+8+ss]
        out.append((st.decode('latin1'), ss, sd))
        sp += 8 + ss
    return out

path = r"c:\OMEN\Morrowind\tes3conv\Morrowind.esm"
data = open(path, "rb").read()
pos, n = 0, len(data)
shown = 0
while pos + 16 <= n and shown < 8:
    rt = data[pos:pos+4].decode('latin1')
    rs = struct.unpack_from("<I", data, pos+4)[0]
    bs = pos + 16
    be = bs + rs
    if rt == "INFO":
        subs = subrecs(data, bs, be)
        types = [s[0] for s in subs]
        if "SCVR" in types:
            print("--- INFO record sub-sequence ---")
            for st, ss, sd in subs:
                show = sd if st in ("SCVR","INTV","FLTV","INAM","NAME","SNAM") else b''
                print(f"  {st:4} size={ss} {show!r}")
            shown += 1
    pos = be
