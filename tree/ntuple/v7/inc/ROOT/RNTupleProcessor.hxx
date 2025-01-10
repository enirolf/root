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
#include <ROOT/RNTupleIndex.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleUtil.hxx>
#include <ROOT/RPageStorage.hxx>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ROOT {
namespace Experimental {

/// Used to specify the underlying RNTuples in RNTupleProcessor
struct RNTupleOpenSpec {
   std::string fNTupleName;
   std::string fStorage;
   RNTupleReadOptions fOptions;

   RNTupleOpenSpec(std::string_view n, std::string_view s) : fNTupleName(n), fStorage(s) {}
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
using ROOT::Experimental::RNTupleOpenSpec;

std::vector<RNTupleOpenSpec> ntuples = {{"ntuple1", "ntuple1.root"}, {"ntuple2", "ntuple2.root"}};
auto processor = RNTupleProcessor::CreateChain(ntuples);

for (const auto &entry : processor) {
   std::cout << "pt = " << *entry.GetPtr<float>("pt") << std::endl;
}
~~~

An RNTupleProcessor is created by providing one or more RNTupleOpenSpecs, each of which contains the name and storage
location of a single RNTuple. The RNTuples are processed in the order in which they were provided.

The RNTupleProcessor constructor also (optionally) accepts an RNTupleModel, which determines which fields should be
read. If no model is provided, a default model based on the descriptor of the first specified RNTuple will be used.
If a field that was present in the first RNTuple is not found in a subsequent one, an error will be thrown.

The RNTupleProcessor provides an iterator which gives access to the REntry containing the field data for the current
entry. Additional bookkeeping information can be obtained through the RNTupleProcessor itself.
*/
// clang-format on
class RNTupleProcessor {
protected:
   std::string fProcessorName;
   std::unique_ptr<REntry> fEntry;
   std::unique_ptr<RNTupleModel> fModel;

   NTupleSize_t fNEntriesProcessed = 0;     //< Total number of entries processed so far
   NTupleSize_t fCurrentEntryNumber = 0;    //< Entry number within the current ntuple
   std::size_t fCurrentProcessorNumber = 0; //< Number of the currently open inner processor

   RNTupleProcessor(std::string_view processorName, std::unique_ptr<RNTupleModel> model)
      : fProcessorName(processorName), fModel(std::move(model))
   {
   }

public:
   RNTupleProcessor(const RNTupleProcessor &) = delete;
   RNTupleProcessor(RNTupleProcessor &&) = delete;
   RNTupleProcessor &operator=(const RNTupleProcessor &) = delete;
   RNTupleProcessor &operator=(RNTupleProcessor &&) = delete;
   virtual ~RNTupleProcessor() = default;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Get the total number of entries in this processor
   virtual NTupleSize_t GetNEntries() = 0;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Get the total number of entries processed so far.
   ///
   /// When only one RNTuple is present in the processor chain, the return value is equal to GetLocalEntryNumber.
   NTupleSize_t GetNEntriesProcessed() const { return fNEntriesProcessed; }

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Get the entry number local to the RNTuple that is currently being processed.
   ///
   /// When only one RNTuple is present in the processor chain, the return value is equal to GetGlobalEntryNumber.
   NTupleSize_t GetCurrentEntryNumber() const { return fCurrentEntryNumber; }

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Get the number of the inner processor currently being read.
   ///
   /// This method is only relevant for the RNTupleChainProcessor. For the other processors, 0 is always returned.
   std::size_t GetCurrentProcessorNumber() const { return fCurrentProcessorNumber; }

   std::string GetProcessorName() const { return fProcessorName; }

   const RNTupleModel &GetModel() const { return *fModel; }

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Load the entry identified by the provided entry number.
   ///
   /// \param[in] entryNumber Entry number to load
   ///
   /// \return `entryNumber` if the entry was successfully loaded, `kInvalidNTupleIndex` otherwise.
   virtual NTupleSize_t LoadEntry(NTupleSize_t entryNumber) = 0;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Returns a reference to the entry used by the processor.
   ///
   /// \return A reference to the entry used by the processor.
   virtual const REntry &GetEntry() const = 0;

   virtual void SetEntryPointers(const REntry &entry, std::string_view fieldNamePrefix = "") = 0;

