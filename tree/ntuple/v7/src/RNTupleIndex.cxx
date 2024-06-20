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

void ROOT::Experimental::Internal::RNTupleIndexHash::Build(NTupleSize_t firstEntry, NTupleSize_t lastEntry)
{
   fNElems = lastEntry - firstEntry;
   std::vector<RFieldBase::RValue> fieldValues;
   for (const auto &field : fFields) {
      fieldValues.emplace_back(field->CreateValue());
   }

   for (std::uint64_t i = firstEntry; i < lastEntry; ++i) {
      for (auto &fieldValue : fieldValues) {
         fieldValue.Read(i);
      }
      AddEntry(fieldValues, i);
   }

   Freeze();
}

void ROOT::Experimental::Internal::RNTupleIndexHash::AddEntry(std::vector<RFieldBase::RValue> &values,
                                                              NTupleSize_t entryIdx)
{
   RIndexValue indexValue;
   for (unsigned i = 0; i < fFields.size(); ++i) {
      indexValue += fFields[i]->GetHash(values[i].GetPtr<void>().get());
   }
   fIndex[indexValue.fValue].push_back(entryIdx);
}

ROOT::Experimental::NTupleSize_t
ROOT::Experimental::Internal::RNTupleIndexHash::GetEntryIndex(std::vector<void *> valuePtrs) const
{
   auto entryIndices = GetEntryIndices(valuePtrs);
   if (entryIndices.empty())
      return kInvalidNTupleIndex;
   return entryIndices.front();
}

std::vector<ROOT::Experimental::NTupleSize_t>
ROOT::Experimental::Internal::RNTupleIndexHash::GetEntryIndices(std::vector<void *> valuePtrs) const
{
   // TODO(fdegeus) make proper std::hash function
   RIndexValue indexValue;
   for (unsigned i = 0; i < fFields.size(); ++i) {
      indexValue += fFields[i]->GetHash(valuePtrs[i]);
   }

   if (!fIndex.count(indexValue.fValue))
      return {};

   return fIndex.at(indexValue.fValue);
}

// ---------------------------------------------------------------------------------------------------------------------

void ROOT::Experimental::Internal::RNTupleIndexVector::Build(NTupleSize_t firstEntry, NTupleSize_t lastEntry)
{
   std::vector<RFieldBase::RValue> fieldValues;
   for (const auto &field : fFields) {
      fieldValues.emplace_back(field->CreateValue());
   }

   fIndexValues.reserve(lastEntry - firstEntry);
   fEntryIndices.reserve(lastEntry - firstEntry);

   for (std::uint64_t i = firstEntry; i < lastEntry; ++i) {
      for (auto &fieldValue : fieldValues) {
         fieldValue.Read(i);
      }
      AddEntry(fieldValues, i);
   }

   Freeze();
}

void ROOT::Experimental::Internal::RNTupleIndexVector::AddEntry(std::vector<RFieldBase::RValue> &values,
                                                                NTupleSize_t entryIdx)
{
   RIndexValue indexValue;
   for (unsigned i = 0; i < fFields.size(); ++i) {
      indexValue += fFields[i]->GetHash(values[i].GetPtr<void>().get());
   }

   if (fIndexValues.empty() || (fIndexValues.size() > 0 && indexValue.fValue >= fIndexValues.back())) {
      fIndexValues.push_back(indexValue.fValue);
      fEntryIndices.push_back(entryIdx);
   } else {
      auto pos = std::distance(fIndexValues.cbegin(),
                               std::upper_bound(fIndexValues.cbegin(), fIndexValues.cend(), indexValue.fValue));
      fIndexValues.insert(fIndexValues.cbegin() + pos, indexValue.fValue);
      fEntryIndices.insert(fEntryIndices.cbegin() + pos, entryIdx);
   }
}

ROOT::Experimental::NTupleSize_t
ROOT::Experimental::Internal::RNTupleIndexVector::GetEntryIndex(std::vector<void *> valuePtrs) const
{
   RIndexValue indexValue;
   for (unsigned i = 0; i < fFields.size(); ++i) {
      indexValue += fFields[i]->GetHash(valuePtrs[i]);
   }

   auto entryIdx = std::lower_bound(fIndexValues.cbegin(), fIndexValues.cend(), indexValue.fValue);

   if (*entryIdx != indexValue.fValue)
      return kInvalidNTupleIndex;

   return fEntryIndices.at(std::distance(fIndexValues.cbegin(), entryIdx));
}

std::vector<ROOT::Experimental::NTupleSize_t>
ROOT::Experimental::Internal::RNTupleIndexVector::GetEntryIndices(std::vector<void *> valuePtrs) const
{
   RIndexValue indexValue;
   for (unsigned i = 0; i < fFields.size(); ++i) {
      indexValue += fFields[i]->GetHash(valuePtrs[i]);
   }

   auto entryIdxRange = std::equal_range(fIndexValues.cbegin(), fIndexValues.cend(), indexValue.fValue);

   if (entryIdxRange.first == fIndexValues.cend())
      return {};

   auto firstIdx = fEntryIndices.cbegin() + std::distance(fIndexValues.cbegin(), entryIdxRange.first);
   auto lastIdx = fEntryIndices.cbegin() + std::distance(fIndexValues.cbegin(), entryIdxRange.second);

   return std::vector<NTupleSize_t>(firstIdx, lastIdx);
}
