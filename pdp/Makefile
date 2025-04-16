.PHONY: pdpfw clean

pdpfw: 
	macro11 pdpfw.mac -o pdpfw.obj
	obj2bin.pl --rt11 --binary --bytes=100000 --outfile=- pdpfw.obj | tail -c +7 | head -c -8 > autorun.bin

clean:
	rm -f pdpfw.obj
	rm -f autorun.bin