   // clang-format off
   /**
   \class ROOT::Experimental::RNTupleProcessor::RIterator
   \ingroup NTuple
   \brief Iterator over the entries of an RNTuple, or vertical concatenation thereof.
   */
   // clang-format on
   class RIterator {
   private:
      RNTupleProcessor &fProcessor;
      NTupleSize_t fCurrentEntryNumber;

   public:
      using iterator_category = std::forward_iterator_tag;
      using iterator = RIterator;
      using value_type = REntry;
      using difference_type = std::ptrdiff_t;
      using pointer = REntry *;
      using reference = const REntry &;

      RIterator(RNTupleProcessor &processor, NTupleSize_t entryNumber)
         : fProcessor(processor), fCurrentEntryNumber(entryNumber)
      {
         // This constructor is called with kInvalidNTupleIndex for RNTupleProcessor::end(). In that case, we already
         // know there is nothing to advance to.
         if (fCurrentEntryNumber != kInvalidNTupleIndex) {
            fCurrentEntryNumber = fProcessor.LoadEntry(fCurrentEntryNumber);
         }
      }

      iterator operator++()
      {
         fCurrentEntryNumber = fProcessor.LoadEntry(fCurrentEntryNumber + 1);
         return *this;
      }

      iterator operator++(int)
      {
         auto obj = *this;
         ++(*this);
         return obj;
      }

      reference operator*() { return fProcessor.GetEntry(); }

      friend bool operator!=(const iterator &lh, const iterator &rh)
      {
         return lh.fCurrentEntryNumber != rh.fCurrentEntryNumber;
      }
      friend bool operator==(const iterator &lh, const iterator &rh)
      {
         return lh.fCurrentEntryNumber == rh.fCurrentEntryNumber;
      }
   };

   RIterator begin() { return RIterator(*this, 0); }
   RIterator end() { return RIterator(*this, kInvalidNTupleIndex); }

   static std::unique_ptr<RNTupleProcessor>
   Create(const RNTupleOpenSpec &ntuple, std::unique_ptr<RNTupleModel> model = nullptr);

   static std::unique_ptr<RNTupleProcessor>
   Create(const RNTupleOpenSpec &ntuple, std::string_view processorName, std::unique_ptr<RNTupleModel> model = nullptr);

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Create a new RNTuple processor chain for vertical concatenation of RNTuples.
   ///
   /// \param[in] ntuples A list specifying the names and locations of the ntuples to process.
   /// \param[in] model An RNTupleModel specifying which fields can be read by the processor. If no model is provided,
   /// one will be created based on the descriptor of the first ntuple specified.
   ///
   /// \return A pointer to the newly created RNTupleProcessor.
   static std::unique_ptr<RNTupleProcessor>
   CreateChain(const std::vector<RNTupleOpenSpec> &ntuples, std::unique_ptr<RNTupleModel> model = nullptr);

   static std::unique_ptr<RNTupleProcessor> CreateChain(const std::vector<RNTupleOpenSpec> &ntuples,
                                                        std::string_view processorName,
                                                        std::unique_ptr<RNTupleModel> model = nullptr);

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Create a new RNTuple processor chain for vertical concatenation of previously created processors.
   ///
   /// \param[in] innerProcessors A list with the processors to chain.
   /// \param[in] model An RNTupleModel specifying which fields can be read by the processor. If no model is provided,
   /// one will be created based on the descriptor of the first ntuple specified.
   ///
   /// \return A pointer to the newly created RNTupleProcessor.
   static std::unique_ptr<RNTupleProcessor> CreateChain(std::vector<std::unique_ptr<RNTupleProcessor>> &innerProcessors,
                                                        std::unique_ptr<RNTupleModel> model = nullptr);

