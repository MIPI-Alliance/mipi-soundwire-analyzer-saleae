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

#include <AnalyzerChannelData.h>

#include "CBitstreamDecoder.h"
#include "CFrameReader.h"
#include "CSyncFinder.h"
#include "SoundWireAnalyzer.h"
#include "SoundWireAnalyzerSettings.h"
#include "SoundWireAnalyzerResults.h"

SoundWireAnalyzer::SoundWireAnalyzer()
  :     Analyzer2(),
        mSettings(new SoundWireAnalyzerSettings())
{
    SetAnalyzerSettings(mSettings.get());
}

SoundWireAnalyzer::~SoundWireAnalyzer()
{
    KillThread();
}

void SoundWireAnalyzer::SetupResults()
{
    mResults.reset(new SoundWireAnalyzerResults(this, mSettings.get()));
    SetAnalyzerResults(mResults.get());
    mResults->AddChannelBubblesWillAppearOn(mSettings->mInputChannelClock);
    mResults->AddChannelBubblesWillAppearOn(mSettings->mInputChannelData);
}

void SoundWireAnalyzer::WorkerThread()
{
    mSoundWireClock = GetAnalyzerChannelData(mSettings->mInputChannelClock);
    mSoundWireData = GetAnalyzerChannelData(mSettings->mInputChannelData);

    mDecoder.reset(new CBitstreamDecoder(mSoundWireClock, mSoundWireData));

    // Advance one bit to get an initial data line state
    mDecoder->NextBitValue();

    CSyncFinder syncFinder(*this, *mDecoder);
    CFrameReader frameReader;
    bool inSync = false;
    bool isFirstFrame = true;
    bool actualParityIsOdd;
    Frame f;

    // The sync finder will need to rewind so CBitStreamDecoder must be
    // collecting history
    mDecoder->CollectHistory(true);

    for (;;) {
        if (!inSync) {
            syncFinder.FindSync(mSettings->mNumRows, mSettings->mNumCols);
            inSync = true;
            isFirstFrame = true;
            frameReader.Reset();
            frameReader.SetShape(mSettings->mNumRows, mSettings->mNumCols);

            // Now we have a good frame we don't need any history before this point
            mDecoder->DiscardHistoryBeforeCurrentPosition();
        }

        bool bitValue = mDecoder->NextBitValue();
        U64 sampleNumber = mDecoder->CurrentSampleNumber();

        mResults->AddMarker(sampleNumber,
                            bitValue ? AnalyzerResults::One : AnalyzerResults::Zero,
                            mSettings->mInputChannelData);

        switch (frameReader.PushBit(bitValue)) {
        case CFrameReader::eFrameStart:
            f.mStartingSampleInclusive = sampleNumber;

            // Mark start of frame with a green dot on the clock
            mResults->AddMarker(sampleNumber,
                                AnalyzerResults::Start,
                                mSettings->mInputChannelClock);
            break;
        case CFrameReader::eNeedMoreBits:
            break;
        case CFrameReader::eCaptureParity:
            actualParityIsOdd = mDecoder->IsParityOdd();
            mDecoder->ResetParity();
            break;
        case CFrameReader::eFrameComplete:
            f.mEndingSampleInclusive = sampleNumber;
            f.mData1 = frameReader.ControlWord().Value();
            f.mType = 0;
            f.mFlags = 0;

            // We can't calculate parity for the first frame because parity
            // includes the end of the previous frame.
            if (actualParityIsOdd != frameReader.ControlWord().Par()) {
                if (!isFirstFrame) {
                    f.mFlags |= SoundWireAnalyzerResults::kFlagParityBad;
                }
            }

            mResults->AddFrame(f);
            frameReader.Reset();
            isFirstFrame = false;

            // Now we've decoded this frame the history bits can be discarded
            // save memory. History collection must remain enabled in case we
            // lose sync on the next frame and have to rewind it.
            mDecoder->DiscardHistoryBeforeCurrentPosition();
            break;
        }

        mResults->CommitResults();
        ReportProgress(sampleNumber);
        CheckIfThreadShouldExit();
    }
}

bool SoundWireAnalyzer::NeedsRerun()
{
    return false;
}

U32 SoundWireAnalyzer::GenerateSimulationData(U64 minimum_sample_index,
                                              U32 device_sample_rate,
                                              SimulationChannelDescriptor** simulation_channels)
{
    if (!mSimulationDataGenerator) {
        mSimulationDataGenerator.reset(new SoundWireSimulationDataGenerator());
        mSimulationDataGenerator->Initialize(GetSimulationSampleRate(), mSettings.get());
    }

    return mSimulationDataGenerator->GenerateSimulationData(minimum_sample_index,
                                                            device_sample_rate,
                                                            simulation_channels);
}

U32 SoundWireAnalyzer::GetMinimumSampleRateHz()
{
    return 1000000;
}

const char* SoundWireAnalyzer::GetAnalyzerName() const
{
    return "SoundWire";
}

const char* GetAnalyzerName()
{
    return "SoundWire";
}

Analyzer* CreateAnalyzer()
{
    return new SoundWireAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
    delete analyzer;
}
