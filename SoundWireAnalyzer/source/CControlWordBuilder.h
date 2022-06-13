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

#ifndef CCONTROLWORDBUILDER_H
#define CCONTROLWORDBUILDER_H

#include <LogicPublicTypes.h>
#include "SoundWireProtocolDefs.h"

class CControlWordBuilder
{
public:
    CControlWordBuilder();

    void Reset();
    void PushBit(bool isOne);
    void SkipBits(int numBits);

    inline void SetValue(const U64 value)
        { mWord = value; }

    inline U64 Value() const
        { return mWord; }

    inline unsigned int PeekField(int firstRow, int numRows) const
        {
            U64 mask = (1ULL << numRows) - 1;
            int shift = kCtrlWordLastRow - firstRow - numRows + 1;
            return (mWord >> shift) & mask;
        }

    inline bool PeekBit(int row) const
        {
            return mWord & (1ULL << (kCtrlWordLastRow - row));
        }

    inline bool Preq() const
        { return PeekBit(kCtrlPREQRow); }

    inline bool Par() const
        { return PeekBit(kCtrlPARRow); }

    inline bool Nak() const
        { return PeekBit(kCtrlNAKRow); }

    inline bool Ack() const
        { return PeekBit(kCtrlACKRow); }

    inline SdwOpCode OpCode() const
        { return static_cast<SdwOpCode>(PeekField(kCtrlOpCodeRow, kCtrlOpCodeNumRows)); }

    inline unsigned int StaticSync() const
        { return PeekField(kCtrlStaticSyncRow, kCtrlStaticSyncNumRows); }

    inline unsigned int DynamicSync() const
        { return PeekField(kCtrlDynamicSyncRow, kCtrlDynamicSyncNumRows); }

    // PING words
    inline bool Ssp() const
        { return PeekBit(kPingSSPRow); }

    inline unsigned int PeripheralStat() const
        {
            return (PeekField(kPingStat4_11Row, kPingStat4_11NumRows) << 8) |
                    PeekField(kPingStat0_3Row, kPingStat0_3NumRows);
        }

    // Read/Write words
    inline unsigned int DeviceAddress() const
        { return PeekField(kDevAddrRow, kDevAddrNumRows); }

    inline unsigned int RegisterAddress() const
        { return PeekField(kRegAddrRow, kRegAddrNumRows); }

    inline unsigned int DataValue() const
        { return PeekField(kRegDataRow, kRegDataNumRows); }

private:
    U64 mWord;
    U64 mNextPushBitMask;
};

#endif // CCONTROLWORDBUILDER_H
