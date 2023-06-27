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

#define _SHIFT(firstRow, numRows) (kCtrlWordLastRow - (firstRow) - (numRows) + 1)
#define _MASK(firstRow, numRows) (((1ULL << (numRows)) - 1) << _SHIFT(firstRow, numRows))

class CControlWordBuilder
{
private:
    const U64 kCtrlPREQMask         = _MASK(kCtrlPREQRow, 1);
    const U64 kCtrlOpCodeMask       = _MASK(kCtrlOpCodeRow, kCtrlOpCodeNumRows);
    const U64 kCtrlOpCodeShift      = _SHIFT(kCtrlOpCodeRow, kCtrlOpCodeNumRows);
    const U64 kCtrlStaticSyncMask   = _MASK(kCtrlStaticSyncRow, kCtrlStaticSyncNumRows);
    const U64 kCtrlStaticSyncShift  = _SHIFT(kCtrlStaticSyncRow, kCtrlStaticSyncNumRows);
    const U64 kCtrlPhySyncMask      = _MASK(kCtrlPhySyncRow, 1);
    const U64 kCtrlDynamicSyncMask  = _MASK(kCtrlDynamicSyncRow, kCtrlDynamicSyncNumRows);
    const U64 kCtrlDynamicSyncShift = _SHIFT(kCtrlDynamicSyncRow, kCtrlDynamicSyncNumRows);
    const U64 kCtrlPARMask          = _MASK(kCtrlPARRow, 1);
    const U64 kCtrlNAKMask          = _MASK(kCtrlNAKRow, 1);
    const U64 kCtrlACKMask          = _MASK(kCtrlACKRow, 1);

    // PING command control word rows
    const U64 kPingSSPMask          = _MASK(kPingSSPRow, 1);
    const U64 kPingBREQMask         = _MASK(kPingBREQRow, 1);
    const U64 kPingBRELMask         = _MASK(kPingBRELRow, 1);
    const U64 kPingStat4_11Mask     = _MASK(kPingStat4_11Row, kPingStat4_11NumRows);
    const U64 kPingStat4_11Shift    = _SHIFT(kPingStat4_11Row, kPingStat4_11NumRows);
    const U64 kPingStat0_3Mask      = _MASK(kPingStat0_3Row, kPingStat0_3NumRows);
    const U64 kPingStat0_3Shift     = _SHIFT(kPingStat0_3Row, kPingStat0_3NumRows);

    // Read/Write command controls word rows
    const U64 kDevAddrMask          = _MASK(kDevAddrRow, kDevAddrNumRows);
    const U64 kDevAddrShift         = _SHIFT(kDevAddrRow, kDevAddrNumRows);
    const U64 kRegAddrMask          = _MASK(kRegAddrRow, kRegAddrNumRows);
    const U64 kRegAddrShift         = _SHIFT(kRegAddrRow, kRegAddrNumRows);
    const U64 kRegDataMask          = _MASK(kRegDataRow, kRegDataNumRows);
    const U64 kRegDataShift         = _SHIFT(kRegDataRow, kRegDataNumRows);

public:
    CControlWordBuilder();

    void Reset();
    void PushBit(bool isOne);
    void SkipBits(int numBits);

    inline void SetValue(const U64 value) { mValue = value; }

	inline U64 Value() const { return mValue; }
	inline bool Preq() const { return mValue & kCtrlPREQMask; }
	inline bool Par() const { return mValue & kCtrlPARMask; }
	inline bool Nak() const { return mValue & kCtrlNAKMask; }
	inline bool Ack() const { return mValue & kCtrlACKMask; }
	inline SdwOpCode OpCode() const
		{ return static_cast<SdwOpCode>((mValue & kCtrlOpCodeMask) >> kCtrlOpCodeShift); }
	inline unsigned int StaticSync() const
        { return static_cast<unsigned int>((mValue & kCtrlStaticSyncMask) >> kCtrlStaticSyncShift); }
	inline unsigned int DynamicSync() const
        { return static_cast<unsigned int>((mValue & kCtrlDynamicSyncMask) >> kCtrlDynamicSyncShift); }

	// PING words
	inline bool Ssp() const { return mValue & kPingSSPMask; }
	inline unsigned int PeripheralStat() const
	{
		return static_cast<unsigned int>(
            (((mValue & kPingStat4_11Mask) >> kPingStat4_11Shift) << 8)
			| ((mValue & kPingStat0_3Mask) >> kPingStat0_3Shift)
        );
	}

	// Read/Write words
	inline unsigned int DeviceAddress() const
        { return static_cast<unsigned int>((mValue & kDevAddrMask) >> kDevAddrShift); }
	inline unsigned int RegisterAddress() const
        { return static_cast<unsigned int>((mValue & kRegAddrMask) >> kRegAddrShift); }
	inline unsigned int DataValue() const
        { return static_cast<unsigned int>((mValue & kRegDataMask) >> kRegDataShift); }

    inline bool IsPingSameAs(const CControlWordBuilder& other) const
        {
            // True if if the reported status or errors differ.
            // The SSP flag state is not counted as a difference.
            return (PeripheralStat() == other.PeripheralStat()) &&
                    (Preq() == other.Preq()) &&
                    (Ack() == other.Ack()) &&
                    (Nak() == other.Nak());
        }

    inline bool IsFrameShapeChange() const
        {
            if (OpCode() != kOpWrite) {
                return false;
            }
            unsigned int addr = RegisterAddress();
            return (addr == kRegAddrScpFrameCtrl0) || (addr == kRegAddrScpFrameCtrl1);
        }

    void GetNewShape(int& rows, int& columns) const;

private:
    U64 mValue;
    U64 mNextPushBitMask;
};

#endif // CCONTROLWORDBUILDER_H
