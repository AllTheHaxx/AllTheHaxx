# ##################################### #
# #   make_callstack_creators.py      # #
# #   (c) 2016 The AllTheHaxx Team    # #
# #   script written by Henritees     # #
# ##################################### #

import os
import re
import time

exclude_files = ["sound.cpp", "db_sqlite3.cpp", "debug.cpp"]
exclude_functions = [
    "::OnReset", "CTranslator::Init", "CTranslator::TranslationWorker", "CVoting::ClearOptions"
]

# this will cover the most, but unfortunately not all
cpp_return_keywords = [
    "const", "double", "float", "int", "short", "unsigned", "bool",
    "signed", "void", "char", "class", "CServerInfo", ""
]

Stats = { "FilesScanned": 0, "FunctionsScanned": 0, "CallstackCreatorsAdded": 0, "FilesIgnored": 0, "FunctionsExcluded": 0 }


def check_file(filename):
    # print "checking " + filename
    for f in exclude_files:
        if filename.find(f) > 0:
            print "Ignoring file '" + filename + "'"
            Stats["FilesIgnored"] += 1
            return

    Stats["FilesScanned"] += 1

    NewFileContents = []
    WholeFile = []
    file = open(filename, 'r')
    while 1:
        line = file.readline()
        if len(line) == 0:
            break
        WholeFile.append(line)

    LastWasFound = False
    for i in range(0, len(WholeFile)):  # iterate over all lines; no iterator used because we need the line number
        curr_line = WholeFile[i]

        if not LastWasFound:
            NewFileContents.append(curr_line)  # line in the line in the line of the line on top of the line

        LastWasFound = False
        for keyword in cpp_return_keywords:
            found = re.findall(r'' + keyword + r'[\*|\s]*' + r'C[A-Z][a-z]*::[\w|_]+\([A-Za-z\*\w\s&:,^\)]*\)', curr_line)  # (\*?)*\s(\*?)*  ##[\w|_|\*|\s|::]  ###(.*?)

            # is this a function?
            if len(found) == 0:
                continue

            Stats["FunctionsScanned"] += 1

            # does a callstack creator already exist here?
            if line.lstrip('\t').startswith("//") or WholeFile[i+2].find("CALLSTACK_ADD();") > 0:
                continue

            # do we want to exclude this function?
            ExcludeThis = False
            for k in exclude_functions:
                if found[0].find(k) > 0:
                    ExcludeThis = True
            if ExcludeThis:
                print "Excluding function " + found[0]
                Stats["FunctionsExcluded"] += 1
                break

            #while curr_line.find(')') <= 0: # this should allow multi-line function headers, but is currently disabled due to being shit
            #    i += 1
            #    curr_line = WholeFile[i]
            #    NewFileContents.append(curr_line)
            try:
                print "Adding in " + filename.lstrip("../src/") + ":" + str(i+1) + ' to function "' + found[0] + '"'
            except AttributeError as e:
                print "strange error! " + str(e) + ":"
                print '\t' + str(found)

            NewFileContents += WholeFile[i+1]  # bracket
            NewFileContents.append("\tCALLSTACK_ADD();\n\n")
            LastWasFound = True
            Stats["CallstackCreatorsAdded"] += 1
            break

    file.close()

    wfile = open(filename, "w")
    for line in NewFileContents:
        wfile.write(line)
    wfile.close()


def check_dir(directory):
    list = os.listdir(directory)
    for file in list:
        if os.path.isdir(directory + file):
            if file != "external" and file != "generated" and file != "server":
                check_dir(directory + file + "/")
        elif file[-4:] == ".cpp":
            check_file(directory + file)


# create ourselves a nice timer
timer = time.clock()

# we only want to have callstack creation in the client-only code I guess...
check_dir("../src/engine/client/")
check_dir("../src/game/client/")

seconds = time.clock()-timer
print "\ndone (%i:%02i)" % (round(seconds/60.0), round(seconds % 60.0))
print "  Scanned %i functions in %i files and added %i Callstack Creators." % (Stats["FunctionsScanned"], Stats["FilesScanned"], Stats["CallstackCreatorsAdded"])
print "  A total of %i files were ignored" % Stats["FilesIgnored"]
print "  and %i functions in scanned files were excluded." % Stats["FunctionsExcluded"]
print "The whole job took exactly %f seconds" % seconds
print ""
