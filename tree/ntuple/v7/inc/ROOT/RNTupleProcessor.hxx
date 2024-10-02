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
#include <unordered_map>
#include <vector>

namespace ROOT {
namespace Experimental {
namespace Internal {
class RNTupleProcessor;
class RNTupleChainProcessor;

/// Helper type representing the name and storage location of an RNTuple.
struct RNTupleSourceSpec {
   std::string fName;
   std::string fLocation;

   RNTupleSourceSpec() = default;
   RNTupleSourceSpec(std::string_view n, std::string_view s) : fName(n), fLocation(s) {}
};

class RNTupleProcessorIterator;

class RNTupleProcessorIteratorHelperBase {
   friend class RNTupleProcessorIterator;

public:
   // clang-format off
   /**
   \class ROOT::Experimental::RNTupleProcessor::RIterator::RProcessorState
   \ingroup NTuple
   \brief View on the RNTupleProcessor iterator state.
   */
   // clang-format on
   class RProcessorState {
      // friend class RIterator;
      // friend class RChainIterator;

   public:
      const REntry &fEntry;
      NTupleSize_t fGlobalEntryIndex;
      NTupleSize_t fLocalEntryIndex;
      /// Index of the currently open RNTuple in the chain of ntuples
      std::size_t fNTupleIndex;

      RProcessorState(const REntry &entry, NTupleSize_t globalEntryIndex, NTupleSize_t localEntryIndex,
                      std::size_t ntupleIndex)
         : fEntry(entry),
           fGlobalEntryIndex(globalEntryIndex),
           fLocalEntryIndex(localEntryIndex),
           fNTupleIndex(ntupleIndex)
      {
      }
   };

protected:
   RProcessorState fState;

public:
   RNTupleProcessorIteratorHelperBase(REntry &entry, NTupleSize_t globalEntryIndex, std::size_t ntupleIndex)
      : fState(entry, globalEntryIndex, 0, ntupleIndex)
   {
   }
   virtual ~RNTupleProcessorIteratorHelperBase() = default;
   virtual RProcessorState &operator*() = 0;
   virtual void incr() = 0;
   // iterator operator++(int) { assert(false); }
};

template <typename ProcessorT>
class RNTupleProcessorIteratorHelper : public RNTupleProcessorIteratorHelperBase {
private:
   ProcessorT &fProcessor;

public:
   RNTupleProcessorIteratorHelper(ProcessorT &processor, NTupleSize_t globalEntryIndex, std::size_t ntupleIndex)
      : RNTupleProcessorIteratorHelperBase(processor.GetEntry(), globalEntryIndex, ntupleIndex), fProcessor(processor)
   {
   }

   RProcessorState &operator*() final
   {
      std::cout << "Hi" << std::endl;
      return fState;
   }

   void incr() final { std::cout << "Incr!" << std::endl; }
};

// clang-format off
/**
\class ROOT::Experimental::RNTupleProcessor::RIterator
\ingroup NTuple
\brief Iterator over the entries of an RNTuple, or vertical concatenation thereof.
*/
// clang-format on
class RNTupleProcessorIterator {
private:
   std::unique_ptr<RNTupleProcessorIteratorHelperBase> fIteratorHelper;

public:
   using iterator_category = std::forward_iterator_tag;
   using iterator = RNTupleProcessorIterator;
   using value_type = RNTupleProcessorIteratorHelperBase::RProcessorState;
   using difference_type = std::ptrdiff_t;
   using pointer = RNTupleProcessorIteratorHelperBase::RProcessorState *;
   using reference = const RNTupleProcessorIteratorHelperBase::RProcessorState &;

   template <typename ProcessorT>
   RNTupleProcessorIterator(ProcessorT &processor, NTupleSize_t globalEntryIndex, std::size_t ntupleIndex)
   {
      fIteratorHelper =
         std::make_unique<RNTupleProcessorIteratorHelper<ProcessorT>>(processor, globalEntryIndex, ntupleIndex);
   }

   void LoadEntry() { assert(false); }
   void Advance() { assert(false); }

