/// \file ROOT/RNTupleView.hxx
/// \ingroup NTuple ROOT7
/// \author Florine de Geus <florine.de.geus@cern.ch>
/// \date 2024-02-08
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

#ifndef ROOT7_RNTupleIndex
#define ROOT7_RNTupleIndex

#include <ROOT/RField.hxx>
#include <ROOT/RNTupleUtil.hxx>

#include <unordered_map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace ROOT {
namespace Experimental {
namespace Internal {

// clang-format off
/**
\class ROOT::Experimental::RNTupleIndex
\ingroup NTuple
\brief Build an index for an RNTuple so it can be joined onto other RNTuples.
*/
// clang-format on
template <class IndexValueT>
class RNTupleIndex {
   friend class RNTupleReader;

private:
   /// The maximum number of index elements we allow to be kept in memory. Used as a failsafe.
   std::size_t fMaxElemsInMemory = 64 * 1024 * 1024;
   std::size_t fNElems = 0;
   std::string fFieldName; // TODO store more field info (for merging checks)
   std::unordered_map<IndexValueT, std::set<NTupleSize_t>> fIndex;

   void Merge(const RNTupleIndex<IndexValueT> &other)
   {
      if (fFieldName != other.fFieldName)
         throw RException(R__FAIL("can only merge indices for the same field"));

      fNElems += other.fNElems;

      for (const auto &val : other.fIndex) {
         auto res = fIndex.insert(val);
         if (!res.second) {
            // The index value is already present so the insertion failed. Instead, we have to merge the values.
            fIndex.at(val.first).insert(val.second.begin(), val.second.end());
         }
      }
   }

public:
   RNTupleIndex(std::string_view fieldName) : fFieldName(std::string(fieldName)) {}
   RNTupleIndex() : fFieldName("") {}
   RNTupleIndex<IndexValueT> &operator=(const RNTupleIndex<IndexValueT> &other)
   {
      fFieldName = other.fFieldName;
      fNElems = other.fNElems;
      fIndex = other.fIndex;
      return *this;
   }

   void SetMaxElementsInMemory(std::size_t maxElems) { fMaxElemsInMemory = maxElems; }

   std::size_t GetNElems() const { return fNElems; }

   void Add(const IndexValueT &value, NTupleSize_t entry)
   {
      if (fNElems > fMaxElemsInMemory) {
         throw RException(
            R__FAIL("in-memory index exceeds maximum allowed size (" + std::to_string(fMaxElemsInMemory) + ")"));
      }

      fIndex[value].insert(entry);
      fNElems++;
   }

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Get the entry number containing the given index value **after** the provided minimum entry.
   ///
   /// \param[in] value The indexed value
   /// \param[in] lowerBound The minimum entry number (inclusive) to retrieve. By default, all entries are considered.
   /// \return The entry number, starting from `lowerBound`, containing the specified index value. When no such entry
   /// exists, return `kInvalidNTupleIndex`
   NTupleSize_t GetEntry(const IndexValueT &value, NTupleSize_t lowerBound = 0) const
   {
      if (!fIndex.count(value)) {
         return kInvalidNTupleIndex;
      }

      auto indexEntries = fIndex.at(value);

      if (auto entry = indexEntries.lower_bound(lowerBound); entry != indexEntries.end())
         return *entry;

      return kInvalidNTupleIndex;
   }

   /////////////////////////////////////////////////////////////////////////////
   /// \brief Concatenate two RNTupleIndex objects
   ///
   /// \param[in] left The left index
   /// \param[in] right The right index
   ///
   /// \return A new RNTupleIndex resulting from the concatenation
   ///
   static RNTupleIndex<IndexValueT>
   Concatenate(const RNTupleIndex<IndexValueT> &left, const RNTupleIndex<IndexValueT> &right)
   {
      RNTupleIndex<IndexValueT> index(left.fFieldName);
      index.Merge(left);
      index.Merge(right);
      return index;
   }
};

} // namespace Internal
} // namespace Experimental
} // namespace ROOT

#endif // ROOT7_RNTupleIndex
