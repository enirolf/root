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
#include <ROOT/RHashValueVisitor.hxx>

ROOT::Experimental::Internal::RNTupleIndex::RNTupleIndex(std::vector<std::unique_ptr<RFieldBase>> &fields,
                                                         NTupleSize_t nEntries)
   : fFields(std::move(fields))
{
   std::vector<RFieldBase::RValue> fieldValues;
   fieldValues.reserve(fields.size());
   for (const auto &field : fFields) {
      fieldValues.emplace_back(field->CreateValue());
   }

   fIndexValues.reserve(nEntries);
   fEntryNumbers.reserve(nEntries);

   for (std::uint64_t i = 0; i < nEntries; ++i) {
      for (auto &fieldValue : fieldValues) {
         // TODO(fdegeus): use bulk reading
         fieldValue.Read(i);
      }
      Add(fieldValues, i);
   }
}

void ROOT::Experimental::Internal::RNTupleIndex::Add(const std::vector<RFieldBase::RValue> &values, NTupleSize_t entry)
{
   RIndexValue indexValue;
   for (unsigned i = 0; i < fFields.size(); ++i) {
      auto &field = fFields[i];
      auto valuePtr = values[i].GetPtr<void>();
      RHashValueVisitor visitor(valuePtr.get());
      field->AcceptVisitor(visitor);
      indexValue += visitor.GetHash();
   }

   if (fIndexValues.empty() || (fIndexValues.size() > 0 && indexValue >= fIndexValues.back())) {
      fIndexValues.push_back(indexValue);
      fEntryNumbers.push_back(entry);
   } else {
      auto pos =
         std::distance(fIndexValues.cbegin(), std::upper_bound(fIndexValues.cbegin(), fIndexValues.cend(), indexValue));
      fIndexValues.insert(fIndexValues.cbegin() + pos, indexValue);
      fEntryNumbers.insert(fEntryNumbers.cbegin() + pos, entry);
   }
}

std::unique_ptr<ROOT::Experimental::Internal::RNTupleIndex>
ROOT::Experimental::Internal::RNTupleIndex::Create(const std::vector<std::string> &fieldNames, RPageSource &pageSource)
{
   pageSource.Attach();
   auto desc = pageSource.GetSharedDescriptorGuard();

   std::vector<std::unique_ptr<RFieldBase>> fields;
   fields.reserve(fieldNames.size());

   for (const auto &fieldName : fieldNames) {
      auto fieldId = desc->FindFieldId(fieldName);
      if (fieldId == kInvalidDescriptorId)
         throw RException(R__FAIL("could not find field \"" + std::string(fieldName) + ""));

      const auto &fieldDesc = desc->GetFieldDescriptor(fieldId);
      auto field = fieldDesc.CreateField(desc.GetRef());

      CallConnectPageSourceOnField(*field, pageSource);

      fields.push_back(std::move(field));
   }

   return std::unique_ptr<RNTupleIndex>(new RNTupleIndex(fields, pageSource.GetNEntries()));
}

ROOT::Experimental::NTupleSize_t
ROOT::Experimental::Internal::RNTupleIndex::GetFirstEntryNumber(const std::vector<void *> &valuePtrs) const
{
   // TODO move to seperate function for deduplication
   RIndexValue indexValue;
   for (unsigned i = 0; i < fFields.size(); ++i) {
      auto &field = fFields[i];
      auto valuePtr = valuePtrs[i];
      RHashValueVisitor visitor(valuePtr);
      field->AcceptVisitor(visitor);
      indexValue += visitor.GetHash();
   }

   auto entryNumber = std::lower_bound(fIndexValues.cbegin(), fIndexValues.cend(), indexValue);

   if (*entryNumber != indexValue)
      return kInvalidNTupleIndex;

   return fEntryNumbers.at(std::distance(fIndexValues.cbegin(), entryNumber));
}

const std::vector<ROOT::Experimental::NTupleSize_t>
ROOT::Experimental::Internal::RNTupleIndex::GetAllEntryNumbers(const std::vector<void *> &valuePtrs) const
{
   RIndexValue indexValue;
   for (unsigned i = 0; i < fFields.size(); ++i) {
      auto &field = fFields[i];
      auto valuePtr = valuePtrs[i];
      RHashValueVisitor visitor(valuePtr);
      field->AcceptVisitor(visitor);
      indexValue += visitor.GetHash();
   }

   auto entryNumberRange = std::equal_range(fIndexValues.cbegin(), fIndexValues.cend(), indexValue);

   if (entryNumberRange.first == fIndexValues.cend())
      return {};

   auto firstIdx = fEntryNumbers.cbegin() + std::distance(fIndexValues.cbegin(), entryNumberRange.first);
   auto lastIdx = fEntryNumbers.cbegin() + std::distance(fIndexValues.cbegin(), entryNumberRange.second);

   return std::vector<NTupleSize_t>(firstIdx, lastIdx);
}
