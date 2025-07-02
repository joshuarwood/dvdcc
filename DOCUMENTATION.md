# Introduction

Welcome to the documentation for the dvdcc project!

This page provides short descriptions of the techniques
used by the dvdcc code to interface with DVD drives.
It is intended to help users understand the code
base. 

It is organized into the following topics:

* [SCSI Command Interface](#scsi-command-interface)
* [DVD data format](#dvd-data-format)
* [Linear Feedback Shift Register](#linear-feedback-shift-register)
* [Cyclical Redundancy Check](#cyclical-redundancy-check)

with each topic described using publicly available information.
Primary references for this information are provided at the bottom of the
page for readers who want to learn more or who want to
double check the summaries provided here.

# SCSI Command Interface

Communication with the DVD drive relies on the
[Small Computer System Interface (SCSI)](https://en.wikipedia.org/wiki/SCSI)
set of standards. These standards define a set of
12 byte commands that can be issued to the drive
in order to control it. The commands themselves are
maintained by [Technical Committee T10](https://www.t10.org/index.html)
which is responsible for approving and maintaining
documentation for the allowed command formats.

# DVD Data Format

The following is an adaptation of the DVD format description from reference [2].

To Do: Description of DVD sectors, raw sectors, ID, EID, EDC

DVD-ROM:
```
   4bytes   2bytes      6bytes                 2048bytes             4bytes
 -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  - 
|     ID    | IED |     CPR_MAI     |               Data Sector               |    EDC    |
 -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  - 
          ^                         |         2048bytes cipher stream         |
          ^                          -  -  -  -  -  -  -  -  -  -  -  -  -  - 
       scrambling
       seed index   
```
Gamecube/WII Optical Disc:
```
   4bytes   2bytes                  2048bytes                     6bytes         4bytes
 -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  - 
|     ID    | IED |               Data Sector               |     unknown     |    EDC    |
 -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  - 
                                    |         2048bytes cipher stream         |
                                     -  -  -  -  -  -  -  -  -  -  -  -  -  -  
```

First 4 bytes are the sector ID. These do not start at 0.

# Linear Feedback Shift Register

Describe use of seed to generate cipher.

# Cyclical Redundancy Check

Describe formulation of CRC as long division using bit math

# References

1. [friidump project](https://github.com/bradenmcd/friidump)
2. [Understanding Wii/Gamecube DVD formats](https://hitmen.c02.at/files/docs/gc/Ingenieria-Inversa-Understanding_WII_Gamecube_Optical_Disks.html)
3. [SCSI MMC-5 Reference Manual](https://www.13thmonkey.org/documentation/SCSI/mmc5r02c.pdf)
