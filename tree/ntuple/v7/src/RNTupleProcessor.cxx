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

namespace {
using ROOT::Experimental::RNTupleOpenSpec;
void EnsureUniqueNTupleNames(const std::vector<RNTupleOpenSpec> &ntuples)
{
   std::unordered_set<std::string> uniqueNTupleNames;
   for (const auto &ntuple : ntuples) {
      auto res = uniqueNTupleNames.emplace(ntuple.fNTupleName);
      if (!res.second) {
         throw ROOT::RException(R__FAIL("horizontal joining of RNTuples with the same name is not allowed"));
      }
   }
}
} // anonymous namespace

std::unique_ptr<ROOT::Experimental::RNTupleProcessor>
ROOT::Experimental::RNTupleProcessor::Create(const RNTupleOpenSpec &ntuple, std::unique_ptr<RNTupleModel> model)
{
   return Create(ntuple, ntuple.fNTupleName, std::move(model));
}

std::unique_ptr<ROOT::Experimental::RNTupleProcessor>
ROOT::Experimental::RNTupleProcessor::Create(const RNTupleOpenSpec &ntuple, std::string_view processorName,
                                             std::unique_ptr<RNTupleModel> model)
{
   if (!model) {
      auto pageSource = Internal::RPageSource::Create(ntuple.fNTupleName, ntuple.fStorage);
      pageSource->Attach();
      model = pageSource->GetSharedDescriptorGuard()->CreateModel();
   }
   return std::unique_ptr<RNTupleSingleProcessor>(new RNTupleSingleProcessor(ntuple, processorName, std::move(model)));
}

std::unique_ptr<ROOT::Experimental::RNTupleProcessor>
ROOT::Experimental::RNTupleProcessor::CreateChain(const std::vector<RNTupleOpenSpec> &ntuples,
                                                  std::unique_ptr<RNTupleModel> model)
{
   if (ntuples.empty())
      throw RException(R__FAIL("at least one RNTuple must be provided"));

   return CreateChain(ntuples, ntuples[0].fNTupleName, std::move(model));
}

std::unique_ptr<ROOT::Experimental::RNTupleProcessor>
ROOT::Experimental::RNTupleProcessor::CreateChain(const std::vector<RNTupleOpenSpec> &ntuples,
                                                  std::string_view processorName, std::unique_ptr<RNTupleModel> model)
{
   if (ntuples.empty())
      throw RException(R__FAIL("at least one RNTuple must be provided"));

   std::vector<std::unique_ptr<RNTupleProcessor>> innerProcessors;
   innerProcessors.reserve(ntuples.size());

   if (!model) {
      auto firstPageSource = Internal::RPageSource::Create(ntuples[0].fNTupleName, ntuples[0].fStorage);
      firstPageSource->Attach();
      model = firstPageSource->GetSharedDescriptorGuard()->CreateModel();
   }

   for (const auto &ntuple : ntuples) {
      innerProcessors.emplace_back(Create(ntuple, model->Clone()));
   }

   return CreateChain(innerProcessors, processorName, std::move(model));
}

std::unique_ptr<ROOT::Experimental::RNTupleProcessor>
ROOT::Experimental::RNTupleProcessor::CreateChain(std::vector<std::unique_ptr<RNTupleProcessor>> &innerProcessors,
                                                  std::unique_ptr<RNTupleModel> model)
{
   if (innerProcessors.empty())
      throw RException(R__FAIL("at least one inner processor must be provided"));

   auto processorName = innerProcessors[0]->GetProcessorName();
   return CreateChain(innerProcessors, processorName, std::move(model));
}

std::unique_ptr<ROOT::Experimental::RNTupleProcessor>
ROOT::Experimental::RNTupleProcessor::CreateChain(std::vector<std::unique_ptr<RNTupleProcessor>> &innerProcessors,
                                                  std::string_view processorName, std::unique_ptr<RNTupleModel> model)
{
   if (innerProcessors.empty())
      throw RException(R__FAIL("at least one inner processor must be provided"));

   // TODO(fdegeus) apply the provided model to all inner processors. Right now we assume they share the same model.
   if (!model) {
      model = innerProcessors[0]->GetModel().Clone();
   }

   return std::unique_ptr<RNTupleChainProcessor>(
      new RNTupleChainProcessor(innerProcessors, processorName, std::move(model)));
}