   static std::unique_ptr<RNTupleProcessor> CreateChain(std::vector<std::unique_ptr<RNTupleProcessor>> &innerProcessors,
                                                        std::string_view processorName,
                                                        std::unique_ptr<RNTupleModel> model = nullptr);

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Create a new RNTuple processor for horizontallly concatenated RNTuples.
   ///
   /// \param[in] ntuples A list specifying the names and locations of the ntuples to process. The first ntuple in the
   /// list will be considered the primary ntuple and drives the processor iteration loop. Subsequent ntuples are
   /// considered auxiliary, whose entries to be read are determined by the primary ntuple (which does not necessarily
   /// have to be sequential).
   /// \param[in] joinFields The names of the fields on which to join, in case the specified ntuples are unaligned.
   /// The join is made based on the combined join field values, and therefore each field has to be present in each
   /// specified RNTuple. If an empty list is provided, it is assumed that the specified ntuple are fully aligned, and
   /// `RNTupleIndex` will not be used.
   /// \param[in] models A list of models for the ntuples. This list must either contain a model for each ntuple in
   /// `ntuples` (following the specification order), or be empty. When the list is empty, the default model (i.e.
   /// containing all fields) will be used for each ntuple.
   ///
   /// \return A pointer to the newly created RNTupleProcessor.
   static std::unique_ptr<RNTupleProcessor> CreateJoin(const std::vector<RNTupleOpenSpec> &ntuples,
                                                       const std::vector<std::string> &joinFields,
                                                       std::vector<std::unique_ptr<RNTupleModel>> models = {});

   static std::unique_ptr<RNTupleProcessor>
   CreateJoin(const std::vector<RNTupleOpenSpec> &ntuples, const std::vector<std::string> &joinFields,
              std::string_view processorName, std::vector<std::unique_ptr<RNTupleModel>> models = {});

   static std::unique_ptr<RNTupleProcessor> CreateJoin(std::unique_ptr<RNTupleProcessor> mainProcessor,
                                                       std::vector<std::unique_ptr<RNTupleProcessor>> &auxProcessors,
                                                       const std::vector<std::string> &joinFields,
                                                       std::vector<std::unique_ptr<RNTupleModel>> models = {});

   static std::unique_ptr<RNTupleProcessor>
   CreateJoin(std::unique_ptr<RNTupleProcessor> mainProcessor,
              std::vector<std::unique_ptr<RNTupleProcessor>> &auxProcessors, const std::vector<std::string> &joinFields,
              std::string_view processorName, std::vector<std::unique_ptr<RNTupleModel>> models = {});
};

// clang-format off
/**
\class ROOT::Experimental::RNTupleSingleProcessor
\ingroup NTuple
\brief Processor specializiation for processing a single RNTuple.
*/
// clang-format on
class RNTupleSingleProcessor : public RNTupleProcessor {
   friend class RNTupleProcessor;

private:
   std::unique_ptr<Internal::RPageSource> fPageSource;
   NTupleSize_t fNEntries = kInvalidNTupleIndex;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Constructs a new RNTupleProcessor for processing a single RNTuple.
   ///
   /// \param[in] ntuple The source specification (name and storage location) for the RNTuple to process.
   /// \param[in] model The model that specifies which fields should be read by the processor.
   RNTupleSingleProcessor(const RNTupleOpenSpec &ntuple, std::string_view processorName,
                          std::unique_ptr<RNTupleModel> model);

public:
   RNTupleSingleProcessor(const RNTupleSingleProcessor &) = delete;
   RNTupleSingleProcessor(RNTupleSingleProcessor &&) = delete;
   RNTupleSingleProcessor &operator=(const RNTupleSingleProcessor &) = delete;
   RNTupleSingleProcessor &operator=(RNTupleSingleProcessor &&) = delete;
   ~RNTupleSingleProcessor() { fModel.release(); }

   NTupleSize_t GetNEntries() final
   {
      Connect();
      return fNEntries;
   }

   const REntry &GetEntry() const final { return *fEntry; }

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Load the entry identified by the provided entry number.
   ///
   /// \sa ROOT::Experimental::RNTupleProcessor::LoadEntry
   NTupleSize_t LoadEntry(NTupleSize_t entryNumber) final;

   void SetEntryPointers(const REntry &entry, std::string_view fieldNamePrefix = "") final;

   void Connect();

