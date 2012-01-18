/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

//_________________________________________________________________________
//
// Split clusters with some criteria and calculate invariant mass
// to identify them as pi0 or conversion
//
//
//-- Author: Gustavo Conesa (LPSC-Grenoble)  
//_________________________________________________________________________

//////////////////////////////////////////////////////////////////////////////
  
  
// --- ROOT system --- 
#include <TList.h>
#include <TClonesArray.h>
#include <TObjString.h>
#include <TH3F.h>

// --- Analysis system --- 
#include "AliAnaInsideClusterInvariantMass.h" 
#include "AliCaloTrackReader.h"
#include "AliMCAnalysisUtils.h"
#include "AliStack.h"
#include "AliFiducialCut.h"
#include "TParticle.h"
#include "AliVCluster.h"
#include "AliAODEvent.h"
#include "AliAODMCParticle.h"
#include "AliEMCALGeoParams.h"

// --- Detectors --- 
//#include "AliPHOSGeoUtils.h"
#include "AliEMCALGeometry.h"

ClassImp(AliAnaInsideClusterInvariantMass)
  
//__________________________________________________________________
AliAnaInsideClusterInvariantMass::AliAnaInsideClusterInvariantMass() : 
  AliAnaCaloTrackCorrBaseClass(),  
  fCalorimeter(""),  
  fM02Cut(0),       fMinNCells(0),  
  fMassEtaMin(0),   fMassEtaMax(0),
  fMassPi0Min(0),   fMassPi0Max(0),
  fMassConMin(0),   fMassConMax(0)
{
  //default ctor
  
  // Init array of histograms
  for(Int_t i = 0; i < 7; i++){
    fhMassNLocMax1[i]  = 0;
    fhMassNLocMax2[i]  = 0;
    fhMassNLocMaxN[i]  = 0;
    fhNLocMax[i]       = 0;
    fhNLocMaxM02Cut[i] = 0;
    fhM02NLocMax1[i]   = 0;
    fhM02NLocMax2[i]   = 0;
    fhM02NLocMaxN[i]   = 0;
    fhNCellNLocMax1[i] = 0;
    fhNCellNLocMax2[i] = 0;
    fhNCellNLocMaxN[i] = 0;
    fhM02Pi0LocMax1[i] = 0;
    fhM02EtaLocMax1[i] = 0;
    fhM02ConLocMax1[i] = 0;
    fhM02Pi0LocMax2[i] = 0;
    fhM02EtaLocMax2[i] = 0;
    fhM02ConLocMax2[i] = 0;
    fhM02Pi0LocMaxN[i] = 0;
    fhM02EtaLocMaxN[i] = 0;
    fhM02ConLocMaxN[i] = 0;
    
  }
   
  InitParameters();
  
}

//_______________________________________________________________
TObjString *  AliAnaInsideClusterInvariantMass::GetAnalysisCuts()
{	
	//Save parameters used for analysis
  TString parList ; //this will be list of parameters used for this analysis.
  const Int_t buffersize = 255;
  char onePar[buffersize] ;
  
  snprintf(onePar,buffersize,"--- AliAnaInsideClusterInvariantMass ---\n") ;
  parList+=onePar ;	
  
  snprintf(onePar,buffersize,"Calorimeter: %s\n",        fCalorimeter.Data()) ;
  parList+=onePar ;
  snprintf(onePar,buffersize,"fM02Cut =%f \n"   ,        fM02Cut) ;
  parList+=onePar ;
  snprintf(onePar,buffersize,"fMinNCells =%f \n",        fMinNCells) ;
  parList+=onePar ;  
  snprintf(onePar,buffersize,"pi0 : %2.1f < m <%2.1f\n", fMassPi0Min,fMassPi0Max);
  parList+=onePar ;
  snprintf(onePar,buffersize,"eta : %2.1f < m <%2.1f\n", fMassEtaMin,fMassEtaMax);
  parList+=onePar ;
  snprintf(onePar,buffersize,"conv: %2.1f < m <%2.1f\n", fMassConMin,fMassConMax);
  parList+=onePar ;

  return new TObjString(parList) ;
  
}

//____________________________________________________________________________________________________
Bool_t AliAnaInsideClusterInvariantMass::AreNeighbours( const Int_t absId1, const Int_t absId2 ) const
{
  // Tells if (true) or not (false) two digits are neighbours
  // A neighbour is defined as being two digits which share a corner
	
  Bool_t areNeighbours = kFALSE ;
  Int_t nSupMod =0, nModule =0, nIphi =0, nIeta =0;
  Int_t nSupMod1=0, nModule1=0, nIphi1=0, nIeta1=0;
  Int_t relid1[2],  relid2[2] ;
  Int_t rowdiff=0,  coldiff=0;
  
  areNeighbours = kFALSE ;
  
  GetEMCALGeometry()->GetCellIndex(absId1, nSupMod,nModule,nIphi,nIeta);
  GetEMCALGeometry()->GetCellPhiEtaIndexInSModule(nSupMod,nModule,nIphi,nIeta, relid1[0],relid1[1]);
  
  GetEMCALGeometry()->GetCellIndex(absId2, nSupMod1,nModule1,nIphi1,nIeta1);
  GetEMCALGeometry()->GetCellPhiEtaIndexInSModule(nSupMod1,nModule1,nIphi1,nIeta1, relid2[0],relid2[1]);
  
  // In case of a shared cluster, index of SM in C side, columns start at 48 and ends at 48*2-1
  // C Side impair SM, nSupMod%2=1; A side pair SM nSupMod%2=0
  if(nSupMod1!=nSupMod){ 
    if(nSupMod1%2) relid1[1]+=AliEMCALGeoParams::fgkEMCALCols;
    else           relid2[1]+=AliEMCALGeoParams::fgkEMCALCols;
  }
	
  rowdiff = TMath::Abs( relid1[0] - relid2[0] ) ;  
  coldiff = TMath::Abs( relid1[1] - relid2[1] ) ;  
  
  if (( coldiff <= 1 )  && ( rowdiff <= 1 ) && (coldiff + rowdiff > 0)) 
    areNeighbours = kTRUE ;
  
  return areNeighbours;
}

