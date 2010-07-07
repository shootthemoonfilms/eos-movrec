#!/bin/sh

infile="$1"
outfile="$2"

vbr=4000
threads=4

# first pass
rm -f divx2pass.log*
mencoder "$infile" -o /dev/null -ovc x264 -x264encopts \
    threads=$threads:subq=6:partitions=all:8x8dct:me=umh:frameref=5:bframes=3:weight_b:bitrate=$vbr:turbo=1:pass=1

# second pass
mencoder "$infile" -o "$outfile" -ovc x264 -x264encopts \
    threads=$threads:subq=6:partitions=all:8x8dct:me=umh:frameref=5:bframes=3:weight_b:bitrate=$vbr:pass=2 \
    -ffourcc x264

# clean
rm -f divx2pass.log*
