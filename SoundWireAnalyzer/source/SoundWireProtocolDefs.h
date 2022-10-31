// Copyright (C) 2022-2023 Cirrus Logic, Inc. and
//                         Cirrus Logic International Semiconductor Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SOUNDWIRE_PROTOCOL_DEFS_H
#define SOUNDWIRE_PROTOCOL_DEFS_H

#include <vector>

static const int kMaxRows                = 256;
static const int kMaxColumns             = 16;

// Control word bit positions in transmission order, counting the
// first frame row from 0.
static const int kCtrlWordLastRow        = 47;

static const int kCtrlPREQRow            = 0;
static const int kCtrlOpCodeRow          = 1;
static const int kCtrlOpCodeNumRows      = 3;
static const int kCtrlStaticSyncRow      = 24;
static const int kCtrlStaticSyncNumRows  = 8;
static const int kCtrlPhySyncRow         = 32;
static const int kCtrlDynamicSyncRow     = 41;
static const int kCtrlDynamicSyncNumRows = 4;
static const int kCtrlPARRow             = 45;
static const int kCtrlNAKRow             = 46;
static const int kCtrlACKRow             = 47;

// PING command control word rows
static const int kPingSSPRow             = 5;
static const int kPingBREQRow            = 6;
static const int kPingBRELRow            = 7;
static const int kPingStat4_11Row        = 8;
static const int kPingStat4_11NumRows    = 16;
static const int kPingStat0_3Row         = 33;
static const int kPingStat0_3NumRows     = 8;

// Read/Write command controls word rows
static const int kDevAddrRow             = 4;
static const int kDevAddrNumRows         = 4;
static const int kRegAddrRow             = 8;
static const int kRegAddrNumRows         = 16;
static const int kRegDataRow             = 33;
static const int kRegDataNumRows         = 8;

// Opcodes
enum SdwOpCode {
    kOpPing     = 0,
    kOpRead     = 2,
    kOpWrite    = 3,
};

// Ping status
enum SdwPingStat {
    kStatNotPresent = 0,
    kStatOk         = 1,
    kStatAlert      = 2,
};

// Static sync value in reconstructed order (first row is MSB)
static const unsigned int kStaticSyncVal = 0xb1;

// Number of consecutive logic ones to signal a bus reset
static const unsigned int kBusResetOnesCount = 4096;

// Registers we are interested in
const U16 kRegAddrScpFrameCtrl0 = 0x60;
const U16 kRegAddrScpFrameCtrl1 = 0x70;

// Array of possible rows count indexed by enumeration in ScpFrameCtrl register
extern const std::vector<int> kFrameShapeRows;

// Array of possible columns count indexed by enumeration in ScpFrameCtrl register
extern const std::vector<int> kFrameShapeColumns;

// Size of frame in bits
static inline int TotalBitsInFrame(int rows, int columns)
        { return rows * columns; }

// Bit position with a frame of a (row,column)
static inline int BitOffsetInFrame(int columns, int row, int column)
    { return (row * columns) + column; }

#endif // ifndef SOUNDWIRE_PROTOCOL_DEFS_H
