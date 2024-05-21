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

ROOT::Experimental::Internal::RNTupleIndex::RNTupleIndex(std::vector<std::unique_ptr<RFieldBase>> fields,
                                                         RPageSource &pageSource)
   : fFields(std::move(fields))
{
   std::vector<RFieldBase::RValue> fieldValues;
   for (const auto &field : fFields) {
      fieldValues.emplace_back(field->CreateValue());
   }

   for (std::uint64_t i = 0; i < pageSource.GetNEntries(); ++i) {
      std::vector<void *> ptrs;
      for (auto &fieldValue : fieldValues) {
         fieldValue.Read(i);
         ptrs.push_back(fieldValue.GetPtr<void>().get());
      }
      Add(ptrs, i);
   }
}

void ROOT::Experimental::Internal::RNTupleIndex::Add(std::vector<void *> objPtrs, NTupleSize_t entry)
{
   RIndexValue indexValue;
   for (unsigned i = 0; i < fFields.size(); ++i) {
      indexValue += fFields[i]->GetHash(objPtrs[i]);
   }
   fIndex[indexValue.fValue].insert(entry);
}

ROOT::Experimental::NTupleSize_t
ROOT::Experimental::Internal::RNTupleIndex::GetEntryIndex(std::vector<void *> valuePtrs, NTupleSize_t lowerBound) const
{
   RIndexValue indexValue;
   for (unsigned i = 0; i < fFields.size(); ++i) {
      indexValue += fFields[i]->GetHash(valuePtrs[i]);
   }

   if (!fIndex.count(indexValue.fValue)) {
      return kInvalidNTupleIndex;
   }

   auto indexEntries = fIndex.at(indexValue.fValue);

   if (auto entry = indexEntries.lower_bound(lowerBound); entry != indexEntries.end())
      return *entry;

   return kInvalidNTupleIndex;
}

//------------------------------------------------------------------------------

std::unique_ptr<ROOT::Experimental::Internal::RNTupleIndex>
ROOT::Experimental::Internal::CreateRNTupleIndex(std::vector<std::string_view> fieldNames, RPageSource &pageSource)
{
   pageSource.Attach();
   auto desc = pageSource.GetSharedDescriptorGuard();

   std::vector<std::unique_ptr<RFieldBase>> fields;

   for (const auto &fieldName : fieldNames) {
      auto fieldId = desc->FindFieldId(fieldName);
      if (fieldId == kInvalidDescriptorId)
         throw RException(R__FAIL("could not find field \"" + std::string(fieldName) + ""));

      const auto &fieldDesc = desc->GetFieldDescriptor(fieldId);
      auto fieldOrException = RFieldBase::Create(fieldDesc.GetFieldName(), fieldDesc.GetTypeName());
      if (!fieldOrException) {
         throw RException(R__FAIL("could not construct field \"" + std::string(fieldName) + "\""));
      }
      auto field = fieldOrException.Unwrap();
      field->SetOnDiskId(fieldDesc.GetId());

      CallConnectPageSourceOnField(*field, pageSource);

      fields.push_back(std::move(field));
   }

   return std::unique_ptr<RNTupleIndex>(new RNTupleIndex(std::move(fields), pageSource));
}
