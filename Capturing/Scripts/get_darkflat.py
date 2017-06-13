import time
import sys
import os
import shutil

directory = "darkflat"

duration = sys.argv[1] # ms
gain = sys.argv[2]
number = int(sys.argv[3])

# Create repositories
if not os.path.isdir("work"):
	os.mkdir("work")

if not os.path.isdir("results"):
	os.mkdir("results")
	
if not os.path.isdir("results/raw"):
	os.mkdir("results/raw")
	
if not os.path.isdir("results/raw/"+directory):
	os.mkdir("results/raw/"+directory)
	
os.chdir("work")

for i in range(number):
	now = time.localtime()
	base =time.strftime("%Y%m%d_%H%M%S",now)
	cmd = path + "MonoCapture8 " + duration + " " + gain
	os.system(cmd)
	destination = "../results/raw/" + directory + "/" + base + "_" + str(i) +".fits"
	shutil.move("mono.fits",destination)
