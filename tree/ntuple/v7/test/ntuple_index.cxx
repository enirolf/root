#include "ntuple_test.hxx"

TEST(RNTupleIndex, FromReade)
{
   FileRaii fileGuard("test_ntuple_index_from_reader.root");
   {
      auto model = RNTupleModel::Create();
      auto fld = model->MakeField<std::uint64_t>("fld");

      auto ntuple = RNTupleWriter::Recreate(std::move(model), "ntuple", fileGuard.GetPath());

      *fld = 0;
      ntuple->Fill();
      for (int i = 0; i < 10; ++i) {
         *fld = i;
         ntuple->Fill();
      }
   }

   auto ntuple = RNTupleReader::Open("ntuple", fileGuard.GetPath());
   auto fld = ntuple->GetView<std::uint64_t>("fld");
   auto index = ntuple->CreateIndex<std::uint64_t>("fld");

   EXPECT_EQ(index->GetEntry(0), 0);
   EXPECT_EQ(index->GetEntry(0, 1), 1);
   // The first two entries are 0, so start the loop from the third.
   for (unsigned i = 2; i < ntuple->GetNEntries(); ++i) {
      EXPECT_EQ(index->GetEntry(fld(i)), i);
   }
}
