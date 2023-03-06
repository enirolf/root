#include "gtest/gtest.h"

// Backward compatibility for gtest version < 1.10.0
#ifndef INSTANTIATE_TEST_SUITE_P
#define SetUpTestSuite SetUpTestCase
#define TearDownTestSuite TearDownTestCase
#endif

#include "ReadSpeed.hxx"
#include "ReadSpeedCLI.hxx"

#ifdef R__USE_IMT
#include "ROOT/TTreeProcessorMT.hxx" // for TTreeProcessorMT::GetTasksPerWorkerHint
#endif

#include "TFile.h"
#include "TSystem.h"
#include "TTree.h"

#include "ROOT/RNTuple.hxx"

using namespace ReadSpeed;

using ROOT::Experimental::RNTupleModel;
using ROOT::Experimental::RNTupleWriter;

// Helper function to generate a .root file containing a RNTuple with some dummy data in it.
void RequireNTuple(const std::string &fname, const std::vector<std::string> &fieldNames = {"x"})
{
   if (gSystem->AccessPathName(fname.c_str()) == false) // then the file already exists: weird return value convention
      return;                                           // nothing to do

   auto model = RNTupleModel::Create();

   for (const auto &f : fieldNames) {
      auto var = model->MakeField<int>(f);
      *var = 42;
   }

   auto ntuple = RNTupleWriter::Recreate(std::move(model), "n", fname);

   for (int i = 0; i < 10000000; ++i)
      ntuple->Fill();
}

// Helper function to concatenate two vectors of strings.
std::vector<std::string> ConcatVectors(const std::vector<std::string> &first, const std::vector<std::string> &second)
{
   std::vector<std::string> all;

   all.insert(all.end(), first.begin(), first.end());
   all.insert(all.end(), second.begin(), second.end());

   return all;
}

// Creates all of our needed .root files and deletes them once the testing is over.
class ReadSpeedIntegration : public ::testing::Test {
protected:
   static void SetUpTestSuite()
   {
      RequireNTuple("readspeedinput1.root");
      RequireNTuple("readspeedinput2.root");
      RequireNTuple("readspeedinput3.root", {"x", "x_field", "y_feild", "mismatched"});
   }

   static void TearDownTestSuite()
   {
      gSystem->Unlink("readspeedinput1.root");
      gSystem->Unlink("readspeedinput2.root");
      gSystem->Unlink("readspeedinput3.root");
   }
};

// TEST_F(ReadSpeedIntegration, SingleThread)
// {
//    const auto result =
//       EvalThroughput({true, {"t"}, {"readspeedinput1root", "readspeedinput2root"}, {"x"}}, 0);

//    EXPECT_EQ(result.fUncompressedBytesRead, 80000000) << "Wrong number of uncompressed bytes read";
//    EXPECT_EQ(result.fCompressedBytesRead, 643934) << "Wrong number of compressed bytes read";
// }

// #ifdef R__USE_IMT
// TEST_F(ReadSpeedIntegration, MultiThread)
// {
//    const auto result =
//       EvalThroughput({true, {"t"}, {"readspeedinput1root", "readspeedinput2root"}, {"x"}}, 2);

//    EXPECT_EQ(result.fUncompressedBytesRead, 80000000) << "Wrong number of uncompressed bytes read";
//    EXPECT_EQ(result.fCompressedBytesRead, 643934) << "Wrong number of compressed bytes read";
// }
// #endif

// TEST_F(ReadSpeedIntegration, NonExistentFile)
// {
//    EXPECT_THROW(EvalThroughput({true, {"t"}, {"test_fake.root"}, {"x"}}, 0), std::runtime_error)
//       << "Should throw for non-existent file";
// }

// TEST_F(ReadSpeedIntegration, NonExistentTree)
// {
//    EXPECT_THROW(EvalThroughput({true, {"t_fake"}, {"readspeedinput1root"}, {"x"}}, 0), std::runtime_error)
//       << "Should throw for non-existent tree";
// }

// TEST_F(ReadSpeedIntegration, NonExistentBranch)
// {
//    EXPECT_THROW(EvalThroughput({true, {"t"}, {"readspeedinput1root"}, {"z"}}, 0), std::runtime_error)
//       << "Should throw for non-existent branch";
// }

