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

#include <TROOT.h>
#ifdef R__USE_IMT
#include <ROOT/TThreadExecutor.hxx>
#endif

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

//------------------------------------------------------------------------------

std::unique_ptr<ROOT::Experimental::Internal::RNTupleIndex>
ROOT::Experimental::Internal::CreateRNTupleIndex(std::string_view fieldName, RPageSource &pageSource)
{
   auto desc = pageSource.GetSharedDescriptorGuard();
   const auto &fieldDesc = desc->GetFieldDescriptor(desc->FindFieldId(fieldName));
   auto fieldOrException = RFieldBase::Create(fieldDesc.GetFieldName(), fieldDesc.GetTypeName());
   if (!fieldOrException) {
      throw RException(R__FAIL("could not create field \"" + std::string(fieldName) + "\""));
   }
   auto field = fieldOrException.Unwrap();
   field->SetOnDiskId(fieldDesc.GetId());
   CallConnectPageSourceOnField(*field, pageSource);

   auto fnMakeIndex = [&field](std::pair<std::uint64_t, std::uint64_t> range) -> RNTupleIndex {
      RNTupleIndex partialIndex(field->Clone(field->GetFieldName()));
      auto value = field->CreateValue();

      for (std::uint64_t i = range.first; i < range.second; ++i) {
         value.Read(i);
         auto ptr = value.GetPtr<void>();
         partialIndex.Add(ptr.get(), i);
      }

      return partialIndex;
   };

#ifdef R__USE_IMT
   if (ROOT::IsImplicitMTEnabled()) {
      std::vector<std::pair<std::uint64_t, std::uint64_t>> ranges;

      {
         auto descriptorGuard = pageSource.GetSharedDescriptorGuard();
         const std::size_t nClusters = descriptorGuard->GetNClusters();
         std::size_t globalClusterId = 0;
         std::size_t rangeStart = 0;

         for (std::size_t c = 0; c < nClusters; ++c) {
            const auto &clusterDesc = descriptorGuard->GetClusterDescriptor(globalClusterId++);
            ranges.emplace_back(rangeStart, rangeStart + clusterDesc.GetNEntries());
            rangeStart += clusterDesc.GetNEntries();
         }
      }

      auto reducePartialIndices = [&field, &fieldName](const std::vector<RNTupleIndex> indices) -> RNTupleIndex {
         return std::accumulate(indices.begin(), indices.end(), RNTupleIndex(field->Clone(fieldName)),
                                RNTupleIndex::Concatenate);
      };

      auto index = ROOT::TThreadExecutor{}.MapReduce(fnMakeIndex, ranges, reducePartialIndices);

      return std::make_unique<RNTupleIndex>(index);
   }
#endif

   auto index = fnMakeIndex(std::pair<std::uint64_t, std::uint64_t>{0, pageSource.GetNEntries()});
   return std::unique_ptr<RNTupleIndex>(new RNTupleIndex(index));
}
