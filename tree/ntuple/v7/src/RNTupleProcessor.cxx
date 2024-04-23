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

#include <ROOT/RField.hxx>

ROOT::Experimental::Internal::RNTupleProcessorContext::RNTupleProcessorContext(const RNTupleSourceSpec &ntuple)
{
   fPageSource = RPageSource::Create(ntuple.fName, ntuple.fStorage);
   fPageSource->Attach();
   fModel = fPageSource->GetSharedDescriptorGuard()->CreateModel();
   fEntry = fModel->CreateBareEntry();
}

void ROOT::Experimental::Internal::RNTupleProcessorContext::Initialize()
{
   fPageSource->Attach();
   auto desc = fPageSource->GetSharedDescriptorGuard();

   auto fnSetProcessorFields = [&](DescriptorId_t fieldId, auto &setFunc) -> void {
      auto &fieldDesc = desc->GetFieldDescriptor(fieldId);

      auto fieldOrException = RFieldBase::Create(fieldDesc.GetFieldName(), fieldDesc.GetTypeName());
      if (fieldOrException) {
         auto field = fieldOrException.Unwrap();
         field->SetOnDiskId(fieldId);
         fFieldContexts.emplace_back(std::move(field), fEntry->GetToken(fieldDesc.GetFieldName()));
      }

      for (const auto &subFieldDesc : desc->GetFieldIterable(fieldId)) {
         setFunc(subFieldDesc.GetId(), setFunc);
      }
   };

   fnSetProcessorFields(desc->GetFieldZeroId(), fnSetProcessorFields);
   ConnectFields();
}

ROOT::Experimental::NTupleSize_t
ROOT::Experimental::Internal::RNTupleProcessorContext::Update(const RNTupleSourceSpec &ntuple)
{
   for (auto &field : fFieldContexts) {
      field.ResetConcreteField();
   }
   fPageSource = RPageSource::Create(ntuple.fName, ntuple.fStorage);
   fPageSource->Attach();
   ConnectFields();
   return fPageSource->GetNEntries();
}

void ROOT::Experimental::Internal::RNTupleProcessorContext::ConnectFields()
{
   auto desc = fPageSource->GetSharedDescriptorGuard();
   for (auto &field : fFieldContexts) {
      field.SetConcreteField();

      field.fConcreteField->SetOnDiskId(desc->FindFieldId(field.fProtoField->GetFieldName()));
      Internal::CallConnectPageSourceOnField(*field.fConcreteField, *fPageSource);

      if (field.fValuePtr) {
         fEntry->UpdateValue(field.fToken, field.fConcreteField->BindValue(field.fValuePtr));
      } else {
         auto value = field.fConcreteField->CreateValue();
         field.fValuePtr = value.GetPtr<void>();
         fEntry->UpdateValue(field.fToken, value);
      }
   }
}

ROOT::Experimental::RNTupleProcessor::RNTupleProcessor(const std::vector<RNTupleSourceSpec> &ntuples)
   : fNTuples(ntuples)
{
   if (fNTuples.empty())
      throw RException(R__FAIL("at least one RNTuple must be provided"));

   fContext = std::make_unique<Internal::RNTupleProcessorContext>(fNTuples[0]);
   fContext->Initialize();
}
