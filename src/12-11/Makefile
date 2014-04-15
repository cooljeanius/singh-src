all: hfs_change_next_allocation

hfs_change_next_allocation: hfs_change_next_allocation.c
	gcc -Wall -o $@ $<

# can change this if you still have the original hfsdebug lying around:
HFSDEBUG = fileXray

check: hfshint.dmg hfs_change_next_allocation
	open hfshint.dmg
	sleep 1
	$(HFSDEBUG) -V /Volumes/HFSHint -0
	echo "hello" >> /Volumes/HFSHint/file.txt
	-$(HFSDEBUG) /Volumes/HFSHint/file.txt 2>/dev/null
	$(HFSDEBUG) -V /Volumes/HFSHint -0
	./hfs_change_next_allocation /Volumes/HFSHint 0xbbb
	echo "helloagain" >> /Volumes/HFSHint/anotherfile.txt
	-$(HFSDEBUG) /Volumes/HFSHint/anotherfile.txt 2>/dev/null

hfshint.dmg:
	hdiutil create -size 32m -fs HFSJ -volname HFSHint hfshint.dmg

# not sure how to safely clean the disk image...
clean:
	rm -f hfs_change_next_allocation
