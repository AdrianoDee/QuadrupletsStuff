#include "Validation/RecoTrack/interface/MultiTrackValidator.h"
#include "Validation/RecoTrack/interface/trackFromSeedFitFailed.h"

#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/GsfTrackReco/interface/GsfTrack.h"
#include "DataFormats/GsfTrackReco/interface/GsfTrackFwd.h"
#include "SimDataFormats/Track/interface/SimTrackContainer.h"
#include "SimDataFormats/Vertex/interface/SimVertexContainer.h"
#include "SimDataFormats/Associations/interface/TrackToTrackingParticleAssociator.h"
#include "SimTracker/TrackerHitAssociation/interface/TrackerHitAssociator.h"
#include "SimDataFormats/TrackingAnalysis/interface/TrackingParticle.h"
#include "SimDataFormats/TrackingAnalysis/interface/TrackingVertex.h"
#include "SimDataFormats/TrackingAnalysis/interface/TrackingVertexContainer.h"
#include "SimDataFormats/PileupSummaryInfo/interface/PileupSummaryInfo.h"
#include "SimDataFormats/EncodedEventId/interface/EncodedEventId.h"
#include "TrackingTools/TrajectoryState/interface/FreeTrajectoryState.h"
#include "TrackingTools/PatternTools/interface/TSCBLBuilderNoMaterial.h"
#include "SimTracker/TrackAssociation/plugins/ParametersDefinerForTPESProducer.h"
#include "SimTracker/TrackAssociation/plugins/CosmicParametersDefinerForTPESProducer.h"

#include "Geometry/CommonDetUnit/interface/GeomDetType.h"
#include "DataFormats/TrackerRecHit2D/interface/SiPixelRecHit.h"
#include "DataFormats/SiPixelCluster/interface/SiPixelCluster.h"

#include "DataFormats/TrackReco/interface/DeDxData.h"
#include "DataFormats/Common/interface/ValueMap.h"
#include "DataFormats/Common/interface/Ref.h"
#include "CommonTools/Utils/interface/associationMapFilterValues.h"
#include <type_traits>
#include <unordered_set>



#include "TMath.h"
#include <TF1.h>
#include <TFile.h>
#include <TH2D.h>
#include <TTree.h>
#include <TDirectory.h>
#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/Math/interface/PtEtaPhiMass.h"
//#include <iostream>

using namespace std;
using namespace edm;

typedef edm::Ref<edm::HepMCProduct, HepMC::GenParticle > GenParticleRef;

MultiTrackValidator::MultiTrackValidator(const edm::ParameterSet& pset):
  MultiTrackValidatorBase(pset,consumesCollector()),
  parametersDefinerIsCosmic_(parametersDefiner == "CosmicParametersDefinerForTP"),
  calculateDrSingleCollection_(pset.getUntrackedParameter<bool>("calculateDrSingleCollection")),
  doPlotsOnlyForTruePV_(pset.getUntrackedParameter<bool>("doPlotsOnlyForTruePV")),
  doSummaryPlots_(pset.getUntrackedParameter<bool>("doSummaryPlots")),
  doSimPlots_(pset.getUntrackedParameter<bool>("doSimPlots")),
  doSimTrackPlots_(pset.getUntrackedParameter<bool>("doSimTrackPlots")),
  doRecoTrackPlots_(pset.getUntrackedParameter<bool>("doRecoTrackPlots")),
  dodEdxPlots_(pset.getUntrackedParameter<bool>("dodEdxPlots")),
  doPVAssociationPlots_(pset.getUntrackedParameter<bool>("doPVAssociationPlots")),
  doSeedPlots_(pset.getUntrackedParameter<bool>("doSeedPlots")),
  simPVMaxZ_(pset.getUntrackedParameter<double>("simPVMaxZ"))
{
  ParameterSet psetForHistoProducerAlgo = pset.getParameter<ParameterSet>("histoProducerAlgoBlock");
  histoProducerAlgo_ = std::make_unique<MTVHistoProducerAlgoForTracker>(psetForHistoProducerAlgo, doSeedPlots_, consumesCollector());

  dirName_ = pset.getParameter<std::string>("dirName");
  UseAssociators = pset.getParameter< bool >("UseAssociators");

  tpNLayersToken_ = consumes<edm::ValueMap<unsigned int> >(pset.getParameter<edm::InputTag>("label_tp_nlayers"));
  tpNPixelLayersToken_ = consumes<edm::ValueMap<unsigned int> >(pset.getParameter<edm::InputTag>("label_tp_npixellayers"));
  tpNStripStereoLayersToken_ = consumes<edm::ValueMap<unsigned int> >(pset.getParameter<edm::InputTag>("label_tp_nstripstereolayers"));

  if(dodEdxPlots_) {
    m_dEdx1Tag = consumes<edm::ValueMap<reco::DeDxData> >(pset.getParameter< edm::InputTag >("dEdx1Tag"));
    m_dEdx2Tag = consumes<edm::ValueMap<reco::DeDxData> >(pset.getParameter< edm::InputTag >("dEdx2Tag"));
  }

  label_tv = consumes<TrackingVertexCollection>(pset.getParameter< edm::InputTag >("label_tv"));
  if(doPlotsOnlyForTruePV_ || doPVAssociationPlots_) {
    recoVertexToken_ = consumes<edm::View<reco::Vertex> >(pset.getUntrackedParameter<edm::InputTag>("label_vertex"));
    vertexAssociatorToken_ = consumes<reco::VertexToTrackingVertexAssociator>(pset.getUntrackedParameter<edm::InputTag>("vertexAssociator"));
  }

  tpSelector = TrackingParticleSelector(pset.getParameter<double>("ptMinTP"),
					pset.getParameter<double>("minRapidityTP"),
					pset.getParameter<double>("maxRapidityTP"),
					pset.getParameter<double>("tipTP"),
					pset.getParameter<double>("lipTP"),
					pset.getParameter<int>("minHitTP"),
					pset.getParameter<bool>("signalOnlyTP"),
					pset.getParameter<bool>("intimeOnlyTP"),
					pset.getParameter<bool>("chargedOnlyTP"),
					pset.getParameter<bool>("stableOnlyTP"),
					pset.getParameter<std::vector<int> >("pdgIdTP"));

  cosmictpSelector = CosmicTrackingParticleSelector(pset.getParameter<double>("ptMinTP"),
						    pset.getParameter<double>("minRapidityTP"),
						    pset.getParameter<double>("maxRapidityTP"),
						    pset.getParameter<double>("tipTP"),
						    pset.getParameter<double>("lipTP"),
						    pset.getParameter<int>("minHitTP"),
						    pset.getParameter<bool>("chargedOnlyTP"),
						    pset.getParameter<std::vector<int> >("pdgIdTP"));


  ParameterSet psetVsPhi = psetForHistoProducerAlgo.getParameter<ParameterSet>("TpSelectorForEfficiencyVsPhi");
  dRtpSelector = TrackingParticleSelector(psetVsPhi.getParameter<double>("ptMin"),
					  psetVsPhi.getParameter<double>("minRapidity"),
					  psetVsPhi.getParameter<double>("maxRapidity"),
					  psetVsPhi.getParameter<double>("tip"),
					  psetVsPhi.getParameter<double>("lip"),
					  psetVsPhi.getParameter<int>("minHit"),
					  psetVsPhi.getParameter<bool>("signalOnly"),
					  psetVsPhi.getParameter<bool>("intimeOnly"),
					  psetVsPhi.getParameter<bool>("chargedOnly"),
					  psetVsPhi.getParameter<bool>("stableOnly"),
					  psetVsPhi.getParameter<std::vector<int> >("pdgId"));

  dRtpSelectorNoPtCut = TrackingParticleSelector(0.0,
                                                 psetVsPhi.getParameter<double>("minRapidity"),
                                                 psetVsPhi.getParameter<double>("maxRapidity"),
                                                 psetVsPhi.getParameter<double>("tip"),
                                                 psetVsPhi.getParameter<double>("lip"),
                                                 psetVsPhi.getParameter<int>("minHit"),
                                                 psetVsPhi.getParameter<bool>("signalOnly"),
                                                 psetVsPhi.getParameter<bool>("intimeOnly"),
                                                 psetVsPhi.getParameter<bool>("chargedOnly"),
                                                 psetVsPhi.getParameter<bool>("stableOnly"),
                                                 psetVsPhi.getParameter<std::vector<int> >("pdgId"));

  useGsf = pset.getParameter<bool>("useGsf");

  _simHitTpMapTag = mayConsume<SimHitTPAssociationProducer::SimHitTPAssociationList>(pset.getParameter<edm::InputTag>("simHitTpMapTag"));

  if(calculateDrSingleCollection_) {
    labelTokenForDrCalculation = consumes<edm::View<reco::Track> >(pset.getParameter<edm::InputTag>("trackCollectionForDrCalculation"));
  }

  if(UseAssociators) {
    for (auto const& src: associators) {
      associatorTokens.push_back(consumes<reco::TrackToTrackingParticleAssociator>(src));
    }
  } else {
    for (auto const& src: associators) {
      associatormapStRs.push_back(consumes<reco::SimToRecoCollection>(src));
      associatormapRtSs.push_back(consumes<reco::RecoToSimCollection>(src));
    }
  }
}