//_____________________________________________________________________________________
TLorentzVector AliAnaInsideClusterInvariantMass::GetCellMomentum(const Int_t absId,
                                                                 Float_t en,
                                                                 AliVCaloCells * cells)
{

  // Cell momentum calculation for invariant mass
  
  Double_t cellpos[] = {0, 0, 0};
  GetEMCALGeometry()->GetGlobal(absId, cellpos);
  
  if(GetVertex(0)){//calculate direction from vertex
    cellpos[0]-=GetVertex(0)[0];
    cellpos[1]-=GetVertex(0)[1];
    cellpos[2]-=GetVertex(0)[2];  
  }
  
  Double_t r = TMath::Sqrt(cellpos[0]*cellpos[0]+cellpos[1]*cellpos[1]+cellpos[2]*cellpos[2] ) ;
  
  //If not calculated before, get the energy
  if(en<=0)
  {
    en = cells->GetCellAmplitude(absId);
    RecalibrateCellAmplitude(en,absId);  
  }
  
  TLorentzVector cellMom ;   
  cellMom.SetPxPyPzE( en*cellpos[0]/r,  en*cellpos[1]/r, en*cellpos[2]/r,  en) ;   

  return cellMom;
  
}

//________________________________________________________________
TList * AliAnaInsideClusterInvariantMass::GetCreateOutputObjects()
{  
  // Create histograms to be saved in output file and 
  // store them in outputContainer
  TList * outputContainer = new TList() ; 
  outputContainer->SetName("InsideClusterHistos") ; 
  
  Int_t nptbins  = GetHistogramRanges()->GetHistoPtBins();           Float_t ptmax  = GetHistogramRanges()->GetHistoPtMax();           Float_t ptmin  = GetHistogramRanges()->GetHistoPtMin();
  Int_t ssbins   = GetHistogramRanges()->GetHistoShowerShapeBins();  Float_t ssmax  = GetHistogramRanges()->GetHistoShowerShapeMax();  Float_t ssmin  = GetHistogramRanges()->GetHistoShowerShapeMin();
  Int_t mbins    = GetHistogramRanges()->GetHistoMassBins();         Float_t mmax   = GetHistogramRanges()->GetHistoMassMax();         Float_t mmin   = GetHistogramRanges()->GetHistoMassMin();
  Int_t ncbins   = GetHistogramRanges()->GetHistoNClusterCellBins(); Int_t   ncmax  = GetHistogramRanges()->GetHistoNClusterCellMax(); Int_t   ncmin  = GetHistogramRanges()->GetHistoNClusterCellMin(); 

  TString ptype[] ={"","#gamma","#gamma->e^{#pm}","#pi^{0}","#eta","e^{#pm}", "hadron"}; 
  TString pname[] ={"","Photon","Conversion",     "Pi0",    "Eta", "Electron","Hadron"};
  
  Int_t n = 1;
  
  if(IsDataMC()) n = 7;
  
  Int_t nMaxBins = 10;
  
  for(Int_t i = 0; i < n; i++){  
    
    fhMassNLocMax1[i]  = new TH2F(Form("hMassNLocMax1%s",pname[i].Data()),
                                  Form("Invariant mass of 2 highest energy cells %s",ptype[i].Data()),
                                  nptbins,ptmin,ptmax,mbins,mmin,mmax); 
    fhMassNLocMax1[i]->SetYTitle("M (MeV/c^2)");
    fhMassNLocMax1[i]->SetXTitle("E (GeV)");
    outputContainer->Add(fhMassNLocMax1[i]) ;   
    
    fhMassNLocMax2[i]  = new TH2F(Form("hMassNLocMax2%s",pname[i].Data()),
                                  Form("Invariant mass of 2 local maxima cells %s",ptype[i].Data()),
                                  nptbins,ptmin,ptmax,mbins,mmin,mmax); 
    fhMassNLocMax2[i]->SetYTitle("M (MeV/c^2)");
    fhMassNLocMax2[i]->SetXTitle("E (GeV)");
    outputContainer->Add(fhMassNLocMax2[i]) ;   

    fhMassNLocMaxN[i]  = new TH2F(Form("hMassNLocMaxN%s",pname[i].Data()),
                                  Form("Invariant mass of N>2 local maxima cells %s",ptype[i].Data()),
                                  nptbins,ptmin,ptmax,mbins,mmin,mmax); 
    fhMassNLocMaxN[i]->SetYTitle("M (MeV/c^2)");
    fhMassNLocMaxN[i]->SetXTitle("E (GeV)");
    outputContainer->Add(fhMassNLocMaxN[i]) ;   
    
    
    fhNLocMax[i]     = new TH2F(Form("hNLocMax%s",pname[i].Data()),
                             Form("Number of local maxima in cluster %s",ptype[i].Data()),
                             nptbins,ptmin,ptmax,nMaxBins,0,nMaxBins); 
    fhNLocMax[i]   ->SetYTitle("N maxima");
    fhNLocMax[i]   ->SetXTitle("E (GeV)");
    outputContainer->Add(fhNLocMax[i]) ; 

    fhNLocMaxM02Cut[i] = new TH2F(Form("hNLocMaxM02Cut%s",pname[i].Data()),
                              Form("Number of local maxima in cluster %s for M02 > %2.2f",ptype[i].Data(),fM02Cut),
                              nptbins,ptmin,ptmax,nMaxBins,0,nMaxBins); 
    fhNLocMaxM02Cut[i]->SetYTitle("N maxima");
    fhNLocMaxM02Cut[i]->SetXTitle("E (GeV)");
    outputContainer->Add(fhNLocMaxM02Cut[i]) ; 
    
    
    fhM02NLocMax1[i]     = new TH2F(Form("hM02NLocMax1%s",pname[i].Data()),
                                     Form("#lambda_{0}^{2} vs E for N max  = 1 %s",ptype[i].Data()),
                                     nptbins,ptmin,ptmax,ssbins,ssmin,ssmax); 
    fhM02NLocMax1[i]   ->SetYTitle("#lambda_{0}^{2}");
    fhM02NLocMax1[i]   ->SetXTitle("E (GeV)");
    outputContainer->Add(fhM02NLocMax1[i]) ; 
    
    fhM02NLocMax2[i]     = new TH2F(Form("hM02NLocMax2%s",pname[i].Data()),
                                     Form("#lambda_{0}^{2} vs E for N max  = 2 %s",ptype[i].Data()),
                                     nptbins,ptmin,ptmax,ssbins,ssmin,ssmax); 
    fhM02NLocMax2[i]   ->SetYTitle("#lambda_{0}^{2}");
    fhM02NLocMax2[i]   ->SetXTitle("E (GeV)");
    outputContainer->Add(fhM02NLocMax2[i]) ; 
    
    
    fhM02NLocMaxN[i]    = new TH2F(Form("hM02NLocMaxN%s",pname[i].Data()),
                                   Form("#lambda_{0}^{2} vs E for N max  > 2 %s",ptype[i].Data()),
                                   nptbins,ptmin,ptmax,ssbins,ssmin,ssmax); 
    fhM02NLocMaxN[i]   ->SetYTitle("#lambda_{0}^{2}");
    fhM02NLocMaxN[i]   ->SetXTitle("E (GeV)");
    outputContainer->Add(fhM02NLocMaxN[i]) ; 

    
    fhNCellNLocMax1[i]  = new TH2F(Form("hNCellNLocMax1%s",pname[i].Data()),
                                   Form("#lambda_{0}^{2} vs E for N max  = 1 %s",ptype[i].Data()),
                                   nptbins,ptmin,ptmax,ncbins,ncmin,ncmax); 
    fhNCellNLocMax1[i] ->SetYTitle("N cells");
    fhNCellNLocMax1[i] ->SetXTitle("E (GeV)");
    outputContainer->Add(fhNCellNLocMax1[i]) ; 
    
    fhNCellNLocMax2[i]     = new TH2F(Form("hNCellNLocMax2%s",pname[i].Data()),
                                    Form("#lambda_{0}^{2} vs E for N max  = 2 %s",ptype[i].Data()),
                                    nptbins,ptmin,ptmax,ncbins,ncmin,ncmax); 
    fhNCellNLocMax2[i]   ->SetYTitle("N cells");
    fhNCellNLocMax2[i]   ->SetXTitle("E (GeV)");
    outputContainer->Add(fhNCellNLocMax2[i]) ; 
    
    
    fhNCellNLocMaxN[i]     = new TH2F(Form("hNCellNLocMaxN%s",pname[i].Data()),
                                    Form("#lambda_{0}^{2} vs E for N max  > 2 %s",ptype[i].Data()),
                                    nptbins,ptmin,ptmax,ncbins,ncmin,ncmax); 
    fhNCellNLocMaxN[i]   ->SetYTitle("N cells");
    fhNCellNLocMaxN[i]   ->SetXTitle("E (GeV)");
    outputContainer->Add(fhNCellNLocMaxN[i]) ;
    
    
    fhM02Pi0LocMax1[i]     = new TH2F(Form("hM02Pi0LocMax1%s",pname[i].Data()),
                                    Form("#lambda_{0}^{2} vs E for mass range [%2.2f-%2.2f] MeV/c^{2} %s, for N Local max = 1",fMassPi0Min,fMassPi0Max,ptype[i].Data()),
                                    nptbins,ptmin,ptmax,ssbins,ssmin,ssmax); 
    fhM02Pi0LocMax1[i]   ->SetYTitle("#lambda_{0}^{2}");
    fhM02Pi0LocMax1[i]   ->SetXTitle("E (GeV)");
    outputContainer->Add(fhM02Pi0LocMax1[i]) ; 
    
    fhM02EtaLocMax1[i]     = new TH2F(Form("hM02EtaLocMax1%s",pname[i].Data()),
                                    Form("#lambda_{0}^{2} vs E for mass range [%2.2f-%2.2f] MeV/c^{2}, %s, for N Local max = 1",fMassEtaMin,fMassEtaMax,ptype[i].Data()),
                                    nptbins,ptmin,ptmax,ssbins,ssmin,ssmax); 
    fhM02EtaLocMax1[i]   ->SetYTitle("#lambda_{0}^{2}");
    fhM02EtaLocMax1[i]   ->SetXTitle("E (GeV)");
    outputContainer->Add(fhM02EtaLocMax1[i]) ; 
    
    fhM02ConLocMax1[i]    = new TH2F(Form("hM02ConLocMax1%s",pname[i].Data()),
                                   Form("#lambda_{0}^{2} vs E for mass range [%2.2f-%2.2f] MeV/c^{2}, %s, for N Local max = 1",fMassConMin,fMassConMax,ptype[i].Data()),
                                   nptbins,ptmin,ptmax,ssbins,ssmin,ssmax); 
    fhM02ConLocMax1[i]   ->SetYTitle("#lambda_{0}^{2}");
    fhM02ConLocMax1[i]   ->SetXTitle("E (GeV)");
    outputContainer->Add(fhM02ConLocMax1[i]) ; 
   
    fhM02Pi0LocMax2[i]     = new TH2F(Form("hM02Pi0LocMax2%s",pname[i].Data()),
                                     Form("#lambda_{0}^{2} vs E for mass range [%2.2f-%2.2f] MeV/c^{2} %s, for N Local max = 2",fMassPi0Min,fMassPi0Max,ptype[i].Data()),
                                     nptbins,ptmin,ptmax,ssbins,ssmin,ssmax); 
    fhM02Pi0LocMax2[i]   ->SetYTitle("#lambda_{0}^{2}");
    fhM02Pi0LocMax2[i]   ->SetXTitle("E (GeV)");
    outputContainer->Add(fhM02Pi0LocMax2[i]) ; 
    
    fhM02EtaLocMax2[i]     = new TH2F(Form("hM02EtaLocMax2%s",pname[i].Data()),
                                     Form("#lambda_{0}^{2} vs E for mass range [%2.2f-%2.2f] MeV/c^{2}, %s, for N Local max = 2",fMassEtaMin,fMassEtaMax,ptype[i].Data()),
                                     nptbins,ptmin,ptmax,ssbins,ssmin,ssmax); 
    fhM02EtaLocMax2[i]   ->SetYTitle("#lambda_{0}^{2}");
    fhM02EtaLocMax2[i]   ->SetXTitle("E (GeV)");
    outputContainer->Add(fhM02EtaLocMax2[i]) ; 
    
    fhM02ConLocMax2[i]    = new TH2F(Form("hM02ConLocMax2%s",pname[i].Data()),
                                    Form("#lambda_{0}^{2} vs E for mass range [%2.2f-%2.2f] MeV/c^{2}, %s, for N Local max = 2",fMassConMin,fMassConMax,ptype[i].Data()),
                                    nptbins,ptmin,ptmax,ssbins,ssmin,ssmax); 
    fhM02ConLocMax2[i]   ->SetYTitle("#lambda_{0}^{2}");
    fhM02ConLocMax2[i]   ->SetXTitle("E (GeV)");
    outputContainer->Add(fhM02ConLocMax2[i]) ; 

    fhM02Pi0LocMaxN[i]     = new TH2F(Form("hM02Pi0LocMaxN%s",pname[i].Data()),
                                     Form("#lambda_{0}^{2} vs E for mass range [%2.2f-%2.2f] MeV/c^{2} %s, for N Local max > 2",fMassPi0Min,fMassPi0Max,ptype[i].Data()),
                                     nptbins,ptmin,ptmax,ssbins,ssmin,ssmax); 
    fhM02Pi0LocMaxN[i]   ->SetYTitle("#lambda_{0}^{2}");
    fhM02Pi0LocMaxN[i]   ->SetXTitle("E (GeV)");
    outputContainer->Add(fhM02Pi0LocMaxN[i]) ; 
    
    fhM02EtaLocMaxN[i]     = new TH2F(Form("hM02EtaLocMaxN%s",pname[i].Data()),
                                     Form("#lambda_{0}^{2} vs E for mass range [%2.2f-%2.2f] MeV/c^{2}, %s, for N Local max > 2", fMassEtaMin,fMassEtaMax,ptype[i].Data()),
                                     nptbins,ptmin,ptmax,ssbins,ssmin,ssmax); 
    fhM02EtaLocMaxN[i]   ->SetYTitle("#lambda_{0}^{2}");
    fhM02EtaLocMaxN[i]   ->SetXTitle("E (GeV)");
    outputContainer->Add(fhM02EtaLocMaxN[i]) ; 
    
    fhM02ConLocMaxN[i]    = new TH2F(Form("hM02ConLocMaxN%s",pname[i].Data()),
                                    Form("#lambda_{0}^{2} vs E for mass range [%2.2f-%2.2f], %s, for N Local max > 2",fMassConMin,fMassConMax,ptype[i].Data()),
                                    nptbins,ptmin,ptmax,ssbins,ssmin,ssmax); 
    fhM02ConLocMaxN[i]   ->SetYTitle("#lambda_{0}^{2}");
    fhM02ConLocMaxN[i]   ->SetXTitle("E (GeV)");
    outputContainer->Add(fhM02ConLocMaxN[i]) ; 
    
  }
  
  return outputContainer ;
  
}

