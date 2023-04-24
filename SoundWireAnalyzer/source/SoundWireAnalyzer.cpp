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

#include <string>
#include <sstream>
#include <AnalyzerChannelData.h>

#include "CBitstreamDecoder.h"
#include "CDynamicSyncGenerator.h"
#include "CFrameReader.h"
#include "CSyncFinder.h"
#include "SoundWireAnalyzer.h"
#include "SoundWireAnalyzerSettings.h"
#include "SoundWireAnalyzerResults.h"

SoundWireAnalyzer::SoundWireAnalyzer()
  :     Analyzer2(),
        mSettings(new SoundWireAnalyzerSettings()),
        mAddBubbleFrames(false),
        mAnnotateBitValues(false)

{
    SetAnalyzerSettings(mSettings.get());
    UseFrameV2();
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

void SoundWireAnalyzer::addFrameShapeMessage(U64 sampleNumber, int rows, int columns)
{
    // The Saleae API doesn't provide a way to declare a column header, it
    // appears to have its own method of picking a column order.
    // The frame shape will always be the first entry in the table so log
    // something for every column to try to define the columns in a fixed order.
    FrameV2 f;
    std::stringstream shapeType;

    shapeType << "shape " << rows << " x " << columns;

    // We are mainly interested in read and write so put those columns first
    f.AddString("DevId", "");
    f.AddString("Reg",  "");
    f.AddString("Data",  "");

    // SSP is infrequent but important
    f.AddString("SSP", "");

    // ACK, NAK and Preq are short and apply to all frames so put those next
    f.AddString("ACK", "");
    f.AddString("NAK", "");
    f.AddString("Preq", "");

    f.AddString("Par", "");
    f.AddString("Dsync", "");

    // Slave status is only useful in PING messages so put those last
    for (int i = 0; i < 12; ++i) {
        char slvStatTitle[4];
        snprintf(slvStatTitle, sizeof(slvStatTitle), "P%d", i);
        f.AddString(slvStatTitle, "");
    }

    mResults->AddFrameV2(f, shapeType.str().c_str(), sampleNumber, sampleNumber);

    if (mAddBubbleFrames) {
        Frame f1;
        f1.mStartingSampleInclusive = sampleNumber;
        f1.mEndingSampleInclusive = sampleNumber + 1; // end is not allowed to be same as start
        f1.mType = SoundWireAnalyzerResults::EBubbleFrameShape;
        f1.mData1 = rows;
        f1.mData2 = columns;
        mResults->AddFrame(f1);
    }
}

void SoundWireAnalyzer::addFrameV2(const CControlWordBuilder& controlWord, const Frame& fv1)
{
    FrameV2 f;
    const char* type = "??";
    U8 addrArray[2];
    unsigned int addr;
    unsigned int pingStat;
    char statTitle[4];

    SdwOpCode opCode = controlWord.OpCode();
    switch (opCode) {
    case kOpPing:
        type = "PING";

        f.AddBoolean("SSP", controlWord.Ssp());

        // There are 12 status reports of 2 bits each
        pingStat = controlWord.PeripheralStat();
        for (int i = 0; i < 12; ++i) {
            snprintf(statTitle, sizeof(statTitle), "P%d", i);
            switch (pingStat & 3) {
            case kStatNotPresent:
                f.AddString(statTitle, "");
                break;
            case kStatOk:
                f.AddString(statTitle, "Ok");
                break;
            case kStatAlert:
                f.AddString(statTitle, "AL");
                break;
            default:
                f.AddString(statTitle, "??");
                break;
            }
            pingStat >>= 2;
        }
        break;
    case kOpRead:
    case kOpWrite:
        type = (opCode == kOpRead) ? "READ" : "WRITE";
        f.AddByte("DevId", controlWord.DeviceAddress());

        // Saleae say AddByteArray() is preferred to AddInteger()
        addr = controlWord.RegisterAddress();
        addrArray[0] = addr >> 8;
        addrArray[1] = addr & 0xFF;
        f.AddByteArray("Reg",  addrArray, sizeof(addrArray));
        f.AddByte("Data",  controlWord.DataValue());
        break;
    default:
        break;
    }

    if (fv1.mFlags & SoundWireAnalyzerResults::kFlagSyncLoss) {
        // Replace table entry type with sync loss marker
        type = "SYNC LOST";
    }

    f.AddBoolean("ACK", controlWord.Ack());
    f.AddBoolean("NAK", controlWord.Nak());
    f.AddBoolean("Preq", controlWord.Preq());

    if (fv1.mFlags & SoundWireAnalyzerResults::kFlagParityBad) {
        f.AddString("Par", "BAD");
    } else {
        f.AddString("Par", "Ok");
    }

    U8 dsyncByte = static_cast<U8>(controlWord.DynamicSync());
    f.AddByteArray("Dsync", &dsyncByte, 1);

    // The UI has a default column of "value" so use this for the raw word value
    U8 wordArray[(kCtrlWordLastRow + 1) / 8];
    U64 wordValue = controlWord.Value();
    for (int i = sizeof(wordArray) - 1; i >= 0; --i) {
        wordArray[i] = wordValue & 0xFF;
        wordValue >>= 8;
    }
    f.AddByteArray("value", wordArray, sizeof(wordArray));

    U64 startSample = fv1.mStartingSampleInclusive;
    if (startSample == 0) {
        // Don't overlap the dummy column header frame
        startSample = 1;
    }
    mResults->AddFrameV2(f, type, startSample, fv1.mEndingSampleInclusive);
}

void SoundWireAnalyzer::NotifyBusReset(U64 startSampleNumber, U64 endSampleNumber)
{
    if (mAddBubbleFrames) {
        Frame f1;
        f1.mStartingSampleInclusive = startSampleNumber;
        f1.mEndingSampleInclusive = endSampleNumber;
        f1.mType = SoundWireAnalyzerResults::EBubbleBusReset;
        mResults->AddFrame(f1);
    }

    FrameV2 f2;
    mResults->AddFrameV2(f2, "BUS RESET", startSampleNumber, endSampleNumber);
}

void SoundWireAnalyzer::WorkerThread()
{
    mInputChannelClock = mSettings->mInputChannelClock;
    mInputChannelData = mSettings->mInputChannelData;
    mSoundWireClock = GetAnalyzerChannelData(mInputChannelClock);
    mSoundWireData = GetAnalyzerChannelData(mInputChannelData);
    const bool suppressDuplicatePings = mSettings->mSuppressDuplicatePings;
    const bool annotateFrameStarts = mSettings->mAnnotateFrameStarts;
    mAddBubbleFrames = mSettings->mAnnotateTrace;
    mAnnotateBitValues = mSettings->mAnnotateBitValues;

    mDecoder.reset(new CBitstreamDecoder(*this, mSoundWireClock, mSoundWireData));

    // Advance one bit to get an initial data line state
    mDecoder->NextBitValue();

    CBitstreamDecoder::CMark startMark = mDecoder->Mark();
    CSyncFinder syncFinder(*this, *mDecoder);
    CFrameReader frameReader;
    CControlWordBuilder lastPing;
    CDynamicSyncGenerator dynamicSync;
    bool inSync = false;
    bool isFirstFrame = true;
    bool actualParityIsOdd;
    Frame f;

    // The sync finder will need to rewind so CBitStreamDecoder must be
    // collecting history
    mDecoder->CollectHistory(true);

    for (;;) {
        if (!inSync) {
            mDecoder->SetToMark(startMark);

            // Try to find sync at default frame shape
            syncFinder.FindSync(mSettings->mNumRows, mSettings->mNumCols);
            inSync = true;
            isFirstFrame = true;
            frameReader.Reset();
            frameReader.SetShape(syncFinder.Rows(), syncFinder.Columns());
            addFrameShapeMessage(mDecoder->CurrentSampleNumber(),
                                 syncFinder.Rows(), syncFinder.Columns());

            // Now we have a good frame we don't need any history before this point
            mDecoder->DiscardHistoryBeforeCurrentPosition();
        }

        bool bitValue = mDecoder->NextBitValue();
        U64 sampleNumber = mDecoder->CurrentSampleNumber();

        switch (frameReader.PushBit(bitValue)) {
        case CFrameReader::eFrameStart:
            f.mStartingSampleInclusive = sampleNumber;

            // Mark start of frame with a green dot on the clock
            // TODO: If we lost sync we will revisit some bits but must not
            // add the marker again
            if (annotateFrameStarts) {
                mResults->AddMarker(sampleNumber,
                                    AnalyzerResults::Start,
                                    mSettings->mInputChannelClock);
            }
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
            f.mType = SoundWireAnalyzerResults::EBubbleNormal;
            f.mFlags = 0;

            // Seed dynamic sequence from value in first frame
            if (isFirstFrame) {
                dynamicSync.SetValue(frameReader.ControlWord().DynamicSync());
            } else {
                // We can't calculate parity for the first frame because parity
                // includes the end of the previous frame.
                if (actualParityIsOdd != frameReader.ControlWord().Par()) {
                    if (!isFirstFrame) {
                        f.mFlags |= SoundWireAnalyzerResults::kFlagParityBad;
                    }
                }

                // Check whether we've lost sync. Don't consider parity in this
                // because that would make it more difficult to analyze bus
                // corruption.
                if ((frameReader.ControlWord().StaticSync() != kStaticSyncVal) ||
                    (frameReader.ControlWord().DynamicSync() != dynamicSync.Next())) {
                    inSync = false;
                    f.mFlags |= SoundWireAnalyzerResults::kFlagSyncLoss;
                    if (mAddBubbleFrames) {
                        mResults->AddFrame(f);
                    }
                    addFrameV2(frameReader.ControlWord(), f);
                    break;
                }
            }

            if (mAddBubbleFrames) {
                mResults->AddFrame(f);
            }

            if (suppressDuplicatePings &&
                (frameReader.ControlWord().OpCode() == kOpPing)) {
                    if (isFirstFrame ||
                        !frameReader.ControlWord().IsPingSameAs(lastPing)) {
                        addFrameV2(frameReader.ControlWord(), f);
                    }
                lastPing.SetValue(frameReader.ControlWord().Value());
            } else {
                addFrameV2(frameReader.ControlWord(), f);
            }

            // Has frame shape changed?
            if (frameReader.ControlWord().IsFrameShapeChange()) {
                int rows, cols;
                frameReader.ControlWord().GetNewShape(rows, cols);
                frameReader.SetShape(rows, cols);
                addFrameShapeMessage(sampleNumber, rows, cols);
            }

            frameReader.Reset();
            isFirstFrame = false;

            // Now we've decoded this frame the history bits can be discarded
            // save memory. History collection must remain enabled in case we
            // lose sync on the next frame and have to rewind it.
            mDecoder->DiscardHistoryBeforeCurrentPosition();

            startMark = mDecoder->Mark();
            ReportProgress(sampleNumber);
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
