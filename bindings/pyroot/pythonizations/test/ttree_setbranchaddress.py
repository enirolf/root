import unittest
from array import array

import ROOT
import numpy as np


class TTreeSetBranchAddress(unittest.TestCase):
    """
    Test for the pythonization of TTree::SetBranchAddress, which allows to pass proxy
    references as arguments from the Python side. Example:
    `v = ROOT.std.vector('int')()`
    `t.SetBranchAddress("my_vector_branch", v)`

    Since this pythonization is common to TTree and its subclasses, TChain, TNtuple
    and TNtupleD are also tested here.
    """

    filename = "treesetbranchaddress.root"
    treename = "mytree"
    tuplename = "mytuple"
    nentries = 2
    arraysize = 10
    more = 10

    # Setup
    @classmethod
    def setUpClass(cls):
        ROOT.gInterpreter.Declare('#include "TreeHelper.h"')
        ROOT.CreateTTree(cls.filename, cls.treename, cls.nentries, cls.arraysize, cls.more, "RECREATE")
        ROOT.CreateTNtuple(cls.filename, cls.tuplename, cls.nentries, cls.more, "UPDATE")

    # Helpers
    def get_tree_and_chain(self):
        f = ROOT.TFile(self.filename)
        t = f.Get(self.treename)
        # Prevent double deletion of the tree (Python and C++ TFile)
        ROOT.SetOwnership(t, False)

        c = ROOT.TChain(self.treename)
        c.Add(self.filename)
        c.Add(self.filename)

        return f, t, c

    def get_ntuples(self):
        f = ROOT.TFile(self.filename)

        nt = f.Get(self.tuplename)
        ROOT.SetOwnership(nt, False)

        ntd = f.Get(self.tuplename + "D")
        ROOT.SetOwnership(ntd, False)

        return f, nt, ntd

    # Tests
    # Basic type, array and struct leaf list do not actually need the pythonization,
    # but testing anyway for the sake of completeness
    def test_basic_type_branch(self):
        f, t, c = self.get_tree_and_chain()

        for ds in t, c:
            n = array("f", [0.0])
            ds.SetBranchAddress("floatb", n)
            ds.GetEntry(0)

            self.assertEqual(n[0], self.more)

    def test_array_branch(self):
        f, t, c = self.get_tree_and_chain()

        for ds in t, c:
            a = array("d", self.arraysize * [0.0])

            status = ds.SetBranchAddress("arrayb", a)
            self.assertEqual(status, 0)  # should return status 0 for success

            ds.GetEntry(0)

            for j in range(self.arraysize):
                self.assertEqual(a[j], j)

            # check if type mismatch is correctly detected
            a_wrong_type = array("f", self.arraysize * [0.0])
            status = ds.SetBranchAddress("arrayb", a_wrong_type)
            self.assertEqual(status, -2)  # should return error code because of type mismatch

    def test_numpy_array_branch(self):
        f, t, c = self.get_tree_and_chain()

        for ds in t, c:
            a = np.array(self.arraysize * [0.0])  # dtype='float64'

            status = ds.SetBranchAddress("arrayb", a)
            self.assertEqual(status, 0)  # should return status 0 for success

            ds.GetEntry(0)

            for j in range(self.arraysize):
                self.assertEqual(a[j], j)

            # check if type mismatch is correctly detected
            a_wrong_type = np.zeros(self.arraysize, dtype=np.float32)
            status = ds.SetBranchAddress("arrayb", a_wrong_type)
            self.assertEqual(status, -2)  # should return error code because of type mismatch

    def test_struct_branch_leaflist(self):
        f, t, c = self.get_tree_and_chain()

        for ds in t, c:
            ms = ROOT.MyStruct()
            ds.SetBranchAddress("structleaflistb", ms)
            ds.GetEntry(0)

            self.assertEqual(ms.myint1, self.more)
            self.assertEqual(ms.myint2, 0)

    # Vector and struct do benefit from the pythonization
    def test_vector_branch(self):
        f, t, c = self.get_tree_and_chain()

        for ds in t, c:
            v = ROOT.std.vector("double")()
            ds.SetBranchAddress("vectorb", v)
            ds.GetEntry(0)

            for j in range(self.arraysize):
                self.assertEqual(v[j], j)

    def test_struct_branch(self):
        f, t, c = self.get_tree_and_chain()

        for ds in t, c:
            ms = ROOT.MyStruct()
            ds.SetBranchAddress("structb", ms)
            ds.GetEntry(0)

            self.assertEqual(ms.myint1, self.more)
            self.assertEqual(ms.myint2, 0)

    def test_fallback_case(self):
        f, t, c = self.get_tree_and_chain()

        for ds in t, c:
            n = array("f", [0.0])
            b = ds.GetBranch("floatb")
            # Test an overload that uses the original SetBranchAddress proxy
            ds.SetBranchAddress("floatb", n, b)
            ds.GetEntry(0)

            self.assertEqual(n[0], self.more)

    def test_ntuples(self):
        f, nt, ntd = self.get_ntuples()

        colnames = ["x", "y", "z"]
        cols = [array("f", [0.0]) for _ in colnames]
        colsd = [array("d", [0.0]) for _ in colnames]
        ncols = len(cols)

        for ds, cs in [(nt, cols), (ntd, colsd)]:
            for i in range(ncols):
                ds.SetBranchAddress(colnames[i], cs[i])

            ds.GetEntry(0)

            for i in range(ncols):
                self.assertEqual(cs[i][0], i * self.more)

    def test_class_with_array_member(self):
        # 6468
        f, t, c = self.get_tree_and_chain()

        for ds in t, c:
            mc = ROOT.MyClass()
            ds.SetBranchAddress("clarrmember", mc)

            ds.GetEntry(0)
            self.assertEqual(mc.foo[0], 0.0)
            self.assertEqual(mc.foo[1], 1.0)

            ds.GetEntry(1)
            self.assertEqual(mc.foo[0], 1.0)
            self.assertEqual(mc.foo[1], 2.0)

    def test_np_conversion(self):
        # 18365
        a = np.zeros(3, np.uint16)
        a[0] = 1
        a[1] = 2
        a[2] = 3
        c = np.zeros(3, np.int16)
        c[0] = 4
        c[1] = 5
        c[2] = 6
        t = ROOT.TTree("t", "t")
        t.Branch("b", a, "b[3]/s")
        t.Branch("d", c, "d[3]/S")
        t.Fill()
        a[0] = 10
        a[1] = 20
        a[2] = 30
        c[0] = 40
        c[1] = 50
        c[2] = 60
        t.Fill()
        # t.Print()
        # t.Scan()
        t.SetBranchAddress("b", a)
        t.SetBranchAddress("d", c)
        t.GetEntry(0)
        self.assertEqual(a[0], 1)
        self.assertEqual(a[1], 2)
        self.assertEqual(a[2], 3)
        self.assertEqual(c[0], 4)
        self.assertEqual(c[1], 5)
        self.assertEqual(c[2], 6)
        t.GetEntry(1)
        self.assertEqual(a[0], 10)
        self.assertEqual(a[1], 20)
        self.assertEqual(a[2], 30)
        self.assertEqual(c[0], 40)
        self.assertEqual(c[1], 50)
        self.assertEqual(c[2], 60)


if __name__ == "__main__":
    unittest.main()
