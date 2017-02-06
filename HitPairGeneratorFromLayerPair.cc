#include "RecoTracker/TkHitPairs/interface/HitPairGeneratorFromLayerPair.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "TrackingTools/DetLayers/interface/DetLayer.h"
#include "TrackingTools/DetLayers/interface/BarrelDetLayer.h"
#include "TrackingTools/DetLayers/interface/ForwardDetLayer.h"

#include "RecoTracker/TkTrackingRegions/interface/HitRZCompatibility.h"
#include "RecoTracker/TkTrackingRegions/interface/TrackingRegion.h"
#include "RecoTracker/TkTrackingRegions/interface/TrackingRegionBase.h"
#include "RecoTracker/TkHitPairs/interface/OrderedHitPairs.h"
#include "RecoTracker/TkHitPairs/src/InnerDeltaPhi.h"
#include "RecoTracker/TkHitPairs/interface/RecHitsSortedInPhi.h"

#include "FWCore/Framework/interface/Event.h"
#include "DataFormats/SiPixelDetId/interface/PXBDetId.h"
#include "DataFormats/SiPixelDetId/interface/PXFDetId.h"
#include "Geometry/CommonTopologies/interface/PixelTopology.h"
#include "Geometry/TrackerGeometryBuilder/interface/PixelGeomDetUnit.h"
#include "DataFormats/TrackerRecHit2D/interface/SiPixelRecHit.h"

#include <utility>
#include <algorithm>
#include <thread>
#include <iostream>
#include <string>
#include <fstream>
#include "TH2D.h"

using namespace GeomDetEnumerators;
using namespace std;

typedef PixelRecoRange<float> Range;

namespace {
  template<class T> inline T sqr( T t) {return t*t;}
}


#include "TrackingTools/TransientTrackingRecHit/interface/TransientTrackingRecHitBuilder.h"
#include "TrackingTools/Records/interface/TransientRecHitRecord.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
#include "RecoTracker/Record/interface/TrackerRecoGeometryRecord.h"
#include "FWCore/Framework/interface/ESHandle.h"


HitPairGeneratorFromLayerPair::HitPairGeneratorFromLayerPair(
							     unsigned int inner,
							     unsigned int outer,
							     LayerCacheType* layerCache,
							     unsigned int max)
  : theLayerCache(layerCache), theOuterLayer(outer), theInnerLayer(inner), theMaxElement(max)
{
}

HitPairGeneratorFromLayerPair::~HitPairGeneratorFromLayerPair() {}

// devirtualizer
#include<tuple>
namespace {

  template<typename Algo>
  struct Kernel {
    using  Base = HitRZCompatibility;
    void set(Base const * a) {
      assert( a->algo()==Algo::me);
      checkRZ=reinterpret_cast<Algo const *>(a);
    }

    void operator()(int b, int e, const RecHitsSortedInPhi & innerHitsMap, bool * ok) const {
      constexpr float nSigmaRZ = 3.46410161514f; // std::sqrt(12.f);
      for (int i=b; i!=e; ++i) {
	Range allowed = checkRZ->range(innerHitsMap.u[i]);
	float vErr = nSigmaRZ * innerHitsMap.dv[i];
	Range hitRZ(innerHitsMap.v[i]-vErr, innerHitsMap.v[i]+vErr);
	Range crossRange = allowed.intersection(hitRZ);
	ok[i-b] = ! crossRange.empty() ;
      }
    }
    Algo const * checkRZ;

  };


  template<typename ... Args> using Kernels = std::tuple<Kernel<Args>...>;

}


void HitPairGeneratorFromLayerPair::hitPairs(
					     const TrackingRegion & region, OrderedHitPairs & result,
					     const edm::Event& iEvent, const edm::EventSetup& iSetup, Layers layers) {

  auto const & ds = doublets(region, iEvent, iSetup, layers);
  for (std::size_t i=0; i!=ds.size(); ++i) {
    result.push_back( OrderedHitPair( ds.hit(i,HitDoublets::inner),ds.hit(i,HitDoublets::outer) ));
  }
  if (theMaxElement!=0 && result.size() >= theMaxElement){
     result.clear();
    edm::LogError("TooManyPairs")<<"number of pairs exceed maximum, no pairs produced";
  }
}

