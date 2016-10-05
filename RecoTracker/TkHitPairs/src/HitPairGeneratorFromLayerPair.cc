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
#include "RecoPixelVertexing/PixelTriplets/plugins/FKDTree.h"

#include "FWCore/Framework/interface/Event.h"

using namespace GeomDetEnumerators;
using namespace std;

typedef PixelRecoRange<float> Range;
using LayerTree =  FKDTree<float,3>;
using LayerPoint = FKDPoint<float,3>;

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
: theLayerCache(*layerCache), theOuterLayer(outer), theInnerLayer(inner), theMaxElement(max)
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
        void operator()(LayerTree* tree,const SeedingLayerSetsHits::SeedingLayer& innerLayer,const PixelRecoRange<float>& phiRange,std::vector<unsigned int>& foundHits,Range searchRange) const {
            
            float rmax,rmin,zmax,zmin;
            
            if(innerLayer.detLayer()->isBarrel()){
                
                zmax = searchRange.max();
                zmin = searchRange.min();
                rmax = 1000;//+(u+thickness+vErr;
                rmin = -1000;//u-thickness-vErr;
                
            }else{
                
                rmax = searchRange.max();
                rmin = searchRange.min();
                zmin = -1000;//u+thickness+vErr;
                zmax = 1000;//u-thickness-vErr;
                
            }
            
            //std::cout<<"r min : "<<rmin<<std::endl;
            //std::cout<<"r max : "<<rmax<<std::endl;
            //std::cout<<"z min : "<<zmin<<std::endl;
            //std::cout<<"z max : "<<zmax<<std::endl;
            
            
            const LayerPoint minPoint(phiRange.min(),zmin,rmin,0);
            
            const LayerPoint maxPoint(phiRange.max(),zmax,rmax,100000);
            
            tree->LayerTree::search_in_the_box(minPoint,maxPoint,foundHits);
            
            //std::cout<<"FKDTree Search : done!"<<std::endl;
            
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
                                                    const edm::Event & iEvent, const edm::EventSetup& iSetup, Layers layers) {
    
    typedef OrderedHitPair::InnerRecHit InnerHit;
    typedef OrderedHitPair::OuterRecHit OuterHit;
    typedef RecHitsSortedInPhi::Hit Hit;
    
    Layer innerLayerObj = innerLayer(layers);
    Layer outerLayerObj = outerLayer(layers);
    
    const RecHitsSortedInPhi & innerHitsMap = theLayerCache(innerLayerObj, region, iEvent, iSetup);
    if (innerHitsMap.empty()) return HitDoublets(innerHitsMap,innerHitsMap);
    
    const RecHitsSortedInPhi& outerHitsMap = theLayerCache(outerLayerObj, region, iEvent, iSetup);
    if (outerHitsMap.empty()) return HitDoublets(innerHitsMap,outerHitsMap);
    HitDoublets result(innerHitsMap,outerHitsMap); result.reserve(std::max(innerHitsMap.size(),outerHitsMap.size()));
    doublets(region,
             *innerLayerObj.detLayer(),*outerLayerObj.detLayer(),
             innerHitsMap,outerHitsMap,iSetup,theMaxElement,result);
    return result;
    
}


