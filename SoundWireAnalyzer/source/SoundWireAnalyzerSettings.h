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

#ifndef SOUNDWIRE_ANALYZER_SETTINGS_H
#define SOUNDWIRE_ANALYZER_SETTINGS_H

#include <memory>
#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class SoundWireAnalyzerSettings : public AnalyzerSettings
{
public:
    enum {
        eExportCsv,
        eExportText
    };

public:
    SoundWireAnalyzerSettings();
    virtual ~SoundWireAnalyzerSettings();

    bool SetSettingsFromInterfaces();
    void UpdateInterfacesFromSettings();
    void LoadSettings(const char* settings);
    const char* SaveSettings();

    Channel mInputChannelClock;
    Channel mInputChannelData;

    unsigned int mNumRows;
    unsigned int mNumCols;
    bool mSuppressDuplicatePings;

protected:
    std::unique_ptr<AnalyzerSettingInterfaceChannel> mInputChannelInterfaceClock;
    std::unique_ptr<AnalyzerSettingInterfaceChannel> mInputChannelInterfaceData;
    std::unique_ptr<AnalyzerSettingInterfaceNumberList> mRowInterface;
    std::unique_ptr<AnalyzerSettingInterfaceNumberList> mColInterface;
    std::unique_ptr<AnalyzerSettingInterfaceBool> mSuppressDuplicatePingsInterface;
};

#endif //SOUNDWIRE_ANALYZER_SETTINGS_H
