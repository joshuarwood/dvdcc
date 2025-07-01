# Introduction

Welcome to the documentation for the dvdcc project!

The goal of this page is to gather publicly available
information describing the techniques behind how the
dvdcc code interfaces with DVD drives. The sections
below contain short overviews of the following topics:

* SCSI Command Interface
* DVD data format
* Linear Feedback Shift Registers
* Cyclical Redundancy Checks

Primary references for this information are also provided
at the bottom of the page for readers who want to learn more
or who want to double check the summaries provided here.

# SCSI Command Interface

Communication with the DVD drive relies on the
[Small Computer System Interface (SCSI)](https://en.wikipedia.org/wiki/SCSI)
set of standards. These standards define a set of
12 byte commands that can be issued to the drive
in order to control it. The commands themselves are
maintained by [Technical Committee T10](https://www.t10.org/index.html)
which is responsible for approving and maintaining
documentation for the allowed command formats.