MultiTrackValidator::~MultiTrackValidator() {}


void MultiTrackValidator::bookHistograms(DQMStore::IBooker& ibook, edm::Run const&, edm::EventSetup const& setup) {

  const auto minColl = -0.5;
  const auto maxColl = label.size()-0.5;
  const auto nintColl = label.size();

  auto binLabels = [&](MonitorElement *me) {
    TH1 *h = me->getTH1();
    for(size_t i=0; i<label.size(); ++i) {
      h->GetXaxis()->SetBinLabel(i+1, label[i].label().c_str());
    }
    return me;
  };

  //Booking histograms concerning with simulated tracks
  if(doSimPlots_) {
    ibook.cd();
    ibook.setCurrentFolder(dirName_ + "simulation");

    histoProducerAlgo_->bookSimHistos(ibook);

    ibook.cd();
    ibook.setCurrentFolder(dirName_);
  }

  for (unsigned int ww=0;ww<associators.size();ww++){
    ibook.cd();
    // FIXME: these need to be moved to a subdirectory whose name depends on the associator
    ibook.setCurrentFolder(dirName_);

    if(doSummaryPlots_) {
      if(doSimTrackPlots_) {
        h_assoc_coll.push_back(binLabels( ibook.book1D("num_assoc(simToReco)_coll", "N of associated (simToReco) tracks vs track collection", nintColl, minColl, maxColl) ));
        h_simul_coll.push_back(binLabels( ibook.book1D("num_simul_coll", "N of simulated tracks vs track collection", nintColl, minColl, maxColl) ));

        h_assoc_coll_allPt.push_back(binLabels( ibook.book1D("num_assoc(simToReco)_coll_allPt", "N of associated (simToReco) tracks vs track collection", nintColl, minColl, maxColl) ));
        h_simul_coll_allPt.push_back(binLabels( ibook.book1D("num_simul_coll_allPt", "N of simulated tracks vs track collection", nintColl, minColl, maxColl) ));

      }
      if(doRecoTrackPlots_) {
        h_reco_coll.push_back(binLabels( ibook.book1D("num_reco_coll", "N of reco track vs track collection", nintColl, minColl, maxColl) ));
        h_assoc2_coll.push_back(binLabels( ibook.book1D("num_assoc(recoToSim)_coll", "N of associated (recoToSim) tracks vs track collection", nintColl, minColl, maxColl) ));
        h_looper_coll.push_back(binLabels( ibook.book1D("num_duplicate_coll", "N of associated (recoToSim) looper tracks vs track collection", nintColl, minColl, maxColl) ));
        h_pileup_coll.push_back(binLabels( ibook.book1D("num_pileup_coll", "N of associated (recoToSim) pileup tracks vs track collection", nintColl, minColl, maxColl) ));
      }
    }

    for (unsigned int www=0;www<label.size();www++){
      ibook.cd();
      InputTag algo = label[www];
      string dirName=dirName_;
      if (algo.process()!="")
        dirName+=algo.process()+"_";
      if(algo.label()!="")
        dirName+=algo.label()+"_";
      if(algo.instance()!="")
        dirName+=algo.instance()+"_";
      if (dirName.find("Tracks")<dirName.length()){
        dirName.replace(dirName.find("Tracks"),6,"");
      }
      string assoc= associators[ww].label();
      if (assoc.find("Track")<assoc.length()){
        assoc.replace(assoc.find("Track"),5,"");
      }
      dirName+=assoc;
      std::replace(dirName.begin(), dirName.end(), ':', '_');

      ibook.setCurrentFolder(dirName.c_str());

      if(doSimTrackPlots_) {
        histoProducerAlgo_->bookSimTrackHistos(ibook);
        if(doPVAssociationPlots_) histoProducerAlgo_->bookSimTrackPVAssociationHistos(ibook);
      }

      //Booking histograms concerning with reconstructed tracks
      if(doRecoTrackPlots_) {
        histoProducerAlgo_->bookRecoHistos(ibook);
        if (dodEdxPlots_) histoProducerAlgo_->bookRecodEdxHistos(ibook);
        if (doPVAssociationPlots_) histoProducerAlgo_->bookRecoPVAssociationHistos(ibook);
      }

      if(doSeedPlots_) {
        histoProducerAlgo_->bookSeedHistos(ibook);
      }
    }//end loop www
  }// end loop ww
}

