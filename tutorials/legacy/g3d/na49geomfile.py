## \file
## \ingroup tutorial_g3d_legacy
## Before executing this macro, the file geometry.C must have been executed
##
## \macro_code
##
## \author Wim Lavrijsen

import ROOT

ROOT.gBenchmark.Start( 'geometry' )
na = ROOT.TFile( 'na49.root', 'RECREATE' )
n49 = ROOT.gROOT.FindObject( 'na49' )
n49.Write()
na.Write()
na.Close()
ROOT.gBenchmark.Show( 'geometry' )

