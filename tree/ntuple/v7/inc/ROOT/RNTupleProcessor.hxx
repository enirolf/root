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

namespace Internal {
class RNTupleProcessorContext {
private:
   struct RFieldContext {
      std::unique_ptr<ROOT::Experimental::RFieldBase> fProtoField;
      std::unique_ptr<ROOT::Experimental::RFieldBase> fConcreteField;
      REntry::RFieldToken fToken;
      std::shared_ptr<void> fValuePtr;

      RFieldContext(std::unique_ptr<ROOT::Experimental::RFieldBase> protoField, REntry::RFieldToken token)
         : fProtoField(std::move(protoField)), fToken(token)
      {
      }

      void ResetConcreteField() { fConcreteField = nullptr; }
      void SetConcreteField() { fConcreteField = fProtoField->Clone(fProtoField->GetFieldName()); }
   };

   std::unique_ptr<RNTupleModel> fModel;
   std::unique_ptr<REntry> fEntry; ///< The entry is based on the first page source
   std::unique_ptr<Internal::RPageSource> fPageSource;
   std::vector<RFieldContext> fFieldContexts;

public:
   RNTupleProcessorContext(const RNTupleSourceSpec &ntuple);

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Initializes the processor context
   ///
   /// Recursively creates so-called 'protofields' based on the model of the first page source, which will serve as
   /// a basis for all subsequent RNTuples to be processed.
   void Initialize();

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Updates the processor context
   ///
   /// \param[in] ntuple The RNTuple to connect for processing
   ///
   /// \return The number of entries in the newly-connected RNTuple
   ///
   /// Creates and attaches new page source for the specified RNTuple, and connects the fields that are known by
   /// the processor context to it.
   NTupleSize_t Update(const RNTupleSourceSpec &ntuple);

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Creates and connects concrete fields to the current page source, based on the protofields
   void ConnectFields();

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Loads an entry
   ///
   /// \param[in] index The index (local to the current RNTuple) of the entry to load.
   void LoadEntry(NTupleSize_t index) { fEntry->Read(index); }

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Returns a reference to the entry in this context
   ///
   /// \return A reference to the entry from this context
   ///
   const REntry &GetEntry() const { return *fEntry; }

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Returns a reference to the page source currently used in this context
   ///
   /// \return A reference to the page source
   Internal::RPageSource &GetPageSource() const { return *fPageSource; }
};
} // namespace Internal

class RNTupleProcessor {
private:
   const std::vector<RNTupleSourceSpec> &fNTuples;
   std::unique_ptr<REntry> fEntry; ///< The entry is based on the first page source
   std::unique_ptr<Internal::RNTupleProcessorContext> fContext;

public:
   class RIterator {
   private:
      Internal::RNTupleProcessorContext &fContext;
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

      RIterator(Internal::RNTupleProcessorContext &context, const std::vector<RNTupleSourceSpec> &ntuples,
                std::size_t ntupleIndex, NTupleSize_t globalEntryIndex)
         : fContext(context),
           fNTuples(ntuples),
           fNTupleIndex(ntupleIndex),
           fGlobalEntryIndex(globalEntryIndex),
           fLocalEntryIndex(0)
      {
      }

      void Advance()
      {
         ++fGlobalEntryIndex;

         if (++fLocalEntryIndex >= fContext.GetPageSource().GetNEntries()) {
            do {
               if (++fNTupleIndex >= fNTuples.size()) {
                  fGlobalEntryIndex = kInvalidNTupleIndex;
                  return;
               }
            } while (fContext.Update(fNTuples.at(fNTupleIndex)) == 0); // Skip over any empty ntuples we encounter

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
         if (fContext.GetPageSource().GetNEntries() == 0)
            Advance();

         fContext.LoadEntry(fLocalEntryIndex);
         return fContext.GetEntry();
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

   RIterator begin() { return RIterator(*fContext, fNTuples, 0, 0); }
   RIterator end() { return RIterator(*fContext, fNTuples, fNTuples.size(), kInvalidNTupleIndex); }
};

} // namespace Experimental
} // namespace ROOT

#endif // ROOT7_RNTupleProcessor
