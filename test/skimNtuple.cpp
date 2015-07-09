// c++ -o skimNtuple `root-config --cflags --glibs --libs` -lm skimNtuple.cpp

/* TODOS
 - calc the HH mass with the proper algorithm
 - dump the second H and the HH info in the ntuple
*/


#include <iostream>
#include "TTree.h"
#include "TH1F.h"
#include "TFile.h"
#include "TBranch.h"
#include "TString.h"
#include "TLorentzVector.h"
#include "OfflineProducerHelper.h"

// bigTree is produced on an existing ntuple as follows (see at the end of the file) 
#include "bigTree.h" 
#include "smallTree.h"

using namespace std ;


// --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---

template <class T>
struct scoreSortSingle: public std::binary_function<pair <T, float> &, pair <T, float> &, bool>
{
  bool operator() (pair <T, float> & x, pair <T, float> & y)
    {
      return x.second < y.second ;
    }
} ;


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----


int main (int argc, char** argv)
{
  if (argc < 3) 
    {
      cerr << "missing input parameters" << endl ;
      exit (1) ;
    }
  TString inputFile = argv[1] ;
  TString outputFile = argv[2] ;

  OfflineProducerHelper oph ;

  TChain * bigChain = new TChain ("HTauTauTree/HTauTauTree") ;
  bigChain->Add (inputFile) ;
  bigTree theBigTree (bigChain) ;

  //Create a new file + a clone of old tree header. Do not copy events
  TFile * smallFile = new TFile (outputFile, "recreate") ;
  smallTree theSmallTree ("HTauTauTree") ;

  // initial parameters, might be read from a cfg file in the future
  float PUjetID_minCut = -0.5 ;

  int eventsNumber = theBigTree.fChain->GetEntries () ;
  eventsNumber = 100 ; //DEBUG
  float selectedEvents = 0 ;

  // loop over events
  for (Long64_t iEvent = 0 ; iEvent < eventsNumber ; ++iEvent) 
    {
      if (iEvent % 10000 == 0) cout << "reading event " << iEvent << endl ;

      theSmallTree.clearVars () ;
      theBigTree.GetEntry (iEvent) ;
      
      if (theBigTree.indexDau1->size () == 0) continue ;
 
      // the H > tautau candidate
      // ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
      
      // by now take the OS pair with largest pT
      int chosenTauPair = 0 ;
      
      int firstDaughterIndex = theBigTree.indexDau1->at (chosenTauPair) ;  
      TLorentzVector tlv_firstLepton
        (
          theBigTree.daughters_px->at (firstDaughterIndex),
          theBigTree.daughters_py->at (firstDaughterIndex),
          theBigTree.daughters_pz->at (firstDaughterIndex),
          theBigTree.daughters_e->at (firstDaughterIndex)
        ) ;
      int secondDaughterIndex = theBigTree.indexDau2->at (chosenTauPair) ;
      TLorentzVector tlv_secondLepton
        (
          theBigTree.daughters_px->at (secondDaughterIndex),
          theBigTree.daughters_py->at (secondDaughterIndex),
          theBigTree.daughters_pz->at (secondDaughterIndex),
          theBigTree.daughters_e->at (secondDaughterIndex)
        ) ;
      TLorentzVector tlv_tauH = tlv_firstLepton + tlv_secondLepton ;

      // the H > bb candidate
      // ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

      vector <pair <int, float> > jets_and_btag ;
      vector <pair <pair <int, int>, float> > pairs_and_btag ;      
      // loop over jets
      for (int iJet = 0 ; iJet < theBigTree.jets_px->size () ; ++iJet)
        {
          // PG filter jets at will
          if (theBigTree.jets_PUJetID->at (iJet) < PUjetID_minCut) continue ;
          jets_and_btag.push_back (std::pair <int, float> (
              iJet, theBigTree.bCSVscore->at (iJet)
            )) ;
          
          for (int jJet = iJet + 1 ; jJet < theBigTree.jets_px->size () ; ++jJet)
            {
              if (theBigTree.jets_PUJetID->at (jJet) < PUjetID_minCut) continue ;
              pairs_and_btag.push_back (pair <pair <int, int>, float> (
                  pair <int, int> (iJet, jJet), 
                  theBigTree.bCSVscore->at (iJet) + theBigTree.bCSVscore->at (jJet) 
                )) ;
            }
        } // loop over jets

      sort (jets_and_btag.rbegin (), jets_and_btag.rend (), scoreSortSingle<int> ()) ;
      sort (pairs_and_btag.rbegin (), pairs_and_btag.rend (), 
            scoreSortSingle<pair<int, int> > ()) ;
      int firstBjetIndex = jets_and_btag.at (0).first ;
      int secondBjetIndex = jets_and_btag.at (0).second ;
//      int firstBjetIndex = pairs_and_btag.at (0).first.first ;
//      int secondBjetIndex = pairs_and_btag.at (0).first.second ;


      // apply some selections here
      // ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

      if (false) continue ;

      // fill the variables of interest
      // ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

      theSmallTree.m_PUReweight = theBigTree.PUReweight ;
      theSmallTree.m_MC_weight = theBigTree.MC_weight ;
      theSmallTree.m_EventNumber = theBigTree.EventNumber ;
      theSmallTree.m_RunNumber = theBigTree.RunNumber ;
      
      theSmallTree.m_tauH_px = tlv_tauH.X () ;
      theSmallTree.m_tauH_py = tlv_tauH.Y () ;
      theSmallTree.m_tauH_pz = tlv_tauH.Z () ;
      theSmallTree.m_tauH_e = tlv_tauH.E () ;
      theSmallTree.m_tauH_mass = theBigTree.SVfitMass->at (chosenTauPair) ;

      theSmallTree.m_dau1_px = theBigTree.daughters_px->at (firstDaughterIndex) ;
      theSmallTree.m_dau1_py = theBigTree.daughters_py->at (firstDaughterIndex) ;
      theSmallTree.m_dau1_pz = theBigTree.daughters_pz->at (firstDaughterIndex) ;
      theSmallTree.m_dau1_e = theBigTree.daughters_e->at (firstDaughterIndex) ;
      theSmallTree.m_dau1_flav = theBigTree.daughters_charge->at (firstDaughterIndex) * 
                                 theBigTree.daughters_genindex->at (firstDaughterIndex) ;

      theSmallTree.m_dau2_px = theBigTree.daughters_px->at (secondDaughterIndex) ;
      theSmallTree.m_dau2_py = theBigTree.daughters_py->at (secondDaughterIndex) ;
      theSmallTree.m_dau2_pz = theBigTree.daughters_pz->at (secondDaughterIndex) ;
      theSmallTree.m_dau2_e = theBigTree.daughters_e->at (secondDaughterIndex) ;
      theSmallTree.m_dau2_flav = theBigTree.daughters_charge->at (secondDaughterIndex) * 
                                 theBigTree.daughters_genindex->at (secondDaughterIndex) ;

      // loop over leptons
      for (int iLep = 0 ; 
           (iLep < theBigTree.daughters_px->size ()) && (theSmallTree.m_nleps < 3) ; 
           ++iLep)
        {
          // skip the H decay candiates
          if (iLep == firstDaughterIndex || iLep == secondDaughterIndex) continue ;
          
          theSmallTree.m_leps_px.push_back (theBigTree.daughters_px->at (iLep)) ;
          theSmallTree.m_leps_py.push_back (theBigTree.daughters_py->at (iLep)) ;
          theSmallTree.m_leps_pz.push_back (theBigTree.daughters_pz->at (iLep)) ;
          theSmallTree.m_leps_e.push_back (theBigTree.daughters_e->at (iLep)) ;
          theSmallTree.m_leps_flav.push_back ( //FIXME check whether the index contains the charge
              theBigTree.daughters_charge->at (iLep) * 
              theBigTree.daughters_genindex->at (iLep)
            ) ;
          ++theSmallTree.m_nleps ;
        } // loop over leptons

      theSmallTree.m_bjet1_px = theBigTree.jets_px->at (firstBjetIndex) ;
      theSmallTree.m_bjet1_py = theBigTree.jets_py->at (firstBjetIndex) ;
      theSmallTree.m_bjet1_pz = theBigTree.jets_pz->at (firstBjetIndex) ;
      theSmallTree.m_bjet1_e = theBigTree.jets_e->at (firstBjetIndex) ;

      theSmallTree.m_bjet2_px = theBigTree.jets_px->at (secondBjetIndex) ;
      theSmallTree.m_bjet2_py = theBigTree.jets_py->at (secondBjetIndex) ;
      theSmallTree.m_bjet2_pz = theBigTree.jets_pz->at (secondBjetIndex) ;
      theSmallTree.m_bjet2_e = theBigTree.jets_e->at (secondBjetIndex) ;

      // loop over jets
      for (int iJet = 0 ; 
           (iJet < theBigTree.jets_px->size ()) && (theSmallTree.m_njets < 5) ; 
           ++iJet)
        {
          // PG filter jets at will
          if (theBigTree.jets_PUJetID->at (iJet) < PUjetID_minCut) continue ;
          
          // skip the H decay candiates
          if (iJet == firstBjetIndex || iJet == secondBjetIndex) continue ;

          theSmallTree.m_jets_px.push_back (theBigTree.jets_px->at (iJet)) ;
          theSmallTree.m_jets_py.push_back (theBigTree.jets_py->at (iJet)) ;
          theSmallTree.m_jets_pz.push_back (theBigTree.jets_pz->at (iJet)) ;
          theSmallTree.m_jets_e.push_back (theBigTree.jets_e->at (iJet)) ;
          theSmallTree.m_jets_btag.push_back (theBigTree.bCSVscore->at (iJet)) ;
          ++theSmallTree.m_njets ;
        } // loop over jets

      ++selectedEvents ; 
      theSmallTree.Fill () ;
    } // loop over events

  TH1F h_eff ("h_eff", "h_eff", 1, 0, 1) ;
  h_eff.Fill (0.5, selectedEvents / eventsNumber) ;
  h_eff.Write () ;
  smallFile->Write () ;
  delete smallFile ;
  return 0 ;
}



