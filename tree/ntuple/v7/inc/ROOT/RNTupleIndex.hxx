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

// clang-format off
/**
\class ROOT::Experimental::RNTupleIndex
\ingroup NTuple
\brief Build an index for an RNTuple so it can be joined onto other RNTuples.
*/
// clang-format on
template <class IndexValueT>
class RNTupleIndex {
private:
   /// The maximum number of index elements we allow to be kept in memory. Used as a failsafe.
   std::size_t fMaxElemsInMemory = 64 * 1024 * 1024;
   std::size_t fNElems;
   const std::string fFieldName;
   std::unordered_map<IndexValueT, std::set<NTupleSize_t>> fIndex;

protected:
public:
   RNTupleIndex(std::string_view fieldName) : fFieldName(std::string(fieldName)) {}

   void SetMaxElementsInMemory(std::size_t maxElems) { fMaxElemsInMemory = maxElems; }

   std::size_t GetNElems() const { return fNElems; }

   void Add(const IndexValueT &value, NTupleSize_t entry)
   {
      if (fNElems++ > fMaxElemsInMemory) {
         throw RException(
            R__FAIL("in-memory index exceeds maximum allowed size (" + std::to_string(fMaxElemsInMemory) + ")"));
      }

      fIndex[value].insert(entry);
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

   // /////////////////////////////////////////////////////////////////////////////
   // /// \brief Get all entry numbers containing the given index value.
   // ///
   // /// \param[in] value The indexed value
   // /// \return A vector with the entry numbers containing the specified index value. When no such entries
   // /// exist, an empty vector is returned
   // const std::vector<NTupleSize_t> &GetEntries(const IndexValueT &value)
   // {
   //    if (fIndex.count(value))
   //       return fIndex.at(value);
   //    else
   //       return std::vector<NTupleSize_t>{};
   // }
};

} // namespace Experimental
} // namespace ROOT

#endif // ROOT7_RNTupleIndex
