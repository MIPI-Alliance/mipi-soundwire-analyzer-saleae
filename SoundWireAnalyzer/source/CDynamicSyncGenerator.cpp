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

#include "CDynamicSyncGenerator.h"

// [0] is never used because the PRNG would get stuck. Each value
// is also the index into this array of the next value in sequence.
static const unsigned int kDynamicSync[] = {
    0, 2, 4, 6, 9, 11, 13, 15, 1, 3, 5, 7, 8, 10, 12, 14
};


CDynamicSyncGenerator::CDynamicSyncGenerator()
    : mValue(1)
{ }

void CDynamicSyncGenerator::SetValue(unsigned int value)
{
    mValue = value;
}

unsigned int CDynamicSyncGenerator::Next()
{
    mValue = kDynamicSync[mValue];
    return mValue;
}