std::unique_ptr<ROOT::Experimental::RNTupleProcessor>
ROOT::Experimental::RNTupleProcessor::CreateJoin(const std::vector<RNTupleOpenSpec> &ntuples,
                                                 const std::vector<std::string> &joinFields,
                                                 std::vector<std::unique_ptr<RNTupleModel>> models)
{
   if (ntuples.size() < 1)
      throw RException(R__FAIL("at least one RNTuple must be provided"));

   return CreateJoin(ntuples, joinFields, ntuples[0].fNTupleName, std::move(models));
}

// TODO simplify and call processor-based Create
std::unique_ptr<ROOT::Experimental::RNTupleProcessor>
ROOT::Experimental::RNTupleProcessor::CreateJoin(const std::vector<RNTupleOpenSpec> &ntuples,
                                                 const std::vector<std::string> &joinFields,
                                                 std::string_view processorName,
                                                 std::vector<std::unique_ptr<RNTupleModel>> models)
{
   if (ntuples.size() < 1)
      throw RException(R__FAIL("at least one RNTuple must be provided"));

   if (models.size() > 0 && models.size() != ntuples.size()) {
      throw RException(R__FAIL("number of provided models must match number of specified ntuples"));
   }

   if (joinFields.size() > 4) {
      throw RException(R__FAIL("a maximum of four join fields is allowed"));
   }

   if (std::set(joinFields.begin(), joinFields.end()).size() < joinFields.size()) {
      throw RException(R__FAIL("join fields must be unique"));
   }

   // TODO(fdegeus) allow for the provision of aliases for ntuples with the same name, removing the constraint of
   // uniquely-named ntuples.
   EnsureUniqueNTupleNames(ntuples);

   std::unique_ptr<RNTupleJoinProcessor> processor;
   if (models.size() > 0) {
      auto mainProc = Create(ntuples[0], models[0]->Clone());
      processor = std::unique_ptr<RNTupleJoinProcessor>(
         new RNTupleJoinProcessor(std::move(mainProc), processorName, std::move(models[0])));
   } else {
      auto mainProc = Create(ntuples[0]);
      processor =
         std::unique_ptr<RNTupleJoinProcessor>(new RNTupleJoinProcessor(std::move(mainProc), processorName, nullptr));
   }

   for (unsigned i = 1; i < ntuples.size(); ++i) {
      if (models.size() > 0) {
         processor->AddAuxiliary(Create(ntuples[i], models[i]->Clone()), joinFields, std::move(models[i]));
      } else {
         processor->AddAuxiliary(Create(ntuples[i]), joinFields);
      }
   }

   return processor;
}

std::unique_ptr<ROOT::Experimental::RNTupleProcessor>
ROOT::Experimental::RNTupleProcessor::CreateJoin(std::unique_ptr<RNTupleProcessor> mainProcessor,
                                                 std::vector<std::unique_ptr<RNTupleProcessor>> &auxProcessors,
                                                 const std::vector<std::string> &joinFields,
                                                 std::vector<std::unique_ptr<RNTupleModel>> models)
{
   auto processorName = mainProcessor->GetProcessorName();
   return CreateJoin(std::move(mainProcessor), auxProcessors, joinFields, processorName, std::move(models));
}

std::unique_ptr<ROOT::Experimental::RNTupleProcessor>
ROOT::Experimental::RNTupleProcessor::CreateJoin(std::unique_ptr<RNTupleProcessor> mainProcessor,
                                                 std::vector<std::unique_ptr<RNTupleProcessor>> &auxProcessors,
                                                 const std::vector<std::string> &joinFields,
                                                 std::string_view processorName,
                                                 std::vector<std::unique_ptr<RNTupleModel>> models)
{
   if (models.size() > 0 && models.size() != auxProcessors.size() + 1) {
      throw RException(R__FAIL("number of provided models must match number of specified ntuples"));
   }

   if (joinFields.size() > 4) {
      throw RException(R__FAIL("a maximum of four join fields is allowed"));
   }

   if (std::set(joinFields.begin(), joinFields.end()).size() < joinFields.size()) {
      throw RException(R__FAIL("join fields must be unique"));
   }

   std::unique_ptr<RNTupleJoinProcessor> processor;
   if (models.size() > 0) {
      processor = std::unique_ptr<RNTupleJoinProcessor>(
         new RNTupleJoinProcessor(std::move(mainProcessor), processorName, std::move(models[0])));
   } else {
      processor = std::unique_ptr<RNTupleJoinProcessor>(
         new RNTupleJoinProcessor(std::move(mainProcessor), processorName, nullptr));
   }

   for (unsigned i = 0; i < auxProcessors.size(); ++i) {
      if (models.size() > 0) {
         processor->AddAuxiliary(std::move(auxProcessors[i]), joinFields, std::move(models[i + 1]));
      } else {
         processor->AddAuxiliary(std::move(auxProcessors[i]), joinFields);
      }
   }

   return processor;
}