HitDoublets HitPairGeneratorFromLayerPair::doublets( const TrackingRegion& region,
                                                     const edm::Event & iEvent, const edm::EventSetup& iSetup, const Layer& innerLayer, const Layer& outerLayer,
                                                     LayerCacheType& layerCache) {




  const RecHitsSortedInPhi & innerHitsMap = layerCache(innerLayer, region, iSetup, iEvent);
  if (innerHitsMap.empty()) return HitDoublets(innerHitsMap,innerHitsMap);

  const RecHitsSortedInPhi& outerHitsMap = layerCache(outerLayer, region, iSetup, iEvent);
  if (outerHitsMap.empty()) return HitDoublets(innerHitsMap,outerHitsMap);
  HitDoublets result(innerHitsMap,outerHitsMap);
  result.reserve(std::max(innerHitsMap.size(),outerHitsMap.size()));
  doublets(region,*innerLayer.detLayer(),*outerLayer.detLayer(),innerHitsMap,outerHitsMap,iSetup,theMaxElement,result);


  int detOnArr[10] = {0,1,2,3,14,15,16,29,30,31};
  std::vector<int> detOn(detOnArr,detOnArr+sizeof(detOnArr)/sizeof(int));
  std::vector<int>::iterator detOnItOne = find(detOn.begin(),detOn.end(),innerLayer.detLayer()->seqNum());
  std::vector<int>::iterator detOnItTwo = find(detOn.begin(),detOn.end(),outerLayer.detLayer()->seqNum());
  Float_t padHalfSize = 4.0;
  // std::cout<<"clusters"<<innerLayer.name()<<" "<<outerLayer.name()<<std::endl;

  if(detOnItOne!=detOn.end() && detOnItTwo!=detOn.end())
  {
    // std::cout<<"clusters"<<std::endl;

    auto threadId = std::this_thread::get_id();
    std::stringstream streamThreadId;
    streamThreadId << threadId;

    size_t detCounterIn = detOnItOne - detOn.begin();
    size_t detCounterOu = detOnItTwo - detOn.begin();

    Int_t eveNumber = iEvent.id().event();

    std::string bufferstring = "./Hits/clusters/";
    bufferstring += std::to_string(eveNumber);
    // bufferstring +="_Event_";
    bufferstring +="_";
    bufferstring += std::to_string(detCounterIn);
    bufferstring +="_";
    bufferstring += std::to_string(detCounterOu);
    bufferstring +="_";
    // bufferstring += std::to_string(lumNumber);
    // bufferstring +="_Lumi_";
    // bufferstring += std::to_string(runNumber);
    // bufferstring +="_Run_";
    bufferstring += streamThreadId.str();
    bufferstring += "_clusters.txt";

    std::ofstream clustersFile;

    if (result.size()>0)
      clustersFile.open(bufferstring, std::ofstream::app);


    for (size_t i = 0; i < result.size(); i++) {

      int inId = result.innerHitId(i);
      int ouId = result.outerHitId(i);

      RecHitsSortedInPhi::Hit innerHit = result.hit(i, HitDoublets::inner);
      RecHitsSortedInPhi::Hit outerHit = result.hit(i, HitDoublets::outer);

      auto const & xin = (static_cast<BaseTrackerRecHit const &>(*(innerHit->hit())).globalState()).position.x();
      auto const & xou = (static_cast<BaseTrackerRecHit const &>(*(outerHit->hit())).globalState()).position.x();



      // if ( !(innerHit->isValid()) || !(outerHit->isValid())) continue;
      // if ( !(innerHit->geographicalId().det() != DetId::Tracker) || !(outerHit->geographicalId().det() != DetId::Tracker)) continue;
      // if( (*recHit)->geographicalId().det() != DetId::Tracker ) continue;
      // //std::cout<<"valid & on the tracker"<<std::endl;
      // const DetId & hit_detIdIn = (innerHit)->geographicalId();
      // const DetId & hit_detIdOu = (outerHit)->geographicalId();
      // //
      // uint IntSubDetIDIn = (hit_detIdIn.subdetId());
      // uint IntSubDetIDOu = (hit_detIdOu.subdetId());
      // //
      //  if(IntSubDetIDIn == 0 || IntSubDetIDOu == 0) continue;
      //  if(IntSubDetIDIn != PixelSubdetector::PixelBarrel && IntSubDetIDIn != PixelSubdetector::PixelEndcap) continue;
      //  if(IntSubDetIDOu != PixelSubdetector::PixelBarrel && IntSubDetIDOu != PixelSubdetector::PixelEndcap) continue;
      //
      const SiPixelRecHit* siHitIn = dynamic_cast<const SiPixelRecHit*>((innerHit));
      const SiPixelRecHit* siHitOu = dynamic_cast<const SiPixelRecHit*>((outerHit));

      SiPixelRecHit::ClusterRef const& clusterIn = siHitIn->cluster();
      SiPixelRecHit::ClusterRef const& clusterOu = siHitOu->cluster();

      TH2D hitClusterIn("hitClusterIn","hitClusterIn",(Int_t)(padHalfSize*2),floor(clusterIn->x())-padHalfSize,floor(clusterIn->x())+padHalfSize,(Int_t)(padHalfSize*2),floor(clusterIn->y())-padHalfSize,floor(clusterIn->y())+padHalfSize);
      TH2D hitClusterOu("hitClusterOu","hitClusterOu",(Int_t)(padHalfSize*2),floor(clusterOu->x())-padHalfSize,floor(clusterOu->x())+padHalfSize,(Int_t)(padHalfSize*2),floor(clusterOu->y())-padHalfSize,floor(clusterOu->y())+padHalfSize);

      // std::cout<<hitClusterIn.GetNbinsX()<<hitClusterOu.GetNbinsX()<<std::endl;

      for (int nx = 0; nx < hitClusterIn.GetNbinsX(); ++nx)
        for (int ny = 0; ny < hitClusterIn.GetNbinsY(); ++ny)
        {
           hitClusterIn.SetBinContent(nx,ny,0.0);
           hitClusterOu.SetBinContent(nx,ny,0.0);
        }
      for (int k = 0; k < clusterIn->size(); ++k)
          hitClusterIn.SetBinContent(hitClusterIn.FindBin((Double_t)clusterIn->pixel(k).x, (Double_t)clusterIn->pixel(k).y),clusterIn->pixel(k).adc);

      for (int k = 0; k < clusterOu->size(); ++k)
          hitClusterOu.SetBinContent(hitClusterOu.FindBin((Double_t)clusterOu->pixel(k).x, (Double_t)clusterOu->pixel(k).y),clusterOu->pixel(k).adc);


       clustersFile<<inId;
       clustersFile<<"\t"<<xin;
       clustersFile<<"\t"<<ouId;
       clustersFile<<"\t"<<xou;

       for (int nx = 0; nx < hitClusterIn.GetNbinsX(); ++nx)
        for (int ny = 0; ny < hitClusterIn.GetNbinsY(); ++ny)
            clustersFile<<"\t"<<hitClusterIn.GetBinContent(nx,ny);

        for (int nx = 0; nx < hitClusterOu.GetNbinsX(); ++nx)
         for (int ny = 0; ny < hitClusterOu.GetNbinsY(); ++ny)
           clustersFile<<"\t"<<hitClusterOu.GetBinContent(nx,ny);

        clustersFile<<std::endl<<std::endl;
    }

    if (result.size()>0)
      clustersFile.close();

  }

  // std::cout<<"Result size : "<<result.size()<<std::endl;

  return result;

}

