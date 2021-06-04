# removes the existing code signature from a dyld cache
import sys
import struct
with open(sys.argv[1], "r+b") as infile:
  indata = infile.read(0x40);
  codeoff = struct.unpack_from("Q", indata, 0x28)[0]
  print(hex(codeoff))
  infile.truncate(codeoff)
  infile.seek(0x30)
  infile.write(b"\x00"*8)