   iterator &operator++()
   {
      fIteratorHelper->incr();
      return *this;
   }
   // iterator operator++(int) { auto iter = *fIteratorHelper; iter.incr(); return iter; }
   reference operator*() { return fIteratorHelper->fState; }
};

// clang-format off
/**
\class ROOT::Experimental::RNTupleProcessor
\ingroup NTuple
\brief Interface for iterating over entries of RNTuples and vertically concatenated RNTuples (chains).

Example usage (see ntpl012_processor.C for a full example):

~~~{.cpp}
#include <ROOT/RNTupleProcessor.hxx>
using ROOT::Experimental::RNTupleProcessor;
using ROOT::Experimental::RNTupleSourceSpec;

std::vector<RNTupleSourceSpec> ntuples = {{"ntuple1", "ntuple1.root"}, {"ntuple2", "ntuple2.root"}};
RNTupleProcessor processor(ntuples);
auto ptrPt = processor.GetEntry().GetPtr<float>("pt");

for (const auto &entry : processor) {
   std::cout << "pt = " << *ptrPt << std::endl;
}
~~~

An RNTupleProcessor is created by providing one or more RNTupleSourceSpecs, each of which contains the name and storage
location of a single RNTuple. The RNTuples are processed in the order in which they were provided.

The RNTupleProcessor constructor also (optionally) accepts an RNTupleModel, which determines which fields should be
read. If no model is provided, a default model based on the descriptor of the first specified RNTuple will be used.
If a field that was present in the first RNTuple is not found in a subsequent one, an error will be thrown.

The object returned by the RNTupleProcessor iterator is a view on the current state of the processor, and provides
access to the global entry index (i.e., the entry index taking into account all processed ntuples), local entry index
(i.e. the entry index for only the currently processed ntuple), the index of the ntuple currently being processed (with
respect to the order of provided RNTupleSpecs) and the actual REntry containing the values for the current entry.
*/
// clang-format on
class RNTupleProcessor {
protected:
   // clang-format off
   /**
   \class ROOT::Experimental::RNTupleProcessor::RFieldContext
   \ingroup NTuple
   \brief Manager for a field as part of the RNTupleProcessor.

   An RFieldContext contains two fields: a proto-field which is not connected to any page source but serves as the
   blueprint for this particular field, and a concrete field that is connected to the page source currently connected
   to the RNTupleProcessor for reading. When a new page source is connected, the current concrete field gets reset. A
   new concrete field that is connected to this new page source is subsequently created from the proto-field.
   */
   // clang-format on
   class RFieldContext {
      friend class RNTupleProcessor;
      friend class RNTupleChainProcessor;
      friend class RNTupleJoinProcessor;

   private:
      std::unique_ptr<RFieldBase> fProtoField;
      std::unique_ptr<RFieldBase> fConcreteField;
      REntry::RFieldToken fToken;
      std::string fNTupleName; //< Only set for auxiliary fields
      bool fIsAuxiliary = false;

   public:
      RFieldContext(std::unique_ptr<RFieldBase> protoField, REntry::RFieldToken token, std::string_view ntupleName = "",
                    bool isAuxiliary = false)
         : fProtoField(std::move(protoField)), fToken(token), fNTupleName(ntupleName), fIsAuxiliary(isAuxiliary)
      {
      }

      const RFieldBase &GetProtoField() const { return *fProtoField; }
      /// We need to disconnect the concrete fields before swapping the page sources
      void ResetConcreteField() { fConcreteField.reset(); }
      void SetConcreteField() { fConcreteField = fProtoField->Clone(fProtoField->GetFieldName()); }
   };

   std::vector<RNTupleSourceSpec> fNTuples;
   std::unique_ptr<REntry> fEntry;
   std::unique_ptr<Internal::RPageSource> fPageSource;
   std::unordered_map<std::string, RFieldContext> fFieldContexts;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Connect an RNTuple for processing.
   ///
   /// \param[in] ntuple The RNTupleSourceSpec describing the RNTuple to connect.
   ///
   /// \return The number of entries in the newly-connected RNTuple.
   ///
   /// Creates and attaches new page source for the specified RNTuple, and connects the fields that are known by
   /// the processor to it.
   virtual NTupleSize_t ConnectNTuple(const RNTupleSourceSpec &ntuple) = 0;

