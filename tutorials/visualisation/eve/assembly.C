/// \file
/// \ingroup tutorial_eve
/// Geometry detector assembly example
///
/// Modified to save the assembly as shape-extract.
/// 1. Run `root assembly.C`
///    This will produce assembly.root containing the extract.
/// 2. Display the assembly as:
///    `root show_extract.C("assembly.root")`
///
/// \image html eve_assembly.png
/// \macro_code
///
/// \author Andrei Gheata

void assembly()
{
   //--- Definition of a simple geometry
   gSystem->Load("libGeom");
   auto geom = new TGeoManager("Assemblies", "Geometry using assemblies");
   Int_t i;
   //--- define some materials
   auto matVacuum = new TGeoMaterial("Vacuum", 0, 0, 0);
   auto matAl = new TGeoMaterial("Al", 26.98, 13, 2.7);
   //--- define some media
   auto Vacuum = new TGeoMedium("Vacuum", 1, matVacuum);
   auto Al = new TGeoMedium("Aluminium", 2, matAl);

   //--- make the top container volume
   auto top = geom->MakeBox("TOP", Vacuum, 1000., 1000., 100.);
   geom->SetTopVolume(top);

   // Make the elementary assembly of the whole structure
   auto tplate = new TGeoVolumeAssembly("TOOTHPLATE");

   Int_t ntooth = 5;
   Double_t xplate = 25;
   Double_t yplate = 50;
   Double_t xtooth = 10;
   Double_t ytooth = 0.5 * yplate / ntooth;
   Double_t dshift = 2. * xplate + xtooth;
   Double_t xt, yt;

   auto plate = geom->MakeBox("PLATE", Al, xplate, yplate, 1);
   plate->SetLineColor(kBlue);
   auto tooth = geom->MakeBox("TOOTH", Al, xtooth, ytooth, 1);
   tooth->SetLineColor(kBlue);
   tplate->AddNode(plate, 1);
   for (i = 0; i < ntooth; i++) {
      xt = xplate + xtooth;
      yt = -yplate + (4 * i + 1) * ytooth;
      tplate->AddNode(tooth, i + 1, new TGeoTranslation(xt, yt, 0));
      xt = -xplate - xtooth;
      yt = -yplate + (4 * i + 3) * ytooth;
      tplate->AddNode(tooth, ntooth + i + 1, new TGeoTranslation(xt, yt, 0));
   }

   auto rot1 = new TGeoRotation();
   rot1->RotateX(90);
   TGeoRotation *rot;
   // Make a hexagone cell out of 6 toothplates. These can zip togeather
   // without generating overlaps (they are self-contained)
   TGeoVolume *cell = new TGeoVolumeAssembly("CELL");
   for (i = 0; i < 6; i++) {
      Double_t phi = 60. * i;
      Double_t phirad = phi * TMath::DegToRad();
      Double_t xp = dshift * TMath::Sin(phirad);
      Double_t yp = -dshift * TMath::Cos(phirad);
      rot = new TGeoRotation(*rot1);
      rot->RotateZ(phi);
      cell->AddNode(tplate, i + 1, new TGeoCombiTrans(xp, yp, 0, rot));
   }

   // Make a row as an assembly of cells, then combine rows in a honeycomb
   // structure. This again works without any need to define rows as
   // "overlapping"
   auto row = new TGeoVolumeAssembly("ROW");
   Int_t ncells = 5;
   for (i = 0; i < ncells; i++) {
      Double_t ycell = (2 * i + 1) * (dshift + 10);
      row->AddNode(cell, ncells + i + 1, new TGeoTranslation(0, ycell, 0));
      row->AddNode(cell, ncells - i, new TGeoTranslation(0, -ycell, 0));
   }

   Double_t dxrow = 3. * (dshift + 10.) * TMath::Tan(30. * TMath::DegToRad());
   Double_t dyrow = dshift + 10.;
   Int_t nrows = 5;
   for (i = 0; i < nrows; i++) {
      Double_t xrow = 0.5 * (2 * i + 1) * dxrow;
      Double_t yrow = 0.5 * dyrow;
      if ((i % 2) == 0)
         yrow = -yrow;
      top->AddNode(row, nrows + i + 1, new TGeoTranslation(xrow, yrow, 0));
      top->AddNode(row, nrows - i, new TGeoTranslation(-xrow, -yrow, 0));
   }

   //--- close the geometry
   geom->CloseGeometry();

   TEveManager::Create();

   auto node = gGeoManager->GetTopNode();
   auto en = new TEveGeoTopNode(gGeoManager, node);
   en->SetVisLevel(4);
   en->GetNode()->GetVolume()->SetVisibility(kFALSE);

   gEve->AddGlobalElement(en);

   gEve->Redraw3D(kTRUE);

   en->ExpandIntoListTreesRecursively();
   en->Save("assembly.root", "Assembly");
}
