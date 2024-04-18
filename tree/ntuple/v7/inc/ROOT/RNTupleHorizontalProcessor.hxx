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

#ifndef ROOT7_RNTupleHorizontalProcessor
#define ROOT7_RNTupleHorizontalProcessor

#include <ROOT/REntry.hxx>
#include <ROOT/RError.hxx>
#include <ROOT/RNTupleDescriptor.hxx>
#include <ROOT/RNTupleIndex.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleProcessor.hxx>
#include <ROOT/RNTupleUtil.hxx>
#include <ROOT/RPageStorage.hxx>
#include <ROOT/RSpan.hxx>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ROOT {
namespace Experimental {
struct RNTupleSourceSpec;
}
} // namespace ROOT

template <>
struct std::hash<ROOT::Experimental::RNTupleSourceSpec> {
   std::size_t operator()(const ROOT::Experimental::RNTupleSourceSpec &s) const noexcept
   {
      std::size_t h1 = std::hash<std::string>{}(s.fName);
      std::size_t h2 = std::hash<std::string>{}(s.fStorage);
      return h1 ^ (h2 << 1);
   }
};

namespace ROOT {
namespace Experimental {
class RNTupleHorizontalProcessor {
private:
   class RFieldContext {
   private:
      std::unique_ptr<RFieldBase> fField;
      Internal::RPageSource &fPageSource;
      REntry::RFieldToken fToken;
      std::shared_ptr<void> fValuePtr;

   public:
      RFieldContext(std::unique_ptr<ROOT::Experimental::RFieldBase> field, Internal::RPageSource &pageSource,
                    REntry::RFieldToken token)
         : fField(std::move(field)), fPageSource(pageSource), fToken(token)
      {
      }

      RFieldBase &GetField() const { return *fField; }
      Internal::RPageSource &GetPageSource() const { return fPageSource; }
      const REntry::RFieldToken &GetToken() const { return fToken; }
      std::shared_ptr<void> GetValuePtr() const { return fValuePtr; }
      void SetValuePtr(std::shared_ptr<void> ptr) { fValuePtr = ptr; }
   };

   const std::vector<RNTupleSourceSpec> &fNTuples;
   std::unique_ptr<RNTupleModel> fModel;
   std::unique_ptr<REntry> fEntry; ///< The entry is based on the first page source
   std::unique_ptr<Internal::RPageSource> fPrimaryPageSource;
   std::vector<std::unique_ptr<Internal::RPageSource>> fSecondaryPageSources;
   std::vector<RFieldContext> fFieldContexts;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Make sure none of the horizontally concatenated RNTuples bear the same name
   ///
   /// If two or more RNTuples have the same name, an RException is raised.
   void EnsureUniqueNTupleNames();

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Connect an RNTuple for processing.
   ///
   /// \param[in] ntuple The RNTupleSourceSpec describing the RNTuple to connect.
   ///
   /// \return The number of entries in the newly-connected RNTuple.
   ///
   /// Creates and attaches new page source for the specified RNTuple, and connects the fields that are known by
   /// the processor to it.
   NTupleSize_t ConnectNTuples(const RNTupleSourceSpec &ntuple);

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Creates and connects concrete fields to the current page source, based on the protofields
   void ConnectFields();

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Loads an entry.
   ///
   /// \param[in] index The index (local to the current RNTuple) of the entry to load.
   void LoadEntry(NTupleSize_t index) { fEntry->Read(index); }

public:
   RNTupleHorizontalProcessor(const RNTupleHorizontalProcessor &other) = delete;
   RNTupleHorizontalProcessor &operator=(const RNTupleHorizontalProcessor &other) = delete;
   RNTupleHorizontalProcessor(RNTupleHorizontalProcessor &&other) = delete;
   RNTupleHorizontalProcessor &operator=(RNTupleHorizontalProcessor &&other) = delete;
   ~RNTupleHorizontalProcessor() = default;

   RNTupleHorizontalProcessor(const std::vector<RNTupleSourceSpec> &ntuples);

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Returns a reference to the entry used by the processor.
   ///
   /// \return A reference to the entry used by the processor.
   const REntry &GetEntry() const { return *fEntry; }

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Returns a reference to the model used by the processor.
   ///
   /// \return A reference to the model used by the processor.
   const RNTupleModel &GetModel() const { return *fModel; }

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Returns a reference to the primary page source.
   ///
   /// \return A reference to the primary page source.
   Internal::RPageSource &GetPrimaryPageSource() const { return *fPrimaryPageSource; }

   class RIterator {
   private:
      RNTupleHorizontalProcessor &fProcessor;
      const std::vector<RNTupleSourceSpec> &fNTuples;
      NTupleSize_t fEntryIndex;

   public:
      using iterator_category = std::forward_iterator_tag;
      using iterator = RIterator;
      using value_type = REntry;
      using difference_type = std::ptrdiff_t;
      using pointer = REntry *;
      using reference = const REntry &;

      RIterator(RNTupleHorizontalProcessor &processor, const std::vector<RNTupleSourceSpec> &ntuples,
                std::size_t entryIndex)
         : fProcessor(processor), fNTuples(ntuples), fEntryIndex(entryIndex)
      {
      }

      void Advance()
      {
         if (++fEntryIndex >= fProcessor.GetPrimaryPageSource().GetNEntries())
            fEntryIndex = kInvalidNTupleIndex;
      }

      iterator operator++()
      {
         Advance();
         return *this;
      }

      iterator operator++(int)
      {
         auto obj = *this;
         Advance();
         return obj;
      }

      reference operator*()
      {
         if (fProcessor.GetPrimaryPageSource().GetNEntries() == 0)
            Advance();

         fProcessor.LoadEntry(fEntryIndex);
         return fProcessor.GetEntry();
      }
      bool operator!=(const iterator &rh) const { return fEntryIndex != rh.fEntryIndex; }
      bool operator==(const iterator &rh) const { return fEntryIndex == rh.fEntryIndex; }
   };

   RIterator begin() { return RIterator(*this, fNTuples, 0); }
   RIterator end() { return RIterator(*this, fNTuples, kInvalidNTupleIndex); }
};

} // namespace Experimental
} // namespace ROOT

#endif // ROOT7_RNTupleHorizontalProcessor
