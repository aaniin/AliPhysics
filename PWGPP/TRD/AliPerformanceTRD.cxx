//TRD QA HLT
//Author: Sebastian Syrkowski, syrkowski@physi.uni-heidelberg.de

#include "TH1F.h"
#include "TH2F.h"
#include "TChain.h"
#include "TList.h"
#include "AliESDEvent.h"
#include "AliESDtrack.h"
#include "AliESDTrdTracklet.h"

#include "AliPerformanceTRD.h"
#include "AliESDInputHandler.h"
#include "AliVEventHandler.h"
#include "AliAnalysisManager.h"

#include "AliLog.h"

//______________________________________________________________________________
AliPerformanceTRD::AliPerformanceTRD():
    AliAnalysisTaskSE(),
    fOutputList(0),
    fESDEvent(0),
    fUseHLT(kFALSE),
    fHistPt_global(0),
    fHistNTracklets_global(0),
    fHistPhi_global(0),
    fHistPt_trd(0),
    fHistNTracklets_trd(0),
    fHistPhi_trd(0),
    fHistNTrdClustersPerTrack_trd(0),
    fHistNTrdClustersPerTracklet_trd(0),
    fHistTrdSclice_trd(0),
    fHistNTracks(0),
    fHistNTrdTracks(0),
    fHistNTrdTracklets_counted(0),
    fHistNTrdTracklets(0),
    fHistHalfChamberId(0)
{
  //  ROOT  IO  constructor , don 't  allocate  memory  here!
}
//______________________________________________________________________________
AliPerformanceTRD::AliPerformanceTRD(const  char* name):
    AliAnalysisTaskSE(name),
    fOutputList(0),
    fESDEvent(0),
    fUseHLT(kFALSE),
    fHistPt_global(0),
    fHistNTracklets_global(0),
    fHistPhi_global(0),
    fHistPt_trd(0),
    fHistNTracklets_trd(0),
    fHistPhi_trd(0),
    fHistNTrdClustersPerTrack_trd(0),
    fHistNTrdClustersPerTracklet_trd(0),
    fHistTrdSclice_trd(0),
    fHistNTracks(0),
    fHistNTrdTracks(0),
    fHistNTrdTracklets_counted(0),
    fHistNTrdTracklets(0),
    fHistHalfChamberId(0)
{
    DefineInput (0, TChain::Class ());
    DefineOutput (1, TList::Class());
}
//______________________________________________________________________________
AliPerformanceTRD::~AliPerformanceTRD ()
{
    if(fOutputList) delete fOutputList;
    //do not delete because the histogram is added to fOutputList and is deleted by the owner
    //if(fHistPt) delete fHistPt;
}

void AliPerformanceTRD::UserCreateOutputObjects()
{
    //  create  a new  TList  that  OWNS  its  objects
    fOutputList = new  TList();
    fOutputList -> SetOwner(kTRUE);

    //  create  our  histo  and  add  it  to  the  list
    fHistPt_global = new TH1F("fHistPt_global", "transversal momentum, all tracks", 40, 0, 20);
    fHistNTracklets_global = new TH1F("fHistNTracklets_global", "number of TRD tracklets per track, all tracks", 11, -0.5, 10.5);
    fHistPhi_global = new TH1F("fHistPhi_global", "Phi (azimutal angle of momentum), all tracks", 100, 0, 2*3.14159265358979323846);
    
    fHistPt_trd = new TH1F("fHistPt_trd", "transversal momentum, TRD tracks only", 40, 0, 20);
    fHistNTracklets_trd = new TH1F("fHistNTracklets_trd", "number of TRD tracklets per track, TRD tracks only", 11, -0.5, 10.5);
    fHistPhi_trd = new TH1F("fHistPhi_trd", "Phi (azimutal angle of momentum), TRD tracks only", 100, 0, 2*3.14159265358979323846);
    fHistNTrdClustersPerTrack_trd = new TH1F("fHistNTrdClustersPerTrack_trd", "number of TRD clusters per track, TRD tracks only", 200, 0, 200);
    fHistNTrdClustersPerTracklet_trd = new TH1F("fHistNTrdClustersPerTracklet_trd", "number of TRD clusters per track divided by number of tracklets, TRD tracks only", 50, 0, 50);
    fHistTrdSclice_trd = new TH2F("fHistTrdSclice_trd","slice 0 (total integrated charge) for each layer, TRD tracks only", 100, 0, 6000, 9, -1.5, 7.5);

    fHistNTracks = new TH1F("fHistNTracks", "number of tracks per event", 100, 0, 500);
    fHistNTrdTracks = new TH1F("fHistNTrdTracks","number of TRD tracks per event", 100, 0, 100);
    fHistNTrdTracklets_counted = new TH1F("fHistNTrdTracklets_counted","number of counted TRD tracklets per event", 100, 0, 1000);
    fHistNTrdTracklets = new TH1F("fHistNTrdTracklets","number of TRD tracklets per event", 100, 0, 1000);
    
    fHistHalfChamberId = new TH1F("fHistHalfChamberId", "TRD tracklet half-chamber ID", 200,0,200);

    // add histogggrams to our output list
    fOutputList -> Add(fHistPt_global);
    fOutputList -> Add(fHistNTracklets_global);
    fOutputList -> Add(fHistPhi_global);

    fOutputList -> Add(fHistPt_trd);
    fOutputList -> Add(fHistNTracklets_trd);
    fOutputList -> Add(fHistPhi_trd);
    fOutputList -> Add(fHistNTrdClustersPerTrack_trd);
    fOutputList -> Add(fHistNTrdClustersPerTracklet_trd);
    fOutputList -> Add(fHistTrdSclice_trd);

    fOutputList -> Add(fHistNTracks);
    fOutputList -> Add(fHistNTrdTracks);
    fOutputList -> Add(fHistNTrdTracklets_counted);
    fOutputList -> Add(fHistNTrdTracklets);
    
    fOutputList -> Add(fHistHalfChamberId);

    //  add  the  list  to  our  output  file
    PostData(1, fOutputList);
}

