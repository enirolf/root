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

#include <ROOT/RNTupleHorizontalProcessor.hxx>

#include <ROOT/RField.hxx>

#include <algorithm>
#include <unordered_set>

void ROOT::Experimental::RNTupleHorizontalProcessor::ConnectFields()
{
   auto fnConnect = [this](RFieldContext &fieldContext) {
      Internal::CallConnectPageSourceOnField(fieldContext.GetField(), fieldContext.GetPageSource());
      auto value = fieldContext.GetField().CreateValue();
      fieldContext.SetValuePtr(value.GetPtr<void>());
      fEntry->UpdateValue(fieldContext.GetToken(), value);
   };

   for (auto &fieldContext : fFieldContexts) {
      fnConnect(fieldContext);
   }
}

ROOT::Experimental::RNTupleHorizontalProcessor::RNTupleHorizontalProcessor(
   const std::vector<RNTupleSourceSpec> &ntuples)
   : fNTuples(ntuples)
{
   if (fNTuples.empty())
      throw RException(R__FAIL("at least one RNTuple must be provided"));

   EnsureUniqueNTupleNames();

   fPrimaryPageSource = Internal::RPageSource::Create(ntuples[0].fName, ntuples[0].fStorage);
   fPrimaryPageSource->Attach();
   fModel = fPrimaryPageSource->GetSharedDescriptorGuard()->CreateModel();

   for (unsigned i = 1; i < ntuples.size(); ++i) {
      auto pageSource = Internal::RPageSource::Create(ntuples[i].fName, ntuples[i].fStorage);
      pageSource->Attach();
      auto model = pageSource->GetSharedDescriptorGuard()->CreateModel();
      fModel = Internal::MergeModels(*fModel, *model, ntuples[i].fName);
      fSecondaryPageSources.push_back(std::move(pageSource));
   }

   fEntry = fModel->CreateBareEntry();

   auto fnSetProcessorFields = [this](Internal::RPageSource &pageSource, std::string_view fieldNamePrefix) -> void {
      auto desc = pageSource.GetSharedDescriptorGuard();

      for (const auto &fieldDesc : desc->GetTopLevelFields()) {

         auto fieldOrException = RFieldBase::Create(fieldDesc.GetFieldName(), fieldDesc.GetTypeName());
         if (fieldOrException) {
            auto field = fieldOrException.Unwrap();
            field->SetOnDiskId(fieldDesc.GetId());
            auto token = fieldNamePrefix.empty() ? fieldDesc.GetFieldName() : fieldNamePrefix;
            fFieldContexts.emplace_back(std::move(field), pageSource, fEntry->GetToken(token));
         }
      }
   };

   fnSetProcessorFields(*fPrimaryPageSource, "");

   for (unsigned i = 0; i < fSecondaryPageSources.size(); ++i) {
      fnSetProcessorFields(*fSecondaryPageSources[i], fSecondaryPageSources[i]->GetNTupleName());
   }

   ConnectFields();
}

void ROOT::Experimental::RNTupleHorizontalProcessor::EnsureUniqueNTupleNames()
{
   auto uniqueNTuples = std::unordered_set<RNTupleSourceSpec>(fNTuples.cbegin(), fNTuples.cend());

   if (uniqueNTuples.size() != fNTuples.size())
      throw RException(R__FAIL("horizontal joining of RNTuples with the same name is not allowed"));
}
