
/*
 *  See header file for a description of this class.
 *
 *  $Date: 2006/06/20 12:17:42 $
 *  $Revision: 1.1 $
 *  \author G. Cerminara - INFN Torino
 */

#include "DTResolutionAnalysis.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "DQMServices/Core/interface/DaqMonitorBEInterface.h"
#include "Geometry/Vector/interface/Pi.h"

#include "DataFormats/DTRecHit/interface/DTRecSegment4DCollection.h"
#include "DataFormats/DTRecHit/interface/DTRecHitCollection.h"

#include "Geometry/DTGeometry/interface/DTGeometry.h"
#include "Geometry/Records/interface/MuonGeometryRecord.h"

#include <iterator>

using namespace edm;
using namespace std;

DTResolutionAnalysis::DTResolutionAnalysis(const ParameterSet& pset,
					   DaqMonitorBEInterface* dbe) : theDbe(dbe) {
					     debug = pset.getUntrackedParameter<bool>("debug","false");
					     // the name of the 4D rec hits collection
					     theRecHits4DLabel = pset.getParameter<string>("recHits4DLabel");
					     theRecHitLabel = pset.getParameter<string>("recHitLabel");
				     }

DTResolutionAnalysis::~DTResolutionAnalysis(){}


void DTResolutionAnalysis::analyze(const Event& event, const EventSetup& setup) {
  if(debug)
    cout << "[DTResolutionAnalysis] Analyze #Run: " << event.id().run()
	 << " #Event: " << event.id().event() << endl;

  // Get the 4D segment collection from the event
  edm::Handle<DTRecSegment4DCollection> all4DSegments;
  event.getByLabel(theRecHits4DLabel, all4DSegments);

  // Get the rechit collection from the event
  Handle<DTRecHitCollection> dtRecHits;
  event.getByLabel(theRecHitLabel, dtRecHits);

  // Get the DT Geometry
  ESHandle<DTGeometry> dtGeom;
  setup.get<MuonGeometryRecord>().get(dtGeom);



  // Loop over all chambers containing a segment
  DTRecSegment4DCollection::id_iterator chamberId;
  for (chamberId = all4DSegments->id_begin();
       chamberId != all4DSegments->id_end();
       ++chamberId) {
    // Get the range for the corresponding ChamerId
    DTRecSegment4DCollection::range  range = all4DSegments->get(*chamberId);
    int nsegm = distance(range.first, range.second);
    cout << "   Chamber: " << *chamberId << " has " << nsegm
	 << " 4D segments" << endl;
    // Get the chamber
    const DTChamber* chamber = dtGeom->chamber(*chamberId);

    // Loop over the rechits of this ChamerId
    for (DTRecSegment4DCollection::const_iterator segment4D = range.first;
	 segment4D!=range.second;
	 ++segment4D) {
      if(debug)
	cout << "   == RecSegment dimension: " << (*segment4D).dimension() << endl;
      
      // If Statio != 4 skip RecHits with dimension != 4
      // For the Station 4 consider 2D RecHits
      if((*chamberId).station() != 4 && (*segment4D).dimension() != 4) {
	if(debug)
	  cout << "[DTResolutionAnalysis]***Warning: RecSegment dimension is not 4 but "
	       << (*segment4D).dimension() << ", skipping!" << endl;
	continue;
      } else if((*chamberId).station() == 4 && (*segment4D).dimension() != 2) {
	if(debug)
	  cout << "[DTResolutionAnalysis]***Warning: RecSegment dimension is not 2 but "
	       << (*segment4D).dimension() << ", skipping!" << endl;
	continue;
      }


      // Get all 1D RecHits at step 3 within the 4D segment
      vector<DTRecHit1D> recHits1D_S3;
    

      // Get 1D RecHits at Step 3 and select only events with
      // 8 hits in phi and 4 hits in theta (if any)
      const DTChamberRecSegment2D* phiSeg = (*segment4D).phiSegment();
      vector<DTRecHit1D> phiRecHits = phiSeg->specificRecHits();
      if(phiRecHits.size() != 8) {
	cout << "[DTResolutionAnalysis] Phi segments has: " << phiRecHits.size()
	     << " hits, skipping" << endl; // FIXME: info output
	continue;
      }
      copy(phiRecHits.begin(), phiRecHits.end(), back_inserter(recHits1D_S3));
      if((*segment4D).dimension() == 4) {
	const DTSLRecSegment2D* zSeg = (*segment4D).zSegment();
	vector<DTRecHit1D> zRecHits = zSeg->specificRecHits();
	if(zRecHits.size() != 4) {
	  cout << "[DTResolutionAnalysis] Theta segments has: " << zRecHits.size()
	       << " hits, skipping" << endl; // FIXME: info output
	  continue;
	}
	copy(zRecHits.begin(), zRecHits.end(), back_inserter(recHits1D_S3));
      }

      // Loop over 1D RecHit inside 4D segment
      for(vector<DTRecHit1D>::const_iterator recHit1D = recHits1D_S3.begin();
	  recHit1D != recHits1D_S3.end();
	  recHit1D++) {
	const DTWireId wireId = (*recHit1D).wireId();
	
	// Get the layer and the wire position
	const DTLayer* layer = chamber->superLayer(wireId.superlayerId())->layer(wireId.layerId());
	float wireX = layer->specificTopology().wirePosition(wireId.wire());

	// Distance of the 1D rechit from the wire
	float distRecHitToWire = fabs(wireX - (*recHit1D).localPosition().x());
	
	// Extrapolate the segment to the z of the wire
	
	// Get wire position in chamber RF
	LocalPoint wirePosInLay(wireX,0,0);
	GlobalPoint wirePosGlob = layer->toGlobal(wirePosInLay);
	LocalPoint wirePosInChamber = chamber->toLocal(wirePosGlob);

	// Segment position at Wire z in chamber local frame
	LocalPoint segPosAtZWire = (*segment4D).localPosition()
	  + (*segment4D).localDirection()*wirePosInChamber.z()/cos((*segment4D).localDirection().theta());
	
	// Compute the distance of the segment from the wire
	int sl = wireId.superlayer();
  
	double distSegmToWire = -1;	
	if(sl == 1 || sl == 3) {
	  // RPhi SL
	  distSegmToWire = fabs(wirePosInChamber.x() - segPosAtZWire.x());
	} else if(sl == 2) {
	  // RZ SL
	  distSegmToWire = fabs(wirePosInChamber.y() - segPosAtZWire.y());
	}

	if(distSegmToWire > 2.1)
	  cout << "  Warning: dist segment-wire: " << distSegmToWire << endl;

	double residual = distRecHitToWire - distSegmToWire;

	// FIXME: Fill the histos
	fillHistos(wireId.superlayerId(), distSegmToWire, residual);

	if(debug) {
	  cout << "     Dist. segment extrapolation - wire (cm): " << distSegmToWire << endl;
	  cout << "     Dist. RecHit - wire (cm): " << distRecHitToWire << endl;
	  cout << "     Residual (cm): " << residual << endl;
	}

			  
      }// End of loop over 1D RecHit inside 4D segment
    }// End of loop over the rechits of this ChamerId
  }
  // -----------------------------------------------------------------------------
}


  
// Book a set of histograms for a given SL
void DTResolutionAnalysis::bookHistos(DTSuperLayerId slId) {
  if(debug)
    cout << "   Booking histos for SL: " << slId << endl;

  // Compose the chamber name
  stringstream wheel; wheel << slId.wheel();	
  stringstream station; station << slId.station();	
  stringstream sector; sector << slId.sector();	
  stringstream superLayer; superLayer << slId.superlayer();	

  
  string slHistoName =
    "_W" + wheel.str() +
    "_St" + station.str() +
    "_Sec" + sector.str() +
    "_SL" + superLayer.str();
  
  theDbe->setCurrentFolder("DT/DTLocalRecoTask/Wheel" + wheel.str() +
			   "/Station" + station.str() +
			   "/Sector" + sector.str());
  // Create the monitor elements
  vector<MonitorElement *> histos;
  // Note hte order matters
  histos.push_back(theDbe->book1D("hResDist"+slHistoName,
				  "Residuals on the distance from wire (rec_hit - segm_extr) (cm)",
				  200, -0.4, 0.4));
  histos.push_back(theDbe->book2D("hResDistVsDist"+slHistoName,
				  "Residuals on the distance (cm) from wire (rec_hit - segm_extr) vs distance  (cm)",
				  100, 0, 2.5, 200, -0.4, 0.4));
  histosPerSL[slId] = histos;
}




// Fill a set of histograms for a given SL 
void DTResolutionAnalysis::fillHistos(DTSuperLayerId slId,
				      float distExtr,
				      float residual) {
  // FIXME: optimization of the number of searches
  if(histosPerSL.find(slId) == histosPerSL.end()) {
    bookHistos(slId);
  }
  vector<MonitorElement *> histos =  histosPerSL[slId];                          
  histos[0]->Fill(residual);
  histos[1]->Fill(distExtr, residual);
}

