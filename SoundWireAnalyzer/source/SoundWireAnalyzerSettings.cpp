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

#include <algorithm>
#include <string>
#include <AnalyzerHelpers.h>

#include "SoundWireAnalyzerSettings.h"
#include "SoundWireProtocolDefs.h"

SoundWireAnalyzerSettings::SoundWireAnalyzerSettings()
  :     mInputChannelClock(UNDEFINED_CHANNEL),
        mInputChannelData(UNDEFINED_CHANNEL),
        mNumRows(48),
        mNumCols(2),
        mSuppressDuplicatePings(false),
        mAnnotateBitValues(false),
        mAnnotateFrameStarts(false),
        mAnnotateTrace(true)
{
    mInputChannelInterfaceClock.reset( new AnalyzerSettingInterfaceChannel() );
    mInputChannelInterfaceClock->SetTitleAndTooltip("SoundWire Clock", "SoundWire Clock");
    mInputChannelInterfaceClock->SetChannel(mInputChannelClock);

    mInputChannelInterfaceData.reset( new AnalyzerSettingInterfaceChannel() );
    mInputChannelInterfaceData->SetTitleAndTooltip("SoundWire Data", "SoundWire Data");
    mInputChannelInterfaceData->SetChannel(mInputChannelData);

    mRowInterface.reset(new AnalyzerSettingInterfaceNumberList());
    mRowInterface->SetTitleAndTooltip("Num Rows",  "Specify number of rows.");
    mRowInterface->AddNumber(0, "Auto", "Auto detect number of rows");
    auto sortedRows = kFrameShapeRows;
    std::sort(sortedRows.begin(), sortedRows.end());
    for (const auto it : sortedRows) {
        if (it == 0) {
            continue;
        }

        mRowInterface->AddNumber(it, std::to_string(it).c_str(), "");
    }
    // Default to auto-detection
    mRowInterface->SetNumber(0);

    mColInterface.reset(new AnalyzerSettingInterfaceNumberList());
    mColInterface->SetTitleAndTooltip("Num Cols",  "Specify number of columns.");
    mColInterface->AddNumber(0, "Auto", "Auto detect number of columns");
    auto sortedCols = kFrameShapeColumns;
    std::sort(sortedCols.begin(), sortedCols.end());
    for (const auto it : sortedCols) {
        if (it == 0) {
            continue;
        }

        mColInterface->AddNumber(it, std::to_string(it).c_str(), "");
    }
    // Default to auto-detection
    mColInterface->SetNumber(0);

    mSuppressDuplicatePingsInterface.reset(new AnalyzerSettingInterfaceBool());
    mSuppressDuplicatePingsInterface->SetCheckBoxText("Suppress duplicate pings in table");
    mSuppressDuplicatePingsInterface->SetValue(mSuppressDuplicatePings);

    mAnnotateBitValuesInterface.reset(new AnalyzerSettingInterfaceBool());
    mAnnotateBitValuesInterface->SetCheckBoxText("Annotate decoded bit values");
    mAnnotateBitValuesInterface->SetValue(mAnnotateBitValues);

    mAnnotateFrameStartsInterface.reset(new AnalyzerSettingInterfaceBool());
    mAnnotateFrameStartsInterface->SetCheckBoxText("Annotate frame starts");
    mAnnotateFrameStartsInterface->SetValue(mAnnotateFrameStarts);

    mAnnotateTraceInterface.reset(new AnalyzerSettingInterfaceBool());
    mAnnotateTraceInterface->SetCheckBoxText("Annotate trace");
    mAnnotateTraceInterface->SetValue(mAnnotateTrace);

    AddInterface(mInputChannelInterfaceClock.get());
    AddInterface(mInputChannelInterfaceData.get());
    AddInterface(mRowInterface.get());
    AddInterface(mColInterface.get());
    AddInterface(mSuppressDuplicatePingsInterface.get());
    AddInterface(mAnnotateBitValuesInterface.get());
    AddInterface(mAnnotateFrameStartsInterface.get());
    AddInterface(mAnnotateTraceInterface.get());

    ClearChannels();
    AddChannel(mInputChannelClock, "SoundWire Clock", false);
    AddChannel(mInputChannelData,  "SoundWire Data", false);

    // As of Logic 2.3.55 the UI ignores this and has a hardcoded
    // option to export to text/csv
    AddExportOption(0, "Export as text/csv file" );
    AddExportExtension(eExportCsv, "csv", "csv" );
    AddExportExtension(eExportText, "text", "txt" );
}

SoundWireAnalyzerSettings::~SoundWireAnalyzerSettings()
{
}

bool SoundWireAnalyzerSettings::SetSettingsFromInterfaces()
{
    mInputChannelClock = mInputChannelInterfaceClock->GetChannel();
    mInputChannelData  = mInputChannelInterfaceData->GetChannel();
    mNumRows = static_cast<unsigned int>(mRowInterface->GetNumber());
    mNumCols = static_cast<unsigned int>(mColInterface->GetNumber());
    mSuppressDuplicatePings = mSuppressDuplicatePingsInterface->GetValue();
    mAnnotateBitValues = mAnnotateBitValuesInterface->GetValue();
    mAnnotateFrameStarts = mAnnotateFrameStartsInterface->GetValue();
    mAnnotateTrace = mAnnotateTraceInterface->GetValue();

    ClearChannels();
    AddChannel(mInputChannelClock, "SoundWire Clock", true);
    AddChannel(mInputChannelData,  "SoundWire Data",  true);

    return true;
}

void SoundWireAnalyzerSettings::UpdateInterfacesFromSettings()
{
    mInputChannelInterfaceClock->SetChannel(mInputChannelClock);
    mInputChannelInterfaceData->SetChannel(mInputChannelData);

    mRowInterface->SetNumber(mNumRows);
    mColInterface->SetNumber(mNumCols);
    mSuppressDuplicatePingsInterface->SetValue(mSuppressDuplicatePings);
    mAnnotateBitValuesInterface->SetValue(mAnnotateBitValues);
    mAnnotateFrameStartsInterface->SetValue(mAnnotateFrameStarts);
    mAnnotateTraceInterface->SetValue(mAnnotateTrace);
}

void SoundWireAnalyzerSettings::LoadSettings(const char* settings)
{
    try {
        SimpleArchive text_archive;
        text_archive.SetString( settings );

        text_archive >> mInputChannelClock;
        text_archive >> mInputChannelData;
        text_archive >> mNumRows;
        text_archive >> mNumCols;
        text_archive >> mSuppressDuplicatePings;
        text_archive >> mAnnotateBitValues;
        text_archive >> mAnnotateFrameStarts;
        text_archive >> mAnnotateTrace;

        ClearChannels();
        AddChannel(mInputChannelClock, "SoundWire Clock", true);
        AddChannel(mInputChannelData,  "SoundWire Data", true);

        UpdateInterfacesFromSettings();
    } catch(...) {
    }
}

const char* SoundWireAnalyzerSettings::SaveSettings()
{
    SimpleArchive text_archive;

    text_archive << mInputChannelClock;
    text_archive << mInputChannelData;
    text_archive << mNumRows;
    text_archive << mNumCols;
    text_archive << mSuppressDuplicatePings;
    text_archive << mAnnotateBitValues;
    text_archive << mAnnotateFrameStarts;
    text_archive << mAnnotateTrace;

    return SetReturnString(text_archive.GetString());
}
