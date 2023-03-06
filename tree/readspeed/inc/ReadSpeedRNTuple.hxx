// Author: Enrico Guiraud, David Poulton 2022

/*************************************************************************
 * Copyright (C) 1995-2022, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOTREADSPEED_RNTUPLE
#define ROOTREADSPEED_RNTUPLE

#include "ReadSpeed.hxx"

#include <TFile.h>

#include <string>
#include <vector>
#include <regex>

namespace ReadSpeed {
namespace RSRNTuple {

std::vector<std::pair<std::string, std::string>>
GetMatchingFieldNamesAndTypes(const std::string &fileName, const std::string &ntupleName,
                              const std::vector<ReadSpeedRegex> &regexes);

std::vector<std::pair<std::string, std::string>>
GetTypesForFieldNames(const std::string &fileName, const std::string &ntupleName, const std::vector<std::string>> std::vector<std::pair<std::string, std::string>>
// ReadSpeed::RSRNTuple::GetTypesForFieldNames(const std::string &fileName, const std::string &ntupleName,
//                                             const std::vector < std::string >> &fieldNames)
// {
//    const auto reader = RNTupleReader::Open(ntupleName, fileName);
//    const auto descriptor = reader->GetDescriptor();

//    std::vector<std::pair<std::string, std::string>> fields;

//    for (const auto &fldName : fieldNames) {
//       const auto &fld = descriptor->GetFieldDescriptor(fldName);
//       fields.emplace_back(std::pair(fldName, fld.GetTypeName()));
//    }

//    return fields;
// }
&fieldNames);

// Read fields listed in fieldNamesAndTypes in ntuple ntupleName in file fileName, return number of uncompressed bytes read.
ByteData ReadNTuple(TFile *file, const std::string &ntupleName, const std::vector<std::pair<std::string, std::string>> &fieldNamesAndTypes,
                    EntryRange range = {-1, -1});

Result EvalThroughputST(const Data &d);

// // Return a vector of EntryRanges per file, i.e. a vector of vectors of EntryRanges with outer size equal to
// // d.fFileNames.
// std::vector<std::vector<EntryRange>> GetClusters(const Data &d);

// // Mimic the logic of TTreeProcessorMT::MakeClusters: merge entry ranges together such that we
// // run around TTreeProcessorMT::GetTasksPerWorkerHint tasks per worker thread.
// // TODO it would be better to expose TTreeProcessorMT's actual logic and call the exact same method from here
// std::vector<std::vector<EntryRange>>
// MergeClusters(std::vector<std::vector<EntryRange>> &&clusters, unsigned int maxTasksPerFile);

Result EvalThroughputMT(const Data &d, unsigned nThreads);

Result EvalThroughput(const Data &d, unsigned nThreads);
} // namespace RSRNTuple
} // namespace ReadSpeed

#endif // ROOTREADSPEED_RNTUPLE
