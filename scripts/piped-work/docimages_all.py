# docimages_all.py
# Sends commands to get images for the manual.
# Execs all the docimage scripts.

# Make sure Audacity is running first and that mod-script-pipe is enabled
# before running this script.


exec( open("docimages_tracks.py" ).read() )
exec( open("docimages_labels.py" ).read() )
exec( open("docimages_spectro.py" ).read() )
exec( open("docimages_after.py" ).read() )