// TEST_F(ReadSpeedIntegration, SingleBranch)
// {
//    const auto result = EvalThroughput({true, {"t"}, {"readspeedinput3root"}, {"x"}}, 0);

//    EXPECT_EQ(result.fUncompressedBytesRead, 40000000) << "Wrong number of uncompressed bytes read";
//    EXPECT_EQ(result.fCompressedBytesRead, 321967) << "Wrong number of compressed bytes read";
// }

// TEST_F(ReadSpeedIntegration, PatternBranch)
// {
//    const auto result = EvalThroughput({true, {"t"}, {"readspeedinput3root"}, {"(x|y)_.*nch"}, true}, 0);

//    EXPECT_EQ(result.fUncompressedBytesRead, 80000000) << "Wrong number of uncompressed bytes read";
//    EXPECT_EQ(result.fCompressedBytesRead, 661576) << "Wrong number of compressed bytes read";
// }

// TEST_F(ReadSpeedIntegration, NoMatches)
// {
//    EXPECT_THROW(EvalThroughput({true, {"t"}, {"readspeedinput3root"}, {"x_.*"}, false}, 0), std::runtime_error)
//       << "Should throw for no matching branch";
//    EXPECT_DEATH(EvalThroughput({true, {"t"}, {"readspeedinput3root"}, {"z_.*"}, true}, 0),
//                 "branch regexes didn't match any branches")
//       << "Should terminate for no matching branch";
//    EXPECT_DEATH(EvalThroughput({true, {"t"}, {"readspeedinput3root"}, {".*", "z_.*"}, true}, 0),
//                 "following regexes didn't match any branches")
//       << "Should terminate for no matching branch";
// }

// TEST_F(ReadSpeedIntegration, AllBranches)
// {
//    const auto result = EvalThroughput({true, {"t"}, {"readspeedinput3root"}, {".*"}, true}, 0);

//    EXPECT_EQ(result.fUncompressedBytesRead, 160000000) << "Wrong number of uncompressed bytes read";
//    EXPECT_EQ(result.fCompressedBytesRead, 1316837) << "Wrong number of compressed bytes read";
// }

// TEST(ReadSpeedCLI, CheckFilenames)
// {
//    const std::vector<std::string> baseArgs{"root-readspeed", "--ntuples", "n", "--fields", "x", "--files"};
//    const std::vector<std::string> inFiles{"file-a.root", "file-b.root", "file-c.root"};

//    const auto allArgs = ConcatVectors(baseArgs, inFiles);

//    const auto parsedArgs = ParseArgs(allArgs);
//    const auto outFiles = parsedArgs.fData.fFileNames;

//    EXPECT_EQ(outFiles.size(), inFiles.size()) << "Number of parsed files does not match number of provided files.";
//    EXPECT_EQ(outFiles, inFiles) << "List of parsed files does not match list of provided files.";
// }

// TEST(ReadSpeedCLI, CheckNTuples)
// {
//    const std::vector<std::string> baseArgs{"root-readspeed", "--files", "doesnotexist.root",
//                                            "--fields",     "x",       "--trees"};
//    const std::vector<std::string> inTrees{"t1", "t2", "tree3"};

//    const auto allArgs = ConcatVectors(baseArgs, inTrees);

//    const auto parsedArgs = ParseArgs(allArgs);
//    const auto outTrees = parsedArgs.fData.fTreeOrNTupleNames;

//    EXPECT_EQ(outTrees.size(), inTrees.size()) << "Number of parsed trees does not match number of provided trees.";
//    EXPECT_EQ(outTrees, inTrees) << "List of parsed trees does not match list of provided trees.";
// }

TEST(ReadSpeedCLI, CheckFields)
{
   const std::vector<std::string> baseArgs{
      "root-readspeed", "--files", "doesnotexist.root", "--ntuples", "n", "--fields",
   };
   const std::vector<std::string> inFields{"x", "x_field", "long_field_name"};

   const auto allArgs = ConcatVectors(baseArgs, inFields);

   const auto parsedArgs = ParseArgs(allArgs);
   const auto outFields = parsedArgs.fData.fBranchOrFieldNames;

   EXPECT_EQ(outFields.size(), inFields.size())
      << "Number of parsed ntuples does not match number of provided ntuples.";
   EXPECT_EQ(outFields, inFields) << "List of parsed ntuples does not match list of provided ntuples.";
}

