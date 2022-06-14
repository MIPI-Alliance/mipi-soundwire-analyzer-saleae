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

#ifndef SOUNDWIRE_ANALYZER_RESULTS_H
#define SOUNDWIRE_ANALYZER_RESULTS_H

#include <AnalyzerResults.h>

class SoundWireAnalyzer;
class SoundWireAnalyzerSettings;

class SoundWireAnalyzerResults : public AnalyzerResults
{
public:
    // Used in mFlags field of frame.
    static const int kFlagParityBad = (1 << 0);

public:
    SoundWireAnalyzerResults(SoundWireAnalyzer* analyzer, SoundWireAnalyzerSettings* settings);
    virtual ~SoundWireAnalyzerResults();

    void GenerateBubbleText(U64 frame_index, Channel& channel,
                            DisplayBase display_base);
    void GenerateExportFile(const char* file, DisplayBase display_base,
                            U32 export_type_user_id);

    void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base);
    void GeneratePacketTabularText(U64 packet_id, DisplayBase display_base);
    void GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base);

protected:
    SoundWireAnalyzerSettings* mSettings;
    SoundWireAnalyzer* mAnalyzer;
};

#endif //SOUNDWIRE_ANALYZER_RESULTS_H