void AliPerformanceTRD::UserExec(Option_t*)
{
    // Main loop
    // Called for each event

    AliESDInputHandler *esdH = dynamic_cast<AliESDInputHandler*>(AliAnalysisManager::GetAnalysisManager()->GetInputEventHandler());
    if(!esdH)
    {
      AliInfo("ERROR: Could not get ESDInputHandler");
      return;
    }
    if(fUseHLT)
    {
      fESDEvent = esdH->GetHLTEvent();
      if(!fESDEvent)
      {
        AliInfo("ERROR: HLTEvent unavailable from ESDInputHandler");
        return;
      }
    }
    else
    {
      fESDEvent = dynamic_cast<AliESDEvent*>(esdH->GetEvent());
      //fVEvent =InputEvent(); //this one does not currently work, TaskSE makes stupid assumptions about the tree
      if(!fESDEvent)
      {
        AliInfo("ERROR: Event unavailable from ESDInputHandler!");
        return;
      }
    }
    
    int nTracks=0;
    int nTrdTracks=0;
    int nTrdTracklets=0;
    
    //loop over all tracks
    const Int_t  nT(fESDEvent->GetNumberOfTracks());
    for(Int_t iT(0); iT < nT; iT++)
    {
      AliESDtrack *track = fESDEvent->GetTrack(iT);
      if(!track) continue;
      nTracks++;
      
      //  fill histograms with all tracks
      fHistPt_global->Fill(track->Pt());
      fHistNTracklets_global->Fill(track->GetTRDntracklets());
      fHistPhi_global->Fill(track->Phi());
      
      // fill histograms with tracks containing TRD information
      if (track->GetTRDntracklets()>0)
      {
        nTrdTracks++;
        nTrdTracklets+=track->GetTRDntracklets();
        
        fHistPt_trd->Fill(track->Pt());
        fHistNTracklets_trd->Fill(track->GetTRDntracklets());
        fHistPhi_trd->Fill(track->Phi());
        fHistNTrdClustersPerTrack_trd->Fill(track->GetNumberOfTRDClusters());
        fHistNTrdClustersPerTracklet_trd->Fill(track->GetNumberOfTRDClusters()/((Float_t)track->GetTRDntracklets()));
        
        for (int iLayer(0); iLayer<track->kTRDnPlanes; iLayer++)
        {
          fHistTrdSclice_trd->Fill(track->GetTRDslice(iLayer,0), iLayer); //slice 0: integrated total charge
        }
      }
    }
    
    // fill tracklet based histograms
    for (Int_t iTracklet(0); iTracklet<fESDEvent->GetNumberOfTrdTracklets(); iTracklet++)
    {
      AliESDTrdTracklet *tracklet =	fESDEvent->GetTrdTracklet(iTracklet);
      fHistHalfChamberId->Fill(tracklet->GetHCId());
    }
    
    // fill event based histograms
    fHistNTracks->Fill(nTracks);
    fHistNTrdTracks->Fill(nTrdTracks);
    fHistNTrdTracklets_counted->Fill(nTrdTracklets);
    fHistNTrdTracklets->Fill(fESDEvent->GetNumberOfTrdTracklets());

    //  and  save  the  data  gathered  in  this  iteration
    PostData(1,  fOutputList);
}

void AliPerformanceTRD::Terminate(Option_t *)
{
  // Draw some histogram at the end.
}
