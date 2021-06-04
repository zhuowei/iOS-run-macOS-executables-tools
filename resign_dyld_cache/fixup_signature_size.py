# sets the signature size
import sys
import struct
with open(sys.argv[1], "r+b") as infile:
  indata = infile.read(0x40);
  codeoff = struct.unpack_from("Q", indata, 0x28)[0]
  print(hex(codeoff))
  endloc = infile.seek(0, 2) # end
  sigsize = endloc - codeoff
  print(hex(endloc), hex(sigsize))
  infile.seek(0x30)
  infile.write(struct.pack("Q", sigsize))
