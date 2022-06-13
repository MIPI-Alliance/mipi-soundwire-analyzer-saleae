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
}

void SoundWireAnalyzerResults::generateDataBubble(U64 frame_index)
{
    Frame frame = GetFrame(frame_index);
    CControlWordBuilder controlWord;
    controlWord.SetValue(frame.mData1);
    unsigned int pingStat;

    std::stringstream str;

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

void SoundWireAnalyzerResults::GenerateExportFile(const char* file,
                                                  DisplayBase display_base,
                                                  U32 export_type_user_id)
{
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
