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

#ifndef ROOT7_RNTupleJoinProcessor
#define ROOT7_RNTupleJoinProcessor

#include <ROOT/RError.hxx>
#include <ROOT/RField.hxx>
#include <ROOT/RNTupleDescriptor.hxx>
#include <ROOT/RNTupleIndex.hxx>
#include <ROOT/RPageStorage.hxx>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ROOT {
namespace Experimental {
namespace Internal {
struct RNTupleSourceSpec;
}
} // namespace Experimental
} // namespace ROOT

template <>
struct std::hash<ROOT::Experimental::Internal::RNTupleSourceSpec> {
   std::size_t operator()(const ROOT::Experimental::Internal::RNTupleSourceSpec &s) const noexcept
   {
      std::size_t h1 = std::hash<std::string>{}(s.fName);
      std::size_t h2 = std::hash<std::string>{}(s.fLocation);
      return h1 ^ (h2 << 1);
   }
};

namespace ROOT {
namespace Experimental {
namespace Internal {
class RNTupleJoinProcessor {
private:
   class RFieldContext {
      friend class RNTupleJoinProcessor;

   private:
      std::unique_ptr<RFieldBase> fProtoField;
      std::unique_ptr<RFieldBase> fConcreteField;
      std::unique_ptr<RFieldBase::RValue> fValue;
      Internal::RPageSource &fPageSource;

   public:
      RFieldContext(std::unique_ptr<ROOT::Experimental::RFieldBase> field, Internal::RPageSource &pageSource)
         : fProtoField(std::move(field)), fPageSource(pageSource)
      {
      }

      /// We need to disconnect the concrete fields before swapping the page sources
      void ResetConcreteField() { fConcreteField = nullptr; }
      RFieldBase &CreateConcreteField()
      {
         fConcreteField = fProtoField->Clone(fProtoField->GetFieldName());
         return *fConcreteField;
      }
   };

   const std::vector<RNTupleSourceSpec> &fNTuples;
   std::vector<std::string> fIndexFieldNames;

   std::unique_ptr<Internal::RPageSource> fPrimaryPageSource;
   std::unordered_map<std::string, std::unique_ptr<Internal::RPageSource>> fSecondaryPageSources;
   std::unordered_map<std::string, std::unique_ptr<RNTupleIndex>> fJoinIndices;
   std::unordered_map<std::string, RFieldContext> fFieldContexts;

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
   /// \brief Creates and connects a concrete field to the current page source, based on its protofield.
   void ConnectField(RFieldContext &fieldContext);

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Add a field to be read by the processor.
   ///
   /// \param[in] fieldName The name of the field to activate.
   void ActivateField(std::string_view fieldName);

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Check whether a field has been activated in the processor.
   ///
   /// \param[in] fieldName The name of the field to check.
   ///
   /// \return Whether the field with the given name is activated.
   bool HasField(std::string_view fieldName) const { return fFieldContexts.count(std::string(fieldName)) > 0; }

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Loads an entry.
   ///
   /// \param[in] idx The index (local to the current RNTuple) of the entry to load.
   void LoadEntry(NTupleSize_t idx)
   {
      std::vector<void *> valPtrs;
      valPtrs.reserve(fIndexFieldNames.size());
      for (const auto &fieldName : fIndexFieldNames) {
         auto &fieldContext = fFieldContexts.at(fieldName);
         fieldContext.fValue->Read(idx);
         valPtrs.push_back(fieldContext.fValue->GetPtr<void>().get());
      }

      for (auto &[fieldName, fieldContext] : fFieldContexts) {
         if (std::find(fIndexFieldNames.cbegin(), fIndexFieldNames.cend(), fieldName) != fIndexFieldNames.end())
            continue;

         auto ntupleName = fieldContext.fPageSource.GetNTupleName();
         if (fJoinIndices.find(ntupleName) != fJoinIndices.end()) {
            auto joinIdx = fJoinIndices.at(ntupleName)->GetFirstEntryNumber(valPtrs);

            if (joinIdx == kInvalidNTupleIndex) {
               fieldContext.fProtoField->ConstructValue(fieldContext.fValue->GetPtr<void>().get());
            } else {
               fieldContext.fValue->Read(joinIdx);
            }
         } else {
            fieldContext.fValue->Read(idx);
         }
      }
   }

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Returns a reference to the primary page source.
   ///
   /// \return A reference to the primary page source.
   Internal::RPageSource &GetPrimaryPageSource() const { return *fPrimaryPageSource; }

public:
   class RIterator {
   public:
      class RState {
         friend class RIterator;

