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

#include "CControlWordBuilder.h"

CControlWordBuilder::CControlWordBuilder()
    : mValue(0),
      mNextPushBitMask(1ULL << kCtrlWordLastRow)
{
}

void CControlWordBuilder::Reset()
{
    mValue = 0;
    mNextPushBitMask = 1ULL << kCtrlWordLastRow;
}

void CControlWordBuilder::PushBit(bool isOne)
{
    // Bits are transmitted MSB first. The are pushed into their correct final
    // position in the word rather than shifting, so that fields can immediately
    // be read from a partially-constructed word.
    if (isOne) {
        mValue |= mNextPushBitMask;
    }

    mNextPushBitMask >>= 1;
}

// Use to skip over bits that are not available in the bitstream so that
// sunbequent bits can still be accumulated and read out using the field
// access bits.
void CControlWordBuilder::SkipBits(int numBits)
{
    mNextPushBitMask >>= numBits;
}

void CControlWordBuilder::GetNewShape(int& rows, int& columns) const
{
    unsigned int data = DataValue();
    unsigned int rowsIndex = data >> 3;
    unsigned int columnsIndex = data & 7;

    if (rowsIndex < kFrameShapeRows.size()) {
        rows = kFrameShapeRows[rowsIndex];
    } else {
        rows = 0;
    }

    if (columnsIndex < kFrameShapeColumns.size()) {
        columns = kFrameShapeColumns[columnsIndex];
    } else {
        columns = 0;
    }
}
