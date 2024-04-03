/// \file RNTupleIndex.cxx
/// \ingroup NTuple ROOT7
/// \author Florine de Geus <florine.de.geus@cern.ch>
/// \date 2024-04-02
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2024, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include <ROOT/RNTupleIndex.hxx>

ROOT::Experimental::Internal::RNTupleIndex &
ROOT::Experimental::Internal::RNTupleIndex::operator=(const RNTupleIndex &other)
{
   fField = other.fField->Clone(other.fField->GetFieldName());
   fNElems = other.fNElems;
   fIndex = other.fIndex;
   return *this;
}

void ROOT::Experimental::Internal::RNTupleIndex::Add(void *objPtr, NTupleSize_t entry)
{
   if (fNElems > fMaxElemsInMemory) {
      throw RException(
         R__FAIL("in-memory index exceeds maximum allowed size (" + std::to_string(fMaxElemsInMemory) + ")"));
   }

   NTupleIndexValue_t indexValue = fField->GetHash(objPtr);
   fIndex[indexValue].insert(entry);
   fNElems++;
}

ROOT::Experimental::NTupleSize_t
ROOT::Experimental::Internal::RNTupleIndex::GetEntryIndex(void *valuePtr, NTupleSize_t lowerBound) const
{
   auto indexedValue = fField->GetHash(valuePtr);

   if (!fIndex.count(indexedValue)) {
      return kInvalidNTupleIndex;
   }

   auto indexEntries = fIndex.at(indexedValue);

   if (auto entry = indexEntries.lower_bound(lowerBound); entry != indexEntries.end())
      return *entry;

   return kInvalidNTupleIndex;
}
