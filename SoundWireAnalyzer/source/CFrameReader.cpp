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

#include "CBitstreamDecoder.h"
#include "CControlWordBuilder.h"
#include "CFrameReader.h"

CFrameReader::CFrameReader()
    : mState(eFrameStart),
      mRows(0),
      mColumns(0),
      mCurrentRow(0),
      mCurrentColumn(0)

{
}

void CFrameReader::SetShape(int rows, int columns)
{
    Reset();
    mRows = rows;
    mColumns = columns;
}

void CFrameReader::Reset()
{
    mControlWord.Reset();
    mCurrentRow = 0;
    mCurrentColumn = 0;
    mState = eFrameStart;
}

CFrameReader::TState CFrameReader::PushBit(bool isOne)
{
    TState ret = mState;

    switch (mState) {
    case eFrameStart:
        mState = eNeedMoreBits;
        break;
    case eFrameComplete:
        return mState;
    default:
        break;
    }

    if (mCurrentColumn == 0) {
        if (mCurrentRow <= kCtrlWordLastRow) {
            mControlWord.PushBit(isOne);
        }

        // Parity is calculated up to the first bit of the row before the PAR bit
        if (mCurrentRow == kCtrlPARRow - 1) {
            ret = eCaptureParity;
        }
    }

    if (++mCurrentColumn == mColumns) {
        mCurrentColumn = 0;
        if (++mCurrentRow == mRows) {
            mState = eFrameComplete;
            ret = eFrameComplete;
        }
    }

    return ret;
}