void HitPairGeneratorFromLayerPair::doublets(const TrackingRegion& region,
						    const DetLayer & innerHitDetLayer,
						    const DetLayer & outerHitDetLayer,
						    const RecHitsSortedInPhi & innerHitsMap,
						    const RecHitsSortedInPhi & outerHitsMap,
						    const edm::EventSetup& iSetup,
						    const unsigned int theMaxElement,
						    HitDoublets & result){

  //  HitDoublets result(innerHitsMap,outerHitsMap); result.reserve(std::max(innerHitsMap.size(),outerHitsMap.size()));
  typedef RecHitsSortedInPhi::Hit Hit;
  InnerDeltaPhi deltaPhi(outerHitDetLayer, innerHitDetLayer, region, iSetup);

  // std::cout << "layers " << theInnerLayer.detLayer()->seqNum()  << " " << outerLayer.detLayer()->seqNum() << std::endl;

  // constexpr float nSigmaRZ = std::sqrt(12.f);
  constexpr float nSigmaPhi = 3.f;
  for (int io = 0; io!=int(outerHitsMap.theHits.size()); ++io) {
    if (!deltaPhi.prefilter(outerHitsMap.x[io],outerHitsMap.y[io])) continue;
    Hit const & ohit =  outerHitsMap.theHits[io].hit();
    PixelRecoRange<float> phiRange = deltaPhi(outerHitsMap.x[io],
					      outerHitsMap.y[io],
					      outerHitsMap.z[io],
					      nSigmaPhi*outerHitsMap.drphi[io]
					      );

    if (phiRange.empty()) continue;

    const HitRZCompatibility *checkRZ = region.checkRZ(&innerHitDetLayer, ohit, iSetup, &outerHitDetLayer,
						       outerHitsMap.rv(io),outerHitsMap.z[io],
						       outerHitsMap.isBarrel ? outerHitsMap.du[io] :  outerHitsMap.dv[io],
						       outerHitsMap.isBarrel ? outerHitsMap.dv[io] :  outerHitsMap.du[io]
						       );
    if(!checkRZ) continue;

    Kernels<HitZCheck,HitRCheck,HitEtaCheck> kernels;

    auto innerRange = innerHitsMap.doubleRange(phiRange.min(), phiRange.max());
    LogDebug("HitPairGeneratorFromLayerPair")<<
      "preparing for combination of: "<< innerRange[1]-innerRange[0]+innerRange[3]-innerRange[2]
				      <<" inner and: "<< outerHitsMap.theHits.size()<<" outter";
    for(int j=0; j<3; j+=2) {
      auto b = innerRange[j]; auto e=innerRange[j+1];
      bool ok[e-b];
      switch (checkRZ->algo()) {
	case (HitRZCompatibility::zAlgo) :
	  std::get<0>(kernels).set(checkRZ);
	  std::get<0>(kernels)(b,e,innerHitsMap, ok);
	  break;
	case (HitRZCompatibility::rAlgo) :
	  std::get<1>(kernels).set(checkRZ);
	  std::get<1>(kernels)(b,e,innerHitsMap, ok);
	  break;
	case (HitRZCompatibility::etaAlgo) :
	  std::get<2>(kernels).set(checkRZ);
	  std::get<2>(kernels)(b,e,innerHitsMap, ok);
	  break;
      }
      for (int i=0; i!=e-b; ++i) {
	if (!ok[i]) continue;
	if (theMaxElement!=0 && result.size() >= theMaxElement){
	  result.clear();
	  edm::LogError("TooManyPairs")<<"number of pairs exceed maximum, no pairs produced";
	  delete checkRZ;
	  return;
	}
        result.add(b+i,io);
      }
    }
    delete checkRZ;
  }

  LogDebug("HitPairGeneratorFromLayerPair")<<" total number of pairs provided back: "<<result.size();
  result.shrink_to_fit();

}
