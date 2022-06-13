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
#include "CSyncFinder.h"
#include "SoundWireAnalyzer.h"
#include "SoundWireAnalyzerSettings.h"

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
    bool inSync = false;

    // The sync finder will need to rewind so CBitStreamDecoder must be
    // collecting history
    mDecoder->CollectHistory(true);

    for (;;) {
        if (!inSync) {
          syncFinder.FindSync(mSettings->mNumRows, mSettings->mNumCols);

          inSync = true;
          mResults->AddMarker(mDecoder->CurrentSampleNumber(),
                              AnalyzerResults::Stop,
                              mSettings->mInputChannelClock);

          // Now we have a good frame we don't need any history before this point
          mDecoder->DiscardHistoryBeforeCurrentPosition();
        }

        bool bitValue = mDecoder->NextBitValue();
        U64 sampleNumber = mDecoder->CurrentSampleNumber();

        mResults->AddMarker(sampleNumber,
                            bitValue ? AnalyzerResults::One : AnalyzerResults::Zero,
                            mSettings->mInputChannelData);

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