//________________________________________________________________________________________________________
Int_t AliAnaInsideClusterInvariantMass::GetNumberOfLocalMaxima(AliVCluster* cluster, AliVCaloCells* cells,
                                                               Int_t *absIdList,     Float_t *maxEList) 
{
  // Find local maxima in cluster
    
  Float_t locMaxCut = 0; // not used for the moment
  
  Int_t iDigitN = 0 ;
  Int_t iDigit  = 0 ;
  Int_t absId1 = -1 ;
  Int_t absId2 = -1 ;
  const Int_t nCells = cluster->GetNCells();
  
  //printf("cluster : ncells %d \n",nCells);
  for(iDigit = 0; iDigit < nCells ; iDigit++){
    absIdList[iDigit] = cluster->GetCellsAbsId()[iDigit]  ; 
   /* 
    Float_t en = cells->GetCellAmplitude(absIdList[iDigit]);
    RecalibrateCellAmplitude(en,absIdList[iDigit]);  
    Int_t icol = -1, irow = -1, iRCU = -1;
    Int_t sm = GetCaloUtils()->GetModuleNumberCellIndexes(absIdList[iDigit], fCalorimeter, icol, irow, iRCU) ;

    printf("\t cell %d, id %d, sm %d, col %d, row %d, e %f\n", iDigit, absIdList[iDigit], sm, icol, irow, en );
    */ 
  }
  
  
  for(iDigit = 0 ; iDigit < nCells; iDigit++) {   
    if(absIdList[iDigit]>=0) {
      
      absId1 = absIdList[iDigit] ;
      //printf("%d : absID111 %d, %s\n",iDigit, absId1,fCalorimeter.Data());

      Float_t en1 = cells->GetCellAmplitude(absId1);
      RecalibrateCellAmplitude(en1,absId1);  
      
      for(iDigitN = 0; iDigitN < nCells; iDigitN++) {	
        
        absId2 = absIdList[iDigitN] ;
        
        if(absId2==-1) continue;
        
        //printf("\t %d : absID222 %d, %s\n",iDigitN, absId2,fCalorimeter.Data());

        Float_t en2 = cells->GetCellAmplitude(absId2);
        RecalibrateCellAmplitude(en2,absId2);
        
        if ( AreNeighbours(absId1, absId2) ) {
          
          if (en1 > en2 ) {    
            absIdList[iDigitN] = -1 ;
            //printf("\t \t indexN %d not local max\n",iDigitN);
            // but may be digit too is not local max ?
            if(en1 < en2 + locMaxCut) {
              //printf("\t \t index %d not local max cause locMaxCut\n",iDigit);
              absIdList[iDigit] = -1 ;
            }
          }
          else {
            absIdList[iDigit] = -1 ;
            //printf("\t \t index %d not local max\n",iDigitN);
            // but may be digitN too is not local max ?
            if(en1 > en2 - locMaxCut) 
            {
              absIdList[iDigitN] = -1 ; 
              //printf("\t \t indexN %d not local max cause locMaxCut\n",iDigit);
            }
          } 
        } // if Areneighbours
      } // while digitN
    } // slot not empty
  } // while digit
  
  iDigitN = 0 ;
  for(iDigit = 0; iDigit < nCells; iDigit++) { 
    if(absIdList[iDigit]>=0 ){
      absIdList[iDigitN] = absIdList[iDigit] ;
      Float_t en = cells->GetCellAmplitude(absIdList[iDigit]);
      RecalibrateCellAmplitude(en,absIdList[iDigit]);  
      if(en < 0.1) continue; // Maxima only with seed energy at least
      maxEList[iDigitN] = en ;
      //printf("Local max %d, id %d, en %f\n", iDigit,absIdList[iDigitN],en);
      iDigitN++ ; 
    }
  }
  
  //printf("N maxima %d \n",iDigitN);
  //for(Int_t imax = 0; imax < iDigitN; imax++) printf("imax %d, absId %d, Ecell %f\n",imax,absIdList[imax],maxEList[imax]);
  
  return iDigitN ;
  
}


