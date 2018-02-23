# docimages_core.py
# Sends commands to get images for the manual.
# This is the shared part reused in a bunch of scripts.

# Make sure Audacity is running first and that mod-script-pipe is enabled
# before running this script.

import os
import sys

def startPipes() :
    global tofile
    global fromfile
    global EOL
    if( sys.platform  == 'win32' ):
        print( "pipe-test.py, running on windows" )
        toname = '\\\\.\\pipe\\ToSrvPipe'
        fromname = '\\\\.\\pipe\\FromSrvPipe'
        EOL = '\r\n\0'
    else:
        print( "pipe-test.py, running on linux or mac" )
        toname = '/tmp/audacity_script_pipe.to.' + str(os.getuid())
        fromname = '/tmp/audacity_script_pipe.from.' + str(os.getuid())
        EOL = '\n'

    print( "Write to  \"" + toname +"\"" )
    if not os.path.exists( toname ) :
       print( " ..does not exist.  Ensure Audacity is running with mod-script-pipe." )
       sys.exit();
        
    print( "Read from \"" + fromname +"\"")
    if not os.path.exists( fromname ) :
       print( " ..does not exist.  Ensure Audacity is running with mod-script-pipe." )
       sys.exit();

    print( "-- Both pipes exist.  Good." )

    tofile = open( toname, 'wt+' )
    print( "-- File to write to has been opened" )
    fromfile = open( fromname, 'rt')
    print( "-- File to read from has now been opened too\r\n" )


def sendCommand( command ) :
    global tofile
    global EOL
    print( "Send: >>> \n"+command )
    tofile.write( command + EOL )	
    tofile.flush()

def getResponse() :
    global fromfile
    result = ''
    line = ''
    while line != '\n' :
        result += line
        line = fromfile.readline()
	#print(" I read line:["+line+"]")
    return result

def doCommand( command ) :
    sendCommand( command )
    response = getResponse()
    print( "Rcvd: <<< \n" + response )
    return response

def do( command ) :
    doCommand( command )

def quickTest() :
    do( 'Help: Command=Help' )

def setup() :
    global path
    global sample
    global sample2
    path = 'C:\\Users\\James Crook\\'
    sample ='C:\\Users\\James Crook\\Music\\The Poodle Podcast.wav'
    sample2 ='C:\\Users\\James Crook\\Music\\PoodlePodStereo.wav'
    startPipes()
    do( 'SetProject: X=10 Y=10 Width=850 Height=800' )

def imageSet(name):
    print("****************** " + name + " ***************************")

def makeWayForTracks(  ) :
    do( 'Select: First=0 Last=20' )
    do( 'RemoveTracks' )

def capture( name, what ) :
    global path
    do( 'Screenshot: Path="'+path+name+'" CaptureWhat=' + what )

def loadMonoTrack():
    global sample
    makeWayForTracks( )
    do( 'Import2: Filename="'+sample+'"' )
    do( 'Select: First=0 Last=0 Start=0 End=150')
    do( 'Trim')
    do( 'ZoomSel' )

def loadStereoTrack():
    global sample2
    makeWayForTracks( )
    do( 'Import2: Filename="'+sample2+'"' )
    do( 'Select: First=0 Last=0 Start=0 End=150')
    do( 'Trim')
    do( 'ZoomSel' )
    
def loadMonoTracks( num ) :
    makeWayForTracks( )
    loadMonoTrack()
    do( 'SetTrack: Track=0 Name="Foxy Lady"')
    for i in range( 0, num-1 ):
        do( 'Duplicate' )
    do( 'FitInWindow' )
    do( 'Select: Start=55 End=70')

def loadStereoTracks( num ) :
    makeWayForTracks( )
    loadStereoTrack()
    do( 'SetTrack: Track=0 Name="Foxy Lady"')
    for i in range( 0, num-1 ):
        do( 'Duplicate' )
    do( 'FitInWindow' )
    do( 'Select: Start=55 End=70 First=0 Last=' + str(num*2-1) )

def makeMonoTracks( num ) :
    makeWayForTracks( )
    for i in range( 0, num ):
        do( 'NewMonoTrack' )
    do( 'SetTrack: Track=0 Name="Foxy Lady"')
    do( 'Select: Start=0 End=150 First=0 Last=' + str(num-1) )
    do( 'Chirp: StartAmp=0.5' )
    do( 'Wahwah' )
    do( 'FitInWindow' )
    do( 'Select: Start=55 End=70')
 
def makeStereoTracks( num ) :
    makeWayForTracks( )
    for i in range( 0, num ):
       do( 'NewStereoTrack' )
    do( 'SetTrack: Track=0 Name="Voodoo Children IN STEREO"')
    do( 'Select: Start=0 End=150 First=0 Last=' + str(num*2-1) )
    do( 'Chirp: StartAmp=0.5' )
    do( 'Wahwah' )
    do( 'FitInWindow' )
    do( 'Select: Start=55 End=70')

try:
    coreLoaded
except NameError:    
    setup()
    coreLoaded=True
    print( "Set up done")
else :
    print( "Already set up")