   bool IsConnected() const { return fNEntries != kInvalidNTupleIndex; }
};

// clang-format off
/**
\class ROOT::Experimental::RNTupleChainProcessor
\ingroup NTuple
\brief Processor specializiation for vertically concatenated RNTuples (chains).
*/
// clang-format on
class RNTupleChainProcessor : public RNTupleProcessor {
   friend class RNTupleProcessor;

private:
   std::vector<std::unique_ptr<RNTupleProcessor>> fInnerProcessors;
   std::vector<NTupleSize_t> fInnerNEntries;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Load the entry identified by the provided (global) entry number (i.e., considering all RNTuples in this
   /// processor).
   ///
   /// \sa ROOT::Experimental::RNTupleProcessor::LoadEntry
   NTupleSize_t LoadEntry(NTupleSize_t entryNumber) final;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Constructs a new RNTupleChainProcessor.
   ///
   /// \param[in] ntuples The source specification (name and storage location) for each RNTuple to process.
   /// \param[in] model The model that specifies which fields should be read by the processor. The pointer returned by
   /// RNTupleModel::MakeField can be used to access a field's value during the processor iteration. When no model is
   /// specified, it is created from the descriptor of the first RNTuple specified in `ntuples`.
   ///
   /// RNTuples are processed in the order in which they are specified.
   RNTupleChainProcessor(std::vector<std::unique_ptr<RNTupleProcessor>> &processors, std::string_view processorName,
                         std::unique_ptr<RNTupleModel> model);

public:
   NTupleSize_t GetNEntries() final;

   const REntry &GetEntry() const final { return *fEntry; }

   void SetEntryPointers(const REntry &entry, std::string_view fieldNamePrefix = "") final;
};

// clang-format off
/**
\class ROOT::Experimental::RNTupleJoinProcessor
\ingroup NTuple
\brief Processor specializiation for horizontally concatenated RNTuples (joins).
*/
// clang-format on
class RNTupleJoinProcessor : public RNTupleProcessor {
   friend class RNTupleProcessor;

private:
   std::unique_ptr<RNTupleProcessor> fMainProcessor;
   std::vector<std::unique_ptr<RNTupleProcessor>> fAuxiliaryProcessors;

   std::vector<std::unique_ptr<Internal::RPageSource>> fAuxiliaryPageSources;
   std::vector<std::unique_ptr<Internal::RPageSource>> fAuxiliaryEntries;
   /// Tokens representing the join fields present in the main RNTuple
   std::vector<REntry::RFieldToken> fJoinFieldTokens;
   std::vector<std::unique_ptr<Internal::RNTupleIndex>> fJoinIndices;

   bool IsUsingIndex() const { return fJoinIndices.size() > 0; }

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Load the entry identified by the provided entry number of the primary RNTuple.
   ///
   /// \sa ROOT::Experimental::RNTupleProcessor::LoadEntry
   NTupleSize_t LoadEntry(NTupleSize_t entryNumber) final;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Constructs a new RNTupleJoinProcessor.
   ///
   /// \param[in] mainProcessor The processor that will drive the entry iteration.
   /// \param[in] model The model that specifies which fields should be read by the processor. The pointer returned by
   /// RNTupleModel::MakeField can be used to access a field's value during the processor iteration. When no model is
   /// specified, it is created from the RNTuple's descriptor.
   RNTupleJoinProcessor(std::unique_ptr<RNTupleProcessor> mainProcessor, std::string_view processorName,
                        std::unique_ptr<RNTupleModel> model);

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Add an auxiliary RNTuple to the processor.
   ///
   /// \param[in] auxProcessor The processor that will be joined on the main processor.
   /// \param[in] joinFields The names of the fields used in the join.
   /// \param[in] model The model that specifies which fields should be read by the processor. The pointer returned by
   /// RNTupleModel::MakeField can be used to access a field's value during the processor iteration. When no model is
   /// specified, it is created from the RNTuple's descriptor.
   void AddAuxiliary(std::unique_ptr<RNTupleProcessor> auxProcessor, const std::vector<std::string> &joinFields,
                     std::unique_ptr<RNTupleModel> model = nullptr);

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Populate fJoinFieldTokens with tokens for join fields belonging to the main RNTuple in the join model.
   ///
   /// \param[in] joinFields The names of the fields used in the join.
   void SetJoinFieldTokens(const std::vector<std::string> &joinFields)
   {
      fJoinFieldTokens.reserve(joinFields.size());
      for (const auto &fieldName : joinFields) {
         fJoinFieldTokens.emplace_back(fEntry->GetToken(fieldName));
      }
   }

public:
   const REntry &GetEntry() const final { return *fEntry; }

   NTupleSize_t GetNEntries() final { return fMainProcessor->GetNEntries(); }

   void SetEntryPointers(const REntry &entry, std::string_view fieldNamePrefix = "") final;
};

} // namespace Experimental
} // namespace ROOT

#endif // ROOT7_RNTupleProcessor