//___________________________________________
void AliAnaInsideClusterInvariantMass::Init()
{ 
  //Init
  //Do some checks
  if(fCalorimeter == "PHOS" && !GetReader()->IsPHOSSwitchedOn() && NewOutputAOD()){
    printf("AliAnaInsideClusterInvariantMass::Init() - !!STOP: You want to use PHOS in analysis but it is not read!! \n!!Check the configuration file!!\n");
    abort();
  }
  else  if(fCalorimeter == "EMCAL" && !GetReader()->IsEMCALSwitchedOn() && NewOutputAOD()){
    printf("AliAnaInsideClusterInvariantMass::Init() - !!STOP: You want to use EMCAL in analysis but it is not read!! \n!!Check the configuration file!!\n");
    abort();
  }
  
  if( GetReader()->GetDataType() == AliCaloTrackReader::kMC ){
    printf("AliAnaInsideClusterInvariantMass::Init() - !!STOP: You want to use pure MC data!!\n");
    abort();
    
  }
  
}

//_____________________________________________________
void AliAnaInsideClusterInvariantMass::InitParameters()
{
  //Initialize the parameters of the analysis.  
  AddToHistogramsName("AnaPi0InsideClusterInvariantMass_");
  
  fCalorimeter = "EMCAL" ;
  fM02Cut      = 0.26 ;
  fMinNCells   = 4 ;
  
  fMassEtaMin = 0.4;
  fMassEtaMax = 0.6;
  
  fMassPi0Min = 0.08;
  fMassPi0Max = 0.20;
  
  fMassConMin = 0.0;
  fMassConMax = 0.05;
  
}