//------------------------------------------------------------------------------

ROOT::Experimental::RNTupleSingleProcessor::RNTupleSingleProcessor(const RNTupleOpenSpec &ntuple,
                                                                   std::string_view processorName,
                                                                   std::unique_ptr<RNTupleModel> model)
   : RNTupleProcessor(processorName, std::move(model))
{
   fPageSource = Internal::RPageSource::Create(ntuple.fNTupleName, ntuple.fStorage);

   fModel->Freeze();
   fEntry = fModel->CreateEntry();

   for (const auto &value : *fEntry) {
      auto &field = value.GetField();
      auto token = fEntry->GetToken(field.GetQualifiedFieldName());

      // If the model has a default entry, use the value pointers from the entry in the entry managed by the
      // processor. This way, the pointers returned by RNTupleModel::MakeField can be used in the processor loop to
      // access the corresponding field values.
      if (!fModel->IsBare()) {
         auto valuePtr = fModel->GetDefaultEntry().GetPtr<void>(token);
         fEntry->BindValue(token, valuePtr);
      }
   }
}

ROOT::Experimental::NTupleSize_t ROOT::Experimental::RNTupleSingleProcessor::LoadEntry(NTupleSize_t entryNumber)
{
   Connect();

   if (entryNumber >= fNEntries)
      return kInvalidNTupleIndex;

   fEntry->Read(entryNumber);

   fNEntriesProcessed++;
   fCurrentEntryNumber = entryNumber;
   return entryNumber;
}

void ROOT::Experimental::RNTupleSingleProcessor::SetEntryPointers(const REntry &entry, std::string_view fieldNamePrefix)
{
   for (const auto &value : *fEntry) {
      std::string fieldName = value.GetField().GetQualifiedFieldName();
      auto valuePtr = fieldNamePrefix.empty() ? entry.GetPtr<void>(fieldName)
                                              : entry.GetPtr<void>(std::string(fieldNamePrefix) + "." + fieldName);

      fEntry->BindValue(fieldName, valuePtr);
   }
}

void ROOT::Experimental::RNTupleSingleProcessor::Connect()
{
   if (IsConnected())
      return;

   fPageSource->Attach();
   fNEntries = fPageSource->GetNEntries();

   auto desc = fPageSource->GetSharedDescriptorGuard();
   auto &fieldZero = Internal::GetFieldZeroOfModel(*fModel);
   DescriptorId_t fieldZeroId = desc->GetFieldZeroId();
   fieldZero.SetOnDiskId(fieldZeroId);

   for (auto &field : fieldZero.GetSubFields()) {
      auto onDiskFieldId = desc->FindFieldId(field->GetQualifiedFieldName(), fieldZeroId);
      // The field we are trying to connect is not present in the ntuple we are trying to connect.
      if (onDiskFieldId == kInvalidDescriptorId) {
         throw RException(
            R__FAIL("field \"" + field->GetQualifiedFieldName() + "\" not found in the RNTuple currently connected"));
      };
      // The field already has an on-disk ID. This happens when the model was inferred from the page source, which is
      // happens when the user doesn't provide a model themselves.
      if (field->GetOnDiskId() == kInvalidDescriptorId)
         field->SetOnDiskId(onDiskFieldId);

      Internal::CallConnectPageSourceOnField(*field, *fPageSource);
   }
}

//------------------------------------------------------------------------------

ROOT::Experimental::RNTupleChainProcessor::RNTupleChainProcessor(
   std::vector<std::unique_ptr<RNTupleProcessor>> &processors, std::string_view processorName,
   std::unique_ptr<RNTupleModel> model)
   : RNTupleProcessor(processorName, std::move(model)), fInnerProcessors(std::move(processors))
{
   fInnerNEntries.assign(fInnerProcessors.size(), kInvalidNTupleIndex);

   fModel->Freeze();
   fEntry = fModel->CreateEntry();

   for (const auto &value : *fEntry) {
      auto &field = value.GetField();
      auto token = fEntry->GetToken(field.GetQualifiedFieldName());

      // If the model has a default entry, use the value pointers from the entry in the entry managed by the
      // processor. This way, the pointers returned by RNTupleModel::MakeField can be used in the processor loop to
      // access the corresponding field values.
      if (!fModel->IsBare()) {
         auto valuePtr = fModel->GetDefaultEntry().GetPtr<void>(token);
         fEntry->BindValue(token, valuePtr);
      }
   }

   for (auto &innerProc : fInnerProcessors) {
      innerProc->SetEntryPointers(*fEntry);
   }
}

