// Author: Enrico Guiraud, David Poulton 2022

/*************************************************************************
 * Copyright (C) 1995-2022, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "ReadSpeedRNTuple.hxx"

#include <ROOT/TSeq.hxx>

#ifdef R__USE_IMT
#include <ROOT/TThreadExecutor.hxx>
#include <ROOT/TTreeProcessorMT.hxx> // for TTreeProcessorMT::GetTasksPerWorkerHint
#include <ROOT/RSlotStack.hxx>
#endif

#include <ROOT/InternalTreeUtils.hxx> // for ROOT::Internal::TreeUtils::GetTopLevelBranchNames
#include <TBranch.h>
#include <TStopwatch.h>
#include <TTree.h>

#include <ROOT/RNTuple.hxx>

#include <algorithm>
#include <cassert>
#include <cmath> // std::ceil
#include <memory>
#include <numeric> // std::accumulate
#include <stdexcept>
#include <set>
#include <iostream>

using namespace ReadSpeed;
using namespace ReadSpeed::RSRNTuple;

using ROOT::Experimental::RNTuple;
using ROOT::Experimental::RNTupleModel;
using ROOT::Experimental::RNTupleReader;

// std::vector<std::pair<std::string, std::string>>
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

std::vector<std::pair<std::string, std::string>>
ReadSpeed::RSRNTuple::GetMatchingFieldNamesAndTypes(const std::string &fileName, const std::string &ntupleName,
                                                    const std::vector<ReadSpeedRegex> &regexes)
{
   const auto reader = RNTupleReader::Open(ntupleName, fileName);
   const auto descriptor = reader->GetDescriptor();

   std::vector<std::pair<std::string, std::string>> unfilteredFields;

   for (const auto &fld : descriptor->GetTopLevelFields()) {
      unfilteredFields.emplace_back(std::pair(fld.GetFieldName(), fld.GetTypeName()));
   }

   std::set<ReadSpeedRegex> usedRegexes;
   std::vector<std::pair<std::string, std::string>> fields;

   auto filterFieldName = [regexes, &usedRegexes](const std::pair<std::string, std::string> &fPair) {
      if (regexes.size() == 1 && regexes[0].text == ".*") {
         usedRegexes.insert(regexes[0]);
         return true;
      }

      const auto matchField = [&usedRegexes, fPair](const ReadSpeedRegex &regex) {
         bool match = std::regex_match(fPair.first, regex.regex);

         if (match)
            usedRegexes.insert(regex);

         return match;
      };

      const auto iterator = std::find_if(regexes.begin(), regexes.end(), matchField);
      return iterator != regexes.end();
   };
   std::copy_if(unfilteredFields.begin(), unfilteredFields.end(), std::back_inserter(fields), filterFieldName);

   if (fields.empty()) {
      std::cerr << "Provided field regexes didn't match any branches in tree '" + ntupleName + "' from file '" +
                      fileName + ".\n";
      std::terminate();
   }
   if (usedRegexes.size() != regexes.size()) {
      std::string errString = "The following regexes didn't match any fields in ntuple '" + ntupleName +
                              "' from file '" + fileName + "', this is probably unintended :\n ";
      for (const auto &regex : regexes) {
         if (usedRegexes.find(regex) == usedRegexes.end())
            errString += '\t' + regex.text + '\n';
      }
      std::cerr << errString;
      std::terminate();
   }

   return fields;
}

std::vector<std::vector<std::pair<std::string, std::string>>> GetPerFileFieldNamesAndTypes(const Data &d)
{
   auto ntupleIdx = 0;
   std::vector<std::vector<std::pair<std::string, std::string>>> fileFieldNamesAndTypes;

   std::vector<ReadSpeedRegex> regexes;
   if (d.fUseRegex)
      std::transform(d.fBranchOrFieldNames.begin(), d.fBranchOrFieldNames.end(), std::back_inserter(regexes),
                     [](std::string text) {
                        return ReadSpeedRegex{text, std::regex(text)};
                     });

   for (const auto &fName : d.fFileNames) {
      std::vector<std::pair<std::string, std::string>> fieldNames;
      if (d.fUseRegex) {
         fieldNames = GetMatchingFieldNamesAndTypes(fName, d.fTreeOrNTupleNames[ntupleIdx], regexes);
      } else
         fieldNames = GetTypesForFieldNames(fName, d.fTreeOrNTupleNames[ntupleIdx], d.fBranchOrFieldNames);

      fileFieldNamesAndTypes.push_back(fieldNames);

      if (d.fTreeOrNTupleNames.size() > 1)
         ++ntupleIdx;
   }

   return fileFieldNamesAndTypes;
}

// Read fields listed in fieldNames in ntuple ntupleName in file fileName, return number of uncompressed bytes read.
// NOTE in stopwatch
ByteData ReadSpeed::RSRNTuple::ReadNTuple(TFile *f, const std::string &ntupleName,
                                          const std::vector<std::pair<std::string, std::string>> &fieldNamesAndTypes,
                                          EntryRange range)
{
   auto model = RNTupleModel::Create();

   for (const auto &fieldName : fieldNamesAndTypes) {
      // TODO: Need to find out the type of the field (can be as string, see RNTupleImporter for reference).
      // Preferably outside of the stopwatch!!
      // model->MakeField
   }

   // std::unique_ptr<TTree> t(f->Get<TTree>(treeName.c_str()));
   // if (t == nullptr)
   //    throw std::runtime_error("Could not retrieve tree '" + treeName + "' from file '" + f->GetName() + '\'');

   // t->SetBranchStatus("*", 0);

   // std::vector<TBranch *> branches;
   // for (const auto &bName : branchNames) {
   //    auto *b = t->GetBranch(bName.c_str());
   //    if (b == nullptr)
   //       throw std::runtime_error("Could not retrieve branch '" + bName + "' from tree '" + t->GetName() +
   //                                "' in file '" + t->GetCurrentFile()->GetName() + '\'');

   //    b->SetStatus(1);
   //    branches.push_back(b);
   // }

   // const auto nEntries = t->GetEntries();
   // if (range.fStart == -1ll)
   //    range = EntryRange{0ll, nEntries};
   // else if (range.fEnd > nEntries)
   //    throw std::runtime_error("Range end (" + std::to_string(range.fEnd) + ") is beyond the end of tree '" +
   //                             t->GetName() + "' in file '" + t->GetCurrentFile()->GetName() + "' with " +
   //                             std::to_string(nEntries) + " entries.");

   // ULong64_t bytesRead = 0;
   // const ULong64_t fileStartBytes = f->GetBytesRead();
   // for (auto e = range.fStart; e < range.fEnd; ++e)
   //    for (auto *b : branches)
   //       bytesRead += b->GetEntry(e);

   // const ULong64_t fileBytesRead = f->GetBytesRead() - fileStartBytes;
   // return {bytesRead, fileBytesRead};
   return {};
}

Result ReadSpeed::RSRNTuple::EvalThroughputST(const Data &d)
{
   auto ntupleIdx = 0;
   auto fileIdx = 0;
   ULong64_t uncompressedBytesRead = 0;
   ULong64_t compressedBytesRead = 0;

   TStopwatch sw;
   const auto fileFieldNamesAndTypes = GetPerFileFieldNamesAndTypes(d);

   for (const auto &fileName : d.fFileNames) {
      auto f = std::unique_ptr<TFile>(TFile::Open(fileName.c_str(), "READ_WITHOUT_GLOBALREGISTRATION"));
      if (f == nullptr || f->IsZombie())
         throw std::runtime_error("Could not open file '" + fileName + '\'');

      sw.Start(kFALSE);

      const auto byteData = ReadNTuple(f.get(), d.fTreeOrNTupleNames[ntupleIdx], fileFieldNamesAndTypes[fileIdx]);
      uncompressedBytesRead += byteData.fUncompressedBytesRead;
      compressedBytesRead += byteData.fCompressedBytesRead;

      if (d.fTreeOrNTupleNames.size() > 1)
         ++ntupleIdx;
      ++fileIdx;

      sw.Stop();
   }

   return {sw.RealTime(), sw.CpuTime(), 0., 0., uncompressedBytesRead, compressedBytesRead, 0};
}

// Return a vector of EntryRanges per file, i.e. a vector of vectors of EntryRanges with outer size equal to
// d.fFileNames.
// std::vector<std::vector<EntryRange>> ReadSpeed::RSRNTuple::GetClusters(const Data &d)
// {
//    const auto nFiles = d.fFileNames.size();
//    std::vector<std::vector<EntryRange>> ranges(nFiles);
//    for (auto fileIdx = 0u; fileIdx < nFiles; ++fileIdx) {
//       const auto &fileName = d.fFileNames[fileIdx];
//       std::unique_ptr<TFile> f(TFile::Open(fileName.c_str(), "READ_WITHOUT_GLOBALREGISTRATION"));
//       if (f == nullptr || f->IsZombie())
//          throw std::runtime_error("There was a problem opening file '" + fileName + '\'');
//       const auto &treeName = d.fTreeOrNTupleNames.size() > 1 ? d.fTreeOrNTupleNames[fileIdx] :
//       d.fTreeOrNTupleNames[0]; auto *t = f->Get<TTree>(treeName.c_str()); // TFile owns this TTree if (t == nullptr)
//          throw std::runtime_error("There was a problem retrieving TTree '" + treeName + "' from file '" + fileName +
//                                   '\'');

//       const auto nEntries = t->GetEntries();
//       auto it = t->GetClusterIterator(0);
//       Long64_t start = 0;
//       std::vector<EntryRange> rangesInFile;
//       while ((start = it.Next()) < nEntries)
//          rangesInFile.emplace_back(EntryRange{start, it.GetNextEntry()});
//       ranges[fileIdx] = std::move(rangesInFile);
//    }
//    return ranges;
// }

// Mimic the logic of TTreeProcessorMT::MakeClusters: merge entry ranges together such that we
// run around TTreeProcessorMT::GetTasksPerWorkerHint tasks per worker thread.
// TODO it would be better to expose TTreeProcessorMT's actual logic and call the exact same method from here
// std::vector<std::vector<EntryRange>>
// ReadSpeed::RSRNTuple::MergeClusters(std::vector<std::vector<EntryRange>> &&clusters, unsigned int maxTasksPerFile)
// {
//    std::vector<std::vector<EntryRange>> mergedClusters(clusters.size());

//    auto clustersIt = clusters.begin();
//    auto mergedClustersIt = mergedClusters.begin();
//    for (; clustersIt != clusters.end(); clustersIt++, mergedClustersIt++) {
//       const auto nClustersInThisFile = clustersIt->size();
//       const auto nFolds = nClustersInThisFile / maxTasksPerFile;
//       // If the number of clusters is less than maxTasksPerFile
//       // we take the clusters as they are
//       if (nFolds == 0) {
//          *mergedClustersIt = *clustersIt;
//          continue;
//       }
//       // Otherwise, we have to merge clusters, distributing the reminder evenly
//       // between the first clusters
//       auto nReminderClusters = nClustersInThisFile % maxTasksPerFile;
//       const auto &clustersInThisFile = *clustersIt;
//       for (auto i = 0ULL; i < nClustersInThisFile; ++i) {
//          const auto start = clustersInThisFile[i].fStart;
//          // We lump together at least nFolds clusters, therefore
//          // we need to jump ahead of nFolds-1.
//          i += (nFolds - 1);
//          // We now add a cluster if we have some reminder left
//          if (nReminderClusters > 0) {
//             i += 1U;
//             nReminderClusters--;
//          }
//          const auto end = clustersInThisFile[i].fEnd;
//          mergedClustersIt->emplace_back(EntryRange({start, end}));
//       }
//       assert(nReminderClusters == 0 && "This should never happen, cluster-merging logic is broken.");
//    }

//    return mergedClusters;
// }

Result ReadSpeed::RSRNTuple::EvalThroughputMT(const Data &d, unsigned nThreads)
{
   // #ifdef R__USE_IMT
   //    ROOT::TThreadExecutor pool(nThreads);
   //    const auto actualThreads = ROOT::GetThreadPoolSize();
   //    if (actualThreads != nThreads)
   //       std::cerr << "Running with " << actualThreads << " threads even though " << nThreads << " were
   //       requested.\n";

   //    TStopwatch clsw;
   //    clsw.Start();
   //    const unsigned int maxTasksPerFile =
   //       std::ceil(float(ROOT::TTreeProcessorMT::GetTasksPerWorkerHint() * actualThreads) /
   //       float(d.fFileNames.size()));

   //    const auto rangesPerFile = MergeClusters(GetClusters(d), maxTasksPerFile);
   //    clsw.Stop();

   //    const size_t nranges =
   //       std::accumulate(rangesPerFile.begin(), rangesPerFile.end(), 0u, [](size_t s, auto &r) { return s + r.size();
   //       });
   //    std::cout << "Total number of tasks: " << nranges << '\n';

   //    const auto fileBranchOrFieldNames = GetPerFileFieldNames(d);

   //    ROOT::Internal::RSlotStack slotStack(actualThreads);
   //    std::vector<int> lastFileIdxs(actualThreads, -1);
   //    std::vector<std::unique_ptr<TFile>> lastTFiles(actualThreads);

   //    auto processFile = [&](int fileIdx) {
   //       const auto &fileName = d.fFileNames[fileIdx];
   //       const auto &treeName = d.fTreeOrNTupleNames.size() > 1 ? d.fTreeOrNTupleNames[fileIdx] :
   //       d.fTreeOrNTupleNames[0]; const auto &branchNames = fileBranchOrFieldNames[fileIdx];

   //       auto readRange = [&](const EntryRange &range) -> ByteData {
   //          ROOT::Internal::RSlotStackRAII slotRAII(slotStack);
   //          auto slotIndex = slotRAII.fSlot;
   //          auto &file = lastTFiles[slotIndex];
   //          auto &lastIndex = lastFileIdxs[slotIndex];

   //          if (lastIndex != fileIdx) {
   //             file.reset(TFile::Open(fileName.c_str(), "READ_WITHOUT_GLOBALREGISTRATION"));
   //             lastIndex = fileIdx;
   //          }

   //          if (file == nullptr || file->IsZombie())
   //             throw std::runtime_error("Could not open file '" + fileName + '\'');

   //          auto result = ReadTree(file.get(), treeName, branchNames, range);

   //          return result;
   //       };

   //       const auto byteData = pool.MapReduce(readRange, rangesPerFile[fileIdx], SumBytes);

   //       return byteData;
   //    };

   //    TStopwatch sw;
   //    sw.Start();
   //    const auto totalByteData = pool.MapReduce(processFile, ROOT::TSeqUL(d.fFileNames.size()), SumBytes);
   //    sw.Stop();

   //    return {sw.RealTime(),
   //            sw.CpuTime(),
   //            clsw.RealTime(),
   //            clsw.CpuTime(),
   //            totalByteData.fUncompressedBytesRead,
   //            totalByteData.fCompressedBytesRead,
   //            actualThreads};
   // #else
   //    (void)d;
   //    (void)nThreads;
   return {};
   // #endif // R__USE_IMT
}

Result ReadSpeed::RSRNTuple::EvalThroughput(const Data &d, unsigned nThreads)
{
   if (d.fTreeOrNTupleNames.empty()) {
      std::cerr << "Please provide at least one ntuple name\n";
      std::terminate();
   }
   if (d.fFileNames.empty()) {
      std::cerr << "Please provide at least one file name\n";
      std::terminate();
   }
   if (d.fBranchOrFieldNames.empty()) {
      std::cerr << "Please provide at least one field name\n";
      std::terminate();
   }
   if (d.fTreeOrNTupleNames.size() != 1 && d.fTreeOrNTupleNames.size() != d.fFileNames.size()) {
      std::cerr << "Please provide either one ntuple name or as many as the file names\n";
      std::terminate();
   }

#ifdef R__USE_IMT
   return nThreads > 0 ? EvalThroughputMT(d, nThreads) : EvalThroughputST(d);
#else
   if (nThreads > 0) {
      std::cerr << nThreads
                << " threads were requested, but ROOT was built without implicit multi-threading (IMT) support.\n";
      std::terminate();
   }
   return EvalThroughputST(d);
#endif
}