HitDoublets HitPairGeneratorFromLayerPair::doublets( const TrackingRegion& region,
                                                    const edm::Event & iEvent, const edm::EventSetup& iSetup, const Layer& innerLayer, const Layer& outerLayer) {
    
    typedef OrderedHitPair::InnerRecHit InnerHit;
    typedef OrderedHitPair::OuterRecHit OuterHit;
    typedef RecHitsSortedInPhi::Hit Hit;
    
    
    const RecHitsSortedInPhi & innerHitsMap = theLayerCache(innerLayer, region, iEvent, iSetup);
    if (innerHitsMap.empty()) return HitDoublets(innerHitsMap,innerHitsMap);
    
    const RecHitsSortedInPhi& outerHitsMap = theLayerCache(outerLayer, region, iEvent, iSetup);
    if (outerHitsMap.empty()) return HitDoublets(innerHitsMap,outerHitsMap);
    HitDoublets result(innerHitsMap,outerHitsMap); result.reserve(std::max(innerHitsMap.size(),outerHitsMap.size()));
    doublets(region,
             *innerLayer.detLayer(),*outerLayer.detLayer(),
             innerHitsMap,outerHitsMap,iSetup,theMaxElement,result);
    
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

HitDoublets HitPairGeneratorFromLayerPair::doubletsCA( const TrackingRegion& region,
                                                      const edm::Event & iEvent, const edm::EventSetup& iSetup, const Layer& innerLayer, const Layer& outerLayer) {
    
    typedef OrderedHitPair::InnerRecHit InnerHit;
    typedef OrderedHitPair::OuterRecHit OuterHit;
    typedef RecHitsSortedInPhi::Hit Hit;
    
    const RecHitsSortedInPhi & innerHitsMap = theLayerCache(innerLayer, region, iEvent, iSetup);
    if (innerHitsMap.empty()) return HitDoublets(innerHitsMap,innerHitsMap);
    
    const RecHitsSortedInPhi& outerHitsMap = theLayerCache(outerLayer, region, iEvent, iSetup);
    if (outerHitsMap.empty()) return HitDoublets(innerHitsMap,outerHitsMap);
    
    LayerTree *innerTree = new FKDTree<float,3>();
    innerTree->FKDTree<float,3>::make_FKDTreeFromRecHitsInPhi(innerHitsMap,region);
    
    //std::cout<<"Hit Doublets CA Generator : in!  -  ";
    HitDoublets result(innerHitsMap,outerHitsMap);
    result.reserve(std::max(innerHitsMap.size(),outerHitsMap.size()));
    //HitDoubletsCA result(innerLayer,outerLayer);
    //std::cout<<"Results initialised : done!"<<std::endl;
    InnerDeltaPhi deltaPhi(*outerLayer.detLayer(),*innerLayer.detLayer(), region, iSetup);
    //std::cout<<"Delta phi : done!"<<std::endl;
    // std::cout << "layers " << theInnerLayer.detLayer()->seqNum()  << " " << outerLayer.detLayer()->seqNum() << std::endl;
    bool rangesDone = false;
    float upperLimit = -10000;
    float lowerLimit = 10000;
    
    constexpr float nSigmaPhi = 3.f;
    for (int io = 0; io!=int(outerLayer.hits().size()); ++io) {
        //std::cout<<"  Outer hit cylce : in!("<<io<<")"<<std::endl;
        Hit const & ohit = outerLayer.hits()[io];
        auto const & gs = static_cast<BaseTrackerRecHit const &>(*ohit).globalState();
        auto loc = gs.position-region.origin().basicVector();
        
        float oX = gs.position.x();
        float oY = gs.position.y();
        float oZ = gs.position.z();
        float oRv = loc.perp();
        //std::cout<<"Hit : "<<oX<<" - "<<oY<<" - "<<oZ<<std::endl;
        float oDrphi = gs.errorRPhi;
        float oDr = gs.errorR;
        float oDz = gs.errorZ;
        //std::cout<<"Outer Hit""Parameters : done!"<<"("<<io<<")"<<std::endl;
        if (!deltaPhi.prefilter(oX,oY)) continue;
        
        PixelRecoRange<float> phiRange = deltaPhi(oX,oY,oZ,nSigmaPhi*oDrphi);
        
        const HitRZCompatibility *checkRZ = region.checkRZ(innerLayer.detLayer(), ohit, iSetup, outerLayer.detLayer(), oRv, oZ, oDr, oDz);
        if(!checkRZ) continue;
        
        float rangeRatios = 0.0;
        
        
        for(int ii = 0; ii!=int(innerLayer.hits().size()); ++ii){
            
            Hit const & ihit = innerLayer.hits()[ii];
            auto const & gsInner = static_cast<BaseTrackerRecHit const &>(*ihit).globalState();
            auto locInner = gsInner.position-region.origin().basicVector();
            
            auto uInner = innerLayer.detLayer()->isBarrel() ? locInner.perp() : gsInner.position.z();
            
            Range bufferrange = checkRZ->range(uInner);
            
            upperLimit = std::max(bufferrange.min(),upperLimit);
            upperLimit = std::max(bufferrange.max(),upperLimit);
            
            lowerLimit = std::min(bufferrange.max(),lowerLimit);
            lowerLimit = std::min(bufferrange.min(),lowerLimit);
            
            //std::cout<<"At : "<<uInner<<" - ";
            //std::cout<<"Allowed range : "<<bufferrange.min()<<" - "<<bufferrange.max()<<std::endl;
            rangeRatios = std::max((upperLimit-lowerLimit)/(bufferrange.min()-bufferrange.max()),rangeRatios);
            
        }
        
        //std::cout<<"Final range : "<<lowerLimit<<" - "<<upperLimit<<std::endl;
        //std::cout<<"Phi : "<<loc.barePhi()<<std::endl;
        //std::cout<<"Zeta : "<<oZ<<std::endl;
        //std::cout<<"R : "<<oRv<<std::endl;
        //rangesDone = true;
        
        
        
        
        
        Range rangeSearch(lowerLimit,upperLimit);
        //std::cout<<"  -  HitRZ Check : done!"<<"("<<io<<")   ";
        Kernels<HitZCheck,HitRCheck,HitEtaCheck> kernels;
        
        std::vector<unsigned int> foundHitsInRange;
        
        
        switch (checkRZ->algo()) {
            case (HitRZCompatibility::zAlgo) :
                std::get<0>(kernels).set(checkRZ);
                std::get<0>(kernels)(innerTree,innerLayer,phiRange,foundHitsInRange,rangeSearch);
                break;
            case (HitRZCompatibility::rAlgo) :
                std::get<1>(kernels).set(checkRZ);
                std::get<1>(kernels)(innerTree,innerLayer,phiRange,foundHitsInRange,rangeSearch);
                break;
            case (HitRZCompatibility::etaAlgo) :
                //std::cout<<"HitRZ Check : etaAlgo CAZZO!"<<"("<<io<<")"<<std::endl;
                break;
        }
        //std::cout<<"Found hits : "<<foundHitsInRange.size()<<" ("<<io<<")"<<std::endl;
        for (auto i=0; i!=(int)foundHitsInRange.size(); ++i) {
            
            if (theMaxElement!=0 && result.size() >= theMaxElement){
                result.clear();
                edm::LogError("TooManyPairs")<<"number of pairs exceed maximum, no pairs produced";
                delete checkRZ;
                return result;
            }
            result.add(foundHitsInRange[i],io);
        }
        delete checkRZ;
        
    }
    LogDebug("HitPairGeneratorFromLayerPairCA")<<" total number of pairs provided back: "<<result.size();
    result.shrink_to_fit();
    return result;
    
}



