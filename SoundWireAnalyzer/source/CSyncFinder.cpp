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
#include "CControlWordBuilder.h"
#include "CDynamicSyncGenerator.h"
#include "CFrameReader.h"
#include "CSyncFinder.h"
#include "SoundWireAnalyzer.h"
#include "SoundWireProtocolDefs.h"

// Row number of last bit in static sync word
static const int kLastStaticSyncRow = kCtrlStaticSyncRow + kCtrlStaticSyncNumRows - 1;

// Sliding search window width
static const int kSearchWindowBits = 4096;

// The 8 static sync bits are in column 0. The maximum number of columns is
// 16 so a full static sync word cannot cover more than 8 * 16 = 128 bits.
class CStaticSyncMatcher
{
public:
    void Reset(int columns);
    bool PushBit(bool isOne);

private:
    enum {
        kHigh = 0,
        kLow  = 1
    };
    std::array<U64, 2> mAccumulator;
    std::array<U64, 2> mMask;
    std::array<U64, 2> mMatch;
};

void CStaticSyncMatcher::Reset(int columns)
{
    mAccumulator.fill(0);

    // Create sync bits match and mask as they will appear in the bitstream.
    // For example if there are 4 columns the 8 sync bits will be at every 4th
    // bit in the bitstream and those are the only bits we need to match.
    // mMask is the mask of bits that we need to check and mMatch is the
    // static sync pattern spread out over those bits.
    switch (columns) {
                // High                   Low
    case 2:
        mMask =  { 0x0000000000000000ULL, 0x0000000000005555ULL };
        mMatch = { 0x0000000000000000ULL, 0x0000000000004501ULL };
        break;
    case 4:
        mMask =  { 0x0000000000000000ULL, 0x0000000011111111ULL };
        mMatch = { 0x0000000000000000ULL, 0x0000000010110001ULL };
        break;
    case 6:
        mMask =  { 0x0000000000000000ULL, 0x0000041041041041ULL };
        mMatch = { 0x0000000000000000ULL, 0x0000040041000001ULL };
        break;
    case 8:
        mMask =  { 0x0000000000000000ULL, 0x0101010101010101ULL };
        mMatch = { 0x0000000000000000ULL, 0x0100010100000001ULL };
        break;
    case 10:
        mMask =  { 0x0000000000000040ULL, 0x1004010040100401ULL };
        mMatch = { 0x0000000000000040ULL, 0x0004010000000001ULL };
        break;
    case 12:
        mMask =  { 0x0000000000100100ULL, 0x1001001001001001ULL };
        mMatch = { 0x0000000000100000ULL, 0x1001000000000001ULL };
        break;
    case 14:
        mMask =  { 0x0000000400100040ULL, 0x0100040010004001ULL };
        mMatch = { 0x0000000400000040ULL, 0x0100000000000001ULL };
        break;
    case 16:
        mMask =  { 0x0001000100010001ULL, 0x0001000100010001ULL };
        mMatch = { 0x0001000000010001ULL, 0x0000000000000001ULL };
        break;
    default:
        // We should never get here!
        mMask.fill(0);
        mMatch.fill(0);
        break;
    }
}

bool CStaticSyncMatcher::PushBit(bool isOne)
{
    if (mMask[CStaticSyncMatcher::kHigh] == 0) {
        // Only need 64-bit window to find the match
        mAccumulator[CStaticSyncMatcher::kLow] <<= 1;
        mAccumulator[CStaticSyncMatcher::kLow] |= isOne;

        if ((mAccumulator[CStaticSyncMatcher::kLow] & mMask[CStaticSyncMatcher::kLow]) == mMatch[CStaticSyncMatcher::kLow]) {
            return true;
        }
    } else {
        // Shift 128-bit value left one place
        mAccumulator[CStaticSyncMatcher::kHigh] <<= 1;
        mAccumulator[CStaticSyncMatcher::kHigh] |= mAccumulator[CStaticSyncMatcher::kLow] >> 63;
        mAccumulator[CStaticSyncMatcher::kLow] <<= 1;
        mAccumulator[CStaticSyncMatcher::kLow] |= isOne;

        if (((mAccumulator[CStaticSyncMatcher::kLow] & mMask[CStaticSyncMatcher::kLow]) == mMatch[CStaticSyncMatcher::kLow]) &&
            ((mAccumulator[CStaticSyncMatcher::kHigh] & mMask[CStaticSyncMatcher::kHigh]) == mMatch[CStaticSyncMatcher::kHigh])) {
            return true;
        }
    }

    return false;
}

CSyncFinder::CSyncFinder(SoundWireAnalyzer& analyzer, CBitstreamDecoder& bitstream)
    : mAnalyzer(analyzer), mBitstream(bitstream)
{
}

