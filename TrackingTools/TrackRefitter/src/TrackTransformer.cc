#include "TrackingTools/TrackRefitter/interface/TrackTransformer.h"

#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "Geometry/Records/interface/GlobalTrackingGeometryRecord.h"
#include "MagneticField/Records/interface/IdealMagneticFieldRecord.h"

#include "TrackingTools/Records/interface/TrackingComponentsRecord.h"
#include "TrackingTools/TrackFitters/interface/TrajectoryFitter.h"
#include "TrackingTools/PatternTools/interface/TrajectorySmoother.h"

#include "TrackingTools/TrajectoryState/interface/TrajectoryStateOnSurface.h"
#include "TrackingTools/TransientTrack/interface/TransientTrack.h"
#include "TrackingTools/TransientTrackingRecHit/interface/TransientTrackingRecHitBuilder.h"
#include "TrackingTools/Records/interface/TransientRecHitRecord.h"
#include "TrackingTools/PatternTools/interface/Trajectory.h"
#include "TrackingTools/GeomPropagators/interface/Propagator.h"

#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/DetId/interface/DetId.h"

using namespace std;
using namespace edm;

/// Constructor
TrackTransformer::TrackTransformer(const ParameterSet& parameterSet){
  
  // Refit direction
  string refitDirectionName = parameterSet.getParameter<string>("RefitDirection");
  theRefitDirection = RefitDirection(refitDirectionName);
  
  theFitterName = parameterSet.getParameter<string>("Fitter");  
  theSmootherName = parameterSet.getParameter<string>("Smoother");  
  thePropagatorName = parameterSet.getParameter<string>("Propagator");

  theTrackerRecHitBuilderName = parameterSet.getParameter<string>("TrackerRecHitBuilder");
  theMuonRecHitBuilderName = parameterSet.getParameter<string>("MuonRecHitBuilder");

  theRPCInTheFit = parameterSet.getParameter<bool>("RefitRPCHits");
  theDoPredictionsOnly = parameterSet.getParameter<bool>("DoPredictionsOnly");

  theCacheId_TC = theCacheId_GTG = theCacheId_MG = theCacheId_TRH = 0;
}

/// Destructor
TrackTransformer::~TrackTransformer(){}


void TrackTransformer::setServices(const EventSetup& setup){
  
  const std::string metname = "Reco|TrackingTools|TrackTransformer";

  edm::ESHandle<TrajectoryFitter> aFitter;
  edm::ESHandle<TrajectorySmoother> aSmoother;
  setup.get<TrajectoryFitter::Record>().get(theFitterName,aFitter);
  setup.get<TrajectoryFitter::Record>().get(theSmootherName,aSmoother);
  theFitter = aFitter->clone();
  theSmoother.reset(aSmoother->clone());
  
  unsigned long long newCacheId_TC = setup.get<TrackingComponentsRecord>().cacheIdentifier();

  if ( newCacheId_TC != theCacheId_TC ){
    LogTrace(metname) << "Tracking Component changed!";
    theCacheId_TC = newCacheId_TC;
    setup.get<TrackingComponentsRecord>().get(thePropagatorName,thePropagator);
  }

  // Global Tracking Geometry
  unsigned long long newCacheId_GTG = setup.get<GlobalTrackingGeometryRecord>().cacheIdentifier();
  if ( newCacheId_GTG != theCacheId_GTG ) {
    LogTrace(metname) << "GlobalTrackingGeometry changed!";
    theCacheId_GTG = newCacheId_GTG;
    setup.get<GlobalTrackingGeometryRecord>().get(theTrackingGeometry); 
  }
  
  // Magfield Field
  unsigned long long newCacheId_MG = setup.get<IdealMagneticFieldRecord>().cacheIdentifier();
  if ( newCacheId_MG != theCacheId_MG ) {
    LogTrace(metname) << "Magnetic Field changed!";
    theCacheId_MG = newCacheId_MG;
    setup.get<IdealMagneticFieldRecord>().get(theMGField);
  }
  
  // Transient Rechit Builders
  unsigned long long newCacheId_TRH = setup.get<TransientRecHitRecord>().cacheIdentifier();
  if ( newCacheId_TRH != theCacheId_TRH ) {
    theCacheId_TRH = newCacheId_TRH;
    LogTrace(metname) << "TransientRecHitRecord changed!";
    setup.get<TransientRecHitRecord>().get(theTrackerRecHitBuilderName,theTrackerRecHitBuilder);
    setup.get<TransientRecHitRecord>().get(theMuonRecHitBuilderName,theMuonRecHitBuilder);
    hitCloner = static_cast<TkTransientTrackingRecHitBuilder const *>(theTrackerRecHitBuilder.product())->cloner();
  }
  theFitter->setHitCloner(&hitCloner);
  theSmoother->setHitCloner(&hitCloner);

}