namespace {
  void ensureEffIsSubsetOfFake(const TrackingParticleRefVector& eff, const TrackingParticleRefVector& fake) {
    // If efficiency RefVector is empty, don't check the product ids
    // as it will be 0:0 for empty. This covers also the case where
    // both are empty. The case of fake being empty and eff not is an
    // error.
    if(eff.empty())
      return;

    // First ensure product ids
    if(eff.id() != fake.id()) {
      throw cms::Exception("Configuration") << "Efficiency and fake TrackingParticle (refs) point to different collections (eff " << eff.id() << " fake " << fake.id() << "). This is not supported. Efficiency TP set must be the same or a subset of the fake TP set.";
    }

    // Same technique as in associationMapFilterValues
    std::unordered_set<reco::RecoToSimCollection::index_type> fakeKeys;
    for(const auto& ref: fake) {
      fakeKeys.insert(ref.key());
    }

    for(const auto& ref: eff) {
      if(fakeKeys.find(ref.key()) == fakeKeys.end()) {
        throw cms::Exception("Configuration") << "Efficiency TrackingParticle " << ref.key() << " is not found from the set of fake TPs. This is not supported. The efficiency TP set must be the same or a subset of the fake TP set.";
      }
    }
  }
}

void MultiTrackValidator::analyze(const edm::Event& event, const edm::EventSetup& setup){
  using namespace reco;

  LogDebug("TrackValidator") << "\n====================================================" << "\n"
                             << "Analyzing new event" << "\n"
                             << "====================================================\n" << "\n";


  edm::ESHandle<ParametersDefinerForTP> parametersDefinerTPHandle;
  setup.get<TrackAssociatorRecord>().get(parametersDefiner,parametersDefinerTPHandle);
  //Since we modify the object, we must clone it
  auto parametersDefinerTP = parametersDefinerTPHandle->clone();

  edm::ESHandle<TrackerTopology> httopo;
  setup.get<TrackerTopologyRcd>().get(httopo);
  const TrackerTopology& ttopo = *httopo;

  // FIXME: we really need to move to edm::View for reading the
  // TrackingParticles... Unfortunately it has non-trivial
  // consequences on the associator/association interfaces etc.
  TrackingParticleRefVector tmpTPeff;
  TrackingParticleRefVector tmpTPfake;
  const TrackingParticleRefVector *tmpTPeffPtr = nullptr;
  const TrackingParticleRefVector *tmpTPfakePtr = nullptr;

  edm::Handle<TrackingParticleCollection>  TPCollectionHeff;
  edm::Handle<TrackingParticleRefVector>  TPCollectionHeffRefVector;

  const bool tp_effic_refvector = label_tp_effic.isUninitialized();
  if(!tp_effic_refvector) {
    event.getByToken(label_tp_effic, TPCollectionHeff);
    for(size_t i=0, size=TPCollectionHeff->size(); i<size; ++i) {
      tmpTPeff.push_back(TrackingParticleRef(TPCollectionHeff, i));
    }
    tmpTPeffPtr = &tmpTPeff;
  }
  else {
    event.getByToken(label_tp_effic_refvector, TPCollectionHeffRefVector);
    tmpTPeffPtr = TPCollectionHeffRefVector.product();
  }
  if(!label_tp_fake.isUninitialized()) {
    edm::Handle<TrackingParticleCollection> TPCollectionHfake ;
    event.getByToken(label_tp_fake,TPCollectionHfake);
    for(size_t i=0, size=TPCollectionHfake->size(); i<size; ++i) {
      tmpTPfake.push_back(TrackingParticleRef(TPCollectionHfake, i));
    }
    tmpTPfakePtr = &tmpTPfake;
  }
  else {
    edm::Handle<TrackingParticleRefVector> TPCollectionHfakeRefVector;
    event.getByToken(label_tp_fake_refvector, TPCollectionHfakeRefVector);
    tmpTPfakePtr = TPCollectionHfakeRefVector.product();
  }

  TrackingParticleRefVector const & tPCeff = *tmpTPeffPtr;
  TrackingParticleRefVector const & tPCfake = *tmpTPfakePtr;

  ensureEffIsSubsetOfFake(tPCeff, tPCfake);

  if(parametersDefinerIsCosmic_) {
    edm::Handle<SimHitTPAssociationProducer::SimHitTPAssociationList> simHitsTPAssoc;
    //warning: make sure the TP collection used in the map is the same used in the MTV!
    event.getByToken(_simHitTpMapTag,simHitsTPAssoc);
    parametersDefinerTP->initEvent(simHitsTPAssoc);
    cosmictpSelector.initEvent(simHitsTPAssoc);
  }

  const reco::Vertex::Point *thePVposition = nullptr;
  const TrackingVertex::LorentzVector *theSimPVPosition = nullptr;
  // Find the sim PV and tak its position
  edm::Handle<TrackingVertexCollection> htv;
  event.getByToken(label_tv, htv);
  {
    const TrackingVertexCollection& tv = *htv;
    for(size_t i=0; i<tv.size(); ++i) {
      const TrackingVertex& simV = tv[i];
      if(simV.eventId().bunchCrossing() != 0) continue; // remove OOTPU
      if(simV.eventId().event() != 0) continue; // pick the PV of hard scatter
      theSimPVPosition = &(simV.position());
      break;
    }
  }
  if(simPVMaxZ_ >= 0) {
    if(!theSimPVPosition) return;
    if(std::abs(theSimPVPosition->z()) > simPVMaxZ_) return;
  }

  // Check, when necessary, if reco PV matches to sim PV
  if(doPlotsOnlyForTruePV_ || doPVAssociationPlots_) {
    edm::Handle<edm::View<reco::Vertex> > hvertex;
    event.getByToken(recoVertexToken_, hvertex);

    edm::Handle<reco::VertexToTrackingVertexAssociator> hvassociator;
    event.getByToken(vertexAssociatorToken_, hvassociator);

    auto v_r2s = hvassociator->associateRecoToSim(hvertex, htv);
    auto pvPtr = hvertex->refAt(0);
    if(!(pvPtr->isFake() || pvPtr->ndof() < 0)) { // skip junk vertices
      auto pvFound = v_r2s.find(pvPtr);
      if(pvFound != v_r2s.end()) {
        bool matchedToSimPV = false;
        for(const auto& vertexRefQuality: pvFound->val) {
          const TrackingVertex& tv = *(vertexRefQuality.first);
          if(tv.eventId().event() == 0 && tv.eventId().bunchCrossing() == 0) {
            matchedToSimPV = true;
            break;
          }
        }
        if(matchedToSimPV) {
          if(doPVAssociationPlots_) {
            thePVposition = &(pvPtr->position());
          }
        }
        else if(doPlotsOnlyForTruePV_)
          return;
      }
      else if(doPlotsOnlyForTruePV_)
        return;
    }
    else if(doPlotsOnlyForTruePV_)
      return;
  }

  edm::Handle<reco::BeamSpot> recoBeamSpotHandle;
  event.getByToken(bsSrc,recoBeamSpotHandle);
  reco::BeamSpot const & bs = *recoBeamSpotHandle;

  edm::Handle< std::vector<PileupSummaryInfo> > puinfoH;
  event.getByToken(label_pileupinfo,puinfoH);
  PileupSummaryInfo puinfo;

  for (unsigned int puinfo_ite=0;puinfo_ite<(*puinfoH).size();++puinfo_ite){
    if ((*puinfoH)[puinfo_ite].getBunchCrossing()==0){
      puinfo=(*puinfoH)[puinfo_ite];
      break;
    }
  }

  /*
  edm::Handle<TrackingVertexCollection> tvH;
  event.getByToken(label_tv,tvH);
  TrackingVertexCollection const & tv = *tvH;
  */

  // Number of 3D layers for TPs
  edm::Handle<edm::ValueMap<unsigned int>> tpNLayersH;
  event.getByToken(tpNLayersToken_, tpNLayersH);
  const auto& nLayers_tPCeff = *tpNLayersH;

  event.getByToken(tpNPixelLayersToken_, tpNLayersH);
  const auto& nPixelLayers_tPCeff = *tpNLayersH;

  event.getByToken(tpNStripStereoLayersToken_, tpNLayersH);
  const auto& nStripMonoAndStereoLayers_tPCeff = *tpNLayersH;

  // Precalculate TP selection (for efficiency), and momentum and vertex wrt PCA
  //
  // TODO: ParametersDefinerForTP ESProduct needs to be changed to
  // EDProduct because of consumes.
  //
  // In principle, we could just precalculate the momentum and vertex
  // wrt PCA for all TPs for once and put that to the event. To avoid
  // repetitive calculations those should be calculated only once for
  // each TP. That would imply that we should access TPs via Refs
  // (i.e. View) in here, since, in general, the eff and fake TP
  // collections can be different (and at least HI seems to use that
  // feature). This would further imply that the
  // RecoToSimCollection/SimToRecoCollection should be changed to use
  // View<TP> instead of vector<TP>, and migrate everything.
  //
  // Or we could take only one input TP collection, and do another
  // TP-selection to obtain the "fake" collection like we already do
  // for "efficiency" TPs.
  std::vector<size_t> selected_tPCeff;
  std::vector<std::tuple<TrackingParticle::Vector, TrackingParticle::Point>> momVert_tPCeff;
  selected_tPCeff.reserve(tPCeff.size());
  momVert_tPCeff.reserve(tPCeff.size());
  int nIntimeTPs = 0;
  if(parametersDefinerIsCosmic_) {
    for(size_t j=0; j<tPCeff.size(); ++j) {
      const TrackingParticleRef& tpr = tPCeff[j];

      TrackingParticle::Vector momentum = parametersDefinerTP->momentum(event,setup,tpr);
      TrackingParticle::Point vertex = parametersDefinerTP->vertex(event,setup,tpr);
      if(doSimPlots_) {
        histoProducerAlgo_->fill_generic_simTrack_histos(momentum, vertex, tpr->eventId().bunchCrossing());
      }
      if(tpr->eventId().bunchCrossing() == 0)
        ++nIntimeTPs;

      if(cosmictpSelector(tpr,&bs,event,setup)) {
        selected_tPCeff.push_back(j);
        momVert_tPCeff.emplace_back(momentum, vertex);
      }
    }
  }
  else {
    size_t j=0;
    for(auto const& tpr: tPCeff) {
      const TrackingParticle& tp = *tpr;

      // TODO: do we want to fill these from all TPs that include IT
      // and OOT (as below), or limit to IT+OOT TPs passing tpSelector
      // (as it was before)? The latter would require another instance
      // of tpSelector with intimeOnly=False.
      if(doSimPlots_) {
        histoProducerAlgo_->fill_generic_simTrack_histos(tp.momentum(), tp.vertex(), tp.eventId().bunchCrossing());
      }
      if(tp.eventId().bunchCrossing() == 0)
        ++nIntimeTPs;

      if(tpSelector(tp)) {
        selected_tPCeff.push_back(j);
        TrackingParticle::Vector momentum = parametersDefinerTP->momentum(event,setup,tpr);
        TrackingParticle::Point vertex = parametersDefinerTP->vertex(event,setup,tpr);
        momVert_tPCeff.emplace_back(momentum, vertex);
      }
      ++j;
    }
  }
  if(doSimPlots_) {
    histoProducerAlgo_->fill_simTrackBased_histos(nIntimeTPs);
  }

  //calculate dR for TPs
  float dR_tPCeff[tPCeff.size()];
  {
    float etaL[tPCeff.size()], phiL[tPCeff.size()];
    for(size_t iTP: selected_tPCeff) {
      //calculare dR wrt inclusive collection (also with PU, low pT, displaced)
      auto const& tp2 = *(tPCeff[iTP]);
      auto  && p = tp2.momentum();
      etaL[iTP] = etaFromXYZ(p.x(),p.y(),p.z());
      phiL[iTP] = atan2f(p.y(),p.x());
    }
    auto i=0U;
    for ( auto const & tpr : tPCeff) {
      auto const& tp = *tpr;
      double dR = std::numeric_limits<double>::max();
      if(dRtpSelector(tp)) {//only for those needed for efficiency!
        auto  && p = tp.momentum();
        float eta = etaFromXYZ(p.x(),p.y(),p.z());
        float phi = atan2f(p.y(),p.x());
        for(size_t iTP: selected_tPCeff) {
          //calculare dR wrt inclusive collection (also with PU, low pT, displaced)
	  if (i==iTP) {continue;}
          auto dR_tmp = reco::deltaR2(eta, phi, etaL[iTP], phiL[iTP]);
          if (dR_tmp<dR) dR=dR_tmp;
        }  // ttp2 (iTP)
      }
      dR_tPCeff[i++] = std::sqrt(dR);
    }  // tp
  }

  edm::Handle<View<Track> >  trackCollectionForDrCalculation;
  if(calculateDrSingleCollection_) {
    event.getByToken(labelTokenForDrCalculation, trackCollectionForDrCalculation);
  }

  // dE/dx
  // at some point this could be generalized, with a vector of tags and a corresponding vector of Handles
  // I'm writing the interface such to take vectors of ValueMaps
  std::vector<const edm::ValueMap<reco::DeDxData> *> v_dEdx;
  if(dodEdxPlots_) {
    edm::Handle<edm::ValueMap<reco::DeDxData> > dEdx1Handle;
    edm::Handle<edm::ValueMap<reco::DeDxData> > dEdx2Handle;
    event.getByToken(m_dEdx1Tag, dEdx1Handle);
    event.getByToken(m_dEdx2Tag, dEdx2Handle);
    v_dEdx.push_back(dEdx1Handle.product());
    v_dEdx.push_back(dEdx2Handle.product());
  }

  int w=0; //counter counting the number of sets of histograms
  for (unsigned int ww=0;ww<associators.size();ww++){
    for (unsigned int www=0;www<label.size();www++, w++){ // need to increment w here, since there are many continues in the loop body
      //
      //get collections from the event
      //
      edm::Handle<View<Track> >  trackCollectionHandle;
      if(!event.getByToken(labelToken[www], trackCollectionHandle)&&ignoremissingtkcollection_)continue;
      const edm::View<Track>& trackCollection = *trackCollectionHandle;

      reco::RecoToSimCollection const * recSimCollP=nullptr;
      reco::SimToRecoCollection const * simRecCollP=nullptr;
      reco::RecoToSimCollection recSimCollL;
      reco::SimToRecoCollection simRecCollL;

      //associate tracks
      LogTrace("TrackValidator") << "Analyzing "
                                 << label[www] << " with "
                                 << associators[ww] <<"\n";
      if(UseAssociators){
        edm::Handle<reco::TrackToTrackingParticleAssociator> theAssociator;
        event.getByToken(associatorTokens[ww], theAssociator);

        // The associator interfaces really need to be fixed...
        edm::RefToBaseVector<reco::Track> trackRefs;
        for(edm::View<Track>::size_type i=0; i<trackCollection.size(); ++i) {
          trackRefs.push_back(trackCollection.refAt(i));
        }


	LogTrace("TrackValidator") << "Calling associateRecoToSim method" << "\n";
        recSimCollL = std::move(theAssociator->associateRecoToSim(trackRefs, tPCfake));
        recSimCollP = &recSimCollL;
	LogTrace("TrackValidator") << "Calling associateSimToReco method" << "\n";
        // It is necessary to do the association wrt. fake TPs,
        // because this SimToReco association is used also for
        // duplicates. Since the set of efficiency TPs are required to
        // be a subset of the set of fake TPs, for efficiency
        // histograms it doesn't matter if the association contains
        // associations of TPs not in the set of efficiency TPs.
        simRecCollL = std::move(theAssociator->associateSimToReco(trackRefs, tPCfake));
        simRecCollP = &simRecCollL;
      }
      else{
	Handle<reco::SimToRecoCollection > simtorecoCollectionH;
	event.getByToken(associatormapStRs[ww], simtorecoCollectionH);
	simRecCollP = simtorecoCollectionH.product();

        // We need to filter the associations of the current track
        // collection only from SimToReco collection, otherwise the
        // SimToReco histograms get false entries
        simRecCollL = associationMapFilterValues(*simRecCollP, trackCollection);
        simRecCollP = &simRecCollL;

	Handle<reco::RecoToSimCollection > recotosimCollectionH;
	event.getByToken(associatormapRtSs[ww],recotosimCollectionH);
	recSimCollP = recotosimCollectionH.product();

        // We need to filter the associations of the fake-TrackingParticle
        // collection only from RecoToSim collection, otherwise the
        // RecoToSim histograms get false entries
        recSimCollL = associationMapFilterValues(*recSimCollP, tPCfake);
        recSimCollP = &recSimCollL;
      }

      reco::RecoToSimCollection const & recSimColl = *recSimCollP;
      reco::SimToRecoCollection const & simRecColl = *simRecCollP;


      // ########################################################
      // fill simulation histograms (LOOP OVER TRACKINGPARTICLES)
      // ########################################################

      //compute number of tracks per eta interval
      //
      LogTrace("TrackValidator") << "\n# of TrackingParticles: " << tPCeff.size() << "\n";
      int ats(0);  	  //This counter counts the number of simTracks that are "associated" to recoTracks
      int st(0);    	  //This counter counts the number of simulated tracks passing the MTV selection (i.e. tpSelector(tp) )
      unsigned sts(0);   //This counter counts the number of simTracks surviving the bunchcrossing cut
      unsigned asts(0);  //This counter counts the number of simTracks that are "associated" to recoTracks surviving the bunchcrossing cut

      //loop over already-selected TPs for tracking efficiency
      for(size_t i=0; i<selected_tPCeff.size(); ++i) {
        size_t iTP = selected_tPCeff[i];
        const TrackingParticleRef& tpr = tPCeff[iTP];
        const TrackingParticle& tp = *tpr;

        auto const& momVert = momVert_tPCeff[i];
	TrackingParticle::Vector momentumTP;
	TrackingParticle::Point vertexTP;

	double dxySim(0);
	double dzSim(0);
        double dxyPVSim = 0;
        double dzPVSim = 0;
	double dR=dR_tPCeff[iTP];

	//---------- THIS PART HAS TO BE CLEANED UP. THE PARAMETER DEFINER WAS NOT MEANT TO BE USED IN THIS WAY ----------
	//If the TrackingParticle is collison like, get the momentum and vertex at production state
	if(!parametersDefinerIsCosmic_)
	  {
	    momentumTP = tp.momentum();
	    vertexTP = tp.vertex();
	    //Calcualte the impact parameters w.r.t. PCA
	    const TrackingParticle::Vector& momentum = std::get<TrackingParticle::Vector>(momVert);
	    const TrackingParticle::Point& vertex = std::get<TrackingParticle::Point>(momVert);
	    dxySim = (-vertex.x()*sin(momentum.phi())+vertex.y()*cos(momentum.phi()));
	    dzSim = vertex.z() - (vertex.x()*momentum.x()+vertex.y()*momentum.y())/sqrt(momentum.perp2())
	      * momentum.z()/sqrt(momentum.perp2());

            if(theSimPVPosition) {
              // As in TrackBase::dxy(Point) and dz(Point)
              dxyPVSim = -(vertex.x()-theSimPVPosition->x())*sin(momentum.phi()) + (vertex.y()-theSimPVPosition->y())*cos(momentum.phi());
              dzPVSim = vertex.z()-theSimPVPosition->z() - ( (vertex.x()-theSimPVPosition->x()) + (vertex.y()-theSimPVPosition->y()) )/sqrt(momentum.perp2()) * momentum.z()/sqrt(momentum.perp2());
            }
	  }
	//If the TrackingParticle is comics, get the momentum and vertex at PCA
	else
	  {
	    momentumTP = std::get<TrackingParticle::Vector>(momVert);
	    vertexTP = std::get<TrackingParticle::Point>(momVert);
	    dxySim = (-vertexTP.x()*sin(momentumTP.phi())+vertexTP.y()*cos(momentumTP.phi()));
	    dzSim = vertexTP.z() - (vertexTP.x()*momentumTP.x()+vertexTP.y()*momentumTP.y())/sqrt(momentumTP.perp2())
	      * momentumTP.z()/sqrt(momentumTP.perp2());

            // Do dxy and dz vs. PV make any sense for cosmics? I guess not
	  }
	//---------- THE PART ABOVE HAS TO BE CLEANED UP. THE PARAMETER DEFINER WAS NOT MEANT TO BE USED IN THIS WAY ----------

        //This counter counts the number of simulated tracks passing the MTV selection (i.e. tpSelector(tp) ), but only for in-time TPs
        if(tp.eventId().bunchCrossing() == 0) {
          st++;
        }

	// in the coming lines, histos are filled using as input
	// - momentumTP
	// - vertexTP
	// - dxySim
	// - dzSim
        if(!doSimTrackPlots_)
          continue;

	// ##############################################
	// fill RecoAssociated SimTracks' histograms
	// ##############################################
	const reco::Track* matchedTrackPointer=0;
	if(simRecColl.find(tpr) != simRecColl.end()){
	  auto const & rt = simRecColl[tpr];
	  if (rt.size()!=0) {
	    ats++; //This counter counts the number of simTracks that have a recoTrack associated
	    // isRecoMatched = true; // UNUSED
	    matchedTrackPointer = rt.begin()->first.get();
	    LogTrace("TrackValidator") << "TrackingParticle #" << st
                                       << " with pt=" << sqrt(momentumTP.perp2())
                                       << " associated with quality:" << rt.begin()->second <<"\n";
	  }
	}else{
	  LogTrace("TrackValidator")
	    << "TrackingParticle #" << st
	    << " with pt,eta,phi: "
	    << sqrt(momentumTP.perp2()) << " , "
	    << momentumTP.eta() << " , "
	    << momentumTP.phi() << " , "
	    << " NOT associated to any reco::Track" << "\n";
	}




        int nSimHits = tp.numberOfTrackerHits();
        int nSimLayers = nLayers_tPCeff[tpr];
        int nSimPixelLayers = nPixelLayers_tPCeff[tpr];
        int nSimStripMonoAndStereoLayers = nStripMonoAndStereoLayers_tPCeff[tpr];
        histoProducerAlgo_->fill_recoAssociated_simTrack_histos(w,tp,momentumTP,vertexTP,dxySim,dzSim,dxyPVSim,dzPVSim,nSimHits,nSimLayers,nSimPixelLayers,nSimStripMonoAndStereoLayers,matchedTrackPointer,puinfo.getPU_NumInteractions(), dR, thePVposition, theSimPVPosition);
          sts++;
          if(matchedTrackPointer)
            asts++;
          if(doSummaryPlots_) {
            if(dRtpSelectorNoPtCut(tp)) {
              h_simul_coll_allPt[ww]->Fill(www);
              if (matchedTrackPointer) {
                h_assoc_coll_allPt[ww]->Fill(www);
              }

              if(dRtpSelector(tp)) {
                h_simul_coll[ww]->Fill(www);
                if (matchedTrackPointer) {
                  h_assoc_coll[ww]->Fill(www);
                }
              }
            }
          }




      } // End  for (TrackingParticleCollection::size_type i=0; i<tPCeff.size(); i++){

      // ##############################################
      // fill recoTracks histograms (LOOP OVER TRACKS)
      // ##############################################
      if(!doRecoTrackPlots_)
        continue;
      LogTrace("TrackValidator") << "\n# of reco::Tracks with "
                                 << label[www].process()<<":"
                                 << label[www].label()<<":"
                                 << label[www].instance()
                                 << ": " << trackCollection.size() << "\n";

      int sat(0); //This counter counts the number of recoTracks that are associated to SimTracks from Signal only
      int at(0); //This counter counts the number of recoTracks that are associated to SimTracks
      int rT(0); //This counter counts the number of recoTracks in general
      int seed_fit_failed = 0;

      //calculate dR for tracks
      const edm::View<Track> *trackCollectionDr = &trackCollection;
      if(calculateDrSingleCollection_) {
        trackCollectionDr = trackCollectionForDrCalculation.product();
      }
      float dR_trk[trackCollection.size()];
      int i=0;
      float etaL[trackCollectionDr->size()];
      float phiL[trackCollectionDr->size()];
      bool validL[trackCollectionDr->size()];
      for (auto const & track2 : *trackCollectionDr) {
         auto  && p = track2.momentum();
         etaL[i] = etaFromXYZ(p.x(),p.y(),p.z());
         phiL[i] = atan2f(p.y(),p.x());
         validL[i] = !trackFromSeedFitFailed(track2);
         ++i;
      }
      for(View<Track>::size_type i=0; i<trackCollection.size(); ++i){
	auto const &  track = trackCollection[i];
	auto dR = std::numeric_limits<float>::max();
        if(!trackFromSeedFitFailed(track)) {
          auto  && p = track.momentum();
          float eta = etaFromXYZ(p.x(),p.y(),p.z());
          float phi = atan2f(p.y(),p.x());
          for(View<Track>::size_type j=0; j<trackCollectionDr->size(); ++j){
            if(!validL[j]) continue;
            auto dR_tmp = reco::deltaR2(eta, phi, etaL[j], phiL[j]);
            if ( (dR_tmp<dR) & (dR_tmp>std::numeric_limits<float>::min())) dR=dR_tmp;
          }
        }
	dR_trk[i] = std::sqrt(dR);
      }

      TFile* clusterFile = new TFile("./Clusters/clusters.root","UPDATE");

      Int_t padHalfSize = 16.5;
      Bool_t goodTrack = false;
      Int_t nHits,trackId,pdgId,charge;
      Int_t detId[nHits];
      TH2D* hitClusters[nHits];

      Double_t px,py,pz,pt;

      TTree *doubletsTree = new TTree("doubletsTree","doubletsTree");

      //Labels
      doubletsTree->Branch("goodTrack",goodTrack);
      doubletsTree->Branch("charge",&charge);
      doubletsTree->Branch("pdgId",&pdgId);
      doubletsTree->Branch("px",&px);
      doubletsTree->Branch("py",&py);
      doubletsTree->Branch("pz",&pz);
      doubletsTree->Branch("pt",&pt);

      //Inputs
      doubletsTree->Branch("hitClusters",&hitClusters,"hitClusters[nHits]/D");
      doubletsTree->Branch("detId",&detId,"detId[nHits]/I");

      //Indeces
      doubletsTree->Branch("nHits",&nHits);
      doubletsTree->Branch("trackId",&trackId);

      for(View<Track>::size_type i=0; i<trackCollection.size(); ++i){
        auto track = trackCollection.refAt(i);
	rT++;
        if(trackFromSeedFitFailed(*track)) ++seed_fit_failed;

	       bool isSigSimMatched(false);
	        bool isSimMatched(false);
        bool isChargeMatched(true);
        int numAssocRecoTracks = 0;
	       int nSimHits = 0;
	        double sharedFraction = 0.;

          goodTrack = isSimMatched;
          trackId  = i;
          nHits = track->recHitsSize();

        auto tpFound = recSimColl.find(track);
        isSimMatched = tpFound != recSimColl.end();

        if (!isSimMatched) {

          px = track->momentum().x();
          py = track->momentum().y();
          pz = track->momentum().z();
          pt = track->pt();
          pdgId = -28121989;

          charge = track->charge();

          for (size_t j = 0; j < track->recHitsSize(); ++j) {

            auto && recHit = track->recHit(j);
            auto && detector = recHit->det();

            if((detector->type()).isTrackerPixel() && (((detector->type()).isBarrel())||((detector->type()).isEndcap()))){

              const SiPixelRecHit * recHitPix = dynamic_cast<const SiPixelRecHit*>(&(*recHit));
              if(!recHitPix) continue;
              SiPixelRecHit::ClusterRef const& cluster = recHitPix->cluster();

              detId[j] = recHit->geographicalId();

              std::string clusterPlot = "Hit_";
              clusterPlot += std::to_string(j);
              clusterPlot +="_pixelCluster";

              double minClusterX = floor(cluster->x())-padHalfSize;
              double maxClusterX = floor(cluster->x())+padHalfSize;

              double minClusterY = floor(cluster->y())-padHalfSize;
              double maxClusterY = floor(cluster->y())+padHalfSize;

              hitClusters[j] = new TH2D (clusterPlot.data(),clusterPlot.data(),padHalfSize*2,minClusterX,maxClusterX,padHalfSize*2,minClusterY,maxClusterY);

              for (int nx = 0; nx < hitClusters[j]->GetNbinsX(); ++nx) {
                for (int ny = 0; ny < hitClusters[j]->GetNbinsY(); ++ny) {
                  hitClusters[j]->SetBinContent(nx,ny,0.0);
                }

              }

              for (int k = 0; k < cluster->size(); ++k) {
               //  std::cout<<"\t Pixel "<<k<<" - - - "<<" x = "<<cluster->pixel(k).x<<" y = "<<cluster->pixel(k).y<<" adc = "<<cluster->pixel(k).adc<<std::endl;
                hitClusters[j]->SetBinContent(hitClusters[j]->FindBin((Double_t)cluster->pixel(k).x, (Double_t)cluster->pixel(k).y),cluster->pixel(k).adc);
             }

             doubletsTree->Fill();

            }
          }
        }


        if (isSimMatched) {
            const auto& tp = tpFound->val;
	          nSimHits = tp[0].first->numberOfTrackerHits();
            sharedFraction = tp[0].second;
            pdgId = tp[0].first->pdgId();
            if (tp[0].first->charge() != track->charge()) isChargeMatched = false;
            if(simRecColl.find(tp[0].first) != simRecColl.end()) numAssocRecoTracks = simRecColl[tp[0].first].size();
	           at++;

             px = track->momentum().x();
             py = track->momentum().y();
             pz = track->momentum().z();
             pt = track->pt();

             charge = track->charge();

             for (size_t j = 0; j < track->recHitsSize(); ++j) {

               auto && recHit = track->recHit(j);
               auto && detector = recHit->det();

               detId[j] = recHit->geographicalId();

               if((detector->type()).isTrackerPixel() && (((detector->type()).isBarrel())||((detector->type()).isEndcap()))){
                 const SiPixelRecHit * recHitPix = dynamic_cast<const SiPixelRecHit*>(&(*recHit));
                 if(!recHitPix) continue;
                 SiPixelRecHit::ClusterRef const& cluster = recHitPix->cluster();

                 std::string clusterPlot = "Hit_";
                 clusterPlot += std::to_string(j);
                 clusterPlot +="_pixelCluster";


                 double minClusterX = floor(cluster->x())-padHalfSize;
                 double maxClusterX = floor(cluster->x())+padHalfSize;

                 double minClusterY = floor(cluster->y())-padHalfSize;
                 double maxClusterY = floor(cluster->y())+padHalfSize;

                hitClusters[j] = new TH2D (clusterPlot.data(),clusterPlot.data(),padHalfSize*2,minClusterX,maxClusterX,padHalfSize*2,minClusterY,maxClusterY);

                for (int nx = 0; nx < hitClusters[j]->GetNbinsX(); ++nx) {
                  for (int ny = 0; ny < hitClusters[j]->GetNbinsY(); ++ny) {
                    hitClusters[j]->SetBinContent(nx,ny,0.0);
                  }

                }

                for (int k = 0; k < cluster->size(); ++k) {
                 //  std::cout<<"\t Pixel "<<k<<" - - - "<<" x = "<<cluster->pixel(k).x<<" y = "<<cluster->pixel(k).y<<" adc = "<<cluster->pixel(k).adc<<std::endl;
                  hitClusters[j]->SetBinContent(hitClusters[j]->FindBin((Double_t)cluster->pixel(k).x, (Double_t)cluster->pixel(k).y),cluster->pixel(k).adc);
               }

                doubletsTree->Fill();

              }
            }

	    for (unsigned int tp_ite=0;tp_ite<tp.size();++tp_ite){
              TrackingParticle trackpart = *(tp[tp_ite].first);
	      if ((trackpart.eventId().event() == 0) && (trackpart.eventId().bunchCrossing() == 0)){
	      	isSigSimMatched = true;
		sat++;
		break;
	      }
            }
	    LogTrace("TrackValidator") << "reco::Track #" << rT << " with pt=" << track->pt()
                                       << " associated with quality:" << tp.begin()->second <<"\n";
	} else {
	  LogTrace("TrackValidator") << "reco::Track #" << rT << " with pt=" << track->pt()
                                     << " NOT associated to any TrackingParticle" << "\n";
	}

	double dR=dR_trk[i];
	histoProducerAlgo_->fill_generic_recoTrack_histos(w,*track, ttopo, bs.position(), thePVposition, theSimPVPosition, isSimMatched,isSigSimMatched, isChargeMatched, numAssocRecoTracks, puinfo.getPU_NumInteractions(), nSimHits, sharedFraction, dR);
        if(doSummaryPlots_) {
          h_reco_coll[ww]->Fill(www);
          if(isSimMatched) {
            h_assoc2_coll[ww]->Fill(www);
            if(numAssocRecoTracks>1) {
              h_looper_coll[ww]->Fill(www);
            }
            if(!isSigSimMatched) {
              h_pileup_coll[ww]->Fill(www);
            }
          }
        }

	// dE/dx
	if (dodEdxPlots_) histoProducerAlgo_->fill_dedx_recoTrack_histos(w,track, v_dEdx);


	//Fill other histos
	if (!isSimMatched) continue;

	histoProducerAlgo_->fill_simAssociated_recoTrack_histos(w,*track);

	TrackingParticleRef tpr = tpFound->val.begin()->first;

	/* TO BE FIXED LATER
	if (associators[ww]=="trackAssociatorByChi2"){
	  //association chi2
	  double assocChi2 = -tp.begin()->second;//in association map is stored -chi2
	  h_assochi2[www]->Fill(assocChi2);
	  h_assochi2_prob[www]->Fill(TMath::Prob((assocChi2)*5,5));
	}
	else if (associators[ww]=="quickTrackAssociatorByHits"){
	  double fraction = tp.begin()->second;
	  h_assocFraction[www]->Fill(fraction);
	  h_assocSharedHit[www]->Fill(fraction*track->numberOfValidHits());
	}
	*/


	//Get tracking particle parameters at point of closest approach to the beamline
	TrackingParticle::Vector momentumTP = parametersDefinerTP->momentum(event,setup,tpr);
	TrackingParticle::Point vertexTP = parametersDefinerTP->vertex(event,setup,tpr);
	int chargeTP = tpr->charge();

	histoProducerAlgo_->fill_ResoAndPull_recoTrack_histos(w,momentumTP,vertexTP,chargeTP,
							     *track,bs.position());

  for (Int_t j = 0; j < nHits; ++j) {
                     delete hitClusters[j];
                   }

	//TO BE FIXED
	//std::vector<PSimHit> simhits=tpr.get()->trackPSimHit(DetId::Tracker);
	//nrecHit_vs_nsimHit_rec2sim[w]->Fill(track->numberOfValidHits(), (int)(simhits.end()-simhits.begin() ));

      } // End of for(View<Track>::size_type i=0; i<trackCollection.size(); ++i){

      clusterFile->Close();
      histoProducerAlgo_->fill_trackBased_histos(w,at,rT,st);
      // Fill seed-specific histograms
      if(doSeedPlots_) {
        histoProducerAlgo_->fill_seed_histos(www, seed_fit_failed, trackCollection.size());
      }


      LogTrace("TrackValidator") << "Total Simulated: " << st << "\n"
                                 << "Total Associated (simToReco): " << ats << "\n"
                                 << "Total Reconstructed: " << rT << "\n"
                                 << "Total Associated (recoToSim): " << at << "\n"
                                 << "Total Fakes: " << rT-at << "\n";
    } // End of  for (unsigned int www=0;www<label.size();www++){
  } //END of for (unsigned int ww=0;ww<associators.size();ww++){

}
