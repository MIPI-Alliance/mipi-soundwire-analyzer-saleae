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

#include <AnalyzerHelpers.h>

#include "SoundWireSimulationDataGenerator.h"
#include "SoundWireAnalyzerSettings.h"
#include "SoundWireProtocolDefs.h"

static const double kDefaultSwireClockHz = 4800000;

// [0] is never used because the PRNG would get stuck. Each value
// is also the index into this array of the next value in sequence.
static const U32 kDynamicSync[] = {
    0, 2, 4, 6, 9, 11, 13, 15, 1, 3, 5, 7, 8, 10, 12, 14
};

SoundWireSimulationDataGenerator::SoundWireSimulationDataGenerator()
    : mClock(NULL), mData(NULL)
{
}

SoundWireSimulationDataGenerator::~SoundWireSimulationDataGenerator()
{
}

void SoundWireSimulationDataGenerator::Initialize(U32 simulation_sample_rate,
                                                  SoundWireAnalyzerSettings* settings)
{
    mSettings = settings;
    mSimulationSampleRate = simulation_sample_rate;

    mDynamicSyncIndex = 1;
    mRunningParity = 0;
    mPingCount = 0;
    mDoneBusReset = false;
    mOpCode = kOpPing;

    mClock = mSimulationChannels.Add(mSettings->mInputChannelClock, simulation_sample_rate, BIT_HIGH);
    mData = mSimulationChannels.Add(mSettings->mInputChannelData, simulation_sample_rate, BIT_LOW);

    mClockGenerator.Init(kDefaultSwireClockHz, simulation_sample_rate);

    // Advance clock by one sample so that the clock edge follows the
    // data edge by one sample.
    mClock->Advance(1);
}

U32 SoundWireSimulationDataGenerator::GenerateSimulationData(U64 largest_sample_requested,
                                                             U32 sample_rate,
                                                             SimulationChannelDescriptor** simulation_channel)
{
    U64 adjLargestSampleRequested =
        AnalyzerHelpers::AdjustSimulationTargetSample(largest_sample_requested,
                                                      sample_rate, mSimulationSampleRate);

    // Start with a bus reset
    if (!mDoneBusReset) {
        for (int i = 0; i < 4096; ++i ) {
            mSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(0.5));
            mClock->Transition();
            mData->Transition();
        }
        mDoneBusReset = true;
    }

    while(mClock->GetCurrentSampleNumber() < adjLargestSampleRequested ) {
        CreateFrame();
        mDynamicSyncIndex = kDynamicSync[mDynamicSyncIndex];
    }

    *simulation_channel = mSimulationChannels.GetArray();

    return mSimulationChannels.GetCount();
}

void SoundWireSimulationDataGenerator::CreateFrame()
{
    const U32 numRows = mSettings->mNumRows;
    const U32 numCols = mSettings->mNumCols;
    U32 nextPAR;
    U64 command = 0;

    command |= static_cast<U64>(mOpCode) << (kCtrlWordLastRow - kCtrlOpCodeRow - kCtrlOpCodeNumRows + 1);
    command |= static_cast<U64>(kStaticSyncVal) << (kCtrlWordLastRow - kCtrlStaticSyncRow - kCtrlStaticSyncNumRows + 1);
    command |= static_cast<U64>(kDynamicSync[mDynamicSyncIndex]) << (kCtrlWordLastRow - kCtrlDynamicSyncRow - kCtrlDynamicSyncNumRows + 1);

    switch (mOpCode) {
    case kOpPing:
        // Insert an SSP every few pings
        if (++mPingCount == 15) {
            command |= 1ULL << (kCtrlWordLastRow - kPingSSPRow);
            mPingCount = 0;
        }
        mOpCode = kOpRead;
        break;
    case kOpRead:
        command |= static_cast<U64>(0x50 + mPingCount) << (kCtrlWordLastRow - kRegAddrRow - kRegAddrNumRows + 1);
        command |= static_cast<U64>(mPingCount) << (kCtrlWordLastRow - kRegDataRow - kRegDataNumRows + 1);
        command |= 1ULL << (kCtrlWordLastRow - kCtrlACKRow);
        if (mPingCount == 13) {
            mOpCode = kOpWrite;
        } else {
            mOpCode = kOpPing;
        }
        break;
    case kOpWrite:
        command |= static_cast<U64>(0x321) << (kCtrlWordLastRow - kRegAddrRow - kRegAddrNumRows + 1);
        command |= static_cast<U64>(0xA5) << (kCtrlWordLastRow - kRegDataRow - kRegDataNumRows + 1);
        command |= 1ULL << (kCtrlWordLastRow - kCtrlACKRow);
        mOpCode = kOpPing;
    }

    for (U32 row = 0; row < numRows; ++row) {
        // First column is command word
        // AdvanceByHalfPeriod(n) actually advances by n full periods!!
        mSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(0.5));
        mClock->Transition();

        if (row == kCtrlPARRow) {
            // Stuff in the PAR bit
            if (nextPAR) {
                mData->Transition();
            }
        } else {
            // NRZI: a 1 is represented by a transition, 0 is no transition
            if (command & (1ULL << kCtrlWordLastRow)) {
                mData->Transition();
            }
        }

        command <<= 1;

        if (mData->GetCurrentBitState() == BIT_HIGH) {
            ++mRunningParity;
        }

        // Parity is up to and including column 0 of the row before the PAR bit.
        // PAR=1 if we've had an odd number of BIT_HIGH.
        if (row == kCtrlPARRow - 1) {
            nextPAR = (mRunningParity & 1);
            mRunningParity = 0;
        }


        // Pad remaining columns with zeros
        for (U32 col = 1; col < numCols; ++col) {
            mSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(0.5));
            mClock->Transition();

            if (mData->GetCurrentBitState() == BIT_HIGH) {
                ++mRunningParity;
            }
        }
    }
}

