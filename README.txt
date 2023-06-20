######################################
Soundwire Analyzer for Saleae Logic UI
######################################
.. This is a ReStructured Text document

:Copyright:
 Copyright(c) 2023 Cirrus Logic, Inc and
 Cirrus Logic International Semiconductor Ltd.  All rights reserved.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

.. contents:: Table of Contents

*******
SUMMARY
*******
This is an analyzer plugin for the Saleae Logic UI version 2.x.
It allows decoding command and status from SoundWire buses.

Limitations
===========
- Only supports single data line (does not support multi-lane).
- BRA transactions are not decoded or reported.
- There must be at least 16 consecutive frames for the analyzer to
  find frame sync.
- DP (dataport) data is not extracted or reported.
- Not compatible with Logic UI 1.x versions.

********
BUILDING
********
Build prerequisites:
- C++ compiler.
- cmake.
- an internet connection to fetch the Saleae SDK.

Windows
=======
- Install Visual C++
- Install cmake (https://cmake.org/download/)

The analyzer binary will be in SoundWireAnalyzer/Analyzers

Linux
=====
On a Debian-based system (such as Ubuntu) the prerequisites can normally
be installed using apt, something like::

 sudo apt-get install build-essential cmake

To build the analyzer::

 cd SoundWireAnalyzer
 cmake .
 cmake --build .

To build::

 cd SoundWireAnalyzer
 mkdir build
 cd build
 cmake ..
 cmake --build .

or for a Release build::

 cmake --build . --config=Release

The analyzer binary will be in SoundWireAnalyzer/Analyzers

**********
INSTALLING
**********
Saleae instructions are here:
https://support.saleae.com/faq/technical-faq/setting-up-developer-directory

**********
HOW TO USE
**********
Electrical considerations
=========================
The SoundWire bus is very sensitive to capacitive/inductive loading
and signal length. For best results it is recommended to:

 - Keep the test lead length as short at possible
 - Use inputs on the Saleae Logic as far apart as possible. For example
   use channel 0 for SDW_CLOCK and channel 15 for SDW_DATA
 - Ground the channels adjacent to the used channel. For example if channels
   0 and 15 are used for SDW_CLOCK and SDW_DATA, ground channels 1 and 14.
 - Connect as many of the ground wires as possible
 - Ground unused channels.

The Saleae Pro8 and Pro16 use low-hysteresis comparator inputs, which
are described by Saleae here:
https://support.saleae.com/user-guide/supported-voltages#logic-pro-8-and-logic-pro-16

There may be some glitches on the capture as the logic level crosses the
comparator threshold. If the SoundWire analyzer does not decode the
capture, or reports a large number of sync loss or parity errors this
could be due to these glitches. The Logic UI glitch filter might be able
to remove the glitches. However, when the glitches are occurring on the
data line very close to the clock edge it is not always possible for the
glitch filter to identify what the correct state should be at the time
the clock edge occurs.

For example if the next data level should be 1, like this::

                           _________________
  CLOCK     ______________|

                     _______________________
  DATA      ________|

but glitches to 0 across the clock edge like this::

                           _________________
  CLOCK     ______________|

                    __      _______________
  DATA      _______|  |____|

The first glitch is actually to the correct level and it is the longer
glitch that is to the incorrect level. If the glitch filter is set to
remove the shorter glitch it will give the wrong level at the clock
edge::

                           _________________
  CLOCK     ______________|

                            _______________
  DATA      _______________|

but if it is set to also remove the longer glitch this give the same
result that the data line is not reported as high until after the clock
edge.

It may be useful to put a Schmitt-triggered buffer between the
SoundWire bus and the Saleae Logic input to provide a faster edge.

Sampling rate
=============
Unlike many protocols the minimum sampling rate does not scale with
clock frequency. In other words, if you halve the SDW_CLOCK frequency
that does not mean that you can halve the Saleae sampling frequency.

The SoundWire specification allows the data line to change 4ns after
a clock edge. This is independent of SDW_CLOCK frequency. To correctly
distinguish that the data edge occurred after the clock edge would
require a sampling frequency of at least 250MS/s. (that is, one sample
every 4 ns).

It is recommended to always use a sample rate of 250MS/s or more. A
lower rate could lead to capturing an incorrect data line state at the
clock edge.

As a compensation against the small time window the SoundWire analyzer
takes the data line state from the sample *before* the clock edge. The
data line typically changes state soon after the clock edge and
therefore is stable in the sample before the clock edge. So for
example if the clock edge is seen at sample n, the data level is taken
from sample n-1::

  Samples:                           |n-1| n |
         _________                          ______________
  CLOCK           |________________________|

                    ________________________
  DATA      _______|                        |_____________

Analyzer settings
=================

SoundWire Clock
---------------
Set to the channel used to capture the SoundWire clock.

SoundWire Data
--------------
Set to the channel used to capture the SoundWire data.

Num Rows
--------
Set the number of rows in SoundWire frames. This should normally
be set to 'auto' and the analyzer will attempt to auto-detect the
number of rows per frame. Change this setting to a number to force
the analyzer to look for frames of a specific size.

Num Cols
--------
Set the number of columns in SoundWire frames. This should normally
be set to 'auto' and the analyzer will attempt to auto-detect the
number of columns per frame. Change this setting to a number to
force the analyzer to look for frames of a specific size.

Suppress duplicate pings in table
---------------------------------
If enabled a PING will only be added to the table view if it reports
a different status from the previous PING. The SoundWire specification
requires the Manager (bus controller) to send PING requests when it
does not have any read/write command to send. This leads to a huge
number of PINGs that are identical.

It is recommended to enable this.

Annotate decoded bit values
---------------------------
If enabled the data channel trace will be annoted with 0 and 1 to show
the NRZI-decoded data bit values.

This adds decoding time and memory overhead so it is recommended to
leave this disabled if not needed.

Annotate frame starts
---------------------
If enabled the clock channel trace will be annotated with a green dot
on the first clock edge of every frame.

This adds decoding time and memory overhead so it is recommended to
leave this disabled if not needed.

Annotate trace
--------------
Show annotation "bubbles" on the clock and data traces. These show
the PING status, read/write commands, parity errors and sync loss.

This adds decoding time and memory overhead so it is recommended to
leave this disabled if not needed. Generally the table view is more
useful than the trace annotations.

Show in protocol results table
------------------------------
Enable this to show decoded frames in the analyzer table view.

Stream to terminal
------------------
This option does nothing in the SoundWire analyzer but it always
displayed by the Logic UI.

********************
INTERPRETING RESULTS
********************

Clock annotation
================
If 'Annotate trace' is enabled an annotation "bubble" will be placed
above the clock trace.

The clock annotation shows the parity state and the control word
value (in hexadecimal). For example a normal frame will show something
like this about the clock::

 Par: ok 200053b11aad

If the SSP bit is set on a frame the clock annotation will indicate
this::

 SSP Par: ok 040000b10098

Data annotation
===============
If 'Annotate trace' is enabled an annotation "bubble" will be placed
above the data trace.

The data annotation shows a decode of control words. The meaning of
bits in a control word vary depending whether it is PING, read or write
command.

For PINGs the annotation shows the state reported by every SoundWire
peripheral ID. The states are shown as:

===========  =========
Status       Shown as
-----------  ---------
No response  \-
Ok           Ok
Alert        Al
===========  =========

For read and write commands the annotation format is::

 RD [DevId] @Address=Value RESPONSE
 WR [DevId] @Address=Value RESPONSE

Where:

- RD/WR indicates whether it is a read or a write
- DevId is the SoundWire device ID of the target peripheral
- Address is the register address in hexadecimal
- Value is the register value in hexadecimal
- RESPONSE is decoded from the ACK/NAK bits to one of
  OK, IGNORED or FAIL.

Table view
==========
Table view columns are:

============    =======
Column title    Purpose
------------    -------
type            Show frame type. One of:

                - READ
                - WRITE
                - PING
                - shape
                - BUS RESET
                - SYNC LOST
value           Value of the command word
DevId           Target Peripheral of read or write command
Reg             Register address of read or write command
Data            Register value of read or write command
ACK             ACK bit state (true or false)
NAK             NAK bit state (true or false)
Preq            State of PREQ bit (true if any Peripheral asserted PREQ)
SSP             SSP bit state (true or false). Only valid for PING.
Par             Parity status (OK or BAD)
Dsync           Dynamic sync word value
P0 to P15       Status reported by each Peripheral in a PING command.
                One of OK or AL (AL = alert).
                If the Peripheral did not respond the table cell will
                be empty.
                If the status bits were invalid the table cell will
                show ??.
============    =======

The special frame 'type' are:

=========  ===============
shape      Indicates the frame shape detected by the analyzer.
           The shape is shown as rows x columns.
BUS RESET  Indicates that a sequence of 4096 logic '1' was detected.
SYNC LOST  Indicates that the analyzer lost sync. This means:

           - Dynamic sync word out-of-sequence
           - Static sync word not correct
=========  ===============

*****************
EXPORTING RESULTS
*****************
Use the "Export Table" option in Logic UI. This exports the table view
and allows selecting which data to export.
