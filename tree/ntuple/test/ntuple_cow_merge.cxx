#include "ntuple_test.hxx"

#include "ROOT/RNTupleCoWMerger.hxx"

using ROOT::Experimental::RNTupleCoWMerger;

TEST(RNTuple, CopyOnWriteMerge)
{
   auto fnWriteNTuple = [](std::string_view filename) {
      auto model = ROOT::RNTupleModel::Create();

      auto intFld = model->MakeField<std::uint64_t>("x");
      auto floatVecFld = model->MakeField<std::vector<float>>("y");

      ROOT::RNTupleWriteOptions opts;
      // opts.SetUseDirectIO(true);

      auto writer = ROOT::RNTupleWriter::Recreate(std::move(model), "ntuple", filename, opts);

      for (ROOT::NTupleSize_t i = 0; i < 300; i++) {
         *intFld = i;
         floatVecFld->clear();
         for (int j = 0; j < 10; j++) {
            floatVecFld->push_back(static_cast<float>(i * j) * .1f);
         }
         writer->Fill();
         if ((i + 1) % 100 == 0)
            writer->FlushCluster();
      }
   };
   FileRaii fileGuard1("ntuple_cow_merge1.root");
   FileRaii fileGuard2("ntuple_cow_merge2.root");
   FileRaii fileGuard3("ntuple_cow_merge3.root");
   fnWriteNTuple(fileGuard1.GetPath());
   fnWriteNTuple(fileGuard2.GetPath());

   auto dest = std::make_unique<RPageSinkFile>("ntuple", fileGuard3.GetPath(), RNTupleWriteOptions());
   RNTupleCoWMerger merger{std::move(dest)};
   std::vector<std::pair<std::string, std::string>> sources = {{"ntuple", fileGuard1.GetPath()},
                                                               {"ntuple", fileGuard2.GetPath()}};
   auto res = merger.Merge(sources);
   EXPECT_TRUE(bool(res));
}
