#Niflyze - Python Version

from __future__ import generators

from niflib import *
import os
import glob
import sys
import stat

# the walktree function is from http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/200131
def walktree(top = ".", depthfirst = True):
    """Walk the directory tree, starting from top. Credit to Noah Spurrier and Doug Fort."""
    names = os.listdir(top)
    if not depthfirst:
        yield top, names
    for name in names:
        try:
            st = os.lstat(os.path.join(top, name))
        except os.error:
            continue
        if stat.S_ISDIR(st.st_mode):
            for (newtop, children) in walktree (os.path.join(top, name), depthfirst):
                yield newtop, children
    if depthfirst:
        yield top, names
def PrintHelpInfo():
    print """Usage:  niflyze -switch_1 value_1 -switch_2 value_2 ... switch_n value_n
Example:  niflyze -i model.nif -o output.txt

Switches:
-? [Help]
   Displays This Information

-i [Input File]
   The nif file(s) to analyze.  Can include wildcards.
   Example: model.nif
   Default: *.nif

-o [Output File]
   The text file to output analysis of NIF file(s) to.
   Example: -o output.txt
   Default:  niflyze.txt

-p [Search Path]
   Specifies the directory to search for files matching the input file name.
   If no path is specified, the present directory is searched.  Don't specify
   a path in the in file or the read will fail.
   Example:  -p 'C:\Program Files\'

-b [Block Match]
   Causes niflyze to output information only for files which contain the
   specified block type. Default behavior is to output information about
   all files read.
   Example: -b NiNode

-v [Verbose]
   Causes niflyze to output all complete data arrays such as vertices and
   unknown data areas in hex form. Default behavior is not to output this
   information.
   Example: -v

-x [Exclusive Mode]
   Causes niflyze to output only the blocks that match the block type
   specified with the -b switch.  Normally the whole file that contians
   the block is output.
   Example: -x"""

def HasBlockType( blocks, block_type ):
    for block in blocks:
        if block.GetBlockType() == block_type:
            return True
    return False

def PrintBlock( block, current_file ):
    return "====[ " + current_file + " | Block " + `block.get_index()` + " | " + block.GetBlockType() + " ]====\r\n" + block.asString() + "\r\n"

#default values
block_match = False
exclusive_mode = False
use_start_dir = False
blk_match_str = ""
in_file = "*.nif"
out_file = "niflyze.txt"
start_dir = os.getcwd()

#examine command line arguments
help_flag = False
prev = ""
for i in sys.argv:
    #Evaluate flags that have an argument
    #If prev is a flag, record the value
    if prev == "-i":
        # input flag
        in_file = i
    elif prev == "-o":
        # output file
        out_file = i
    elif prev == "-p":
        # search path
        start_dir = i
    elif prev == "-b":
        # block match
        block_match = i

    #Evaluate flags that don't have arguments
    if i == "-v":
        # verbose mode
        SetVerboseMode(True)
    elif i == "-x":
        # exclusive mode
        exclusive_mode = True
    elif i == "-?":
        # help mode
        help_flag = True

    #Record previous value in lowercase
    prev = i.lower()

if help_flag == True or len(sys.argv) <= 1:
    #Help request intercepted
    PrintHelpInfo()
    exit

#Open output file
out = open(out_file, 'w')

#Cycle through files
for top, names in walktree( start_dir ):
    #Find files that match the criteria
    os.chdir( top )
    files = glob.glob( in_file )
    
    for current_file in files:
        print "Reading", top + os.sep + current_file + "...",

        blocks = ReadNifList( current_file )
  
        #If in any match mode, check the file for the block type
        #in case it can be skipped
        if block_match == True and HasBlockType(blocks, blk_match_str) == False:
            #this file doesn't have any matching blocks, skip it
            continue

        print "writing...",

        #Cycle through the blocks, writing each one
        for block in blocks:
            #In exclusive block match mode, only write the block if it's type matches
            #Otherwise, write all blocks
            if exclusive_mode == True:
                if block.GetBlockType() == blk_match_str:
                    out.write( PrintBlock(block, current_file) )
            else:
                out.write( PrintBlock(block, current_file) )

        print "done"

#Close output file
out.close()

print "Done!"
