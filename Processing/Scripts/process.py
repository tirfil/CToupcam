import sys
import os
import shutil
import re


def mkdir(d):
	if not os.path.isdir(d):
		os.mkdir(d)
		
def w(d):
	return "work" + os.sep + d

mkdir("work")

if not os.path.isdir("light"):
	print "No light"
	exit(0);
	
if os.path.isfile(w("masterdark.fits")):
	pass
else:
	if os.path.isdir("dark"):
		cmd = "./bin/median8 %s %s" % ("dark",w("masterdark.fits"))
		print cmd
		os.system(cmd)
		
if os.path.isfile(w("masterdarkflat.fits")):
	pass
else:
	if os.path.isdir("darkflat"):
		cmd = "./bin/median8 %s %s" % ("darkflat",w("masterdarkflat.fits"))
		print cmd
		os.system(cmd)
		
mkdir(w("red"))
mkdir(w("green"))
mkdir(w("blue"))

print("---- split light color\n")

for root, dirs, files in os.walk("light"):
	for name in files:
		source = root + os.sep + name
		cmd = "./bin/rgb32fitssplit %s" % source
		os.system(cmd)
		for color in ["red","green","blue"]:
			source = color + ".fits"
			target = color + os.sep + name
			shutil.move(source,w(target))

print("---- split flat color\n")

if os.path.isdir("flat"):
	mkdir(w("redflat"))
	mkdir(w("greenflat"))
	mkdir(w("blueflat"))
	for root, dirs, files in os.walk("flat"):
		for name in files:
			source = root + os.sep + name
			cmd = "./bin/rgb32fitssplit %s" % source
			os.system(cmd)
			for color in ["red","green","blue"]:
				source = color + ".fits"
				target = color + "flat" + os.sep + name
				shutil.move(source,w(target))
				
print("---- make master flats\n")

for directory in ["redflat","greenflat","blueflat"]:				
	if os.path.isdir(w(directory)):
		tempo = "m" + directory + ".fits"
		cmd = "./bin/median8 %s %s" % (w(directory),w(tempo))
		os.system(cmd)
		target = "master" + directory + ".fits"
		cmd = "./bin/substract8 %s %s %s" % (w(tempo),w("masterdarkflat.fits"),w(target))		
		os.system(cmd)
		
print("---- make light minus dark\n")

for color in ["red","green","blue"]:
	directory = color + "1"
	mkdir(w(directory))	
	for root, dirs, files in os.walk(w(color)):
		for name in files:
			source = root + os.sep + name
			target = directory + os.sep + name
			cmd = "./bin/substract8 %s %s %s" % (source,w("masterdark.fits"),w(target))		
			os.system(cmd)

print("---- divide by flat\n")			
					
for color in ["red","green","blue"]:
	flat = "master" + color + "flat.fits"
	directory1 = color + "1"
	directory2 = color + "2"
	mkdir(w(directory2))
	for root, dirs, files in os.walk(w(directory1)):
		for name in files:
			source = root + os.sep + name
			target = directory2 + os.sep + name
			cmd = "./bin/divide8 %s %s %s" % (source, w(flat), w(target))
			os.system(cmd)
			
#align
print("---- align lights\n")

first = True
for color in ["red","green","blue"]:
	directory1 = color + "2"
	directory2 = color + "3"
	mkdir(w(directory2))
	for root, dirs, files in os.walk(w(directory1)):
		for name in files:
			source = root + os.sep + name		
			if first:
				first = False
				refname = name
			reference = root + os.sep + refname
			target = directory2 + os.sep + name
			cmd = "./bin/center3 %s %s %s 20" % (reference, source, w(target))
			os.system(cmd)
			
for color in ["red","green","blue"]:
	directory = color + "3"
	target = "master" + color + ".fits"
	cmd = "./bin/median8 %s %s" % (w(directory),w(target))
	os.system(cmd)
	
cmd = "ds9 -rgb -red work/masterred.fits -green work/mastergreen.fits -blue work/masterblue.fits"
os.system(cmd)
		
			
		
		
