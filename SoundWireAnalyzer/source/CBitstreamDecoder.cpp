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

#include <LogicPublicTypes.h>
#include <AnalyzerChannelData.h>

#include "CBitstreamDecoder.h"
#include "CDynamicSyncGenerator.h"
#include "SoundWireAnalyzer.h"
#include "SoundWireProtocolDefs.h"

// Reduce size of history buffer by storing the delta between sample numbers.
// SWIRE_CLK is typically >1MHz so at 500MS/s the sample number delta between
// bits is usually <250. However we could stray into a gap in the clock so
// larger deltas are stored by adding extra entries with an "overflow" flag
// indicating that more bits must be accumulated from the next entry.
// Each history entry is 16 bits with 14 bits of sample delta, 1 bit for the
// overflow flag and 1 bit for the data line value.
// As an initial sequence would be 4096 bits for bus reset then 16 frames for
// the sync sequence, the worst case is around 69632 bits of history, so at
// 2 bytes per entry this is a considerable memory saving over at least 9 bytes
// per entry (u64 sample number plus at least 1 byte data value), and also
// improves cache locality.
static const int kHistoryDeltaFragmentBits      = 14;
static const unsigned int kHistoryDeltaMask     = 0x3fff;
static const unsigned int kHistoryDeltaOverflow = 0x4000;
static const unsigned int kHistoryBitHighFlag   = 0x8000;

CBitstreamDecoder::CMark::CMark(const CBitstreamDecoder& decoder, size_t nextHistoryReadIndex)
    : mLastDataLevel(decoder.mLastDataLevel),
      mParityIsOdd(decoder.mParityIsOdd),
      mCurrentSampleNumber(decoder.mCurrentSampleNumber),
      mNextHistoryReadIndex(nextHistoryReadIndex)
{
}

CBitstreamDecoder::CBitstreamDecoder(SoundWireAnalyzer& analyzer,
                                     AnalyzerChannelData* clock,
                                     AnalyzerChannelData* data)
    : mAnalyzer(analyzer),
      mClock(clock),
      mData(data),
      mCurrentSampleNumber(0),
      mContiguousOnesCount(0),
      mParityIsOdd(false),
      mLastDataLevel(data->GetBitState()),
      mNextHistoryReadIndex(kInvalidHistoryIndex),
      mCollectHistory(false)
{
    // The history can get quite large, typically needing 4096 bits for bus
    // reset then 16 frames for the sync sequence. Reserve space to avoid
    // a push_back() having to reallocate.
    mHistory.reserve(kBusResetOnesCount +
                     ((kMaxRows * kMaxColumns) * CDynamicSyncGenerator::kSequenceLengthFrames));
}

CBitstreamDecoder::~CBitstreamDecoder()
{
}

inline void CBitstreamDecoder::invalidateHistoryReadIndex()
{
    mNextHistoryReadIndex = kInvalidHistoryIndex;
}

void CBitstreamDecoder::appendBitToHistory(enum BitState level, U64 sampleDelta)
{
    U16 dataLevelFlag = 0;
    if (level == BIT_HIGH) {
        dataLevelFlag |= kHistoryBitHighFlag;
    }

    if (sampleDelta <= kHistoryDeltaMask) {
        // Quick handling of most common case
        mHistory.push_back(dataLevelFlag | static_cast<U16>(sampleDelta));
        return;
    }

    do {
        mHistory.push_back(dataLevelFlag |
                           ((sampleDelta > kHistoryDeltaMask) ? kHistoryDeltaOverflow : 0) |
                           static_cast<U16>(sampleDelta) & kHistoryDeltaMask);
        sampleDelta >>= kHistoryDeltaFragmentBits;
    } while (sampleDelta);
}

enum BitState CBitstreamDecoder::nextBitFromHistory(U64& sampleDelta)
{
    U16 val = mHistory[mNextHistoryReadIndex++];
    enum BitState state = (val & kHistoryBitHighFlag) ? BIT_HIGH : BIT_LOW;
    U64 delta = val & kHistoryDeltaMask;

