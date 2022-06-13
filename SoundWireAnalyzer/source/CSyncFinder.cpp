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

#include <array>
#include "CBitstreamDecoder.h"
#include "CSyncFinder.h"
#include "SoundWireAnalyzer.h"
#include "SoundWireProtocolDefs.h"

// Row number of last bit in static sync word
static const int kLastStaticSyncRow = kCtrlStaticSyncRow + kCtrlStaticSyncNumRows - 1;

// The 8 static sync bits are in column 0. The maximum number of columns is
// 16 so a full static sync word cannot cover more than 8 * 16 = 128 bits.
class CStaticSyncMatcher
{
public:
    void Reset(int columns);
    bool PushBit(bool isOne);

private:
    std::array<U64, 2> mAccumulator;
    std::array<U64, 2> mMask;
    std::array<U64, 2> mMatch;
};

void CStaticSyncMatcher::Reset(int columns)
{
    mAccumulator.fill(0);
    mMask.fill(0);
    mMatch.fill(0);

    // Generate mask and match value bits separated by number of columns bits
    U64 bit = 1;
    for (int i = 0; i < kCtrlStaticSyncNumRows; ++i) {
        mMask[i / 64] |= bit;
        if (kStaticSyncVal & (1 << i)) {
            mMatch[i / 64] |= bit;
        }

        bit <<= columns;
        if (bit == 0) {
            bit = 1;    // move to bit 0 of high word
        }
    }
}

bool CStaticSyncMatcher::PushBit(bool isOne)
{
    // Shift 128-bit value left one place
    mAccumulator[1] <<= 1;
    mAccumulator[1] |= mAccumulator[0] >> 63;
    mAccumulator[0] <<= 1;
    mAccumulator[0] |= isOne;

    // Return true if we have a match
    if (((mAccumulator[0] & mMask[0]) == mMatch[0]) &&
        ((mAccumulator[1] & mMask[1]) == mMatch[1])) {
        return true;
    }

    return false;
}

CSyncFinder::CSyncFinder(SoundWireAnalyzer& analyzer, CBitstreamDecoder& bitstream)
    : mAnalyzer(analyzer), mBitstream(bitstream)
{
}

// Search for a sync and return with the CBitstreamDecoder pointing at the first
// complete frame.
void CSyncFinder::FindSync(int rows, int columns)
{
    unsigned int lastStaticSyncBitOffset = BitOffsetInFrame(columns, kLastStaticSyncRow, 0);
    CStaticSyncMatcher matcher;
    matcher.Reset(columns);

    CBitstreamDecoder::CMark mark = mBitstream.Mark();
    U64 matchedBitOffset = 0;
    for (; !matcher.PushBit(mBitstream.NextBitValue()); ++matchedBitOffset) {
        mAnalyzer.CheckIfThreadShouldExit();
    }

    // Are there enough bits before the static sync word to form a full frame?
    // If not, skip on to where the next frame should start.
    if (matchedBitOffset >= lastStaticSyncBitOffset) {
        mBitstream.SetToMark(mark);
        mBitstream.SkipBits(matchedBitOffset - lastStaticSyncBitOffset);
    } else {
        mBitstream.SkipBits(TotalBitsInFrame(rows, columns) - lastStaticSyncBitOffset);
    }
}