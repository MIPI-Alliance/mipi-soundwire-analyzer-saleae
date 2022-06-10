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

#include <limits>
#include <LogicPublicTypes.h>
#include <AnalyzerChannelData.h>

#include "CBitstreamDecoder.h"
#include "SoundWireProtocolDefs.h"

CBitstreamDecoder::CMark::CMark(const CBitstreamDecoder& decoder, size_t nextHistoryReadIndex)
    : mLastDataLevel(decoder.mLastDataLevel),
      mContiguousOnesCount(decoder.mContiguousOnesCount),
      mParityIsOdd(decoder.mParityIsOdd),
      mCurrentSampleNumber(decoder.mCurrentSampleNumber),
      mNextHistoryReadIndex(nextHistoryReadIndex)
{
}

CBitstreamDecoder::CBitstreamDecoder(AnalyzerChannelData* clock,
                                     AnalyzerChannelData* data)
    : mClock(clock),
      mData(data),
      mCurrentSampleNumber(0),
      mContiguousOnesCount(0),
      mParityIsOdd(false),
      mLastDataLevel(data->GetBitState()),
      mNextHistoryReadIndex(std::numeric_limits<decltype(mNextHistoryReadIndex)>::max()),
      mCollectHistory(false)
{
}

CBitstreamDecoder::~CBitstreamDecoder()
{
}

inline void CBitstreamDecoder::invalidateHistoryReadIndex()
{
    mNextHistoryReadIndex = std::numeric_limits<decltype(mNextHistoryReadIndex)>::max();
}

// Advance mClock and mData to the next clock edge and return the
// decoded bit state.
bool CBitstreamDecoder::NextBitValue()
{
    BitState level;

    // We need to be able to go back to past data when trying to find sync
    // but the Saleae APIs can only go forward. If data has been rewound to
    // a mark fetch the data from the history buffer until we reach
    // the end of the buffer.
    if (mNextHistoryReadIndex < mHistorySampleNumber.size()) {
        level = static_cast<BitState>(mHistoryDataLevel[mNextHistoryReadIndex]);
        mCurrentSampleNumber = mHistorySampleNumber[mNextHistoryReadIndex];
        ++mNextHistoryReadIndex;
        if (mNextHistoryReadIndex == mHistorySampleNumber.size()) {
            // Prevent it becoming a valid index if more data is added to history
            invalidateHistoryReadIndex();
        }
    } else {
        mClock->AdvanceToNextEdge();
        mCurrentSampleNumber = mClock->GetSampleNumber();
        mData->AdvanceToAbsPosition(mCurrentSampleNumber);
        level = mData->GetBitState();

        if (mCollectHistory) {
            mHistoryDataLevel.push_back(static_cast<char>(level));
            mHistorySampleNumber.push_back(mCurrentSampleNumber);
        }
    }

    // NRZ signals a 1 by a change of level, 0 by no change.
    bool decodedBitValue = (level != mLastDataLevel);
    mLastDataLevel = level;

    // Parity counts the number of high levels (not the number of decoded ones).
    if (level == BIT_HIGH) {
        mParityIsOdd = !mParityIsOdd;
    }

    // Bus reset counts a run of decoded ones
    if (decodedBitValue) {
        ++mContiguousOnesCount;
    } else {
        mContiguousOnesCount = 0;
    }

    return decodedBitValue;
}

// Helper to skip a number of bits
void CBitstreamDecoder::SkipBits(U64 numBits)
{
    while (numBits--) {
        NextBitValue();
    }
}

void CBitstreamDecoder::ResetParity()
{
    mParityIsOdd = false;
}

// If enable==true start capturing history. All history before the current
// position is discarded.
// If enable==false stop capturing history but all captured history is kept
// so all marks in that history are valid.
void CBitstreamDecoder::CollectHistory(bool enable)
{
    if (enable) {
        DiscardHistoryBeforeCurrentPosition();
    }

    mCollectHistory = enable;
}

// WARNING: This invalidates all CMarks!!
void CBitstreamDecoder::DiscardHistoryBeforeCurrentPosition()
{
    if (mHistorySampleNumber.size() == 0) {
        return;
    }

    // Discarding the front of a vector is expensive so only discard if
    // the entire buffer is obsolete.
    if (mNextHistoryReadIndex >= mHistorySampleNumber.size()) {
        // Current position is past history: discard all history
        mHistorySampleNumber.clear();
        mHistoryDataLevel.clear();
        return;
    }
}

// Create a CMark pointing to the current position and state.
CBitstreamDecoder::CMark CBitstreamDecoder::Mark() const
{
    auto nextHistoryReadIndex = mNextHistoryReadIndex;

    // Unless we have returned to a mark, the current position will be to
    // read the next bit from the stream. In that case we need to save a
    // history marker that will point back to the end of history so that
    // when we retore the mark either it will point to a bit that is now
    // saved in history, or if no more bits are read it will still point
    // beyond the end of history.
    if (nextHistoryReadIndex >= mHistorySampleNumber.size()) {
        nextHistoryReadIndex = mHistorySampleNumber.size();
    }

    return CMark(*this, nextHistoryReadIndex);
}

void CBitstreamDecoder::SetToMark(const CMark& mark)
{
    mLastDataLevel = mark.mLastDataLevel;
    mContiguousOnesCount = mark.mContiguousOnesCount;
    mParityIsOdd = mark.mParityIsOdd;
    mCurrentSampleNumber = mark.mCurrentSampleNumber;
    mNextHistoryReadIndex = mark.mNextHistoryReadIndex;

    // If the mark was taken when the current position is reading from
    // the stream, and no more bits have been added to the history, it will
    // still (correctly) point beyond history. Invalidate it so that adding
    // to history now won't cause nextHistoryReadIndex to become within
    // the range of history.
    if (mNextHistoryReadIndex >= mHistorySampleNumber.size()) {
       invalidateHistoryReadIndex();
    }

}
