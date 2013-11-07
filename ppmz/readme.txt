
0. PPMZ

by Charles Bloom.  Distribution of this program is subject to the Bloom Public License
(any usage allowed by the GNU License is also allowed under BPL; don't worry about
the BPL if you are in academia)

see algorithm.txt for a semi-technical description of why PPMZ works so well.

This file contains:

	1. brief description of PPMZ

	2. usage tutorial (it should be intuitive, but just in case)

	3. how to use PPMZ as a true archiver

1. Description

PPMZ is a file to file (like gzip) compressor based on the PPM method.
PPM stands for Prediction by Partial Match.  What this means is: PPM
learns the statistics of the file - i.e. it learns how to look at the
data and predict what will happen next.  When PPM is predicting
successfully, it can use arithmetic coding to transmit the data in a
much more compact way (i.e. you only need 1 bit to write a correct-guess
flag, instead of 1 byte for the character; with arithmetic coding this
kind of tradeoff can be adaptively adjusted to the estimated likely-hood
of the guess being right.

2. usage:

to encode:	PPMZ <infile> <ppmzfile>

to decode:  PPMZ <ppmzfile> <outfile>

the PPMZ decoder detects the PPMZ signature in the compressed file and
automatically knows to decode.

Memory requirements are roughly 30 times the file size.

Speed is roughly 1 byte per 20,000 CPU cycles (good lord!)

Compression is reported in terms of Output Bits per Input Byte (bpb)
(i.e. 8 = no compression, 4 = two to one, 2 = four to one, etc.)

advanced usage: flags may be placed anywhere on the command line

-c# : use a different coder.  coder 0 (default) is recommended for
      standard use.  coder 9 acheives the most compression, though
      is much slower.

-fZ : precondition the PPM tables.  This usually doesn't help
      too much, but if you want to pack a large text file, then
      preconditioning may help PPM to learn English ( :^> )
      Occasionally, packing an HTML file is helped as much as 0.2
      on compressing another HTML file.  WARNING : you must have
      the same precondition file in order to decode!!
      (this is the same as "tight channel" in ACB)

-a,d : for my use

-l[#]  : Limit memory use with an LRU.  Specifying [#] is optional;

3. archiver trick:

these are DOS batch files.  anyone who uses UNIX must be enough of a wizard
to translate these :^)

ppz.bat :

@echo off
if exist %1.ppz ppmz %1.ppz %1.zip
izip -0 %1.zip %2 %3 %4 %5
REM izip is infozip
REM pkzip -e0 %1.zip %2 %3 %4 %5
ppmz %1.zip %1.ppz
del %1.zip

unppz.bat :

@echo off
if exist %1.ppz goto found
echo ERROR: file not found
goto end
:found
ppmz %1.ppz %1.zip
iunzip %1.zip %2 %3 %4 %5
REM iunzip is info-unzip
REM pkunzip %1.zip %2 %3 %4 %5
del %1.zip
:end

note: using these will result in less compression than using PPMZ straight, because
zip writes such atrociously bloated headers.  Files are expanded by roughly 90 bytes
(per file). (using lha instead only expands files by roughly 40 bytes (per file)).
(use lha -z -h0 a %1.lzh %2 %3 %4 ; ppmz %1.lzh %1.ppz )

the "ppz" fake archiver works as a "solid" archiver.  Note that tar would work
just as well (as in the popular .tar.gz) but tar isn't well standardized in the
non-UNIX world (whereas pkzip is on every computer in the world).