//__________________________________________________________________
void  AliAnaInsideClusterInvariantMass::MakeAnalysisFillHistograms() 
{
  //Search for pi0 in fCalorimeter with shower shape analysis 
  
  TObjArray * pl       = 0x0; 
  AliVCaloCells* cells = 0x0;

  //Select the Calorimeter of the photon
  if(fCalorimeter == "PHOS"){
    pl    = GetPHOSClusters();
    cells = GetPHOSCells();
  }
  else if (fCalorimeter == "EMCAL"){
    pl    = GetEMCALClusters();
    cells = GetEMCALCells();
  }
  
  if(!pl || !cells) {
    Info("MakeAnalysisFillHistograms","TObjArray with %s clusters is NULL!\n",fCalorimeter.Data());
    return;
  }  
  
	if(fCalorimeter == "PHOS") return; // Not implemented for PHOS yet

  for(Int_t icluster = 0; icluster < pl->GetEntriesFast(); icluster++){
    AliVCluster * cluster = (AliVCluster*) (pl->At(icluster));	

    // Study clusters with large shape parameter
    Float_t en = cluster->E();
    Float_t l0 = cluster->GetM02();
    Int_t   nc = cluster->GetNCells();

    //If too small or big E or low number of cells, skip it
    if( ( en < GetMinEnergy() || en > GetMaxEnergy() ) && nc < fMinNCells) continue ; 
  
    Int_t    absId1    = -1; Int_t absId2 = -1;
    Int_t   *absIdList = new Int_t  [nc]; 
    Float_t *maxEList  = new Float_t[nc]; 
    Int_t    nMax      = GetNumberOfLocalMaxima(cluster, cells, absIdList, maxEList) ;
    
    if (nMax <= 0) {
      printf("AliAnaInsideClusterInvariantMass::MakeAnalysisFillHistograms() - No local maximum found!");
      delete [] absIdList ;
      delete [] maxEList  ;
      return;
    }
    
    fhNLocMax[0]->Fill(en,nMax);
    
    if     ( nMax == 1  ) { fhM02NLocMax1[0]->Fill(en,l0) ; fhNCellNLocMax1[0]->Fill(en,nc) ; }
    else if( nMax == 2  ) { fhM02NLocMax2[0]->Fill(en,l0) ; fhNCellNLocMax2[0]->Fill(en,nc) ; }
    else if( nMax >= 3  ) { fhM02NLocMaxN[0]->Fill(en,l0) ; fhNCellNLocMaxN[0]->Fill(en,nc) ; }
    else printf("N max smaller than 1 -> %d \n",nMax);

    // Play with the MC stack if available
    // Check origin of the candidates
    Int_t mcindex = -1;
    if(IsDataMC()){
      
      Int_t tag	= GetMCAnalysisUtils()->CheckOrigin(cluster->GetLabels(),cluster->GetNLabels(), GetReader(), 0);
            
      if      ( GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCPi0)  )      mcindex = kmcPi0;
      else if ( GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCEta)  )      mcindex = kmcEta;
      else if ( GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCPhoton) && 
               !GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCConversion)) mcindex = kmcPhoton;
      else if ( GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCPhoton) && 
                GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCConversion)) mcindex = kmcConversion;
      else if ( GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCElectron))   mcindex = kmcElectron;
      else                                                                                mcindex = kmcHadron;
      
