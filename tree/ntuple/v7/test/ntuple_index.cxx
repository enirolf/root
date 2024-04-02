#include "ntuple_test.hxx"

#ifdef R__USE_IMT
struct IMTRaii {
   IMTRaii() { ROOT::EnableImplicitMT(4); }
   ~IMTRaii() { ROOT::DisableImplicitMT(); }
};
#endif

TEST(RNTupleIndex, FromReader)
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

#ifdef R__USE_IMT
TEST(RNTupleIndex, FromReaderMT)
{
   FileRaii fileGuard("test_ntuple_index_from_reader_mt.root");
   {
      auto model = RNTupleModel::Create();
      auto fld = model->MakeField<std::uint64_t>("fld");

      RNTupleWriteOptions opts;
      opts.SetApproxZippedClusterSize(512);
      auto ntuple = RNTupleWriter::Recreate(std::move(model), "ntuple", fileGuard.GetPath(), opts);

      *fld = 0;
      ntuple->Fill();
      for (int i = 0; i < 1e4; ++i) {
         *fld = i;
         ntuple->Fill();
      }
   }

   IMTRaii _;

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
#endif
