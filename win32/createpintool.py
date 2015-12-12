#
# Quick 'n dirty script to create a new Pin Tool
# corelanc0d3r
# www.corelan.be
#
# Place this file in the source/tools folder of your pin installation
# and run it from there

import os, sys, shutil


def create_pin_project(projectname):
	print "[+] Creating new PIN project %s" % projectname
	print "    - Copying clean project"
	currentfolder = os.getcwd()
	projectsource = os.path.join(currentfolder,"MyPinTool")
	newproject = os.path.join(currentfolder,projectname)
	shutil.copytree(projectsource,newproject)
	print "[+] Updating project files"
	dirList = os.listdir(newproject)
	for fname in dirList:
		currentfile = os.path.join(newproject,fname)
		newfilename = fname.replace("MyPinTool",projectname)
		print "    - Processing %s -> %s" % (fname,newfilename)
		filecontents = open(currentfile,"rb").read()
		os.remove(os.path.join(newproject,fname))
		updatedcontents = filecontents.replace("MyPinTool",projectname)
		objfile = open(os.path.join(newproject,newfilename),"wb")
		objfile.write(updatedcontents)
		objfile.close()
	print "[+] Done"
	return



# main
projectname = ""
if len(sys.argv) > 1:
	projectname = sys.argv[1]
else:
	print "Please provide a destination Pin project name"
	sys.exit()

create_pin_project(projectname)