/*      printf("AliAnaInsideClusterInvariantMass::FillAnalysisMakeHistograms() - tag %d, photon %d, prompt %d, frag %d, isr %d, pi0 decay %d, eta decay %d, other decay %d \n conv %d, pi0 %d, hadron %d, electron %d, unk %d, muon %d,pion %d, proton %d, neutron %d, kaon %d, antiproton %d, antineutron %d, bad %d\n",tag,
             GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCPhoton),
             GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCPrompt),
             GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCFragmentation),
             GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCISR),
             GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCPi0Decay),
             GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCEtaDecay),
             GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCOtherDecay),

             GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCConversion),
             GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCPi0),
             GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCOther),
             GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCElectron),
             GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCUnknown),
             GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCMuon), 
             GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCPion),
             GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCProton), 
             GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCAntiNeutron),
             GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCKaon), 
             GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCAntiProton), 
             GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCAntiNeutron),
             GetMCAnalysisUtils()->CheckTagBit(tag,AliMCAnalysisUtils::kMCBadLabel)
);
*/      
      
      fhNLocMax[mcindex]->Fill(en,nMax);
      
      if     (nMax == 1 ) { fhM02NLocMax1[mcindex]->Fill(en,l0) ; fhNCellNLocMax1[mcindex]->Fill(en,nc) ; }
      else if(nMax == 2 ) { fhM02NLocMax2[mcindex]->Fill(en,l0) ; fhNCellNLocMax2[mcindex]->Fill(en,nc) ; }
      else if(nMax >= 3 ) { fhM02NLocMaxN[mcindex]->Fill(en,l0) ; fhNCellNLocMaxN[mcindex]->Fill(en,nc) ; }
      
    }  
    
    if( l0 < fM02Cut) {
      delete [] absIdList ;
      delete [] maxEList  ;
      continue;    
    }
        
    fhNLocMaxM02Cut[0]->Fill(en,nMax);
    if(IsDataMC()) fhNLocMaxM02Cut[mcindex]->Fill(en,nMax);
    
    // Get the 2 max indeces and do inv mass
    
    if     ( nMax == 2 ) {
      absId1 = absIdList[0];
      absId2 = absIdList[1];
    }
    else if( nMax == 1 )
    {
      
      absId1 = absIdList[0];

      //Find second highest energy cell

      Float_t enmax = 0 ;
      for(Int_t iDigit = 0 ; iDigit < cluster->GetNCells() ; iDigit++){
        Int_t absId = cluster->GetCellsAbsId()[iDigit];
        if( absId == absId1 ) continue ; 
        Float_t endig = cells->GetCellAmplitude(absId);
        RecalibrateCellAmplitude(endig,absId); 
        if(endig > enmax) {
          enmax  = endig ;
          absId2 = absId ;
        }
      }// cell loop
    }// 1 maxima 
    else {  // n max > 2
      // loop on maxima, find 2 highest
      
      // First max
      Float_t enmax = 0 ;
      for(Int_t iDigit = 0 ; iDigit < nMax ; iDigit++){
        Float_t endig = maxEList[iDigit];
        if(endig > enmax) {
          enmax  = endig ;
          absId1 = absIdList[iDigit];
        }
      }// first maxima loop

      // Second max 
      Float_t enmax2 = 0;
      for(Int_t iDigit = 0 ; iDigit < nMax ; iDigit++){
        if(absIdList[iDigit]==absId1) continue;
        Float_t endig = maxEList[iDigit];
        if(endig > enmax2) {
          enmax2  = endig ;
          absId2 = absIdList[iDigit];
        }
      }// second maxima loop

    } // n local maxima > 2
        
    Float_t en1 = -1, en2 = -1;
    SplitEnergy(absId1,absId2,cluster, cells,en1,en2);
    
    TLorentzVector cellMom1 = GetCellMomentum(absId1, en1, cells);
    TLorentzVector cellMom2 = GetCellMomentum(absId2, en2, cells);
    
    Float_t mass = (cellMom1+cellMom2).M();
    
    if     (nMax==1) 
    { 
      fhMassNLocMax1[0]->Fill(en,mass); 
      if     (mass < fMassConMax && mass > fMassConMin) fhM02ConLocMax1[0]->Fill(en,l0);
      else if(mass < fMassPi0Max && mass > fMassPi0Min) fhM02Pi0LocMax1[0]->Fill(en,l0);
      else if(mass < fMassEtaMax && mass > fMassEtaMin) fhM02EtaLocMax1[0]->Fill(en,l0);
    }  
    else if(nMax==2) {
      fhMassNLocMax2[0]->Fill(en,mass);
      if     (mass < fMassConMax && mass > fMassConMin) fhM02ConLocMax2[0]->Fill(en,l0);
      else if(mass < fMassPi0Max && mass > fMassPi0Min) fhM02Pi0LocMax2[0]->Fill(en,l0);
      else if(mass < fMassEtaMax && mass > fMassEtaMin) fhM02EtaLocMax2[0]->Fill(en,l0);        
    }
    else if(nMax >2) {
      fhMassNLocMaxN[0]->Fill(en,mass);
      if     (mass < fMassConMax && mass > fMassConMin) fhM02ConLocMaxN[0]->Fill(en,l0);
      else if(mass < fMassPi0Max && mass > fMassPi0Min) fhM02Pi0LocMaxN[0]->Fill(en,l0);
      else if(mass < fMassEtaMax && mass > fMassEtaMin) fhM02EtaLocMaxN[0]->Fill(en,l0);
    }
    
    if(IsDataMC()){
            
      if     (nMax==1) 
      { 
        fhMassNLocMax1[mcindex]->Fill(en,mass); 
        if     (mass < fMassConMax && mass > fMassConMin) fhM02ConLocMax1[mcindex]->Fill(en,l0);
        else if(mass < fMassPi0Max && mass > fMassPi0Min) fhM02Pi0LocMax1[mcindex]->Fill(en,l0);
        else if(mass < fMassEtaMax && mass > fMassEtaMin) fhM02EtaLocMax1[mcindex]->Fill(en,l0);
      }  
      else if(nMax==2) {
        fhMassNLocMax2[mcindex]->Fill(en,mass);
        if     (mass < fMassConMax && mass > fMassConMin) fhM02ConLocMax2[mcindex]->Fill(en,l0);
        else if(mass < fMassPi0Max && mass > fMassPi0Min)fhM02Pi0LocMax2[mcindex]->Fill(en,l0);
        else if(mass < fMassEtaMax && mass > fMassEtaMin) fhM02EtaLocMax2[mcindex]->Fill(en,l0);        
      }
      else if(nMax >2) {
        fhMassNLocMaxN[mcindex]->Fill(en,mass);
        if     (mass < fMassConMax && mass > fMassConMin) fhM02ConLocMaxN[mcindex]->Fill(en,l0);
        else if(mass < fMassPi0Max && mass > fMassPi0Min) fhM02Pi0LocMaxN[mcindex]->Fill(en,l0);
        else if(mass < fMassEtaMax && mass > fMassEtaMin) fhM02EtaLocMaxN[mcindex]->Fill(en,l0);
      }
      
    }//Work with MC truth first
  
    delete [] absIdList ;
    delete [] maxEList  ;

  }//loop
  
  if(GetDebug() > 1) printf("AliAnaInsideClusterInvariantMass::MakeAnalysisFillHistograms() - END \n");  
  
}