/* how to prepare bigTree:

 prepare the base class with ROOT
 ------------------------------------------------

   root /data_CMS/cms/govoni/test_submit_to_tier3/HiggsTauTauOutput_DY_-1Events_0Skipped_1436202480.82/output_9.root 
   TTree * t = (TTree *) _file0->Get ("HTauTauTree/HTauTauTree")
   t->MakeSelector ("bigTree")

 remove the following functions from the .h
 ------------------------------------------------
 
   virtual void    Init(TTree *tree);
   virtual Int_t   Version() const { return 2; }
   virtual void    Begin(TTree *tree);
   virtual void    SlaveBegin(TTree *tree);
   virtual Bool_t  Notify();
   virtual Bool_t  Process(Long64_t entry);
   virtual void    SetOption(const char *option) { fOption = option; }
   virtual void    SetObject(TObject *obj) { fObject = obj; }
   virtual void    SetInputList(TList *input) { fInput = input; }
   virtual TList  *GetOutputList() const { return fOutput; }
   virtual void    SlaveTerminate();
   virtual void    Terminate();

 move the Init implementation in the class definition
 ------------------------------------------------
 
 change the following line
 ------------------------------------------------
 
    bigTree(TTree ETC....  --->
    bigTree (TChain * inputChain) : fChain (inputChain) { Init(fChain) ; }

 remove all the rest of the implementations
 ------------------------------------------------

 delete bigTree.C
 ------------------------------------------------

 remove the following lines to simplify things
 ------------------------------------------------

#endif

#ifdef bigTree_cxx

 remove the inheritance:
 ------------------------------------------------

class bigTree : public TSelector { --->
class bigTree { --->

 remove the following lines
 ------------------------------------------------

 #include <TSelector.h>

 remove the following lines from Init
 ------------------------------------------------

   // Set branch addresses and branch pointers
   if (!tree) return;
   fChain = tree;
   fChain->SetMakeClass(1);

 remove the ClassDef call
 ------------------------------------------------

*/