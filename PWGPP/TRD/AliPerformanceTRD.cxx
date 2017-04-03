//TRD QA HLT
//Author: Sebastian Syrkowski, syrkowski@physi.uni-heidelberg.de

#include "TH1F.h"
#include "TChain.h"
#include "TList.h"
#include "AliESDEvent.h"
#include "AliESDtrack.h"
#include "AliESDTrdTrack.h"

#include "AliPerformanceTRD.h"
#include "AliESDInputHandler.h"
#include "AliVEventHandler.h"
#include "AliAnalysisManager.h"

#include "AliLog.h"
//#include "iostream"
//#include <AliSysInfo.h>

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
    fHistTRDSector(0),
    fHistTRDStack(0),
    fHistNTracks(0),
    fHistNTrdTracks(0),
    fHistNTrdTracklets(0)
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
    fHistTRDSector(0),
    fHistTRDStack(0),
    fHistNTracks(0),
    fHistNTrdTracks(0),
    fHistNTrdTracklets(0)
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
    fHistPt_global = new TH1F("fHistPt_global", "transversal momentum, all tracks", 100, 0, 100);
    fHistNTracklets_global = new TH1F("fHistNTracklets_global", "number of TRD tracklets per track, all tracks", 11, -0.5, 10.5);
    fHistPhi_global = new TH1F("fHistPhi_global", "Phi (azimutal angle of momentum), all tracks", 100, 0, 10);

    fHistPt_trd = new TH1F("fHistPt_trd", "transversal momentum, TRD tracks only", 100, 0, 100);
    fHistNTracklets_trd = new TH1F("fHistNTracklets_trd", "number of TRD tracklets per track, TRD tracks only", 11, -0.5, 10.5);
    fHistPhi_trd = new TH1F("fHistPhi_trd", "Phi (azimutal angle of momentum), TRD tracks only", 100, 0, 10);
    fHistTRDSector = new TH1F("fHistTRDSector", "TRD Sector of each TRD track", 21, -0.5, 20.5);
    fHistTRDStack = new TH1F("fHistTRDStack", "TRD Stack of each TRD track", 11, -0.5, 10.5);

    fHistNTracks = new TH1F("fHistNTracks", "number of tracks per event", 100, 0, 100);
    fHistNTrdTracks = new TH1F("fHistNTrdTracks","number of TRD tracks per event", 100, 0, 100);
    fHistNTrdTracklets = new TH1F("fHistNTrdTracklets","number of TRD tracklets per event", 1000, 0, 1000);

    // add histogggrams to our output list
    fOutputList -> Add(fHistPt_global);
    fOutputList -> Add(fHistNTracklets_global);
    fOutputList -> Add(fHistPhi_global);

    fOutputList -> Add(fHistPt_trd);
    fOutputList -> Add(fHistNTracklets_trd);
    fOutputList -> Add(fHistPhi_trd);
    fOutputList -> Add(fHistTRDSector);
    fOutputList -> Add(fHistTRDStack);

    fOutputList -> Add(fHistNTracks);
    fOutputList -> Add(fHistNTrdTracks);
    fOutputList -> Add(fHistNTrdTracklets);

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

    //loop over all tracks
    const Int_t  nTracks(fESDEvent->GetNumberOfTracks());
    for(Int_t iTrack(0); iTrack < nTracks; iTrack++)
    {
      AliESDtrack *track = fESDEvent->GetTrack(iTrack);
      if(!track) continue;
      //  fill  our  histogram
      fHistPt_global->Fill(track->Pt());
      fHistNTracklets_global->Fill(track->GetTRDntracklets());
      fHistPhi_global->Fill(track->Phi());
    }

    //loop over TRD Tracks only
    const Int_t  nTRDTracks(fESDEvent->GetNumberOfTrdTracks());
    for(Int_t iTrack(0); iTrack < nTRDTracks; iTrack++)
    {
      AliESDTrdTrack *trdtrack = fESDEvent->GetTrdTrack(iTrack);
      if(!trdtrack) continue;
      //  fill  our  histogram
      fHistPt_trd->Fill(trdtrack->Pt());
      fHistNTracklets_trd->Fill(trdtrack->GetNTracklets());
      fHistPhi_trd->Fill(trdtrack->Phi());
      fHistTRDSector->Fill(trdtrack->GetSector());
      fHistTRDStack->Fill(trdtrack->GetStack());
    }

    // fill event based histograms
    fHistNTracks->Fill(fESDEvent->GetNumberOfTracks());
    fHistNTrdTracks->Fill(fESDEvent->GetNumberOfTrdTracks());
    fHistNTrdTracklets->Fill(fESDEvent->GetNumberOfTrdTracklets());

    //  and  save  the  data  gathered  in  this  iteration
    PostData(1,  fOutputList);
}

void AliPerformanceTRD::Terminate(Option_t *)
{
  // Draw some histogram at the end.
}