// Return the number of valid frames found from the current position, up to
// the maximum dynamic sequence length. Returns with bitstream position unchanged.
int CSyncFinder::checkSync(int rows, int columns)
{
    CFrameReader frame;
    frame.SetShape(rows, columns);
    CBitstreamDecoder::CMark startMark = mBitstream.Mark();

    // We can't check the first frame validity because we have no previous
    // parity or sync info to compare against, so it is just a seed for
    // checking the subsequent frames.
    CFrameReader::TState state;
    do {
        state = frame.PushBit(mBitstream.NextBitValue());
        if (state == CFrameReader::eCaptureParity) {
            // Reset parity accumulation so it will be valid for next frame
            mBitstream.ResetParity();
        }
    } while (state != CFrameReader::eFrameComplete);

    // The dynamic sync can never be 0
    if (frame.ControlWord().DynamicSync() == 0) {
        return 0;
    }

    // Seed dynamic sequence from value in first frame
    CDynamicSyncGenerator dynamicSync;
    dynamicSync.SetValue(frame.ControlWord().DynamicSync());

    int framesOk = 1; // include the seed frame

    // Try to match the remaining frames in a sync sequence
    for (int i = 0; i < CDynamicSyncGenerator::kSequenceLengthFrames - 1; ++i) {
        // Has frame shape changed?
        if (frame.ControlWord().IsFrameShapeChange()) {
            frame.ControlWord().GetNewShape(rows, columns);
            frame.SetShape(rows, columns);
        }

        frame.Reset();

        CFrameReader::TState state;
        bool parityIsOdd;
        do {
            state = frame.PushBit(mBitstream.NextBitValue());
            if (state == CFrameReader::eCaptureParity) {
                parityIsOdd = mBitstream.IsParityOdd();
                mBitstream.ResetParity();
            }
        } while (state != CFrameReader::eFrameComplete);

        auto expectDynamicSync = dynamicSync.Next();
        if ((frame.ControlWord().Par() != parityIsOdd) ||
            (frame.ControlWord().StaticSync() != kStaticSyncVal) ||
            (frame.ControlWord().DynamicSync() != expectDynamicSync)) {
            // Not a valid frame - give up
            mBitstream.SetToMark(startMark);
            return framesOk;
        }
        ++framesOk;
    }

    mBitstream.SetToMark(startMark);

    return framesOk;
}

bool CSyncFinder::testIfSyncIsReal(const std::vector<int>* rowsList, const int columns, const U64 matchedBitOffset,
                                   const CBitstreamDecoder::CMark& searchStartMark)
{
    // We're called after the last static sync bit has been read.
    // Calculate how many bits of the frame have already been read.
    const U64 lastStaticSyncBitOffset = BitOffsetInFrame(columns, kLastStaticSyncRow, 0);

    // Save position to restart frame sequence search if this doesn't work out
    const CBitstreamDecoder::CMark seqSearchRestartMark = mBitstream.Mark();

    for (auto itRows : *rowsList) {
        if (itRows == 0)
            continue;

        // Are there enough bits before the static sync word to form a full frame?
        // If not, skip on to where the next frame should start.
        if (matchedBitOffset >= lastStaticSyncBitOffset) {
            mBitstream.SetToMark(searchStartMark);
            mBitstream.SkipBits(matchedBitOffset - lastStaticSyncBitOffset);
        } else {
            mBitstream.SkipBits(TotalBitsInFrame(itRows, columns) - lastStaticSyncBitOffset);
        }

        int framesOk = checkSync(itRows, columns);
        if (framesOk > 15) {
            mRows = itRows;
            mColumns = columns;
            return true;
        }

        // Didn't find a frame sequence. Rewind and try a different number of rows
        mBitstream.SetToMark(seqSearchRestartMark);
    }

    return false;
}

// Search for a sync and return with the CBitstreamDecoder pointing at the first
// complete frame.
void CSyncFinder::FindSync(int rows, int columns)
{
    auto triggerSample = mAnalyzer.GetTriggerSample();
    auto sampleRate = mAnalyzer.GetSampleRate();

    CStaticSyncMatcher matcher;
    auto *rowsList = &kFrameShapeRows;
    auto *columnsList = &kFrameShapeColumns;

    if (rows != 0) {
        mSingleRowList = { rows };
        rowsList = &mSingleRowList;
    }

    if (columns != 0) {
        mSingleColumnList = { columns };
        columnsList = &mSingleColumnList;
    }

    for(;;) {
        CBitstreamDecoder::CMark syncSearchStartMark = mBitstream.Mark();

        for (auto itCols : *columnsList) {
            matcher.Reset(itCols);
            U64 matchedBitOffset = 0;

            // Limit the static sync word search to the the search window plus
            // one frame before trying another column count.
            // This prevents having to scan all the way to the end of the data
            // capture before trying another column count, or failing to detect
            // the first possible sync because the current column count matches
            // a sync later in the capture.
            U64 maxStaticSyncSearchBits = kSearchWindowBits + TotalBitsInFrame(kMaxRows, itCols);
            for (; matchedBitOffset < maxStaticSyncSearchBits; ++matchedBitOffset) {
                bool foundStaticSync = matcher.PushBit(mBitstream.NextBitValue());
                if (foundStaticSync) {
                    if (testIfSyncIsReal(rowsList, itCols, matchedBitOffset, syncSearchStartMark)) {
                        return;
                    }
                }
            }

            // Didn't find a sync. Rewind and try a different number of columns
            mBitstream.SetToMark(syncSearchStartMark);
        }

        // No column count matched, wind on to next search window and try again.
        // A static sync could straddle the end of the chunk we searched so
        // don't skip the entire chunk.
        mBitstream.SkipBits(kSearchWindowBits);
        mAnalyzer.CheckIfThreadShouldExit();
    }
}