ROOT::Experimental::NTupleSize_t ROOT::Experimental::RNTupleChainProcessor::GetNEntries()
{
   if (fNEntries == kInvalidNTupleIndex) {
      NTupleSize_t nEntries = 0;

      for (unsigned i = 0; i < fInnerProcessors.size(); ++i) {
         if (fInnerNEntries[i] == kInvalidNTupleIndex) {
            fInnerNEntries[i] = fInnerProcessors[i]->GetNEntries();
         }

         nEntries += fInnerNEntries[i];
      }

      fNEntries = nEntries;
   }

   return fNEntries;
}

void ROOT::Experimental::RNTupleChainProcessor::SetEntryPointers(const REntry &entry, std::string_view fieldNamePrefix)
{
   for (const auto &value : *fEntry) {
      std::string fieldName = value.GetField().GetQualifiedFieldName();
      auto valuePtr = fieldNamePrefix.empty() ? entry.GetPtr<void>(fieldName)
                                              : entry.GetPtr<void>(std::string(fieldNamePrefix) + "." + fieldName);

      fEntry->BindValue(fieldName, valuePtr);
   }

   for (auto &innerProc : fInnerProcessors) {
      innerProc->SetEntryPointers(*fEntry, fieldNamePrefix);
   }
}

ROOT::Experimental::NTupleSize_t ROOT::Experimental::RNTupleChainProcessor::LoadEntry(NTupleSize_t entryNumber)
{
   // We have to deal with a corner case for the first entry and empty first inner processors.
   // TODO (fdegeus) refactor
   if (fCurrentEntryNumber > 0 && entryNumber == fCurrentEntryNumber) {
      fNEntriesProcessed++;
      return fInnerProcessors[fCurrentProcessorNumber]->LoadEntry(fCurrentEntryNumber);
   }

   NTupleSize_t localEntryNumber = entryNumber;
   size_t currProcessor = 0;

   // TODO (fdegeus) nicely refactor
   if (fInnerNEntries[currProcessor] == kInvalidNTupleIndex) {
      fInnerNEntries[currProcessor] = fInnerProcessors[currProcessor]->GetNEntries();
   }

   while (localEntryNumber >= fInnerNEntries[currProcessor]) {
      localEntryNumber -= fInnerNEntries[currProcessor];

      if (++currProcessor >= fInnerProcessors.size())
         return kInvalidNTupleIndex;

      if (fInnerNEntries[currProcessor] == kInvalidNTupleIndex) {
         fInnerNEntries[currProcessor] = fInnerProcessors[currProcessor]->GetNEntries();
      }
   }

   if (currProcessor != fCurrentProcessorNumber)
      fCurrentProcessorNumber = currProcessor;

   fInnerProcessors[fCurrentProcessorNumber]->LoadEntry(localEntryNumber);

   fNEntriesProcessed++;
   fCurrentEntryNumber = entryNumber;
   return entryNumber;
}

//------------------------------------------------------------------------------

ROOT::Experimental::RNTupleJoinProcessor::RNTupleJoinProcessor(std::unique_ptr<RNTupleProcessor> mainProcessor,
                                                               std::string_view processorName,
                                                               std::unique_ptr<RNTupleModel> model)
   : RNTupleProcessor(processorName, std::move(model)), fMainProcessor(std::move(mainProcessor))
{
   fNEntries = fMainProcessor->GetNEntries();

   if (!fModel)
      fModel = fMainProcessor->GetModel().Clone();

   fModel->Freeze();
   fEntry = fModel->CreateEntry();

   for (const auto &value : *fEntry) {
      auto &field = value.GetField();
      const auto &fieldName = field.GetQualifiedFieldName();

      // If the model has a default entry, use the value pointers from the default entry of the model that was passed to
      // this constructor. This way, the pointers returned by RNTupleModel::MakeField can be used in the processor loop
      // to access the corresponding field values.
      if (!fModel->IsBare()) {
         auto valuePtr = fModel->GetDefaultEntry().GetPtr<void>(fieldName);
         fEntry->BindValue(fieldName, valuePtr);
      }
   }

   fMainProcessor->SetEntryPointers(*fEntry);
}

