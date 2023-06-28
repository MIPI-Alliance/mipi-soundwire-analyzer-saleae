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

#ifndef CFRAMEREADER_H
#define CFRAMEREADER_H

#include "CBitstreamDecoder.h"
#include "CControlWordBuilder.h"

class CFrameReader
{
public:
    enum TState {
        eFrameStart,
        eNeedMoreBits,
        eCaptureParity,
        eFrameComplete,
    };

public:
    CFrameReader();

    void SetShape(int rows, int columns);
    void Reset();
    TState PushBit(bool isOne);

    inline const CControlWordBuilder& ControlWord() const
        { return mControlWord; }

private:
    CControlWordBuilder mControlWord;
    TState mState;
    int mRows;
    int mColumns;
    int mCurrentRow;
    int mCurrentColumn;
};

#endif // CFRAMEREADER_H