vector<Trajectory> TrackTransformer::transform(const reco::TrackRef& track) const {
  return transform(*track);
}


TransientTrackingRecHit::ConstRecHitContainer
TrackTransformer::getTransientRecHits(const reco::TransientTrack& track) const {

  TransientTrackingRecHit::ConstRecHitContainer result;
  auto tkbuilder = static_cast<TkTransientTrackingRecHitBuilder const *>(theTrackerRecHitBuilder.product());

  
  for (trackingRecHit_iterator hit = track.recHitsBegin(); hit != track.recHitsEnd(); ++hit) {
    if((*hit)->isValid()) {
      if ( (*hit)->geographicalId().det() == DetId::Tracker ) {
	result.emplace_back((**hit).cloneForFit(*tkbuilder->geometry()->idToDet( (**hit).geographicalId() ) ) );
      } else if ( (*hit)->geographicalId().det() == DetId::Muon ){
	if( (*hit)->geographicalId().subdetId() == 3 && !theRPCInTheFit){
	  LogTrace("Reco|TrackingTools|TrackTransformer") << "RPC Rec Hit discarged"; 
	  continue;
	}
	// AR: Remove the muon hits from the fit
	// result.push_back(theMuonRecHitBuilder->build(&**hit));
      }
    }
  }
  
  return result;
}

// FIXME: check this method!
RefitDirection::GeometricalDirection
TrackTransformer::checkRecHitsOrdering(TransientTrackingRecHit::ConstRecHitContainer& recHits) const {
  
  if (!recHits.empty()){
    GlobalPoint first = trackingGeometry()->idToDet(recHits.front()->geographicalId())->position();
    GlobalPoint last = trackingGeometry()->idToDet(recHits.back()->geographicalId())->position();
    
    double rFirst = first.mag();
    double rLast  = last.mag();
    if(rFirst < rLast) return RefitDirection::insideOut;
    else if(rFirst > rLast) return RefitDirection::outsideIn;
    else{
      LogDebug("Reco|TrackingTools|TrackTransformer") << "Impossible to determine the rechits order" <<endl;
      return RefitDirection::undetermined;
    }
  }
  else{
    LogDebug("Reco|TrackingTools|TrackTransformer") << "Impossible to determine the rechits order" <<endl;
    return RefitDirection::undetermined;
  }
}

// void reorder(TransientTrackingRecHit::ConstRecHitContainer& recHits, RefitDirection::GeometricalDirection recHitsOrder) const{

//   if(theRefitDirection.geometricalDirection() != recHitsOrder) reverse(recHits.begin(),recHits.end());

//   if(theRefitDirection.geometricalDirection() == RefitDirection::insideOut &&recHitsOrder){}
//   else if(theRefitDirection.geometricalDirection() == RefitDirection::outsideIn){} 
//   else LogWarning("Reco|TrackingTools|TrackTransformer") << "Impossible to determine the rechits order" <<endl;
// }


/// Convert Tracks into Trajectories
vector<Trajectory> TrackTransformer::transform(const reco::Track& newTrack) const {

  const std::string metname = "Reco|TrackingTools|TrackTransformer";
  
  reco::TransientTrack track(newTrack,magneticField(),trackingGeometry());   

  // Build the transient Rechits
  TransientTrackingRecHit::ConstRecHitContainer recHitsForReFit = getTransientRecHits(track);

  return transform(track, recHitsForReFit);
}


