/// \file ROOT/RNTupleProcessor.hxx
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

#ifndef ROOT7_RNTupleProcessor
#define ROOT7_RNTupleProcessor

#include <ROOT/REntry.hxx>
#include <ROOT/RError.hxx>
#include <ROOT/RNTupleDescriptor.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleUtil.hxx>
#include <ROOT/RPageStorage.hxx>
#include <ROOT/RSpan.hxx>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ROOT {
namespace Experimental {

struct RNTupleSourceSpec {
   std::string fName;
   std::string fStorage;

   RNTupleSourceSpec() = default;
   RNTupleSourceSpec(std::string_view n, std::string_view s) : fName(n), fStorage(s) {}
};

class RNTupleProcessor {
private:
   class RFieldContext {
   private:
      std::unique_ptr<RFieldBase> fProtoField;
      std::unique_ptr<RFieldBase> fConcreteField;
      REntry::RFieldToken fToken;
      std::shared_ptr<void> fValuePtr;

   public:
      RFieldContext(std::unique_ptr<RFieldBase> protoField, REntry::RFieldToken token)
         : fProtoField(std::move(protoField)), fToken(token)
      {
      }

      RFieldBase &GetProtoField() const { return *fProtoField; }
      void ResetConcreteField() { fConcreteField = nullptr; }
      void SetConcreteField() { fConcreteField = fProtoField->Clone(fProtoField->GetFieldName()); }
      RFieldBase &GetConcreteField() const { return *fConcreteField; }
      const REntry::RFieldToken &GetToken() const { return fToken; }
      std::shared_ptr<void> GetValuePtr() const { return fValuePtr; }
      void SetValuePtr(std::shared_ptr<void> ptr) { fValuePtr = ptr; }
   };

   const std::vector<RNTupleSourceSpec> &fNTuples;
   std::unique_ptr<RNTupleModel> fModel;
   std::unique_ptr<REntry> fEntry; ///< The entry is based on the first page source
   std::unique_ptr<Internal::RPageSource> fPageSource;
   std::vector<RFieldContext> fFieldContexts;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Connect an RNTuple for processing
   ///
   /// \param[in] ntuple The RNTupleSourceSpec describing the RNTuple to connect
   ///
   /// \return The number of entries in the newly-connected RNTuple
   ///
   /// Creates and attaches new page source for the specified RNTuple, and connects the fields that are known by
   /// the processor to it.
   NTupleSize_t ConnectNTuple(const RNTupleSourceSpec &ntuple);

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Creates and connects concrete fields to the current page source, based on the proto-fields
   void ConnectFields();

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Loads an entry
   ///
   /// \param[in] index The index (local to the current RNTuple) of the entry to load.
   void LoadEntry(NTupleSize_t index) { fEntry->Read(index); }

public:
   /////////////////////////////////////////////////////////////////////////////
   /// \brief Returns a reference to the entry used by the processor
   ///
   /// \return A reference to the entry used by the processor
   ///
   const REntry &GetEntry() const { return *fEntry; }

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Returns a reference to the page source currently used in the processor
   ///
   /// \return A reference to the page source
   Internal::RPageSource &GetPageSource() const { return *fPageSource; }

   class RIterator {
   private:
      RNTupleProcessor &fProcessor;
      const std::vector<RNTupleSourceSpec> &fNTuples;
      std::size_t fNTupleIndex;
      NTupleSize_t fGlobalEntryIndex;
      NTupleSize_t fLocalEntryIndex;

   public:
      using iterator_category = std::forward_iterator_tag;
      using iterator = RIterator;
      using value_type = REntry;
      using difference_type = std::ptrdiff_t;
      using pointer = REntry *;
      using reference = const REntry &;

      RIterator(RNTupleProcessor &processor, const std::vector<RNTupleSourceSpec> &ntuples, std::size_t ntupleIndex,
                NTupleSize_t globalEntryIndex)
         : fProcessor(processor),
           fNTuples(ntuples),
           fNTupleIndex(ntupleIndex),
           fGlobalEntryIndex(globalEntryIndex),
           fLocalEntryIndex(0)
      {
      }

      void Advance()
      {
         ++fGlobalEntryIndex;

         if (++fLocalEntryIndex >= fProcessor.GetPageSource().GetNEntries()) {
            do {
               if (++fNTupleIndex >= fNTuples.size()) {
                  fGlobalEntryIndex = kInvalidNTupleIndex;
                  return;
               }
            } while (fProcessor.ConnectNTuple(fNTuples.at(fNTupleIndex)) ==
                     0); // Skip over any empty ntuples we encounter

            fLocalEntryIndex = 0;
         }
      }

      iterator operator++()
      {
         Advance();
         return *this;
      }

      reference operator*()
      {
         if (fProcessor.GetPageSource().GetNEntries() == 0)
            Advance();

         fProcessor.LoadEntry(fLocalEntryIndex);
         return fProcessor.GetEntry();
      }
      bool operator!=(const iterator &rh) const { return fGlobalEntryIndex != rh.fGlobalEntryIndex; }
      bool operator==(const iterator &rh) const { return fGlobalEntryIndex == rh.fGlobalEntryIndex; }
   };

   RNTupleProcessor(const RNTupleProcessor &other) = delete;
   RNTupleProcessor &operator=(const RNTupleProcessor &other) = delete;
   RNTupleProcessor(RNTupleProcessor &&other) = delete;
   RNTupleProcessor &operator=(RNTupleProcessor &&other) = delete;
   ~RNTupleProcessor() = default;

   RNTupleProcessor(const std::vector<RNTupleSourceSpec> &ntuples);

   RIterator begin() { return RIterator(*this, fNTuples, 0, 0); }
   RIterator end() { return RIterator(*this, fNTuples, fNTuples.size(), kInvalidNTupleIndex); }
};

} // namespace Experimental
} // namespace ROOT

#endif // ROOT7_RNTupleProcessor
