from __future__ import annotations

import io
import struct
import zipfile
from pathlib import Path


SRC = Path(r"F:\chuanganqi\Objects\chuangganqi0924_fixed.pdsprj")
DST = Path(r"F:\chuanganqi\Objects\chuangganqi0924_motor_fixed.pdsprj")

NPN_PROPERTIES = b"{PRIMITIVE=ANALOGUE,NPN}\n{MODEL=LX_NPN_SSHF,BIPOLAR}\n{PACKAGE=TO18}\n"
PNP_PROPERTIES = b"{PACKAGE=TO92}\n{SPICELIB=FAIRCHLD}\n{PRIMITIVE=ANALOGUE,PNP}\n{SPICEMODEL=2N4402}\n"


def find_component_id(data: bytes, ref: bytes) -> int:
    ref_pos = data.find(ref, 119000)
    if ref_pos < 0:
        raise RuntimeError(f"reference not found: {ref!r}")
    pos = data.find(b"COMPONENT ID", ref_pos)
    if pos < 0:
        raise RuntimeError(f"component block not found: {ref!r}")
    return pos


def patch_block(data: bytearray, ref: bytes, old: bytes, new: bytes, props: bytes) -> bytearray:
    start = find_component_id(bytes(data), ref)
    next_pos = data.find(b"COMPONENT ID", start + len(b"COMPONENT ID"))
    if next_pos < 0:
        next_pos = data.find(b"ISIS CIRCUIT FILE", start)
    if next_pos < 0:
        raise RuntimeError(f"end of component block not found: {ref!r}")

    block = bytes(data[start:next_pos]).replace(old, new)
    prop = block.find(b"PROPERTIES")
    if prop < 0:
        raise RuntimeError(f"properties not found: {ref!r}")
    payload_start = prop + len(b"PROPERTIES") + 9
    old_len = struct.unpack_from("<I", block, prop + len(b"PROPERTIES") + 5)[0]
    payload_end = payload_start + old_len
    replacement = b"\x00" * 5 + struct.pack("<I", len(props)) + props
    block = block[: prop + len(b"PROPERTIES")] + replacement + block[payload_end:]
    data[start:next_pos] = block
    return data


def main() -> None:
    with zipfile.ZipFile(SRC, "r") as zin:
        entries = {info.filename: zin.read(info.filename) for info in zin.infolist()}

    dsn = bytearray(entries["ROOT.DSN"])
    # Complementary BJT H-bridge: PNP high side (Q1/Q4), NPN low side (Q2/Q3).
    for ref in (b"Q3", b"Q2"):
        dsn = patch_block(dsn, ref, b"2N4402", b"2N2222", NPN_PROPERTIES)
    for ref in (b"Q4", b"Q1"):
        dsn = patch_block(dsn, ref, b"2N2222", b"2N4402", PNP_PROPERTIES)
    entries["ROOT.DSN"] = bytes(dsn)

    with zipfile.ZipFile(DST, "w", compression=zipfile.ZIP_DEFLATED) as zout:
        for name, payload in entries.items():
            zout.writestr(name, payload)


if __name__ == "__main__":
    main()
