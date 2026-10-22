from pathlib import Path
import zipfile

src = Path(r"F:\chuanganqi\Objects\chuangganqi0924_fixed.pdsprj")

def write_project(dst: Path, setpoint: str | None = None) -> None:
    with zipfile.ZipFile(src, "r") as zin:
        entries = {info.filename: zin.read(info.filename) for info in zin.infolist()}
    if setpoint is not None:
        for name in ("ROOT.DSN", "ROOT.CDB"):
            payload = entries[name]
            payload = payload.replace(b"{SETPOINT=29.0}", f"{{SETPOINT={setpoint}}}".encode(), 1)
            entries[name] = payload
    with zipfile.ZipFile(dst, "w", compression=zipfile.ZIP_DEFLATED) as zout:
        for name, payload in entries.items():
            zout.writestr(name, payload)

write_project(Path(r"F:\chuanganqi\Objects\chuangganqi0924_code_fixed.pdsprj"))
write_project(Path(r"F:\chuanganqi\Objects\chuangganqi0924_code_fixed_test35.pdsprj"), "35.0")
