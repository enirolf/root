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

   if (sourceNTuples.empty())
      return R__FAIL("at least one source RNTuple must be provided");

   // we should have a model if and only if the destination is initialized.
   if (!!fModel != fDestination->IsInitialized()) {
      return R__FAIL(
         "passing an already-initialized destination to RNTupleCoWMerger::Merge (i.e. trying to do incremental "
         "merging) is currently unsupported");
   }

   auto firstSource = ROOT::Internal::RPageSource::Create(sourceNTuples[0].first, sourceNTuples[0].second);
   firstSource->Attach();
   auto firstSourceDesc = firstSource->GetSharedDescriptorGuard();
   fModel = fDestination->InitFromDescriptor(firstSourceDesc.GetRef(), /*copyClusters=*/false);

   for (const auto &[ntupleName, ntuplePath] : sourceNTuples) {
      fDestination->CopyOnWrite(std::string(ntupleName), std::string(ntuplePath));
   }

   return RResult<void>::Success();
}
