/// \file RNTupleCoWMerger.cxx
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

#include <ROOT/RNTupleCoWMerger.hxx>
#include <ROOT/RFile.hxx>
#include <ROOT/RNTuple.hxx>

#include <fcntl.h>
#include <stdexcept>
#include <variant>

ROOT::RResult<void>
ROOT::Experimental::RNTupleCoWMerger::Merge(std::span<std::pair<std::string, std::string>> sourceNTuples)
{
   assert(fDestination);

   for (const auto &[ntupleName, ntuplePath] : sourceNTuples) {
      fDestination->CopyOnWrite(std::string(ntupleName), std::string(ntuplePath));
   }

   return RResult<void>::Success();
}
