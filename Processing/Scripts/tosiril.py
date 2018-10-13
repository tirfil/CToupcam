import sys
import os
import shutil
import re

rx_num = re.compile("_([0-9]*)$")


def mkdir(d):
	if not os.path.isdir(d):
		os.mkdir(d)
		
if not os.path.isdir("light"):
	print "No light"
	exit(0);

directory = "siril"
mkdir(directory)
selection="light"

for root, dirs, files in os.walk(selection):
	for name in files:
		source = root + os.sep + name
		base, ext = os.path.splitext(name)
		md = rx_num.search(base)
		if md:
			target = directory + os.sep + selection + "_" + md.group(1) + ".fit"
			shutil.copyfile(source,target)
		
		
if not os.path.isdir("dark"):
	print "No dark"
	exit(0);

mkdir(directory)
selection="dark"

for root, dirs, files in os.walk(selection):
	for name in files:
		source = root + os.sep + name
		base, ext = os.path.splitext(name)
		md = rx_num.search(base)
		if md:
			target = directory + os.sep + selection + "_" + md.group(1) + ".fit"
			shutil.copyfile(source,target)
			
