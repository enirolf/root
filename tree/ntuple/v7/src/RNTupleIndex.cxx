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

void ROOT::Experimental::Internal::RNTupleIndex::Merge(const RNTupleIndex &other)
{
   if (fField->GetTypeName() != other.fField->GetTypeName())
      throw RException(R__FAIL("field types do not match"));
   if (fField->GetFieldName() != other.fField->GetFieldName())
      throw RException(R__FAIL("field names do not match"));

   fNElems += other.fNElems;

   for (const auto &val : other.fIndex) {
      auto res = fIndex.insert(val);
      if (!res.second) {
         // The index value is already present so the insertion failed. Instead, we have to merge the values.
         fIndex.at(val.first).insert(val.second.begin(), val.second.end());
      }
   }
}

void ROOT::Experimental::Internal::RNTupleIndex::Add(void *objPtr, NTupleSize_t entry)
{
   if (fNElems > fMaxElemsInMemory) {
      throw RException(
         R__FAIL("in-memory index exceeds maximum allowed size (" + std::to_string(fMaxElemsInMemory) + ")"));
   }

   NTupleIndexValue_t indexValue = fField->GetIndexRepresentation(objPtr);
   fIndex[indexValue].insert(entry);
   fNElems++;
}

ROOT::Experimental::NTupleSize_t
ROOT::Experimental::Internal::RNTupleIndex::GetEntryIndex(void *valuePtr, NTupleSize_t lowerBound) const
{
   auto indexedValue = fField->GetIndexRepresentation(valuePtr);

   if (!fIndex.count(indexedValue)) {
      return kInvalidNTupleIndex;
   }

   auto indexEntries = fIndex.at(indexedValue);

   if (auto entry = indexEntries.lower_bound(lowerBound); entry != indexEntries.end())
      return *entry;

   return kInvalidNTupleIndex;
}

ROOT::Experimental::Internal::RNTupleIndex
ROOT::Experimental::Internal::RNTupleIndex::Concatenate(const RNTupleIndex &left, const RNTupleIndex &right)
{
   RNTupleIndex index(left.fField->Clone(left.fField->GetFieldName()));
   index.Merge(left);
   index.Merge(right);
   return index;
}
