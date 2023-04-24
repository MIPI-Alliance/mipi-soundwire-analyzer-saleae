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

#ifndef SOUNDWIRE_ANALYZER_H
#define SOUNDWIRE_ANALYZER_H

#include <Analyzer.h>
#include "CBitstreamDecoder.h"
#include "CFrameReader.h"
#include "SoundWireAnalyzerResults.h"
#include "SoundWireSimulationDataGenerator.h"

class SoundWireAnalyzerSettings;

class SoundWireAnalyzer : public Analyzer2
{
public:
    SoundWireAnalyzer();
    virtual ~SoundWireAnalyzer();

    void SetupResults();
    void WorkerThread();

    U32 GenerateSimulationData(U64 newest_sample_requested,
                               U32 sample_rate,
                               SimulationChannelDescriptor** simulation_channels );
    U32 GetMinimumSampleRateHz();

    const char* GetAnalyzerName() const;
    bool NeedsRerun();

    void NotifyBusReset(U64 startSampleNumber, U64 endSampleNumber);

    inline void AnnotateBitValue(U64 sampleNumber, bool value)
    {
            if (mAnnotateBitValues) {
                mResults->AddMarker(sampleNumber,
                                    value ? AnalyzerResults::One : AnalyzerResults::Zero,
                                    mInputChannelData);
            }
    }

private:
    void addFrameShapeMessage(U64 sampleNumber, int rows, int columns);
    void addFrameV2(const CControlWordBuilder& controlWord, const Frame& fv1);

private:
    std::unique_ptr<SoundWireAnalyzerSettings> mSettings;
    std::unique_ptr<SoundWireAnalyzerResults> mResults;
    Channel mInputChannelClock;
    Channel mInputChannelData;
    AnalyzerChannelData* mSoundWireClock;
    AnalyzerChannelData* mSoundWireData;

    std::unique_ptr<CBitstreamDecoder> mDecoder;

    bool mAddBubbleFrames;
    bool mAnnotateBitValues;

    std::unique_ptr<SoundWireSimulationDataGenerator> mSimulationDataGenerator;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //SOUNDWIRE_ANALYZER_H
