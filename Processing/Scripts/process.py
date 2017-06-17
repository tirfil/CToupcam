import sys
import os
import shutil
import re

if not os.path.isdir("light"):
	print "No light"
	exit(0);
	
if os.path.isfile("masterdark.fits"):
	pass
else:
	if os.path.isdir("dark"):
		cmd = "./bin/median8 %s %s" % ("dark","masterdark.fits")
		print cmd
		os.system(cmd)
		
if os.path.isfile("masterdarkflat.fits"):
	pass
else:
	if os.path.isdir("darkflat"):
		cmd = "./bin/median8 %s %s" % ("darkflat","masterdarkflat.fits")
		print cmd
		os.system(cmd)
		
os.mkdir("red")
os.mkdir("green")
os.mkdir("blue")

print("---- split light color\n")

for root, dirs, files in os.walk("light"):
	for name in files:
		source = root + os.sep + name
		cmd = "./bin/rgb32fitssplit %s" % source
		os.system(cmd)
		for color in ["red","green","blue"]:
			source = color + ".fits"
			target = color + os.sep + name
			shutil.move(source,target)

print("---- split flat color\n")

if os.path.isdir("flat"):
	os.mkdir("redflat")
	os.mkdir("greenflat")
	os.mkdir("blueflat")
	for root, dirs, files in os.walk("flat"):
		for name in files:
			source = root + os.sep + name
			cmd = "./bin/rgb32fitssplit %s" % source
			os.system(cmd)
			for color in ["red","green","blue"]:
				source = color + ".fits"
				target = color + "flat" + os.sep + name
				shutil.move(source,target)
				
print("---- make master flats\n")

for directory in ["redflat","greenflat","blueflat"]:				
	if os.path.isdir(directory):
		tempo = "m" + directory + ".fits"
		cmd = "./bin/median8 %s %s" % (directory,tempo)
		os.system(cmd)
		target = "master" + directory + ".fits"
		cmd = "./bin/substract8 %s %s %s" % (tempo,"masterdarkflat.fits",target)		
		os.system(cmd)
		
print("---- make light minus dark\n")

for color in ["red","green","blue"]:
	directory = color + "1"
	os.mkdir(directory)	
	for root, dirs, files in os.walk(color):
		for name in files:
			source = root + os.sep + name
			target = directory + os.sep + name
			cmd = "./bin/substract8 %s %s %s" % (source,"masterdark.fits",target)		
			os.system(cmd)

print("---- divide by flat\n")			
					
for color in ["red","green","blue"]:
	flat = "master" + color + "flat.fits"
	directory1 = color + "1"
	directory2 = color + "2"
	os.mkdir(directory2)
	for root, dirs, files in os.walk(directory1):
		for name in files:
			source = root + os.sep + name
			target = directory2 + os.sep + name
			cmd = "./bin/divide8 %s %s %s" % (source, flat, target)
			os.system(cmd)
			
#align
print("---- align lights\n")

for color in ["red","green","blue"]:
	directory1 = color + "2"
	directory2 = color + "3"
	os.mkdir(directory2)
	for root, dirs, files in os.walk(directory1):
		first = True
		for name in files:
			source = root + os.sep + name		
			if first:
				reference = source
				first = False
			target = directory2 + os.sep + name
			cmd = "./bin/center2 %s %s %s 20" % (reference, source, target)
			os.system(cmd)
			
for color in ["red","green","blue"]:
	directory = color + "3"
	target = "master" + color + ".fits"
	cmd = "./bin/median8 %s %s" % (directory,target)
	os.system(cmd)
	
cmd = "ds9 -rgb -red masterred.fits -green mastergreen.fits -blue masterblue.fits"
os.system(cmd)
		
			
		
		
