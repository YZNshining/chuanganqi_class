from pathlib import Path
import zipfile

src = Path(r"F:\chuanganqi\Objects\chuangganqi0924_motor_fixed.pdsprj")
dst = Path(r"F:\chuanganqi\Objects\chuangganqi0924_motor_fixed3.pdsprj")

NPN = b"{PRIMITIVE=ANALOGUE,NPN}\n{MODEL=LX_NPN_SSHF,BIPOLAR}\n{PACKAGE=TO18}\n"
PNP = b"{PACKAGE=TO92}\n{SPICELIB=FAIRCHLD}\n{PRIMITIVE=ANALOGUE,PNP}\n{SPICEMODEL=2N4402}\n"

with zipfile.ZipFile(src, "r") as zin:
    entries = {info.filename: zin.read(info.filename) for info in zin.infolist()}

data = bytearray(entries["ROOT.CDB"])

def patch_entry(ref: bytes, old: bytes, new: bytes, old_props: bytes, new_props: bytes) -> None:
    global data
    start = bytes(data).find(ref, 3000)
    if start < 0:
        raise RuntimeError(f"missing component {ref!r}")
    prop = bytes(data).find(old_props, start)
    if prop < 0:
        raise RuntimeError(f"missing properties for {ref!r}")
    end = prop + len(old_props)
    block = bytes(data[start:end]).replace(old_props, new_props)
    block = block.replace(old, new)
    data[start:end] = block

# Process from the end of the database so earlier offsets remain valid.
for ref, old, new, old_props, new_props in [
    (b"Q3", b"2N4402", b"2N2222", PNP, NPN),
    (b"Q2", b"2N4402", b"2N2222", PNP, NPN),
    (b"Q4", b"2N2222", b"2N4402", NPN, PNP),
    (b"Q1", b"2N2222", b"2N4402", NPN, PNP),
]:
    patch_entry(ref, old, new, old_props, new_props)

entries["ROOT.CDB"] = bytes(data)
with zipfile.ZipFile(dst, "w", compression=zipfile.ZIP_DEFLATED) as zout:
    for name, payload in entries.items():
        zout.writestr(name, payload)

print(dst)
