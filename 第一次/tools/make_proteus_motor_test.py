from pathlib import Path
import zipfile

src = Path(r"F:\chuanganqi\Objects\chuangganqi0924_motor_fixed.pdsprj")
dst = Path(r"F:\chuanganqi\Objects\chuangganqi0924_motor_test35.pdsprj")

with zipfile.ZipFile(src, "r") as zin:
    entries = {info.filename: zin.read(info.filename) for info in zin.infolist()}

dsn = entries["ROOT.DSN"]
old = b"{SETPOINT=29.0}"
new = b"{SETPOINT=35.0}"
if old not in dsn:
    raise RuntimeError("DS18B20 setpoint 29.0 was not found")
entries["ROOT.DSN"] = dsn.replace(old, new, 1)

with zipfile.ZipFile(dst, "w", compression=zipfile.ZIP_DEFLATED) as zout:
    for name, payload in entries.items():
        zout.writestr(name, payload)

print(dst)
