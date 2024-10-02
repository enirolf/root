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

#include <ROOT/RNTupleProcessor.hxx>

#include <ROOT/RFieldBase.hxx>

std::unique_ptr<ROOT::Experimental::Internal::RNTupleProcessor>
ROOT::Experimental::Internal::RNTupleProcessor::CreateChain(const std::vector<RNTupleSourceSpec> &ntuples,
                                                            std::unique_ptr<RNTupleModel> model)
{
   return std::unique_ptr<RNTupleChainProcessor>(new RNTupleChainProcessor(ntuples, std::move(model)));
}

std::unique_ptr<ROOT::Experimental::Internal::RNTupleProcessor>
ROOT::Experimental::Internal::RNTupleProcessor::CreateJoin(const std::vector<RNTupleSourceSpec> &ntuples)
{
   if (ntuples.size() < 1)
      throw RException(R__FAIL("at least one RNTuple must be provided"));

   // if (ntuples.size() != models.size())
   //    throw RException(R__FAIL("number of provided RNTuples and models do not match"));

   auto processor = std::unique_ptr<RNTupleJoinProcessor>(new RNTupleJoinProcessor(ntuples[0]));

   for (unsigned i = 1; i < ntuples.size(); ++i) {
      processor->AddAuxiliary(ntuples[i]);
   }

   return processor;
}

//------------------------------------------------------------------------------

ROOT::Experimental::Internal::RNTupleChainProcessor::RNTupleChainProcessor(
   const std::vector<RNTupleSourceSpec> &ntuples, std::unique_ptr<RNTupleModel> model)
   : RNTupleProcessor(ntuples)
{
   if (fNTuples.empty())
      throw RException(R__FAIL("at least one RNTuple must be provided"));

   fPageSource = Internal::RPageSource::Create(fNTuples[0].fName, fNTuples[0].fLocation);
   fPageSource->Attach();

   if (fPageSource->GetNEntries() == 0) {
      throw RException(R__FAIL("provided RNTuple is empty"));
   }

   if (!model)
      model = fPageSource->GetSharedDescriptorGuard()->CreateModel();

   model->Freeze();
   fEntry = model->CreateEntry();

   for (const auto &value : *fEntry) {
      auto &field = value.GetField();
      auto token = fEntry->GetToken(field.GetFieldName());

      // If the model has a default entry, use the value pointers from the entry in the entry managed by the
      // processor. This way, the pointers returned by RNTupleModel::MakeField can be used in the processor loop to
      // access the corresponding field values.
      if (!model->IsBare()) {
         auto valuePtr = model->GetDefaultEntry().GetPtr<void>(token);
         fEntry->BindValue(token, valuePtr);
      }

      const auto &[fieldContext, _] =
         fFieldContexts.try_emplace(field.GetFieldName(), field.Clone(field.GetFieldName()), token);
      ConnectField(fieldContext->second);
   }
}

ROOT::Experimental::NTupleSize_t
ROOT::Experimental::Internal::RNTupleChainProcessor::ConnectNTuple(const RNTupleSourceSpec &ntuple)
{
   for (auto &[_, fieldContext] : fFieldContexts) {
      fieldContext.ResetConcreteField();
   }
   fPageSource = Internal::RPageSource::Create(ntuple.fName, ntuple.fLocation);
   fPageSource->Attach();

   for (auto &[_, fieldContext] : fFieldContexts) {
      ConnectField(fieldContext);
   }
   return fPageSource->GetNEntries();
}

void ROOT::Experimental::Internal::RNTupleChainProcessor::ConnectField(RFieldContext &fieldContext)
{
   auto desc = fPageSource->GetSharedDescriptorGuard();

   auto fieldId = desc->FindFieldId(fieldContext.GetProtoField().GetFieldName());
   if (fieldId == kInvalidDescriptorId) {
      throw RException(
         R__FAIL("field \"" + fieldContext.GetProtoField().GetFieldName() + "\" not found in current RNTuple"));
   }

   fieldContext.SetConcreteField();
   fieldContext.fConcreteField->SetOnDiskId(desc->FindFieldId(fieldContext.GetProtoField().GetFieldName()));
   Internal::CallConnectPageSourceOnField(*fieldContext.fConcreteField, *fPageSource);

   auto valuePtr = fEntry->GetPtr<void>(fieldContext.fToken);
   auto value = fieldContext.fConcreteField->CreateValue();
   value.Bind(valuePtr);
   fEntry->UpdateValue(fieldContext.fToken, value);
}

