# dvdpp
C++ interface for DVD drives

# Goal

This project is intended to be an educational tool showing C++ users how to control LG/Hitachi GDR-8164B drives over USB on Linux systems. Users can explore basic IO operations as well as Linear Feedback Shift Registers (LFSRs) and Cyclical Redundancy Checks (CRCs). Windows support may be added in the future, but beware that Windows 11 may not support all features given that these are very old IDE drives.

This code favors readability over speed due to its educational purpose. You can likely find faster alternatives elsewhere. 

# Requirements

* A Hitatchi-LG GDR-8164B drive circa 2007. <br> **Note:** These drives can be labeled as H-L or LG on the drive label.
* Universal drive adapter with IDE support from [iFixit](https://www.ifixit.com/products/universal-drive-adapter) or another vendor of your choosing. I use the iFixit adapter because it includes the auxiliary power cable needed to power the drive. However, you'll need to remove the plastic case around the IDE connector in order to leave enough room to directly connect the power cable and IDE connector at the same time. 
* Linux (currently tested on Fedora) with python3, python3-devel, gcc

# Installation

TBD

# Credits

* the author of the [friidump](https://github.com/bradenmcd/friidump) project
* the authors of the [unscrambler](https://github.com/saramibreak/unscrambler) project for providing a clear explanation of how to decypher raw DVD data
