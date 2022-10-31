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
        mSuppressDuplicatePings(false)
{
    mInputChannelInterfaceClock.reset( new AnalyzerSettingInterfaceChannel() );
    mInputChannelInterfaceClock->SetTitleAndTooltip("SoundWire Clock", "SoundWire Clock");
    mInputChannelInterfaceClock->SetChannel(mInputChannelClock);

    mInputChannelInterfaceData.reset( new AnalyzerSettingInterfaceChannel() );
    mInputChannelInterfaceData->SetTitleAndTooltip("SoundWire Data", "SoundWire Data");
    mInputChannelInterfaceData->SetChannel(mInputChannelData);

    mRowInterface.reset(new AnalyzerSettingInterfaceNumberList());
    mRowInterface->SetTitleAndTooltip("Num Rows",  "Specify number of rows.");
    auto sortedRows = kFrameShapeRows;
    std::sort(sortedRows.begin(), sortedRows.end());
    for (const auto it : sortedRows) {
        if (it == 0) {
            continue;
        }

        mRowInterface->AddNumber(it, std::to_string(it).c_str(), "");
    }

    mColInterface.reset(new AnalyzerSettingInterfaceNumberList());
    mColInterface->SetTitleAndTooltip("Num Cols",  "Specify number of columns.");
    auto sortedCols = kFrameShapeColumns;
    std::sort(sortedCols.begin(), sortedCols.end());
    for (const auto it : sortedCols) {
        if (it == 0) {
            continue;
        }

        mColInterface->AddNumber(it, std::to_string(it).c_str(), "");
    }

    mSuppressDuplicatePingsInterface.reset(new AnalyzerSettingInterfaceBool());
    mSuppressDuplicatePingsInterface->SetCheckBoxText("Suppress duplicate pings in table");
    mSuppressDuplicatePingsInterface->SetValue(mSuppressDuplicatePings);

    AddInterface(mInputChannelInterfaceClock.get());
    AddInterface(mInputChannelInterfaceData.get());
    AddInterface(mRowInterface.get());
    AddInterface(mColInterface.get());
    AddInterface(mSuppressDuplicatePingsInterface.get());

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

    return SetReturnString(text_archive.GetString());
}
