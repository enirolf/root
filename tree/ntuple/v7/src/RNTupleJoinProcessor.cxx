/// \file RNTupleProcessor.cxx
/// \ingroup NTuple ROOT7
/// \author Florine de Geus <florine.de.geus@cern.ch>
/// \date 2024-03-26
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2024, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include <ROOT/RNTupleJoinProcessor.hxx>

#include <ROOT/RField.hxx>

#include <algorithm>
#include <unordered_set>

namespace {
std::string GetFieldNameWithoutPrefix(std::string_view fieldName, std::string_view prefix)
{
   std::string strippedFieldName{fieldName};
   auto pos = fieldName.find(prefix);
   if (pos != fieldName.npos)
      strippedFieldName.erase(pos, pos + prefix.size() + 1);
   return strippedFieldName;
}

std::string_view GetFieldNamePrefix(std::string_view fieldName)
{
   if (fieldName.find(".") == fieldName.npos) {
      return "";
   }
   return fieldName.substr(0, fieldName.find("."));
}
} // namespace

void ROOT::Experimental::Internal::RNTupleJoinProcessor::ConnectField(RFieldContext &fieldContext)
{
   auto desc = fieldContext.fPageSource.GetSharedDescriptorGuard();

   auto fieldId = desc->FindFieldId(fieldContext.fProtoField->GetFieldName());
   if (fieldId == kInvalidDescriptorId) {
      throw RException(R__FAIL("field \"" + std::string(fieldContext.fProtoField->GetFieldName()) +
                               "\" not found in current RNTuple"));
   }

   auto &concreteField = fieldContext.CreateConcreteField();
   concreteField.SetOnDiskId(fieldId);

   auto newValue = std::make_unique<RFieldBase::RValue>(concreteField.CreateValue());
   // True when the field was previously connected to another page source.
   if (fieldContext.fValue) {
      auto fieldPtr = fieldContext.fValue->GetPtr<void>();
      fieldContext.fValue.swap(newValue);
      fieldContext.fValue->Bind(fieldPtr);
   } else {
      fieldContext.fValue.swap(newValue);
   }

   Internal::CallConnectPageSourceOnField(concreteField, fieldContext.fPageSource);
}

void ROOT::Experimental::Internal::RNTupleJoinProcessor::ActivateField(std::string_view fieldName)
{
   auto fnCreateField = [&](Internal::RPageSource &pageSource) -> void {
      auto desc = pageSource.GetSharedDescriptorGuard();
      auto fieldId = desc->FindFieldId(GetFieldNameWithoutPrefix(fieldName, desc->GetName()));
      auto &fieldDesc = pageSource.GetSharedDescriptorGuard()->GetFieldDescriptor(fieldId);

      auto fieldOrException = RFieldBase::Create(fieldDesc.GetFieldName(), fieldDesc.GetTypeName());
      if (!fieldOrException) {
         throw RException(R__FAIL("could not create field \"" + fieldDesc.GetFieldName() + "\" with type \"" +
                                  fieldDesc.GetTypeName() + "\""));
      }
      auto protoField = fieldOrException.Unwrap();

      RFieldContext fieldContext(std::move(protoField), pageSource);
      ConnectField(fieldContext);
      fFieldContexts.try_emplace(std::string(fieldName), std::move(fieldContext));
   };

   std::string fieldNamePrefix{GetFieldNamePrefix(fieldName)};
   if (fieldNamePrefix.empty()) {
      fnCreateField(*fPrimaryPageSource);
   } else if (fSecondaryPageSources.find(fieldNamePrefix) != fSecondaryPageSources.end()) {
      fnCreateField(*fSecondaryPageSources.at(std::string(fieldNamePrefix)));

      if (fJoinIndices.find(fieldNamePrefix) != fJoinIndices.end() && !fJoinIndices.at(fieldNamePrefix)->IsBuilt())
         fJoinIndices.at(fieldNamePrefix)->Build();
   } else {
      throw RException(R__FAIL("could not find field in any of the page sources"));
   }
}

ROOT::Experimental::Internal::RNTupleJoinProcessor::RNTupleJoinProcessor(const std::vector<RNTupleSourceSpec> &ntuples)
   : fNTuples(ntuples)
{
   if (fNTuples.empty())
      throw RException(R__FAIL("at least one RNTuple must be provided"));

   EnsureUniqueNTupleNames();

   fPrimaryPageSource = Internal::RPageSource::Create(ntuples[0].fName, ntuples[0].fLocation);
   fPrimaryPageSource->Attach();

   for (unsigned i = 1; i < ntuples.size(); ++i) {
      auto pageSource = Internal::RPageSource::Create(ntuples[i].fName, ntuples[i].fLocation);
      pageSource->Attach();
      fSecondaryPageSources.emplace(ntuples[i].fName, std::move(pageSource));
   }
}

ROOT::Experimental::Internal::RNTupleJoinProcessor::RNTupleJoinProcessor(const std::vector<RNTupleSourceSpec> &ntuples,
                                                                         const std::vector<std::string> &indexFields)
   : fNTuples(ntuples), fIndexFieldNames(indexFields)
{
   if (fNTuples.empty())
      throw RException(R__FAIL("at least one RNTuple must be provided"));

   EnsureUniqueNTupleNames();

   fPrimaryPageSource = Internal::RPageSource::Create(ntuples[0].fName, ntuples[0].fLocation);
   fPrimaryPageSource->Attach();

   for (const auto &fieldName : indexFields) {
      ActivateField(fieldName);
   }

   for (unsigned i = 1; i < ntuples.size(); ++i) {
      auto pageSource = Internal::RPageSource::Create(ntuples[i].fName, ntuples[i].fLocation);
      pageSource->Attach();
      fJoinIndices.emplace(ntuples[i].fName,
                           RNTupleIndex::Create(fIndexFieldNames, *pageSource, true /* deferBuild */));
      fSecondaryPageSources.emplace(ntuples[i].fName, std::move(pageSource));
   }
}

void ROOT::Experimental::Internal::RNTupleJoinProcessor::EnsureUniqueNTupleNames()
{
   auto uniqueNTuples = std::unordered_set<RNTupleSourceSpec>(fNTuples.cbegin(), fNTuples.cend());

   if (uniqueNTuples.size() != fNTuples.size())
      throw RException(R__FAIL("horizontal joining of RNTuples with the same name is not allowed"));
}

const std::vector<std::string> ROOT::Experimental::Internal::RNTupleJoinProcessor::GetActiveFields() const
{
   std::vector<std::string> fieldNames;
   fieldNames.reserve(fFieldContexts.size());
   std::transform(fFieldContexts.cbegin(), fFieldContexts.cend(), std::back_inserter(fieldNames),
                  [](auto &fldCtx) { return fldCtx.first; });
   return fieldNames;
}