/// Convert Tracks into Trajectories with a given set of hits
vector<Trajectory> TrackTransformer::transform(const reco::TransientTrack& track,
                                               const TransientTrackingRecHit::ConstRecHitContainer& _recHitsForReFit) const {
  
  TransientTrackingRecHit::ConstRecHitContainer recHitsForReFit =  _recHitsForReFit;
  const std::string metname = "Reco|TrackingTools|TrackTransformer";

  if(recHitsForReFit.size() < 2) return vector<Trajectory>();
 
  // 8 cases are foreseen: 
  // [RH = rec hit order, P = momentum dir, FD = fit direction. IO/OI = inside-out/outside-in, AM/OM = along momentum/opposite to momentum]
  // (1) RH IO | P IO | FD AM  ---> Start from IN
  // (2) RH IO | P IO | FD OM  ---> Reverse RH and start from OUT
  // (3) RH IO | P OI | FD AM  ---> Reverse RH and start from IN
  // (4) RH IO | P OI | FD OM  ---> Start from OUT
  // (5) RH OI | P IO | FD AM  ---> Reverse RH and start from IN
  // (6) RH OI | P IO | FD OM  ---> Start from OUT
  // (7) RH OI | P OI | FD AM  ---> Start from IN
  // (8) RH OI | P OI | FD OM  ---> Reverse RH and start from OUT
  //
  // *** Rules: ***
  // -A- If RH-FD agree (IO-AM,OI-OM) do not reverse the RH
  // -B- If FD along momentum start from innermost state, otherwise use outermost
  
  // Other special cases can be handled:
  // (1 bis) RH IO | P IO | GFD IO => FD AM  ---> Start from IN
  // (2 bis) RH IO | P IO | GFD OI => FD OM  ---> Reverse RH and start from OUT
  // (3 bis) RH IO | P OI | GFD OI => FD AM  ---> Reverse RH and start from OUT
  // (4 bis) RH IO | P OI | GFD IO => FD OM  ---> Start from IN
  // (5 bis) RH OI | P IO | GFD IO => FD AM  ---> Reverse RH and start from IN
  // (6 bis) RH OI | P IO | GFD OI => FD OM  ---> Start from OUT
  // (7 bis) RH OI | P OI | GFD OI => FD AM  ---> Start from OUT
  // (8 bis) RH OI | P OI | GFD IO => FD OM  ---> Reverse RH and start from IN
  // 
  // *** Additional rule: ***
  // -A0- If P and GFD agree, then FD is AM otherwise is OM
  // -A00- rechit must be ordered as GFD in order to handle the case of cosmics
  // -B0- The starting state is decided by GFD

  // Determine the RH order
  RefitDirection::GeometricalDirection recHitsOrder = checkRecHitsOrdering(recHitsForReFit); // FIXME change nome of the *type*  --> RecHit order!
  LogTrace(metname) << "RH order (0-insideOut, 1-outsideIn): " << recHitsOrder;

  PropagationDirection propagationDirection = theRefitDirection.propagationDirection();

  // Apply rule -A0-
  if(propagationDirection == anyDirection){
    GlobalVector momentum = track.innermostMeasurementState().globalMomentum();
    GlobalVector position = track.innermostMeasurementState().globalPosition() - GlobalPoint(0,0,0);
    RefitDirection::GeometricalDirection p = (momentum.x()*position.x() > 0 || momentum.y()*position.y() > 0) ? RefitDirection::insideOut : RefitDirection::outsideIn;

    propagationDirection = p == theRefitDirection.geometricalDirection() ? alongMomentum : oppositeToMomentum;
    LogTrace(metname) << "P  (0-insideOut, 1-outsideIn): " << p;
    LogTrace(metname) << "FD (0-OM, 1-AM, 2-ANY): " << propagationDirection;
  }
  // -A0-

  // Apply rule -A-
  if(theRefitDirection.propagationDirection() != anyDirection){
    if((recHitsOrder == RefitDirection::insideOut && propagationDirection == oppositeToMomentum) ||
       (recHitsOrder == RefitDirection::outsideIn && propagationDirection == alongMomentum) ) 
      reverse(recHitsForReFit.begin(),recHitsForReFit.end());}
  // -A-
  // Apply rule -A00-
  else{
    // reorder the rechit as defined in theRefitDirection.geometricalDirection(); 
    if(theRefitDirection.geometricalDirection() != recHitsOrder) reverse(recHitsForReFit.begin(),recHitsForReFit.end()); 
  }
  // -A00-
    
  // Apply rule -B-
  TrajectoryStateOnSurface firstTSOS = track.innermostMeasurementState();
  unsigned int innerId = track.track().innerDetId();
  if(theRefitDirection.propagationDirection() != anyDirection){
    if(propagationDirection == oppositeToMomentum){
      innerId   = track.track().outerDetId();
      firstTSOS = track.outermostMeasurementState();
    }
  }
  else { // if(theRefitDirection.propagationDirection() == anyDirection)
    // Apply rule -B0-
    if(theRefitDirection.geometricalDirection() == RefitDirection::outsideIn){
      innerId   = track.track().outerDetId();
      firstTSOS = track.outermostMeasurementState();
    }
    // -B0-
  }
  // -B-

  if(!firstTSOS.isValid()){
    LogTrace(metname)<<"Error wrong initial state!"<<endl;
    return vector<Trajectory>();
  }

  TrajectorySeed seed(PTrajectoryStateOnDet(),TrajectorySeed::recHitContainer(),propagationDirection);

  if(recHitsForReFit.front()->geographicalId() != DetId(innerId)){
    LogTrace(metname)<<"Propagation occured"<<endl;
    firstTSOS = propagator()->propagate(firstTSOS, recHitsForReFit.front()->det()->surface());
    if(!firstTSOS.isValid()){
      LogTrace(metname)<<"Propagation error!"<<endl;
      return vector<Trajectory>();
    }
  }

  if(theDoPredictionsOnly){
    Trajectory aTraj(seed,propagationDirection);
    TrajectoryStateOnSurface predTSOS = firstTSOS;
    for(TransientTrackingRecHit::ConstRecHitContainer::const_iterator ihit = recHitsForReFit.begin(); 
	ihit != recHitsForReFit.end(); ++ihit ) {
      predTSOS = propagator()->propagate(predTSOS, (*ihit)->det()->surface());
      if (predTSOS.isValid()) aTraj.push(TrajectoryMeasurement(predTSOS, *ihit));
    }
    return vector<Trajectory>(1, aTraj);
  }


  vector<Trajectory> trajectories = theFitter->fit(seed,recHitsForReFit,firstTSOS);
  
  if(trajectories.empty()){
    LogTrace(metname)<<"No Track refitted!"<<endl;
    return vector<Trajectory>();
  }
  
  Trajectory trajectoryBW = trajectories.front();
    
  vector<Trajectory> trajectoriesSM = theSmoother->trajectories(trajectoryBW);

  if(trajectoriesSM.empty()){
    LogTrace(metname)<<"No Track smoothed!"<<endl;
    return vector<Trajectory>();
  }
  

  // second refit

  Trajectory trajectoryFW2 = trajectoriesSM.front();
  trajectoryFW2.reverse();
  vector<Trajectory> trajectories2 = theFitter->fit(trajectoryFW2.seed(),trajectoryFW2.recHits(),trajectoryFW2.geometricalInnermostState());
  if(trajectories2.empty()){
    LogTrace(metname)<<"No Track refitted!"<<endl;
    return vector<Trajectory>();
  }
  Trajectory trajectoryBW2 = trajectories2.front();
  vector<Trajectory> trajectoriesSM2 = theSmoother->trajectories(trajectoryBW2);
  if(trajectoriesSM2.empty()){
    LogTrace(metname)<<"No Track smoothed!"<<endl;
    return vector<Trajectory>();
  }

  // end of second refit

  // third refit

  Trajectory trajectoryFW3 = trajectoriesSM2.front();
  trajectoryFW3.reverse();
  vector<Trajectory> trajectories3 = theFitter->fit(trajectoryFW3.seed(),trajectoryFW3.recHits(),trajectoryFW3.geometricalInnermostState());
  if(trajectories3.empty()){
    LogTrace(metname)<<"No Track refitted!"<<endl;
    return vector<Trajectory>();
  }
  Trajectory trajectoryBW3 = trajectories3.front();
  vector<Trajectory> trajectoriesSM3 = theSmoother->trajectories(trajectoryBW3);
  if(trajectoriesSM3.empty()){
    LogTrace(metname)<<"No Track smoothed!"<<endl;
    return vector<Trajectory>();
  }

  // end of third refit
  
  return trajectoriesSM3;

  // AR. Changed below following instructions at https://twiki.cern.ch/twiki/bin/viewauth/CMS/MuonAlignmentReferenceTarget
  // return trajectoriesSM;

}