//______________________________________________________________________
void AliAnaInsideClusterInvariantMass::Print(const Option_t * opt) const
{
  //Print some relevant parameters set for the analysis
  if(! opt)
    return;
  
  printf("**** Print %s %s ****\n", GetName(), GetTitle() ) ;
  AliAnaCaloTrackCorrBaseClass::Print("");
  printf("Calorimeter     =     %s\n",  fCalorimeter.Data()) ;
  printf("lambda 0 squared >  %2.1f\n", fM02Cut);
  printf("pi0 : %2.2f<m<%2.2f \n",      fMassPi0Min,fMassPi0Max);
  printf("eta : %2.2f<m<%2.2f \n",      fMassEtaMin,fMassEtaMax);
  printf("conv: %2.2f<m<%2.2f \n",      fMassConMin,fMassConMax);

  printf("    \n") ;
  
} 

//____________________________________________________________________________________________
void AliAnaInsideClusterInvariantMass::RecalibrateCellAmplitude(Float_t & amp, const Int_t id)
{
  //Recaculate cell energy if recalibration factor
  
  Int_t icol     = -1; Int_t irow     = -1; Int_t iRCU     = -1;
  Int_t nModule  = GetModuleNumberCellIndexes(id,fCalorimeter, icol, irow, iRCU);
  
  if (GetCaloUtils()->IsRecalibrationOn()) {
    if(fCalorimeter == "PHOS") {
      amp *= GetCaloUtils()->GetPHOSChannelRecalibrationFactor(nModule,icol,irow);
    }
    else		                   {
      amp *= GetCaloUtils()->GetEMCALChannelRecalibrationFactor(nModule,icol,irow);
    }
  }
}

