hello: 
	macro11 pdpfw.mac -o pdpfw.obj
	obj2bin.pl --binary --nocrc --bytes=100000  --outfile=pdpfw.bin pdpfw.obj
	dd if=pdpfw.bin of=smp0.bin bs=1 skip=6
	# We don't really need to pad this to 10kb as it will be in an ATMEGA
	#dd if=/dev/zero of=smp0.bin bs=1 count=1 seek=10240
	rm pdpfw.bin

clean:
	rm pdpfw.obj
	rm smp0.bin

