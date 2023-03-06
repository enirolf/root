// Author: Enrico Guiraud, David Poulton 2022

/*************************************************************************
 * Copyright (C) 1995-2022, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOTREADSPEEDTTREE
#define ROOTREADSPEEDTTREE

#include "ReadSpeed.hxx"

#include <TFile.h>

#include <string>
#include <vector>
#include <regex>

namespace ReadSpeed {

namespace RSTTree {


std::vector<std::string> GetMatchingBranchNames(const std::string &fileName, const std::string &treeName,
                                                const std::vector<ReadSpeedRegex> &regexes);

// Read branches listed in branchNames in tree treeName in file fileName, return number of uncompressed bytes read.
ByteData ReadTree(TFile *file, const std::string &treeName, const std::vector<std::string> &branchNames,
                  EntryRange range = {-1, -1});


// Return a vector of EntryRanges per file, i.e. a vector of vectors of EntryRanges with outer size equal to
// d.fFileNames.
std::vector<std::vector<EntryRange>> GetClusters(const Data &d);
// Mimic the logic of TTreeProcessorMT::MakeClusters: merge entry ranges together such that we
// run around TTreeProcessorMT::GetTasksPerWorkerHint tasks per worker thread.
// TODO it would be better to expose TTreeProcessorMT's actual logic and call the exact same method from here
std::vector<std::vector<EntryRange>>
MergeClusters(std::vector<std::vector<EntryRange>> &&clusters, unsigned int maxTasksPerFile);

Result EvalThroughputST(const Data &d);

Result EvalThroughputMT(const Data &d, unsigned nThreads);

Result EvalThroughput(const Data &d, unsigned nThreads);
} // namespace RSTTree
} // namespace ReadSpeed

#endif // ROOTREADSPEEDTTREE
