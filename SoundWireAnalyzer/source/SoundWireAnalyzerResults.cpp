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

#include <array>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <cstring>
#include <sstream>
#include <AnalyzerHelpers.h>

#include "SoundWireAnalyzer.h"
#include "SoundWireAnalyzerSettings.h"
#include "SoundWireAnalyzerResults.h"

SoundWireAnalyzerResults::SoundWireAnalyzerResults(SoundWireAnalyzer* analyzer,
                                                   SoundWireAnalyzerSettings* settings)
    : AnalyzerResults(),
      mSettings(settings),
      mAnalyzer(analyzer)
{
}

SoundWireAnalyzerResults::~SoundWireAnalyzerResults()
{
}

void SoundWireAnalyzerResults::generateClockBubble(U64 frame_index)
{
    Frame frame = GetFrame(frame_index);
    CControlWordBuilder controlWord;
    controlWord.SetValue(frame.mData1);

    std::stringstream str;

    switch (frame.mType) {
    case EBubbleNormal:
        // Put SSP at the start of the clock bubble so it's easy to see
        if ((controlWord.OpCode() == kOpPing) && (controlWord.Ssp())) {
            str << "SSP ";
        }

        if (frame.mFlags & kFlagParityBad) {
            str << "Par: BAD ";
        } else {
            str << "Par: ok ";
        }

        // Dump raw hex of control word
        str << std::setw(12) << std::setfill('0') << std::hex << frame.mData1;
        AddResultString(str.str().c_str());
        break;

    case EBubbleBusReset:
        AddResultString("BUS RESET");
        break;
    }
}

void SoundWireAnalyzerResults::generateDataBubble(U64 frame_index)
{
    Frame frame = GetFrame(frame_index);
    CControlWordBuilder controlWord;
    controlWord.SetValue(frame.mData1);
    unsigned int pingStat;

    if (frame.mType != EBubbleNormal)
        return;

    std::stringstream str;

    // If sync was lost just skip
    if (frame.mFlags & SoundWireAnalyzerResults::kFlagSyncLoss) {
        return;
    }

    SdwOpCode opCode = controlWord.OpCode();
    switch (opCode) {
    case kOpPing:
        str << "PING ";

        // There are 12 status reports of 2 bits each
        pingStat = controlWord.PeripheralStat();
        str << std::hex;
        for (int i = 0; i < 12; ++i) {
            str << i << ":";
            switch (pingStat & 3) {
            case kStatNotPresent:
                str << "- ";
                break;
            case kStatOk:
                str << "Ok ";
                break;
            case kStatAlert:
                str << "Al ";
                break;
            default:
                str << "?? ";
                break;
            }
            pingStat >>= 2;
        }
        break;
    case kOpRead:
    case kOpWrite:
        if (opCode == kOpRead) {
            str << "RD ";
        } else {
            str << "WR ";
        }
        str << "[" << controlWord.DeviceAddress() << "] @";
        str << std::hex << controlWord.RegisterAddress() << "=" << controlWord.DataValue() << " ";
        break;
    default:
        str << "OP?? ";
        break;
    }

    if (controlWord.Nak()) {
        str << "FAIL";
    } else if (controlWord.Ack()) {
        str << "OK";
    } else if (opCode != kOpPing) {
        // PING always reports Command_IGNORED state on success
        str << "IGNORED";
    }

    if (controlWord.Preq()) {
        if (str.str().back() != ' ') {
            str << ' ';
        }
        str << "PREQ";
    }

    AddResultString(str.str().c_str());
}

void SoundWireAnalyzerResults::GenerateBubbleText(U64 frame_index,
                                                  Channel& channel,
                                                  DisplayBase display_base)
{
    ClearResultStrings();

    if (channel == mSettings->mInputChannelClock) {
        generateClockBubble(frame_index);
    } else {
        generateDataBubble(frame_index);
    }
}

static const int kNumExportColumns = 23;
static const char* const kColumnTitles[] = {
    "Time(s)", "Control Word", "Op", "SSP", "DevId", "Reg", "Data",
    "ACK", "NAK", "PREQ", "Dsync", "P0", "P1", "P2", "P3", "P4", "P5", "P6", "P7",
    "P8", "P9", "P10", "P11"
};

static const int kColumnWidths[kNumExportColumns] = {
//  "Time(s)", "Control Word", "Op", "SSP", "DevId", "Reg", "Data",
    17,        14,             5,    3,     5,       6,     4,
//  "ACK", "NAK", "PREQ", "Dsync", "P0", "P1", "P2", "P3", "P4", "P5", "P6", "P7",
    3,     3,     4,      2,       2,    2,    2,    2,    2,    2,    2,    2,
//  "P8", "P9", "P10", "P11"
    2,    2,    2,     2
};