   RNTupleProcessor(const std::vector<RNTupleSourceSpec> &ntuples) : fNTuples(ntuples) {}

public:
   RNTupleProcessor(const RNTupleProcessor &) = delete;
   RNTupleProcessor(RNTupleProcessor &&) = delete;
   RNTupleProcessor &operator=(const RNTupleProcessor &) = delete;
   RNTupleProcessor &operator=(RNTupleProcessor &&) = delete;
   virtual ~RNTupleProcessor() = default;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Returns a reference to the entry used by the processor.
   ///
   /// \return A reference to the entry used by the processor.
   ///
   const REntry &GetEntry() const { return *fEntry; }

   virtual RNTupleProcessorIterator begin() = 0;
   virtual RNTupleProcessorIterator end() = 0;
   // RNTupleProcessorIterator begin() {
   //    return RNTupleProcessorIterator<RNTupleProcessor>(*this, 0, 0);
   // }

   // RNTupleProcessorIterator end() {
   //    return RNTupleProcessorIterator<RNTupleProcessor>(*this, kInvalidNTupleIndex, kInvalidNTupleIndex);
   // }

   static std::unique_ptr<RNTupleProcessor>
   CreateChain(const std::vector<RNTupleSourceSpec> &ntuples, std::unique_ptr<RNTupleModel> model = nullptr);

   static std::unique_ptr<RNTupleProcessor> CreateJoin(const std::vector<RNTupleSourceSpec> &ntuples);
};

class RNTupleChainProcessor : public RNTupleProcessor {
private:
   NTupleSize_t ConnectNTuple(const RNTupleSourceSpec &ntuple) final;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Create and connect a concrete field to the current page source, based on the corresponding proto-field.
   void ConnectField(RFieldContext &fieldContext);

public:
   /////////////////////////////////////////////////////////////////////////////
   /// \brief Constructs a new RNTupleChainProcessor.
   ///
   /// \param[in] ntuples The source specification (name and storage location) for each RNTuple to process.
   /// \param[in] model The model that specifies which fields should be read by the processor. The pointer returned by
   /// RNTupleModel::MakeField can be used to access a field's value during the processor iteration. When no model is
   /// specified, it is created from the descriptor of the first RNTuple specified in `ntuples`.
   ///
   /// RNTuples are processed in the order in which they are specified.
   RNTupleChainProcessor(const std::vector<RNTupleSourceSpec> &ntuples, std::unique_ptr<RNTupleModel> model = nullptr);

   RNTupleProcessorIterator begin() { return RNTupleProcessorIterator<RNTupleChainProcessor>(*this, 0, 0); }
   RNTupleProcessorIterator end()
   {
      return RNTupleProcessorIterator<RNTupleChainProcessor>(*this, kInvalidNTupleIndex, fNTuples.size());
   }
};

class RNTupleJoinProcessor : public RNTupleProcessor {
private:
   std::unordered_map<std::string, std::unique_ptr<Internal::RPageSource>> fAuxPageSources;

   std::unique_ptr<RNTupleModel> fJoinModel;
   std::unordered_map<std::string, std::unique_ptr<REntry>> fAuxEntries;

   bool fUseIndex = false;

   NTupleSize_t ConnectNTuple(const RNTupleSourceSpec & /* ntuple */) final
   {
      // NOOP
      return kInvalidNTupleIndex;
   }

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Create and connect a concrete field to the provided page source, based on the corresponding proto-field.
   void ConnectField(RFieldContext &fieldContext, RPageSource &pageSource, REntry &entry);

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Loads an entry.
   ///
   /// \param[in] idx The index (local to the current RNTuple) of the entry to load.
   void LoadEntry(NTupleSize_t idx)
   {
      // std::vector<void *> valPtrs;
      // valPtrs.reserve(fIndexFieldNames.size());
      // for (const auto &fieldName : fIndexFieldNames) {
      //    auto &fieldContext = fFieldContexts.at(fieldName);
      //    fieldContext.fValue->Read(idx);
      //    valPtrs.push_back(fieldContext.fValue->GetPtr<void>().get());
      // }

      fEntry->Read(idx);

      for (auto &[ntupleName, entry] : fAuxEntries) {
         entry->Read(idx);
      }

      // for (auto &fieldContext : fFieldContexts) {

      //    if (!fieldContext.fIsAuxiliary || !fUseIndex) {
      //       fieldContext.fValue->Read(idx);
      //       continue;
      //    }

      //    auto ntupleName = fieldContext.fPageSource.GetNTupleName();
      //    auto joinIndex = fJoinIndices.find(ntupleName);
      //    if (joinIndex != fJoinIndices.end()) {
      //       auto joinIdx = joinIndex->second->GetFirstEntryNumber(valPtrs);

      //       if (joinIdx == kInvalidNTupleIndex) {
      //          fieldContext.fProtoField->ConstructValue(fieldContext.fValue->GetPtr<void>().get());
      //       } else {
      //          fieldContext.fValue->Read(joinIdx);
      //       }
      //    }
      // }
   }