void ROOT::Experimental::RNTupleJoinProcessor::AddAuxiliary(std::unique_ptr<RNTupleProcessor> auxProcessor,
                                                            const std::vector<std::string> &joinFields,
                                                            std::unique_ptr<RNTupleModel> model)
{
   assert(fNEntriesProcessed == 0 && "cannot add auxiliary ntuples after processing has started");

   if (!model)
      model = auxProcessor->GetModel().Clone();

   model->Freeze();

   if (!model->IsBare()) {
      auxProcessor->SetEntryPointers(model->GetDefaultEntry());
   }

   auto entry = model->CreateBareEntry();

   // Append the auxiliary fields to the join model
   fModel->Unfreeze();

   // The fields of the auxiliary ntuple are contained in an anonymous record field and subsequently registered as
   // subfields to the join model. This way they can be accessed through the processor as `auxNTupleName.fieldName`,
   // which is necessary in case there are duplicate field names between the main ntuple and (any of the) auxiliary
   // ntuples.
   std::vector<std::unique_ptr<RFieldBase>> auxFields;
   auxFields.reserve(entry->fValues.size());
   for (const auto &val : *entry) {
      auto &field = val.GetField();

      auxFields.emplace_back(field.Clone(field.GetQualifiedFieldName()));
   }

   std::unique_ptr<RFieldBase> auxParentField =
      std::make_unique<RRecordField>(auxProcessor->GetProcessorName(), std::move(auxFields));

   if (!auxParentField) {
      throw RException(R__FAIL("could not create auxiliary RNTuple parent field"));
   }

   const auto &subFields = auxParentField->GetSubFields();
   fModel->AddField(std::move(auxParentField));
   for (const auto &field : subFields) {
      fModel->RegisterSubfield(field->GetQualifiedFieldName());
   }

   fAuxiliaryProcessors.push_back(std::move(auxProcessor));

   fModel->Freeze();
   // After modifying the join model, we need to create a new entry since the old one is invalidated. However, we do
   // want to carry over the value pointers, so the pointers returned by `MakeField` during the creation of the original
   // model by the user can be used in the processor loop.
   auto newEntry = fModel->CreateEntry();

   for (const auto &value : fMainProcessor->GetEntry()) {
      // TODO figure out token business
      auto fieldName = value.GetField().GetQualifiedFieldName();
      auto valuePtr = fModel->GetDefaultEntry().GetPtr<void>(fieldName);
      newEntry->BindValue(fieldName, valuePtr);
   }

   for (const auto &auxProcessor : fAuxiliaryProcessors) {
      for (const auto &value : auxProcessor->GetEntry()) {
         // TODO figure out token business
         auto fieldName = value.GetField().GetQualifiedFieldName();
         auto valuePtr = auxProcessor->GetEntry().GetPtr<void>(fieldName);
         newEntry->BindValue(auxProcessor->GetProcessorName() + "." + fieldName, valuePtr);
      }
   }

   fEntry.swap(newEntry);

   // If no join fields have been specified, an aligned join is assumed and an index won't be necessary.
   // TODO !!!!!!!!!!!
   if (joinFields.size() > 0)
      throw RException(R__FAIL("unaligned joins are temporarily disabled."));
}

void ROOT::Experimental::RNTupleJoinProcessor::SetEntryPointers(const REntry &entry, std::string_view fieldNamePrefix)
{
   for (const auto &value : *fEntry) {
      std::string fieldName = value.GetField().GetQualifiedFieldName();
      auto valuePtr = fieldNamePrefix.empty() ? entry.GetPtr<void>(fieldName)
                                              : entry.GetPtr<void>(std::string(fieldNamePrefix) + "." + fieldName);

      fEntry->BindValue(fieldName, valuePtr);
   }

   fMainProcessor->SetEntryPointers(*fEntry);
   for (auto &auxProc : fAuxiliaryProcessors) {
      auxProc->SetEntryPointers(*fEntry, auxProc->GetProcessorName());
   }
}

ROOT::Experimental::NTupleSize_t ROOT::Experimental::RNTupleJoinProcessor::LoadEntry(NTupleSize_t entryNumber)
{
   // TODO aligned case below here
   if (entryNumber >= fNEntries)
      return kInvalidNTupleIndex;

   fMainProcessor->LoadEntry(entryNumber);

   if (!IsUsingIndex()) {
      for (auto &auxProcessor : fAuxiliaryProcessors) {
         auxProcessor->LoadEntry(entryNumber);
      }
   } else {
      throw RException(R__FAIL("unaligned joins are temporarily disabled."));
   }

   fCurrentEntryNumber = entryNumber;
   fNEntriesProcessed++;

   return entryNumber;
}
