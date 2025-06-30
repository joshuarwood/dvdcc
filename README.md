# dvdcc
C++ interface for DVD drives

# Goal

This project is intended to be an educational tool showing C++ users how to control LG/Hitachi GDR-8164B drives over USB on Linux systems. Users can explore basic IO operations as well as Linear Feedback Shift Registers (LFSRs) and Cyclical Redundancy Checks (CRCs). Windows support may be added in the future, but beware that Windows 11 may not support all features given that these are very old IDE drives.

This code favors readability over speed (currently ~0.65 GB / hour) due to its educational purpose. You can likely find faster alternatives elsewhere. 

# Requirements

* A Hitatchi-LG GDR-8164B drive circa 2007. <br> **Note:** These drives can be labeled as H-L or LG on the drive label.
* Universal drive adapter with IDE support from [iFixit](https://www.ifixit.com/products/universal-drive-adapter) or another vendor of your choosing. I use the iFixit adapter because it includes the auxiliary power cable needed to power the drive. However, you'll need to remove the plastic case around the IDE connector in order to leave enough room to directly connect the power cable and IDE connector at the same time. 
* Linux (currently tested on Fedora) with gcc and kernel headers

# Installation

To compile the code, do the following:
```
g++ -o dvdcc main.cc -Iinclude
sudo chown root:root dvdcc
sudo chmod u+s dvdcc
```
**Note:** This process requires `sudo` in order to give the `dvdcc` executable
the ability to run vender specific DVD commands. These require root
permissions and are only used for reading the drive cache. All other
portions of the executable run under the current user permissions.

# Usage
```
./dvdcc --device /dev/sr0 --eject                 # eject the disc tray
./dvdcc --device /dev/sr0 --load                  # close the disc tray
./dvdcc --device /dev/sr0 --iso path.iso          # create an ISO formatted backup with 2048 byte sectors
./dvdcc --device /dev/sr0 --raw path.bin          # create a RAW formatted backup with 2064 byte raw sectors (sector ID, EID, data sector, error detection code)
./dvdcc --device /dev/sr0 --iso path.iso --resume # resume an ISO formatted backup from an existing file (skips completed sectors)
```

# Example Output
```
user@user:$ ./dvdcc --device /dev/sr0 --iso "NFS ProStreet.iso"

dvdcc version 0.2.0, Copyright (C) 2025 Josh Wood
dvdcc comes with ABSOLUTELY NO WARRANTY; for details see LICENSE.
This is free software, and you are welcome to redistribute it
under certain conditions; see LICENSE for details.

Found drive model: HL-DT-ST/DVD-ROM GDR8164B/0A09

Checking if drive is ready...

Waiting for standby state... elapsed 00:02:44

Finding Disc Type...

Found WII_SINGLE_LAYER with 2294912 sectors.

Finding DVD keys...

 * Block 00 found key
 * Block 01 found key
 * Block 02 found key
 * Block 03 found key
 * Block 04 found key
 * Block 05 found key
 * Block 06 found key
 * Block 07 found key
 * Block 08 found key
 * Block 09 found key
 * Block 10 found key
 * Block 11 found key
 * Block 12 found key
 * Block 13 found key
 * Block 14 found key
 * Block 15 found key
 * Block 16 found key

Done.

Disc information:
--------------------
Disc type..........: WII_SINGLE_LAYER
Disc size..........: 2294912 sectors (4.38 GB iso, 4.41 GB raw)
System ID..........: R (Wii)
Game ID............: NP
Region.............: E (NTSC)
Publisher..........: 69 (Electronic Arts)
Version............: 1.01
Game title.........: Need for Speed(TM) ProStreet
Contains update....: Yes (0x690dbcd4)

Backing up content...

 ISO path: NFS ProStreet.iso

Progress ==================== 100.0% | elapsed 06:33:49 remaining 00:00:00

user@user:$ md5sum "NFS ProStreet.iso" 
a44d380a3e421d0ed96841baf8d359ac  NFS ProStreet.iso
```

# Credits

* the author of the [friidump](https://github.com/bradenmcd/friidump) project
* the authors of the [unscrambler](https://github.com/saramibreak/unscrambler) project for providing a clear explanation of how to decypher raw DVD data
* the author of [Wii_disc](https://wiibrew.org/wiki/Wii_disc) for a description of the Wii disc format
* [SCSI MMC-5 Reference Manual](https://www.13thmonkey.org/documentation/SCSI/mmc5r02c.pdf)