TEST(ReadSpeedCLI, HelpArg)
{
   const std::vector<std::string> allArgs{"root-readspeed", "--help"};

   const auto parsedArgs = ParseArgs(allArgs);

   EXPECT_TRUE(!parsedArgs.fShouldRun) << "Program running when using help argument";
}

TEST(ReadSpeedCLI, NoArgs)
{
   const std::vector<std::string> allArgs{"root-readspeed"};

   const auto parsedArgs = ParseArgs(allArgs);

   EXPECT_TRUE(!parsedArgs.fShouldRun) << "Program running when not using any arguments";
}

TEST(ReadSpeedCLI, InvalidArgs)
{
   const std::vector<std::string> allArgs{
      "root-readspeed", "--files", "doesnotexist.root", "--ntuples", "n", "--fields", "x", "--fake-flag",
   };

   const auto parsedArgs = ParseArgs(allArgs);

   EXPECT_TRUE(!parsedArgs.fShouldRun) << "Program running when using invalid flags";
}

TEST(ReadSpeedCLI, RegularArgs)
{
   const std::vector<std::string> allArgs{
      "root-readspeed", "--files", "doesnotexist.root", "--ntuples", "n", "--fields", "x",
   };

   const auto parsedArgs = ParseArgs(allArgs);

   EXPECT_TRUE(parsedArgs.fShouldRun) << "Program not running when given valid arguments";
   EXPECT_TRUE(!parsedArgs.fData.fUseRegex) << "Program using regex when it should not";
   EXPECT_EQ(parsedArgs.fNThreads, 0) << "Program not set to single thread mode";
}

TEST(ReadSpeedCLI, RegexArgs)
{
   const std::vector<std::string> allArgs{
      "root-readspeed", "--files", "doesnotexist.root", "--ntuples", "n", "--fields-regex", "x.*",
   };

   const auto parsedArgs = ParseArgs(allArgs);

   EXPECT_TRUE(parsedArgs.fShouldRun) << "Program not running when given valid arguments";
   EXPECT_TRUE(parsedArgs.fData.fUseRegex) << "Program not using regex when it should";
}

TEST(ReadSpeedCLI, AllBranches)
{
   const std::vector<std::string> allArgs{
      "root-readspeed", "--files", "doesnotexist.root", "--ntuples", "n", "--all-fields",
   };
   const std::vector<std::string> allBranches = {".*"};

   const auto parsedArgs = ParseArgs(allArgs);

   EXPECT_TRUE(parsedArgs.fShouldRun) << "Program not running when given valid arguments";
   EXPECT_TRUE(parsedArgs.fData.fUseRegex) << "Program not using regex when it should";
   EXPECT_TRUE(parsedArgs.fAllBranches) << "Program not checking for all branches when it should";
   EXPECT_EQ(parsedArgs.fData.fBranchOrFieldNames, allBranches) << "All branch regex not correct";
}

TEST(ReadSpeedCLI, MultipleThreads)
{
   const std::vector<std::string> allArgs{
      "root-readspeed", "--files", "doesnotexist.root", "--ntuples", "n", "--fields", "x", "--threads", "16",
   };
   const unsigned int threads = 16;

   const auto parsedArgs = ParseArgs(allArgs);

   EXPECT_TRUE(parsedArgs.fShouldRun) << "Program not running when given valid arguments";
   EXPECT_EQ(parsedArgs.fNThreads, threads) << "Program not using the correct amount of threads";
}

#ifdef R__USE_IMT
TEST(ReadSpeedCLI, WorkerThreadsHint)
{
   const unsigned int oldTasksPerWorker = ROOT::TTreeProcessorMT::GetTasksPerWorkerHint();
   const std::vector<std::string> allArgs{
      "root-readspeed",
      "--files",
      "doesnotexist.root",
      "--trees",
      "t",
      "--fields",
      "x",
      "--tasks-per-worker",
      std::to_string(oldTasksPerWorker + 10),
   };

   const auto parsedArgs = ParseArgs(allArgs);
   const auto newTasksPerWorker = ROOT::TTreeProcessorMT::GetTasksPerWorkerHint();

   EXPECT_TRUE(parsedArgs.fShouldRun) << "Program not running when given valid arguments";
   EXPECT_EQ(newTasksPerWorker, oldTasksPerWorker + 10) << "Tasks per worker hint not updated correctly";
}
#endif