//------------------------------------------------------------------------------

ROOT::Experimental::Internal::RNTupleJoinProcessor::RNTupleJoinProcessor(const RNTupleSourceSpec &mainNTuple,
                                                                         std::unique_ptr<RNTupleModel> model)
   : RNTupleProcessor({mainNTuple})
{
   fPageSource = Internal::RPageSource::Create(mainNTuple.fName, mainNTuple.fLocation);
   fPageSource->Attach();

   if (fPageSource->GetNEntries() == 0) {
      throw RException(R__FAIL("provvided RNTuple is empty"));
   }

   if (!model)
      model = fPageSource->GetSharedDescriptorGuard()->CreateModel();

   model->Freeze();
   fEntry = model->CreateEntry();

   for (const auto &value : *fEntry) {
      auto &field = value.GetField();
      auto token = fEntry->GetToken(field.GetFieldName());

      // If the model has a default entry, use the value pointers from the entry in the entry managed by the
      // processor. This way, the pointers returned by RNTupleModel::MakeField can be used in the processor loop to
      // access the corresponding field values.
      if (!fJoinModel->IsBare()) {
         auto valuePtr = fJoinModel->GetDefaultEntry().GetPtr<void>(token);
         fEntry->BindValue(token, valuePtr);
      }

      const auto &[fieldContext, _] =
         fFieldContexts.try_emplace(field.GetFieldName(), field.Clone(field.GetFieldName()), token);
      ConnectField(fieldContext->second, *fPageSource, *fEntry);
   }
}

void ROOT::Experimental::Internal::RNTupleJoinProcessor::AddAuxiliary(const RNTupleSourceSpec &auxNTuple,
                                                                      std::unique_ptr<RNTupleModel> model)
{
   fAuxPageSources.emplace(auxNTuple.fName, Internal::RPageSource::Create(auxNTuple.fName, auxNTuple.fLocation));
   auto &pageSource = fAuxPageSources.at(auxNTuple.fName);
   pageSource->Attach();

   if (pageSource->GetNEntries() == 0) {
      throw RException(R__FAIL("provided RNTuple is empty"));
   }

   if (!model)
      model = pageSource->GetSharedDescriptorGuard()->CreateModel();

   fAuxEntries.emplace(auxNTuple.fName, model->CreateEntry());
   auto &entry = fAuxEntries.at(auxNTuple.fName);

   // fAuxEntries.emplace(auxNTuple.fName, model->CreateEntry());
   // auto &entry = fAuxEntries.at(auxNTuple.fName);

   for (const auto &value : *entry) {
      auto &field = value.GetField();
      auto token = entry->GetToken(field.GetFieldName());

      // If the model has a default entry, use the value pointers from the entry in the entry managed by the
      // processor. This way, the pointers returned by RNTupleModel::MakeField can be used in the processor loop to
      // access the corresponding field values.
      if (!model->IsBare()) {
         auto valuePtr = model->GetDefaultEntry().GetPtr<void>(token);
         entry->BindValue(token, valuePtr);
      }

      std::string fieldName = auxNTuple.fName + "." + field.GetFieldName();

      const auto &[fieldContext, _] = fFieldContexts.try_emplace(fieldName, field.Clone(field.GetFieldName()), token,
                                                                 auxNTuple.fName, true /* isAuxiliary */);
      ConnectField(fieldContext->second, *pageSource, *entry);
   }
}

void ROOT::Experimental::Internal::RNTupleJoinProcessor::ConnectField(RFieldContext &fieldContext,
                                                                      RPageSource &pageSource, REntry &entry)
{
   auto desc = pageSource.GetSharedDescriptorGuard();

   auto fieldId = desc->FindFieldId(fieldContext.GetProtoField().GetFieldName());
   if (fieldId == kInvalidDescriptorId) {
      throw RException(
         R__FAIL("field \"" + fieldContext.GetProtoField().GetFieldName() + "\" not found in current RNTuple"));
   }

   fieldContext.SetConcreteField();
   fieldContext.fConcreteField->SetOnDiskId(desc->FindFieldId(fieldContext.GetProtoField().GetFieldName()));
   Internal::CallConnectPageSourceOnField(*fieldContext.fConcreteField, pageSource);

   auto valuePtr = entry.GetPtr<void>(fieldContext.fToken);
   auto value = fieldContext.fConcreteField->CreateValue();
   value.Bind(valuePtr);
   entry.UpdateValue(fieldContext.fToken, value);
}