//________________________________________________________________________________________
void AliAnaInsideClusterInvariantMass::SplitEnergy(const Int_t absId1, const Int_t absId2,
                                                   AliVCluster* cluster, 
                                                   AliVCaloCells* cells,
                                                   Float_t & e1, Float_t & e2 )
{
  
  // Split energy of cluster between the 2 local maxima.
  const Int_t ncells  = cluster->GetNCells();  
  Int_t     absIdList[ncells]; 
  //printf("Split Local Max: 1) %d - 2) %d\n",absId1,absId2);
  for(Int_t iDigit  = 0; iDigit < ncells; iDigit++ ) {
    absIdList[iDigit] = cluster->GetCellsAbsId()[iDigit];
//    printf("iDigit %d, absId %d, Ecell %f\n",iDigit,absIdList[iDigit],cells->GetCellAmplitude(absIdList[iDigit]));
  }
    
  // SubCluster 1

  // Init counters and variables
  Int_t ncells1 = 1;
  Int_t absIdList1[ncells];  
  absIdList1[0] = absId1;
  //printf("First local max : absId1 %d %d \n",absId1, absIdList1[0]);  
  for(Int_t iDigit1 = 1; iDigit1 < ncells; iDigit1++) absIdList1[iDigit1] = -1;
  
  Float_t ecell1 = cells->GetCellAmplitude(absId1);
  RecalibrateCellAmplitude(ecell1,absId1);
  e1 =  ecell1;  
  
  Bool_t added = kTRUE;
  while (added) 
  {
    added = kFALSE;
    Int_t absId1New = absIdList1[ncells1-1];
    //printf("\t absId %d added \n",absId1New);

    Float_t e1New = cells->GetCellAmplitude(absId1New);
    RecalibrateCellAmplitude(e1New,absId1New);

    for(Int_t iDigit = 0; iDigit < ncells ; iDigit++)
    {
      Int_t absId = absIdList[iDigit] ;
      if(absId!=absId1New && absId!=absId2 && absId>=0)
      {
        //printf("\t \t iDig %d, absId %d, absIdNew %d\n",iDigit,absId, absId1New);
        if(AreNeighbours( absId1New,absId )){ 
          //printf("\t neighbours\n");

          Float_t en = cells->GetCellAmplitude(absId);
          RecalibrateCellAmplitude(en,absId);
          //printf("\t \t e1New %f, en %f \n",e1New,en);
          if(e1New > en){
            absIdList1[ncells1++] = absId; 
            absIdList [iDigit]    = -1; 
            e1+=en;
            added = kTRUE;
          } // Decreasing energy with respect reference
        } // Neighbours
      } //Not local maxima or already removed
    } // cell loop
    
  }// while cells added to list of cells for cl1
  
  // SubCluster 2
  
  // Init counters and variables
  Int_t ncells2 = 1;
  Int_t absIdList2[ncells];  
  absIdList2[0] = absId2;
  //printf("Second local max : absId2 %d %d \n",absId2, absIdList2[0]);  
  for(Int_t iDigit2 = 1; iDigit2 < ncells; iDigit2++) absIdList2[iDigit2] = -1;
  
  Float_t ecell2 = cells->GetCellAmplitude(absId2);
  RecalibrateCellAmplitude(ecell2,absId2);
  e2 =  ecell2;  
    
  added = kTRUE;
  while (added) 
  {
    added = kFALSE;
    Int_t absId2New = absIdList2[ncells2-1];
    //printf("\t absId %d added \n",absId2New);
    
    Float_t e2New = cells->GetCellAmplitude(absId2New);
    RecalibrateCellAmplitude(e2New,absId2New);
    
    for(Int_t iDigit = 0; iDigit < ncells ; iDigit++)
    {
      Int_t absId = absIdList[iDigit] ;
      if(absId!=absId2New && absId>=0)
      {
      //  printf("\t \t iDig %d, absId %d, absIdNew %d\n",iDigit,absId, absId2New);
        if(AreNeighbours( absId2New,absId )){ 
      //    printf("\t neighbours\n");
          
          Float_t en = cells->GetCellAmplitude(absId);
          RecalibrateCellAmplitude(en,absId);
          //printf("\t \t e2New %f, en %f \n",e2New,en);
          if(e2New > en){
            absIdList2[ncells2++] = absId; 
            absIdList [iDigit]    = -1; 
            e2+=en;
            added = kTRUE;
          } // Decreasing energy with respect reference
        } // Neighbours
      } //Not local maxima or already removed
    } // cell loop
    
  }// while cells added to list of cells for cl2  
  
  //printf("Cluster energy  = %f, Ecell1 = %f, Ecell2 = %f, Enew1 = %f, Enew2 = %f, ncells %d, ncells1 %d, ncells2 %d \n",
  //       cluster->E(),ecell1,ecell2,e1,e2,ncells,ncells1,ncells2);
  //if(ncells!=(ncells1+ncells2)) printf("\t Not all cells!\n");
 
}


