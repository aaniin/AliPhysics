#ifndef ALIPERFORMANCETRD_H
#define ALIPERFORMANCETRD_H

// forward declarations
class TList;
class TH1F;
class TH2F;
class AliESDEvent;

#include "AliAnalysisTaskSE.h"

class  AliPerformanceTRD : public  AliAnalysisTaskSE
{
  public:
    //  two  class  constructors
    AliPerformanceTRD();
    AliPerformanceTRD(const  char *name); //name constructor

    //  class  destructor
    virtual ~AliPerformanceTRD();
    //  called  once  at  beginning  of  runtime
    virtual void UserCreateOutputObjects();
    //  called  for  each  event
    virtual void UserExec(Option_t* option);
    //  called  at  end  of  analysis
    virtual void Terminate(Option_t* option);

    // Use HLT ESD
    void SetUseHLT(Bool_t useHLT = kFALSE) {fUseHLT = useHLT;}

  private:
    TList *fOutputList; //!  output  list
    AliESDEvent *fESDEvent; //! ESD event
    Bool_t fUseHLT; //! use HLT

    //global track histograms
    TH1F *fHistPt_global; //! transversal momentum, global
    TH1F *fHistNTracklets_global; //! number of TRD tracklets per track, global
    TH1F *fHistPhi_global; //! azimutal angle of momentum, global

    //TRD track histograms
    TH1F *fHistPt_trd; //!  transversal momentum, TRD tracks only
    TH1F *fHistNTracklets_trd; //! number of TRD tracklets per TRD track, TRD tracks only
    TH1F *fHistPhi_trd; //! azimutal angle of momentum, TRD tracks only
    TH1F *fHistNTrdClustersPerTrack_trd; //! number of TRD clusters per track, TRD tracks only
    TH1F *fHistNTrdClustersPerTracklet_trd; //! number of TRD clusters per tracklet, TRD tracks only
    TH2F *fHistTrdSclice_trd; //! value of integrated TRD slices per track for each layer, TRD tracks only

    //event based histograms
    TH1F *fHistNTracks; //! number of tracks per event
    TH1F *fHistNTrdTracks; //! number of TRD tracks per event
    TH1F *fHistNTrdTracklets_counted; //! number of counted TRD tracklets per event
    TH1F *fHistNTrdTracklets; //! number of TRD tracklets per event
    
    //tracklet based histograms
    TH1F *fHistHalfChamberId; //! tracklet half-chamber ID


    //not implemented
    AliPerformanceTRD(const AliPerformanceTRD&);
    AliPerformanceTRD& operator=(const AliPerformanceTRD&);

  ClassDef(AliPerformanceTRD, 1);
};

#endif
