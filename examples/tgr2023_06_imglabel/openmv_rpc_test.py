import json, rpc, serial, serial.tools.list_ports, struct, sys
from datetime import datetime
import time

rpc_master = rpc.rpc_usb_vcp_master('COM86')
result = rpc_master.call("jpeg_image_snapshot", recv_timeout=10000)
print(dir(result))
print(result.nbytes)
jpg_sz = int.from_bytes(result.tobytes(), "little")
print(jpg_sz)

buf = bytearray(b'\x00'*jpg_sz)
result = rpc_master.call("jpeg_image_read", recv_timeout=10000)
rpc_master.get_bytes(buf, jpg_sz)
print(len(buf))
with open("test.jpg", "wb") as f:
    f.write(buf)