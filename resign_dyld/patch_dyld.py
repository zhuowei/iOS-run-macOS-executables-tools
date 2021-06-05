import sys
with open("thepatch_full.o", "rb") as infile:
  indata = infile.read()
patch1 = indata[indata.find(b"GOT1") + 4:indata.find(b"END1")]
patch2 = indata[indata.find(b"GOT2") + 4:indata.find(b"END2")]
with open(sys.argv[1], "r+b") as infile:
  infile.seek(0x0000000000045ac8)
  infile.write(patch2)
  infile.seek(0x0000000000063994)
  infile.write(patch1)