   template <typename T>
   std::shared_ptr<T> GetPtr(std::string_view fieldName)
   {
      auto &field = fFieldContexts.at(std::string(fieldName));
      auto &entry = field.fIsAuxiliary ? fAuxEntries.at(field.fNTupleName) : fEntry;
      return entry->GetPtr<T>(field.fToken);
   }

public:
   RNTupleJoinProcessor(const RNTupleSourceSpec &mainNTuple, std::unique_ptr<RNTupleModel> model = nullptr);

   void AddAuxiliary(const RNTupleSourceSpec &auxNTuple, std::unique_ptr<RNTupleModel> model = nullptr);

   RPageSource &GetPrimaryPageSource() const { return *fPageSource; }

   // class RJoinIterator : public RNTupleProcessor::RIterator {
   // public:
   //    class RProcessorState {
   //       friend class RJoinIterator;

   //    private:
   //       RNTupleJoinProcessor &fProcessor;
   //       NTupleSize_t fEntryIndex;

   //       void UpdateEntry() { fProcessor.LoadEntry(fEntryIndex); }

   //    public:
   //       RProcessorState(RNTupleJoinProcessor &processor, NTupleSize_t entryIndex)
   //          : fProcessor(processor), fEntryIndex(entryIndex)
   //       {
   //       }

   //       NTupleSize_t GetEntryIndex() const { return fEntryIndex; }

   //       template <typename T>
   //       std::shared_ptr<T> GetPtr(std::string_view fieldName)
   //       {
   //          // if (!fProcessor.HasField(fieldName)) {
   //          //    fProcessor.ActivateField(fieldName);
   //          //    UpdateEntry();
   //          // }
   //          return fProcessor.GetPtr<T>(fieldName);
   //       }
   //    };

   // private:
   //    RNTupleJoinProcessor &fProcessor;
   //    RProcessorState fState;

   // public:
   //    using iterator_category = std::forward_iterator_tag;
   //    using iterator = RJoinIterator;
   //    using value_type = RProcessorState;
   //    using difference_type = std::ptrdiff_t;
   //    using pointer = RProcessorState *;
   //    using reference = RProcessorState &;

   //    RJoinIterator(RNTupleJoinProcessor &processor, std::size_t entryIndex)
   //       : RIterator(), fProcessor(processor), fState(processor, entryIndex)
   //    {
   //    }

   //    void Advance()
   //    {
   //       if (++fState.fEntryIndex >= fProcessor.GetPrimaryPageSource().GetNEntries())
   //          fState.fEntryIndex = kInvalidNTupleIndex;
   //    }

   //    iterator operator++()
   //    {
   //       Advance();
   //       return *this;
   //    }

   //    iterator operator++(int)
   //    {
   //       auto obj = *this;
   //       Advance();
   //       return obj;
   //    }

   //    reference operator*()
   //    {
   //       if (fProcessor.GetPrimaryPageSource().GetNEntries() == 0)
   //          Advance();

   //       fState.UpdateEntry();
   //       return fState;
   //    }

   //    pointer operator->()
   //    {
   //       fState.UpdateEntry();
   //       return &fState;
   //    }
   //    bool operator!=(const iterator &rh) const { return fState.fEntryIndex != rh.fState.fEntryIndex; }
   //    bool operator==(const iterator &rh) const { return fState.fEntryIndex == rh.fState.fEntryIndex; }
   // };

