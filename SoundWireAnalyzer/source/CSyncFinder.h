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

#ifndef CSYNCFINDER_H
#define CSYNCFINDER_H

#include <vector>
#include "LogicPublicTypes.h"
#include "CBitstreamDecoder.h"
#include "SoundWireAnalyzer.h"

class CSyncFinder
{
public:
    CSyncFinder(SoundWireAnalyzer& analyzer, CBitstreamDecoder& bitstream);

    void FindSync(int rows, int columns);

    inline int Rows() const
        { return mRows; }

    inline int Columns() const
        { return mColumns; }

private:
    int checkSync(int rows, int columns);
    bool testIfSyncIsReal(const std::vector<int>* rowsList, int columns,
                          U64 matchedBitOffset, const CBitstreamDecoder::CMark& searchStartMark);

private:
    SoundWireAnalyzer& mAnalyzer;
    CBitstreamDecoder& mBitstream;
    int mRows;
    int mColumns;
    std::vector<int> mSingleRowList;
    std::vector<int> mSingleColumnList;
};

#endif // CSYNCFINDER_H
