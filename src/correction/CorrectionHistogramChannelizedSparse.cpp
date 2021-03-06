/// \file QnCorrectionsHistogramChannelizedSparse.cxx
/// \brief Implementation of the single multidimensional sparse histograms with channel support

#include "TList.h"

#include "CorrectionAxisSet.hpp"
#include "CorrectionHistogramChannelizedSparse.hpp"

/// \cond CLASSIMP
ClassImp(Qn::CorrectionHistogramChannelizedSparse);
/// \endcond
namespace Qn {
/// Normal constructor
///
/// Stores the set of variables that identify the
/// different event classes passing them to its parent
/// and prepares the object for actual histogram
/// creation or attachment
///
/// \param name base for the name of the histograms
/// \param title base for the title of the histograms
/// \param ecvs the event classes variables set
/// \param nNoOfChannels the number of channels associated
CorrectionHistogramChannelizedSparse::CorrectionHistogramChannelizedSparse(std::string name,
                                                                           std::string title,
                                                                           const CorrectionAxisSet &ecvs,
                                                                           Int_t nNoOfChannels) : CorrectionHistogramBase(name, title, ecvs), fNoOfChannels(nNoOfChannels) {}

CorrectionHistogramChannelizedSparse::CorrectionHistogramChannelizedSparse(std::string name,
                                                                           const CorrectionAxisSet &ecvs,
                                                                           Int_t nNoOfChannels) : CorrectionHistogramBase(name, name, ecvs), fNoOfChannels(nNoOfChannels) {}
/// Default destructor
/// Releases the memory taken
CorrectionHistogramChannelizedSparse::~CorrectionHistogramChannelizedSparse() {
  delete[] fUsedChannel;
  delete[] fChannelMap;
}

/// Creates the support histogram for the channelize histogram function
///
/// Based in the event classes variables set in the parent class
/// and the channel information passed as parameters
/// the values multidimensional histogram is
/// created.
///
/// The histogram is added to the passed histogram list
///
/// The actual number of channels is stored and a mask from
/// external channel number to histogram channel number. If
/// bUsedChannel is nullptr all channels
/// within fNoOfChannels are assigned to this profile.
/// \param histogramList list where the histograms have to be added
/// \param bUsedChannel array of booleans one per each channel
/// \return true if properly created
Bool_t CorrectionHistogramChannelizedSparse::CreateChannelizedHistogram(TList *histogramList,
                                                                        const Bool_t *bUsedChannel) {
  /* let's build the histograms names and titles */
  TString histoName = GetName();
  TString histoTitle = GetTitle();
  TString entriesHistoName = GetName();
  entriesHistoName += szEntriesHistoSuffix;
  TString entriesHistoTitle = GetTitle();
  entriesHistoTitle += szEntriesHistoSuffix;
  /* we open space for channel variable as well */
  unsigned int nVariables = fEventClassVariables.GetSize();
  auto *minvals = new Double_t[nVariables + 1];
  auto *maxvals = new Double_t[nVariables + 1];
  auto *nbins = new Int_t[nVariables + 1];
  /* get the multidimensional structure */
  fEventClassVariables.GetMultidimensionalConfiguration(nbins, minvals, maxvals);
  /* lets consider now the channel information */
  fUsedChannel = new Bool_t[fNoOfChannels];
  fChannelMap = new Int_t[fNoOfChannels];
  fActualNoOfChannels = 0;
  for (Int_t ixChannel = 0; ixChannel < fNoOfChannels; ixChannel++) {
    if (bUsedChannel != nullptr) {
      fUsedChannel[ixChannel] = bUsedChannel[ixChannel];
    } else {
      fUsedChannel[ixChannel] = kTRUE;
    }
    if (fUsedChannel[ixChannel]) {
      fChannelMap[ixChannel] = fActualNoOfChannels;
      fActualNoOfChannels++;
    }
  }
  /* There will be a wrong external view of the channel number especially */
  /* manifested when there are holes in the channel assignment */
  /* so, lets complete the dimension information */
  /* WARNING: be aware that ROOT does not keep label information when projecting THn */
  minvals[nVariables] = -0.5;
  maxvals[nVariables] = -0.5 + fActualNoOfChannels;
  nbins[nVariables] = fActualNoOfChannels;
  /* create the values multidimensional histogram */
  fValues = new THnSparseF(histoName, histoTitle, nVariables + 1, nbins, minvals, maxvals);
  /* now let's set the proper binning and label on each axis */
  for (unsigned int var = 0; var < nVariables; var++) {
    fValues->GetAxis(var)->Set(fEventClassVariables[var].GetNBins(), fEventClassVariables[var].GetBins());
    fValues->GetAxis(var)->SetTitle(fEventClassVariables[var].GetLabel().data());
  }
  /* and now the channel axis */
  fValues->GetAxis(nVariables)->SetTitle(szChannelAxisTitle);
  /* and now set the proper channel labels if needed */
  if (fActualNoOfChannels != fNoOfChannels) {
    for (Int_t ixChannel = 0; ixChannel < fNoOfChannels; ixChannel++) {
      if (fUsedChannel[ixChannel]) {
        fValues->GetAxis(nVariables)->SetBinLabel(fChannelMap[ixChannel] + 1, Form("%d", ixChannel));
      }
    }
  }
  fValues->Sumw2();
  histogramList->Add(fValues);
  delete[] minvals;
  delete[] maxvals;
  delete[] nbins;
  return kTRUE;
}

/// Get the bin number for the current variable content and passed channel
///
/// The bin number identifies the event class the current
/// variable content points to under the passed channel.
///
/// \param variableContainer the current variables content addressed by var Id
/// \param nChannel the interested external channel number
/// \return the associated bin to the current variables content
Long64_t CorrectionHistogramChannelizedSparse::GetBin(Int_t nChannel) {
  FillBinAxesValues(fChannelMap[nChannel]);
  /* store the channel number */
  return fValues->GetBin(fBinAxesValues);
}

/// Check the validity of the content of the passed bin
/// This kind of histograms cannot validate the bin content so, it
/// is always valid.
/// \param bin the bin to check its content validity
/// \return kTRUE if the content is valid kFALSE otherwise
Bool_t CorrectionHistogramChannelizedSparse::BinContentValidated(Long64_t) {
  return kTRUE;
}

/// Get the bin content for the passed bin number
///
/// The bin number identifies a desired event class whose content
/// is requested.
///
/// \param bin the interested bin number
/// \return the bin number content
Float_t CorrectionHistogramChannelizedSparse::GetBinContent(Long64_t bin) {
  return fValues->GetBinContent(bin);
}

/// Get the bin content error for the passed bin number
///
/// The bin number identifies a desired event class whose content
/// error is requested.
///
/// \param bin the interested bin number
/// \return the bin number content error
Float_t CorrectionHistogramChannelizedSparse::GetBinError(Long64_t bin) {
  return fValues->GetBinError(bin);
}

/// Fills the histogram
///
/// The involved bin is computed according to the current variables
/// content and the passed external channel number. The bin is then
/// increased by the given weight.
///
/// \param variableContainer the current variables content addressed by var Id
/// \param nChannel the interested external channel number
/// \param weight the increment in the bin content
void CorrectionHistogramChannelizedSparse::Fill(Int_t nChannel, Float_t weight) {
  /* keep the total entries in fValues updated */
  Double_t nEntries = fValues->GetEntries();
  FillBinAxesValues(fChannelMap[nChannel]);
  /* and now update the bin */
  fValues->Fill(fBinAxesValues, weight);
  fValues->SetEntries(nEntries + 1);
}
}// namespace Qn
