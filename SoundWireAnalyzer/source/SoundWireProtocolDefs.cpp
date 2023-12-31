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

#include <LogicPublicTypes.h>
#include "SoundWireProtocolDefs.h"

const std::vector<int> kFrameShapeRows( {
    48, 50, 60, 64, 75, 80, 125, 147, 96, 100, 120, 128, 150, 169, 250, 0,
    192, 200, 240, 256, 72, 144, 90, 180
} );

const std::vector<int> kFrameShapeColumns( {
    2, 4, 6, 8, 10, 12, 14, 16
} );
