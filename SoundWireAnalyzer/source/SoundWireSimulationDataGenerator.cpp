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

#include "SoundWireSimulationDataGenerator.h"
#include "SoundWireAnalyzerSettings.h"

SoundWireSimulationDataGenerator::SoundWireSimulationDataGenerator()
{
}

SoundWireSimulationDataGenerator::~SoundWireSimulationDataGenerator()
{
}

void SoundWireSimulationDataGenerator::Initialize(U32 simulation_sample_rate,
                                                  SoundWireAnalyzerSettings* settings)
{

}

U32 SoundWireSimulationDataGenerator::GenerateSimulationData(U64 largest_sample_requested,
                                                             U32 sample_rate,
                                                             SimulationChannelDescriptor** simulation_channel)
{
  return 0;
}

