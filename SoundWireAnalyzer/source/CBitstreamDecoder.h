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

#ifndef CBITSTREAMDECODER_H
#define CBITSTREAMDECODER_H

#include <vector>
#include <AnalyzerChannelData.h>
#include <LogicPublicTypes.h>

class CBitstreamDecoder
{
public:
    class CMark
    {
    public:
        CMark(const CBitstreamDecoder& decoder, size_t nextHistoryReadIndex);

    private:
        CMark();

    private:
        friend class CBitstreamDecoder;

        enum BitState mLastDataLevel;
        bool mParityIsOdd;
        unsigned int mContiguousOnesCount;
        size_t mNextHistoryReadIndex;
        U64 mCurrentSampleNumber;
    };

public:
    CBitstreamDecoder(AnalyzerChannelData* clock, AnalyzerChannelData* data);
    ~CBitstreamDecoder();

    bool NextBitValue();
    void SkipBits(U64 numBits);

    U64 CurrentSampleNumber() const
        { return mCurrentSampleNumber; }

    bool IsParityOdd() const
        { return mParityIsOdd; }

    void ResetParity();

    U64 ContiguousOnesCount() const
        { return mContiguousOnesCount; }

    // The following functions are for use when trying to find sync
    void CollectHistory(bool enable);
    void DiscardHistoryBeforeCurrentPosition();
    CMark Mark() const;
    void SetToMark(const CMark& mark);

private:
    void invalidateHistoryReadIndex();
    void appendBitToHistory(enum BitState level, U64 sampleDelta);
    enum BitState nextBitFromHistory(U64& sampleDelta);

private:
    friend class CMark;

    AnalyzerChannelData* mClock;
    AnalyzerChannelData* mData;
    U64 mCurrentSampleNumber;
    unsigned int mContiguousOnesCount;
    bool mParityIsOdd;
    enum BitState mLastDataLevel;
    size_t mNextHistoryReadIndex;
    bool mCollectHistory;

    std::vector<U16> mHistory;
};

#endif // CBITSTREAMDECODER_H