      private:
         RNTupleJoinProcessor &fProcessor;
         NTupleSize_t fEntryIndex;

         void UpdateEntry() { fProcessor.LoadEntry(fEntryIndex); }

      public:
         RState(RNTupleJoinProcessor &processor, NTupleSize_t entryIndex)
            : fProcessor(processor), fEntryIndex(entryIndex)
         {
         }

         NTupleSize_t GetEntryIndex() const { return fEntryIndex; }

         template <typename T>
         std::shared_ptr<T> GetPtr(std::string_view fieldName)
         {
            if (!fProcessor.HasField(fieldName)) {
               fProcessor.ActivateField(fieldName);
               UpdateEntry();
            }
            return fProcessor.GetPtr<T>(fieldName);
         }
      };

   private:
      RNTupleJoinProcessor &fProcessor;
      RState fState;

   public:
      using iterator_category = std::forward_iterator_tag;
      using iterator = RIterator;
      using value_type = RState;
      using difference_type = std::ptrdiff_t;
      using pointer = RState *;
      using reference = RState &;

      RIterator(RNTupleJoinProcessor &processor, std::size_t entryIndex)
         : fProcessor(processor), fState(processor, entryIndex)
      {
      }

      void Advance()
      {
         if (++fState.fEntryIndex >= fProcessor.GetPrimaryPageSource().GetNEntries())
            fState.fEntryIndex = kInvalidNTupleIndex;
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

         fState.UpdateEntry();
         return fState;
      }

      pointer operator->()
      {
         fState.UpdateEntry();
         return &fState;
      }
      bool operator!=(const iterator &rh) const { return fState.fEntryIndex != rh.fState.fEntryIndex; }
      bool operator==(const iterator &rh) const { return fState.fEntryIndex == rh.fState.fEntryIndex; }
   };

   RNTupleJoinProcessor(const std::vector<RNTupleSourceSpec> &ntuples);
   RNTupleJoinProcessor(const std::vector<RNTupleSourceSpec> &ntuples, const std::vector<std::string> &indexFields);

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Returns the names of the fields currently actively being processed.
   ///
   /// \return A vector with the names of all currently active fields.
   const std::vector<std::string> GetActiveFields() const;

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Returns a pointer to the object representing the provided field during processing.
   ///
   /// \param[in] fieldName the name of the field for which to get a pointer.
   ///
   /// \return A pointer to the object for the provided field.
   ///
   /// \warning If this method is called while iterating, the pointer won't hold an entry value until the *next*
   /// iteration step. To ensure the pointer holds the correct entry value during iteration, use
   /// RNTupleProcessor::RIterator::RState::GetPtr instead.
   template <typename T>
   std::shared_ptr<T> GetPtr(std::string_view fieldName)
   {
      if (!HasField(fieldName))
         ActivateField(fieldName);
      return fFieldContexts.at(std::string(fieldName)).fValue->GetPtr<T>();
   }

   RIterator begin() { return RIterator(*this, 0); }
   RIterator end() { return RIterator(*this, kInvalidNTupleIndex); }
};

} // namespace Internal
} // namespace Experimental
} // namespace ROOT

#endif // ROOT7_RNTupleJoinProcessor
