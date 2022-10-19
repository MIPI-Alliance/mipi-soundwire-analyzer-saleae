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

#include "SoundWireAnalyzerSettings.h"

SoundWireAnalyzerSettings::SoundWireAnalyzerSettings()
  :     mInputChannelClock(UNDEFINED_CHANNEL),
        mInputChannelData(UNDEFINED_CHANNEL),
        mNumRows(48),
        mNumCols(2)
{
    mInputChannelInterfaceClock.reset( new AnalyzerSettingInterfaceChannel() );
    mInputChannelInterfaceClock->SetTitleAndTooltip("SoundWire Clock", "SoundWire Clock");
    mInputChannelInterfaceClock->SetChannel(mInputChannelClock);

    mInputChannelInterfaceData.reset( new AnalyzerSettingInterfaceChannel() );
    mInputChannelInterfaceData->SetTitleAndTooltip("SoundWire Data", "SoundWire Data");
    mInputChannelInterfaceData->SetChannel(mInputChannelData);

    mRowInterface.reset(new AnalyzerSettingInterfaceInteger());
    mRowInterface->SetTitleAndTooltip("Num Rows",  "Specify number of rows.");
    mRowInterface->SetMax(256);
    mRowInterface->SetMin(48);
    mRowInterface->SetInteger(mNumRows);

    mColInterface.reset(new AnalyzerSettingInterfaceInteger());
    mColInterface->SetTitleAndTooltip("Num Cols",  "Specify number of columns.");
    mColInterface->SetMax(16);
    mColInterface->SetMin(2);
    mColInterface->SetInteger(mNumCols);

    AddInterface(mInputChannelInterfaceClock.get());
    AddInterface(mInputChannelInterfaceData.get());
    AddInterface(mRowInterface.get());
    AddInterface(mColInterface.get());

    ClearChannels();
    AddChannel(mInputChannelClock, "SoundWire Clock", false);
    AddChannel(mInputChannelData,  "SoundWire Data", false);
}

SoundWireAnalyzerSettings::~SoundWireAnalyzerSettings()
{
}

bool SoundWireAnalyzerSettings::SetSettingsFromInterfaces()
{
    mInputChannelClock = mInputChannelInterfaceClock->GetChannel();
    mInputChannelData  = mInputChannelInterfaceData->GetChannel();
    mNumRows = mRowInterface->GetInteger();
    mNumCols = mColInterface->GetInteger();

    ClearChannels();
    AddChannel(mInputChannelClock, "SoundWire Clock", true);
    AddChannel(mInputChannelData,  "SoundWire Data",  true);

    return true;
}

void SoundWireAnalyzerSettings::UpdateInterfacesFromSettings()
{
    mInputChannelInterfaceClock->SetChannel(mInputChannelClock);
    mInputChannelInterfaceData->SetChannel(mInputChannelData);

    mRowInterface->SetInteger(mNumRows);
    mColInterface->SetInteger(mNumCols);
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

    return SetReturnString(text_archive.GetString());
}