   // RIterator begin() final { return RJoinIterator(*this, 0); }
   // RIterator end() final { return RJoinIterator(*this, kInvalidNTupleIndex); }
};

// // clang-format off
// /**
// \class ROOT::Experimental::RNTupleProcessor::RIterator
// \ingroup NTuple
// \brief Iterator over the entries of an RNTuple, or vertical concatenation thereof.
// */
// // clang-format on
// class RChainIterator : public RNTupleProcessor::RIterator {
// public:
//    // // clang-format off
//    // /**
//    // \class ROOT::Experimental::RNTupleProcessor::RIterator::RProcessorState
//    // \ingroup NTuple
//    // \brief View on the RNTupleProcessor iterator state.
//    // */
//    // // clang-format on
//    // class RProcessorState {
//    //    friend class RChainIterator;

//    // private:
//    //    const REntry &fEntry;
//    //    NTupleSize_t fGlobalEntryIndex;
//    //    NTupleSize_t fLocalEntryIndex;
//    //    /// Index of the currently open RNTuple in the chain of ntuples
//    //    std::size_t fNTupleIndex;

//    // public:
//    //    RProcessorState(const REntry &entry, NTupleSize_t globalEntryIndex, NTupleSize_t localEntryIndex,
//    //                    std::size_t ntupleIndex)
//    //       : fEntry(entry),
//    //         fGlobalEntryIndex(globalEntryIndex),
//    //         fLocalEntryIndex(localEntryIndex),
//    //         fNTupleIndex(ntupleIndex)
//    //    {
//    //    }

//    //    const REntry *operator->() const { return &fEntry; }
//    //    const REntry &GetEntry() const { return fEntry; }
//    //    NTupleSize_t GetGlobalEntryIndex() const { return fGlobalEntryIndex; }
//    //    NTupleSize_t GetLocalEntryIndex() const { return fLocalEntryIndex; }
//    //    std::size_t GetNTupleIndex() const { return fNTupleIndex; }
//    // };

// private:
//    RNTupleChainProcessor &fProcessor;
//    RProcessorState fState;

// public:
//    using iterator_category = std::forward_iterator_tag;
//    using iterator = RIterator;
//    using value_type = RProcessorState;
//    using difference_type = std::ptrdiff_t;
//    using pointer = RProcessorState *;
//    using reference = const RProcessorState &;

//    RChainIterator(RNTupleChainProcessor &processor, std::size_t ntupleIndex, NTupleSize_t globalEntryIndex)
//       : RIterator(), fProcessor(processor), fState(processor.GetEntry(), globalEntryIndex, 0, ntupleIndex)
//    {
//    }

//    void LoadEntry() final {
//       fProcessor.fEntry->Read(fState.fLocalEntryIndex);
//    }

//    //////////////////////////////////////////////////////////////////////////
//    /// \brief Increments the entry index.
//    ///
//    /// Checks if the end of the currently connected RNTuple is reached. If this is the case, either the next RNTuple
//    /// is connected or the iterator has reached the end.
//    void Advance() final
//    {
//       ++fState.fGlobalEntryIndex;

//       if (++fState.fLocalEntryIndex >= fProcessor.fPageSource->GetNEntries()) {
//          do {
//             if (++fState.fNTupleIndex >= fProcessor.fNTuples.size()) {
//                fState.fGlobalEntryIndex = kInvalidNTupleIndex;
//                return;
//             }
//             // Skip over empty ntuples we might encounter.
//          } while (fProcessor.ConnectNTuple(fProcessor.fNTuples.at(fState.fNTupleIndex)) == 0);

//          fState.fLocalEntryIndex = 0;
//       }
//       fProcessor.fEntry->Read(fState.fLocalEntryIndex);
//    }

//    iterator operator++() final
//    {
//       Advance();
//       return *this;
//    }

//    iterator operator++(int) final
//    {
//       auto obj = *this;
//       Advance();
//       return obj;
//    }

//    reference operator*() final
//    {
//       fProcessor.fEntry->Read(fState.fLocalEntryIndex);
//       return fState;
//    }

//    bool operator!=(const iterator &rh) const { return fState.fGlobalEntryIndex != rh.fState.fGlobalEntryIndex; }
//    bool operator==(const iterator &rh) const { return fState.fGlobalEntryIndex == rh.fState.fGlobalEntryIndex; }
// };

} // namespace Internal
} // namespace Experimental
} // namespace ROOT

#endif // ROOT7_RNTupleProcessor