void SoundWireAnalyzerResults::GenerateExportFile(const char* fileName,
                                                  DisplayBase display_base,
                                                  U32 export_type_user_id)
{
    char delimiter;
    bool fixedWidth = false;

    // As of Logic 2.3.55 the export_type_user_id is not passed into
    // this function so we have to figure it out from the file extension.
    std::string fname(fileName);
    fname = fname.substr(fname.find_last_of('.'), std::string::npos);
    if (fname == ".csv") {
        delimiter = ',';
    } else if (fname == ".txt") {
        delimiter = ' ';
        fixedWidth = true;
    } else {
        return;
    }

    std::ofstream stream(fileName, std::ios::out);

    if (fixedWidth) {
        stream << std::left;    // align left
    }

    for (int i = 0; i < kNumExportColumns; ++i) {
        if (fixedWidth) {
            stream << std::setw(kColumnWidths[i]);
        }
        stream << kColumnTitles[i];
        if (i < kNumExportColumns - 1) {
             stream << delimiter;
        }
    }

    stream << std::endl;

    U64 triggerSample = mAnalyzer->GetTriggerSample();
    U32 sampleRate = mAnalyzer->GetSampleRate();
    U64 numFrames = GetNumFrames();
    for(U64 i = 0; i < numFrames; ++i) {
        const Frame frame = GetFrame(i);
        std::vector<std::string> strings;

        char time[18];
        AnalyzerHelpers::GetTimeString(frame.mStartingSampleInclusive, triggerSample, sampleRate, time, sizeof(time));
        strings.push_back(time);

        switch (frame.mType) {
        case EBubbleNormal:
            exportNormalFrame(frame, strings);
            break;
        case EBubbleBusReset:
            strings.push_back(""); // skip control word column
            strings.push_back("BUS RESET");
            break;
        default:
            break;
        }

        stream << std::setfill(' ') << std::left;
        int colNum = 0;
        for (auto it: strings) {
            if (fixedWidth) {
                stream << std::setw(kColumnWidths[colNum++]);
            }

            stream << it << delimiter;
        }
        stream << std::endl;
    }

    stream.close();
}

void SoundWireAnalyzerResults::exportNormalFrame(const Frame& frame, std::vector<std::string>& strings)
{
    CControlWordBuilder controlWord;
    controlWord.SetValue(frame.mData1);
    bool syncLost = frame.mFlags & SoundWireAnalyzerResults::kFlagSyncLoss;

    // Control word value
    std::ostringstream ss;
    ss << "0x" << std::hex << std::setw(12) << std::setfill('0') << controlWord.Value();
    strings.push_back(ss.str());

    if (syncLost) {
        strings.push_back("SYNC LOST");
    }

    // OpCode specific fields
    const SdwOpCode opCode = controlWord.OpCode();
    switch (opCode) {
    case kOpPing:
        if (!syncLost) {
            strings.push_back("PING");
        }
        strings.push_back(std::to_string(controlWord.Ssp()));

        // Skip DevId, Reg and Data
        strings.push_back("");
        strings.push_back("");
        strings.push_back("");

        break;
    case kOpRead:
    case kOpWrite:
        if (!syncLost) {
            if (opCode == kOpRead) {
                strings.push_back("READ");
            } else {
                strings.push_back("WRITE");
            }
        }

        // Skip SSP
        strings.push_back("");

        ss.str("");
        ss << std::dec << controlWord.DeviceAddress();
        strings.push_back(ss.str());

        ss.str("");
        ss << "0x" << std::hex << std::setw(4) << std::setfill('0') << controlWord.RegisterAddress();
        strings.push_back(ss.str());

        ss.str("");
        ss << "0x" << std::setw(2) << controlWord.DataValue();
        strings.push_back(ss.str());
        break;
    }

    // ACK, NAK and PREQ
    strings.push_back(std::to_string(controlWord.Ack()));
    strings.push_back(std::to_string(controlWord.Nak()));
    strings.push_back(std::to_string(controlWord.Preq()));

    // Dsync
    ss.str("");
    ss << "0x" << std::setw(2) << controlWord.DynamicSync();
    strings.push_back(ss.str());

    if (opCode == kOpPing) {
        // Fill in status
        unsigned int pingStat = controlWord.PeripheralStat();
        for (int i = 0; i < 12; ++i) {
            strings.push_back(std::to_string(pingStat & 3));
            pingStat >>= 2;
        }
    }
}

void SoundWireAnalyzerResults::GenerateFrameTabularText(U64 frame_index,
                                                        DisplayBase display_base)
{
}

void SoundWireAnalyzerResults::GeneratePacketTabularText(U64 packet_id,
                                                         DisplayBase display_base)
{
}

void SoundWireAnalyzerResults::GenerateTransactionTabularText(U64 transaction_id,
                                                              DisplayBase display_base)
{
}