    if (val & kHistoryDeltaOverflow) {
        for (int i = kHistoryDeltaFragmentBits; val & kHistoryDeltaOverflow; i += kHistoryDeltaFragmentBits) {
            val = mHistory[mNextHistoryReadIndex++];
            delta |= static_cast<U64>(val & kHistoryDeltaMask) << i;
        }
    }

    if (mNextHistoryReadIndex == mHistory.size()) {
        // Prevent it becoming a valid index if more data is added to history
        invalidateHistoryReadIndex();
    }

    sampleDelta = delta;
    return state;
}

// Advance mClock and mData to the next clock edge and return the
// decoded bit state.
bool CBitstreamDecoder::NextBitValue()
{
    BitState level;
    bool decodedBitValue;

    // We need to be able to go back to past data when trying to find sync
    // but the Saleae APIs can only go forward. If data has been rewound to
    // a mark fetch the data from the history buffer until we reach
    // the end of the buffer.
    if (mNextHistoryReadIndex < mHistory.size()) {
        U64 delta;
        level = nextBitFromHistory(delta);
        mCurrentSampleNumber += delta;

        // NRZ signals a 1 by a change of level, 0 by no change.
        decodedBitValue = (level != mLastDataLevel);
    } else {
        mClock->AdvanceToNextEdge();
        U64 sampleNum = mClock->GetSampleNumber();
        mData->AdvanceToAbsPosition(sampleNum);
        level = mData->GetBitState();
        decodedBitValue = (level != mLastDataLevel);

        // Bit annotations are only added when a new bit is read from the channel.
        mAnalyzer.AnnotateBitValue(sampleNum, decodedBitValue);

        if (mCollectHistory) {
            appendBitToHistory(level, sampleNum - mCurrentSampleNumber);
        }

        mCurrentSampleNumber = sampleNum;

        // A run of 4096 data line toggles is a bus reset
        if (level != mLastDataLevel) {
            switch (mContiguousOnesCount) {
            case 0:
                mContiguousOnesStartSample = mCurrentSampleNumber;
                ++mContiguousOnesCount;
                break;
            case 4095:
                // Seen 4095 already so this is the 4096th and final
                mAnalyzer.NotifyBusReset(mContiguousOnesStartSample, mCurrentSampleNumber);
                mContiguousOnesCount = 0;
                break;
            default:
                ++mContiguousOnesCount;
                break;
            }
        } else {
            mContiguousOnesCount = 0;
        }
    }

    mLastDataLevel = level;

    // Parity counts the number of high levels (not the number of decoded ones).
    if (level == BIT_HIGH) {
        mParityIsOdd = !mParityIsOdd;
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
    if (mHistory.size() == 0) {
        return;
    }

    // Discarding the front of a vector is expensive so only discard if
    // the entire buffer is obsolete.
    if (mNextHistoryReadIndex >= mHistory.size()) {
        mHistory.clear();
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
    if (nextHistoryReadIndex >= mHistory.size()) {
        nextHistoryReadIndex = mHistory.size();
    }

    return CMark(*this, nextHistoryReadIndex);
}

void CBitstreamDecoder::SetToMark(const CMark& mark)
{
    mLastDataLevel = mark.mLastDataLevel;
    mParityIsOdd = mark.mParityIsOdd;
    mCurrentSampleNumber = mark.mCurrentSampleNumber;
    mNextHistoryReadIndex = mark.mNextHistoryReadIndex;

    // If the mark was taken when the current position is reading from
    // the stream, and no more bits have been added to the history, it will
    // still (correctly) point beyond history. Invalidate it so that adding
    // to history now won't cause nextHistoryReadIndex to become within
    // the range of history.
    if (mNextHistoryReadIndex >= mHistory.size()) {
       invalidateHistoryReadIndex();
    }

}
