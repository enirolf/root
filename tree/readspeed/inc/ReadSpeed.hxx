// Author: Enrico Guiraud, David Poulton 2022

/*************************************************************************
 * Copyright (C) 1995-2022, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOTREADSPEED
#define ROOTREADSPEED

#include <TFile.h>

#include <string>
#include <vector>
#include <regex>

namespace ReadSpeed {

struct Data {
   /// Either a single tree/ntuple name common for all files, or one tree/ntuple name per file.
   std::vector<std::string> fTreeOrNTupleNames;
   /// List of input files.
   std::vector<std::string> fFileNames;
   /// Branches/fields to read.
   std::vector<std::string> fBranchOrFieldNames;
   /// If the branch/field names should use regex matching.
   bool fUseRegex = false;
};

struct Result {
   /// Real time spent reading and decompressing all data, in seconds.
   double fRealTime;
   /// CPU time spent reading and decompressing all data, in seconds.
   double fCpuTime;
   /// Real time spent preparing the multi-thread workload.
   double fMTSetupRealTime;
   /// CPU time spent preparing the multi-thread workload.
   double fMTSetupCpuTime;
   /// Number of uncompressed bytes read in total from TTree branches or RNTuple Fields.
   ULong64_t fUncompressedBytesRead;
   /// Number of compressed bytes read in total from the TFiles.
   ULong64_t fCompressedBytesRead;
   /// Size of ROOT's thread pool for the run (0 indicates a single-thread run with no thread pool present).
   unsigned int fThreadPoolSize;
};

struct EntryRange {
   Long64_t fStart = -1;
   Long64_t fEnd = -1;
};

struct ByteData {
   ULong64_t fUncompressedBytesRead;
   ULong64_t fCompressedBytesRead;
};

struct ReadSpeedRegex {
   std::string text;
   std::regex regex;

   bool operator<(const ReadSpeedRegex &other) const { return text < other.text; }
};

ByteData SumBytes(const std::vector<ByteData> &bytesData);

} // namespace ReadSpeed

#endif // ROOTREADSPEED
