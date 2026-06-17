/// \file ROOT/RNTupleCoWMerger.hxx
/// \author Florine de Geus <florine.de.geus@cern.ch>
/// \date 2026-06-16
/// \warning This is part of the ROOT 7 prototype! It will
/// change without notice. It might trigger earthquakes. Feedback is welcome!

/*************************************************************************
 * Copyright (C) 1995-2026, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_RNTupleCoWMerger
#define ROOT_RNTupleCoWMerger

#include <ROOT/RPageStorageFile.hxx>

#include <string>
#include <span>

namespace ROOT {
namespace Experimental {
class RNTupleCoWMerger {
private:
   std::unique_ptr<ROOT::Internal::RPageSinkFile> fDestination;

public:
   RNTupleCoWMerger(std::unique_ptr<ROOT::Internal::RPageSinkFile> destination) : fDestination(std::move(destination))
   {
   }

   RResult<void> Merge(std::span<std::pair<std::string, std::string>> sourceNTuples);
};
} // namespace Experimental
} // namespace ROOT

#endif // ROOT_RNTupleCoWMerger
