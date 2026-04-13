#include "VertexLeastSquares.h"

VertexLeastSquares::VertexLeastSquares():Tool(){}

//------------------------------------------------------------------------------
bool VertexLeastSquares::Initialise(std::string configfile, DataModel &data)
{

  /////////////////// Useful header ///////////////////////
  if(configfile!="") m_variables.Initialise(configfile); // loading config file
  //m_variables.Print();

  m_data= &data; //assigning transient data pointer
  /////////////////////////////////////////////////////////////////

  // Load my config parameters
  bool gotVerbosity = m_variables.Get("verbosity",verbosity);
  if (!gotVerbosity) {
    verbosity = 0;
    Log("VertexLeastSquares: \"verbosity\" not set in the config, defaulting to 0", v_error, verbosity);
  }

  // ***********************************************************************

  bool gotUseMCHits = m_variables.Get("UseMCHits", fUseMCHits);
  if (!gotUseMCHits) {
    Log("VertexLeastSquares: \"UseMCHits\" not set in the config! Aborting!", v_error, verbosity);
    return false;
  }

  bool gotBreakDist = m_variables.Get("BreakDist", fBreakDist);
  if (!gotBreakDist) {
    fBreakDist = 0.05;
    Log("VertexLeastSquares: \"BreakDist\" not set in the config! Using default value of 5 cm.", v_error, verbosity);
  }

  bool gotYSpacing = m_variables.Get("YSpacing", fYSpacing);
  if (!gotYSpacing) {
    fYSpacing = 0.5;
    Log("VertexLeastSquares: \"YSpacing\" not set in the config! Using default value of 50 cm.", v_error, verbosity);
  }

  bool gotNPlanarPoints = m_variables.Get("NPlanarPoints", fNPlanarPoints);
  if (!gotNPlanarPoints) {
    fNPlanarPoints = 20;
    Log("VertexLeastSquares: \"NPlanarPoints\" not set in the config! Using default value of 20.", v_error, verbosity);
  }

  bool gotRegularizer = m_variables.Get("Regularizer", fRegularizer);
  if (!gotRegularizer) {
    fRegularizer = 1.;
    Log("VertexLeastSquares: \"Regularizer\" not set in the config! Using default value of 1.", v_error, verbosity);
  }

  bool gotFittingSteps = m_variables.Get("FittingSteps", fFittingSteps);
  if (!gotFittingSteps) {
    fFittingSteps = 100.;
    Log("VertexLeastSquares: \"FittingSteps\" not set in the config! Using default value of 100.", v_error, verbosity);
  }

  bool gotMostCompatible = m_variables.Get("FilterHitsCompatibleWindow", fCompatibleHits);
  if (!gotMostCompatible) {
    fCompatibleHits = false;
    Log("VertexLeastSquares: \"FilterHitsCompatibleWindow\" not set in the config! Defaulting to false.", v_error, verbosity);
  }

  bool gotGoodTimes = m_variables.Get("OnlyUseReliablePMTs", fGoodTimes);
  if (!gotGoodTimes) {
    fGoodTimes = false;
    Log("VertexLeastSquares: \"OnlyUseReliablePMTs\" not set in the config! Defaulting to false.", v_error, verbosity);
  }

  bool gotMinGoodTime = m_variables.Get("MinimumReliableTimingStd", fMinGoodTime);
  if (!gotMinGoodTime) {
    fMinGoodTime = 1.5;   // ns
    Log("VertexLeastSquares: \"MinimumReliableTimingStd\" not set in the config! Defaulting to 1.5ns.", v_error, verbosity);
  }

  // ***********************************************************************

  bool gotExternalSeeding = m_variables.Get("ExternalSeeding",fExternalSeeding);
  if (!gotExternalSeeding) {
    fExternalSeeding = false;
    Log("VertexLeastSquares: \"ExternalSeeding\" not set in the config, defaulting to false (only seed the tank).", v_error, verbosity);
  }

  bool gotYbuffer = m_variables.Get("YBuffer", fYBuffer);
  if (!gotYbuffer) {
    fYSpacing = 1.0;
    Log("VertexLeastSquares: \"YBuffer\" not set in the config! Using default value of 100 cm.", v_error, verbosity);
  }

  bool gotRadialbuffer = m_variables.Get("RadialBuffer", fRadialBuffer);
  if (!gotRadialbuffer) {
    fRadialBuffer = 1.0;
    Log("VertexLeastSquares: \"RadialBuffer\" not set in the config! Using default value of 100 cm.", v_error, verbosity);
  }

  bool gotVertexMaxRadius = m_variables.Get("ExternalVertexMaxRadius", fExternalVertexMaxRadius);
  if (!gotVertexMaxRadius) {
    fExternalVertexMaxRadius = 4.0;
    Log("VertexLeastSquares: \"ExternalVertexMaxRadius\" not set in the config! Using default value of 4 m.", v_error, verbosity);
  }

  bool gotVertexMaxHeight = m_variables.Get("ExternalVertexMaxHeight", fExternalVertexMaxHeight);
  if (!gotVertexMaxHeight) {
    fExternalVertexMaxHeight = 4.0;
    Log("VertexLeastSquares: \"ExternalVertexMaxHeight\" not set in the config! Using default value of 4 m.", v_error, verbosity);
  }

  // ***********************************************************************

  fNBoundary = int(sqrt(fRegularizer));
  
  // Load the geometry service
  bool gotGeometry = m_data->Stores.at("ANNIEEvent")->Header->Get("AnnieGeometry",fGeom);
  if(!gotGeometry){
    Log("VertexLeastSquares: Error retrieving Geometry from ANNIEEvent! Aborting!", v_error, verbosity); 
    return false; 
  }

  // Load the PMT timing uncertainties
  if (fGoodTimes) {
    bool got_timing_std = m_data->CStore.Get("ChannelNumToTankPMTTimingSigmaMap",ChannelKeyToTimingSigmaMap); 
    if (!got_timing_std) {
      logmessage = "VertexLeastSquares: \"OnlyUseReliablePMTs\" enabled --> error retrieving PMT timing uncertainty... double check LoadGeometry maybe?";
      Log(logmessage, v_error, verbosity);
      return false;
    }
  }

  // Set up the pointers we're going to save. No need to 
  // delete it at Finalize, the store will handle it (I'm trusting you Andrew!)
  fVertexMap = new std::map<double, Position>;
  fVertexStdevMap = new std::map<double, double>;

  return true;
}

//------------------------------------------------------------------------------
bool VertexLeastSquares::Execute()
{

  fVertexMap->clear();
  fVertexStdevMap->clear();
  if (fUseMCHits) {
    bool gotClusters = m_data->CStore.Get("ClusterMapMC", fClusterMapMC);
    if (!gotClusters) {
      logmessage = "VertexLeastSquares: no ClusterMapMC in the ANNIEEvent!!! Are you sure you ran ClusterFinder?";
      Log(logmessage, v_error, verbosity);
      return false;
    }
    RunLoopMC();
  } else {
    bool gotClusters = m_data->CStore.Get("ClusterMap", fClusterMap);
    if (!gotClusters) {
      logmessage = "VertexLeastSquares: no ClusterMap in the ANNIEEvent!!! Are you sure you ran ClusterFinder?";
      Log(logmessage, v_error, verbosity);
      return false;
    }

    RunLoop();
  }

  m_data->Stores.at("ANNIEEvent")->Set("VertexLeastSquaresMap",  fVertexMap);
  m_data->Stores.at("ANNIEEvent")->Set("VertexLeastSquaresStdevMap",  fVertexStdevMap);
  
  return true;
}

//------------------------------------------------------------------------------
bool VertexLeastSquares::Finalise()
{

  return true;
}

//-----------------------------------------------------------------------------
std::vector<Position> VertexLeastSquares::GenerateVetices()   // this is Andrew's fault there's a typo (and no I'm not going to fix it :) )
{

  // generate vertices in and outside the tank (+ some buffer if ExternalSeeding is enabled)

  std::vector<Position> vertices;
  std::vector<double> ys;
  double yMin = fGeom->GetTankCentre().Y() - fGeom->GetTankHalfheight();
  double yMax = fGeom->GetTankCentre().Y() + fGeom->GetTankHalfheight();
  double max_radius = fGeom->GetTankRadius();

  if (fExternalSeeding) {
	yMin -= fYBuffer;
    yMax += fYBuffer;
  	max_radius += fRadialBuffer;
  }
	  
  for (double yValue = yMin; yValue <= yMax; yValue += fYSpacing) 
  	ys.push_back(yValue);

  std::vector<double> xs;
  std::vector<double> zs;
  for (int n = 1; n < fNPlanarPoints+1; ++n) {
	double rad = ( (n > fNPlanarPoints + fNBoundary) ? 1.0 :
	sqrt((n+0.5)/(fNPlanarPoints - (fNBoundary+1.)/2.)) );

	rad *= max_radius;
	rad = rad > max_radius ? max_radius : rad;

	double angle = 2. * pi * n / phisq;

	xs.push_back(rad*cos(angle));
	zs.push_back(rad*sin(angle));
  }

  
  // generate the x and z values using the sunflower pattern
  std::vector<double> xs;
  std::vector<double> zs;
  double max_radius = fGeom->GetTankRadius() + fRadialBuffer;
  for (int n = 1; n < fNPlanarPoints+1; ++n) {
	double rad = ( (n > fNPlanarPoints + fNBoundary) ? 1.0 :
	  sqrt((n+0.5)/(fNPlanarPoints - (fNBoundary+1.)/2.)) );

	// scale to the extended radius
	rad *= max_radius;
	rad = rad > max_radius ? max_radius : rad;

	double angle = 2. * pi * n / phisq;

	xs.push_back(rad*cos(angle));
	zs.push_back(rad*sin(angle));
  }

  // add vertices from the sunflower layers
  std::vector<Position> vertices;
  for (uint yIdx = 0; yIdx < ys.size(); ++yIdx) {

	// Apply a random rotation to each y level to cover more space
	double rotAng = (double(rand())/RAND_MAX)*2*pi;
	for (uint xzIdx = 0; xzIdx < fNPlanarPoints; ++xzIdx) {
	  double x = xs[xzIdx];
	  double z = zs[xzIdx];

	  vertices.push_back(Position(x*cos(rotAng) - z*sin(rotAng) + fGeom->GetTankCentre().X(),
		  ys[yIdx],
		  z*cos(rotAng) + x*sin(rotAng) + fGeom->GetTankCentre().Z()));
	}
  }

  return vertices;
}


//------------------------------------------------------------------------------
void VertexLeastSquares::EvalAtGuessVertex(util::Matrix &A, util::Vector &b,
					   const Position &guess,
					   const std::vector<Hit> &hits)
{
  // The function f(x) = 0
  // 0 = 1/c * sqrt( (X-xi)^2 + (Y-yi)^2 + (Z-zi)^2 ) + T - ti
  // A = Jacobian(guess), b = -f(guess)
  double mean_T = 0;
  for (uint hitIdx = 0; hitIdx < hits.size(); ++hitIdx) {
    Detector *det = fGeom->ChannelToDetector(hits[hitIdx].GetTubeId());
    Position detpos = det->GetDetectorPosition();
    double dist = (detpos - guess).Mag();    
    double ti = hits[hitIdx].GetTime();
    
    A(hitIdx, 0) = (guess.X() - detpos.X())/(dist * fSoL);
    A(hitIdx, 1) = (guess.Y() - detpos.Y())/(dist * fSoL);
    A(hitIdx, 2) = (guess.Z() - detpos.Z())/(dist * fSoL);
    A(hitIdx, 3) = 1.;

    b(hitIdx) = -dist/fSoL + ti;
    
    mean_T += ti - dist/fSoL;
  }// end loop over hits
  
  mean_T = mean_T / hits.size();

  // Subtract off the mean emission time (T)
  for (uint hitIdx = 0; hitIdx < hits.size(); ++hitIdx) 
    b(hitIdx) -= mean_T;  

  // Tack on the regularization term
  A(hits.size(),     0) = fRegularizer;
  A(hits.size() + 1, 1) = fRegularizer;
  A(hits.size() + 2, 2) = fRegularizer;
  A(hits.size() + 3, 3) = fRegularizer;
  b(hits.size()    ) = 0;
  b(hits.size() + 1) = 0;
  b(hits.size() + 2) = 0;
  b(hits.size() + 3) = 0;


}


//------------------------------------------------------------------------------
void VertexLeastSquares::EvalAtGuessVertexMC(util::Matrix &A, util::Vector &b,
					   const Position &guess,
					   const std::vector<MCHit> &hits)
{
  // The function f(x) = 0
  // 0 = 1/c * sqrt( (X-xi)^2 + (Y-yi)^2 + (Z-zi)^2 ) + T - ti
  // A = Jacobian(guess), b = -f(guess)
  double mean_T = 0;
  for (uint hitIdx = 0; hitIdx < hits.size(); ++hitIdx) {
    Detector *det = fGeom->ChannelToDetector(hits[hitIdx].GetTubeId());
    Position detpos = det->GetDetectorPosition();
    double dist = (detpos - guess).Mag();    
    double ti = hits[hitIdx].GetTime();
    
    A(hitIdx, 0) = (guess.X() - detpos.X())/(dist * fSoL);
    A(hitIdx, 1) = (guess.Y() - detpos.Y())/(dist * fSoL);
    A(hitIdx, 2) = (guess.Z() - detpos.Z())/(dist * fSoL);
    A(hitIdx, 3) = 1.;

    b(hitIdx) = -dist/fSoL + ti;
    
    mean_T += ti - dist/fSoL;
  }// end loop over hits
  
  mean_T = mean_T / hits.size();

  // Subtract off the mean emission time (T)
  for (uint hitIdx = 0; hitIdx < hits.size(); ++hitIdx) 
    b(hitIdx) -= mean_T;

  // Tack on the regularization term
  A(hits.size(),     0) = fRegularizer;
  A(hits.size() + 1, 1) = fRegularizer;
  A(hits.size() + 2, 2) = fRegularizer;
  A(hits.size() + 3, 3) = fRegularizer;
  b(hits.size()    ) = 0;
  b(hits.size() + 1) = 0;
  b(hits.size() + 2) = 0;
  b(hits.size() + 3) = 0;


}

//------------------------------------------------------------------------------
void VertexLeastSquares::RunLoop()
{

  for (auto clusterpair : *fClusterMap) {    
    // Filter hits
    auto filt_hits = FilterHits(clusterpair.second);

    // We need at least 4 hits
    if (filt_hits.size() < 4) {
      std::cout << "Not enough hits. We only have: " << filt_hits.size() << std::endl;
      fVertexMap->emplace(clusterpair.first, Position(-99, -99, -99));
      fVertexStdevMap->emplace(clusterpair.first, -99);
      continue;
    }
    
    // Loop over vertex seeds
    std::vector<Position> seeds = GenerateVetices();

    std::vector<Position> bestVtxs;
    double bestStDev = 999999;
    int bestIdx = 0;
    int seednum = 0;
    for (auto guess : seeds) {
      Position lastGuess;
      int loop_num = 0;
      bool inTank = true;
      auto hits = filt_hits;
      while (true) {
      lastGuess = guess;
      inTank = true;
	
      // Set up Matrix and solution vector
      util::Matrix A(filt_hits.size() + 4, 4);
      util::Vector b(filt_hits.size() + 4);
      EvalAtGuessVertex(A, b, guess, hits);
      
      // Solve it and update the guess vertex
      util::Vector solution(4);
      util::least_squares(A, b, solution);

      // If the solution has nans it's because all rows of A evaluate to the same thing
      // revert to the last vertex and break
      if (std::isnan(solution(0)) || std::isnan(solution(1)) || std::isnan(solution(2))) {
        inTank = false;
        break;

      }

      // Otherwise, update the guess and carry on
      guess = lastGuess + Position(solution(0), solution(1), solution(2));
      
      // Exit if the new guess is close enough to the last one
      if ((guess - lastGuess).Mag() < fBreakDist) break;
      
      if (!fExternalSeeding) {
        // Exit if we're outside of the tank
        if(!fGeom->GetTankContained(guess)) {
          inTank = false;
          break;
        }
      }

      // Don't let the loop last forever
      ++loop_num;
      if (loop_num > fFittingSteps) break;
          }// end while loop

          // even tho we are seeding guesses outside the tank, still check if the guess is "reasonable"
          // the tank-only setting just calls GetTankContained(guess) - we instead must check if the guess is within our defined max radius and half height
          // "reasonable" distance away for a guess: radius, height +/- ExternalVertexMax(Radius)Height defined in config
          if (fExternalSeeding) {

            Position tankCenter = fGeom->GetTankCentre();
            double dx = guess.X() - tankCenter.X();
            double dz = guess.Z() - tankCenter.Z();
            double dy = guess.Y() - tankCenter.Y();
            double radial_dist = std::sqrt(dx*dx + dz*dz);
            double max_radial = fGeom->GetTankRadius() + fExternalVertexMaxRadius;
            double max_vertical = fGeom->GetTankHalfheight() + fExternalVertexMaxHeight;

            if (radial_dist > max_radial || std::abs(dy) > max_vertical) {
              continue;  // reject this guess
            }

          }

          // Skip if no tank contained vertex was found for this seed
          if (!inTank) continue;
          
          bestVtxs.push_back(guess);
          
          // Check the std deviation of b for the best guess from this seed
          util::Matrix A(filt_hits.size() + 4, 4);
          util::Vector b(filt_hits.size() + 4);
          EvalAtGuessVertex(A, b, bestVtxs.back(), filt_hits);
          double sum = 0;
          double sum_sq = 0;
          for (uint idx = 0; idx < b.size - 4; ++idx) {
            sum += b(idx);
            sum_sq += b(idx) * b(idx) ;
          }
          double mean = sum / b.size;
          double stdev = sum_sq / b.size - mean * mean;
          if (stdev < bestStDev) {
            bestStDev = stdev;
            bestIdx = bestVtxs.size()-1;
          }
          ++seednum;
    }// end loop over seed vertices

    // Save the vertex and stdev if we have one, otherwise default to -99s
    if (bestVtxs.size()) {
      fVertexMap->emplace(clusterpair.first, bestVtxs[bestIdx]);
      fVertexStdevMap->emplace(clusterpair.first, bestStDev);

    } else {
      fVertexMap->emplace(clusterpair.first, Position(-99, -99, -99));
      fVertexStdevMap->emplace(clusterpair.first, -99);
    }
      
  }// end loop over the clusters
}

//------------------------------------------------------------------------------
void VertexLeastSquares::RunLoopMC()
{
  for (auto clusterpair : *fClusterMapMC) {
    // Filter hits
    auto filt_hits = FilterHitsMC(clusterpair.second);

    //int PMTID = mcHitsIt.first;

    // We need at least 4 hits
    if (filt_hits.size() < 4) {
      std::cout << "Not enough hits. We only have: " << filt_hits.size() << std::endl;
      fVertexMap->emplace(clusterpair.first, Position(-99, -99, -99));
      fVertexStdevMap->emplace(clusterpair.first, -99);
      continue;
    }

    // Loop over vertex seeds
    std::vector<Position> seeds = GenerateVetices();

    std::vector<Position> bestVtxs;
    double bestStDev = 999999;
    int bestIdx = 0;
    int seednum = 0;
    for (auto guess : seeds) {
      Position lastGuess;
      int loop_num = 0;
      bool inTank = true;
      auto hits = filt_hits;
      while (true) {
      lastGuess = guess;
      inTank = true;
      
      // Set up Matrix and solution vector
      util::Matrix A(hits.size() + 4, 4);
      util::Vector b(hits.size() + 4);
      EvalAtGuessVertexMC(A, b, guess, hits);
      
      // Solve it and update the guess vertex
      util::Vector solution(4);
      util::least_squares(A, b, solution);

      // If the solution has nans it's because all rows of A evaluate to the same thing
      // revert to the last vertex and break
      if (std::isnan(solution(0)) || std::isnan(solution(1)) || std::isnan(solution(2))) {
        inTank = false;
        break;

      }

      // Otherwise, update the guess and carry on
      guess = lastGuess + Position(solution(0), solution(1), solution(2));
      
      // Exit if the new guess is close enough to the last one
      if ((guess - lastGuess).Mag() < fBreakDist) break;

      if (!fExternalSeeding) {
        // Exit if we're outside of the tank
        if(!fGeom->GetTankContained(guess)) {
          inTank = false;
          break;
        }
      }

      // Don't let the loop last forever
      ++loop_num;
      if (loop_num > fFittingSteps) break;
          }// end while loop

          // even tho we are seeding guesses outside the tank, still check if the guess is "reasonable"
          // the tank-only setting just calls GetTankContained(guess) - we instead must check if the guess is within our defined max radius and half height
          // "reasonable" distance away for a guess: radius, height +/- ExternalVertexMax(Radius)Height defined in config
          if (fExternalSeeding) {

            Position tankCenter = fGeom->GetTankCentre();
            double dx = guess.X() - tankCenter.X();
            double dz = guess.Z() - tankCenter.Z();
            double dy = guess.Y() - tankCenter.Y();
            double radial_dist = std::sqrt(dx*dx + dz*dz);
            double max_radial = fGeom->GetTankRadius() + fExternalVertexMaxRadius;
            double max_vertical = fGeom->GetTankHalfheight() + fExternalVertexMaxHeight;

            if (radial_dist > max_radial || std::abs(dy) > max_vertical) {
              continue;  // reject this guess
            }

          }

          // Skip if no tank contained vertex was found for this seed
          if (!inTank) continue;
          
          // Check the std deviation of b for the best guess from this seed
          bestVtxs.push_back(guess);
          util::Matrix A(filt_hits.size() + 4, 4);
          util::Vector b(filt_hits.size() + 4);
          EvalAtGuessVertexMC(A, b, bestVtxs.back(), filt_hits);
          double sum = 0;
          double sum_sq = 0;
          for (uint idx = 0; idx < b.size - 4; ++idx) {
            sum += b(idx);
            sum_sq += b(idx) * b(idx) ;
          }
          double mean = sum / b.size;
          double stdev = sum_sq / b.size - mean * mean;
          if (stdev < bestStDev) {
            bestStDev = stdev;
            bestIdx = bestVtxs.size()-1;
          }
          ++seednum;
    }// end loop over seed vertices

    // Save the vertex if we have one, otherwise default to -99s
    if (bestVtxs.size()) {      
      fVertexMap->emplace(clusterpair.first, bestVtxs[bestIdx]);
      fVertexStdevMap->emplace(clusterpair.first, -99);

    }
    else {
      std::cout << "No good vertex found!!" << std::endl;
      fVertexMap->emplace(clusterpair.first, Position(-99, -99, -99));
      fVertexStdevMap->emplace(clusterpair.first, -99);
    }
  }// end loop over the clusters
}

//------------------------------------------------------------------------------
std::vector<Hit> VertexLeastSquares::FilterHits(std::vector<Hit> hits)
{ 

  // we can do this in two ways:
  //   1. Filter the hits within a window to obtain a set of the most compatible hits with the first hit (fCompatibleHits = true)
  //   2. Filter hits by checking if secondary hits are within 10ns of the mean of the first 4 hits (fCompatibleHits = false)

  // either way the idea is to shave off hits that do not come from the *assumed* point interaction / cherenkov cone (remove hits that could have come from reflections / secondary interactions)

  // if enabled (fGoodTimes == True) we can also reject hits that have poor timing std from the laser analysis

  std::vector<Hit> filtered;
  std::vector<Hit> reliableHits;


  // first, (if applicable) apply a filter to remove unreliable PMTs
  if (fGoodTimes) {
    for (auto& h : hits) {
        int pmtid = h.GetTubeId();

        // look up timing std in map
        double timing_sigma = 1.0; // default
        auto it = ChannelKeyToTimingSigmaMap->find(pmtid);
        if (it != ChannelKeyToTimingSigmaMap->end()) {
            timing_sigma = it->second;
            logmessage = "VertexLeastSquares: PMTID " + std::to_string(pmtid) + ", timing std = " + std::to_string(timing_sigma) + "ns";
            Log(logmessage, v_debug, verbosity);
        } else {
            logmessage = "VertexLeastSquares: Didn't find timing std for PMTID " + std::to_string(pmtid) + ", using default = " + std::to_string(timing_sigma) + "ns";
            Log(logmessage, v_warning, verbosity);
        }

        // reject unreliable PMTs
        if (timing_sigma > fMinGoodTime) {
            logmessage = "VertexLeastSquares: Rejecting PMT " + std::to_string(pmtid) + ", with timing std = " + std::to_string(timing_sigma) + "ns (threshold " + std::to_string(fMinGoodTime) + "ns)";
            Log(logmessage, v_debug, verbosity);
            continue;
        }
        reliableHits.push_back(h);   // keep hit if PMT std is low enough
    }

    // bail if not enough reliable hits
    if (reliableHits.size() < 4) {
        Log("VertexLeastSquares: Not enough reliable hits (" 
            + std::to_string(reliableHits.size()) + ")", 
            v_warning, verbosity);
        return filtered;  // empty, RunLoop function will skip the reconstruction and assign -99 for the vertex 
    }
  }


  // pick which hits to use for the main filtering step
  std::vector<Hit>& hitsToFilter = fGoodTimes ? reliableHits : hits;


  if (fCompatibleHits) {  // option 1

    // Sort the hits by time
    std::sort(hitsToFilter.begin(), hitsToFilter.end(),
         [](const Hit &a, const Hit &b) { return a.GetTime() < b.GetTime(); });

    // Check for causally independent hits
    // time difference between the hits must be less than the relative time of flight between the PMTs
    std::map<int, std::set<int>> compatibleIds;
    int mostCompatibleIds = 0;
    int bestIdx = 0;
    for (uint idx = 0; idx < hitsToFilter.size(); ++idx) {
      Detector *det_i = fGeom->ChannelToDetector(hitsToFilter[idx].GetTubeId());
      Position pos_i = det_i->GetDetectorPosition();

      for (uint jdx = 0; jdx < hitsToFilter.size(); ++jdx) {
        Detector *det_j = fGeom->ChannelToDetector(hitsToFilter[jdx].GetTubeId());
        Position pos_j = det_j->GetDetectorPosition();
        
        double deltaT = abs(hitsToFilter[jdx].GetTime() - hitsToFilter[idx].GetTime());
        double tof = (pos_i - pos_j).Mag() / fSoL;
        
        if (deltaT <= tof) 
          compatibleIds[idx].insert(jdx);
      }

      if (compatibleIds[idx].size() > mostCompatibleIds) {
        mostCompatibleIds = compatibleIds[idx].size();
        bestIdx = idx;
      }
    }

    // Keep the set of most compatible ids if they're within 13 ns (light travel time across the inner structure) of the first hit
    int firstHitIdx = *compatibleIds[bestIdx].begin();
    double t0 = hitsToFilter[firstHitIdx].GetTime();
    for (auto idx : compatibleIds[bestIdx]) {
      if (abs(hitsToFilter[idx].GetTime() - t0) > 13) continue;
      filtered.push_back(hitsToFilter[idx]);
    }

  } else {   // option 2

    // Sort the hits by time
    std::sort(hitsToFilter.begin(), hitsToFilter.end(),
         [](const Hit &a, const Hit &b) { return a.GetTime() < b.GetTime(); });

    // Keep the first four and find the mean time
    double mean = 0;
    for (uint idx = 0; idx < 4; ++idx) {
      mean += hits[idx].GetTime();
      filtered.push_back(hitsToFilter[idx]);
    }
    mean = mean/4.;

    // Look through the rest of the hits,
    // check that they are within 10 ns (for reflections),
    // and check that they could come from the same point
    for (uint idx = 4; idx < hitsToFilter.size(); ++idx) {
      if ((hitsToFilter[idx].GetTime() - mean) > 10)
        continue;

      bool keep = true;
      for (uint jdx = 0; jdx < 4; ++jdx) {
        Detector *det_i = fGeom->ChannelToDetector(hitsToFilter[idx].GetTubeId());
        Position pos_i = det_i->GetDetectorPosition();

        Detector *det_j = fGeom->ChannelToDetector(filtered[jdx].GetTubeId());
        Position pos_j = det_j->GetDetectorPosition();

        double deltaT = abs(filtered[jdx].GetTime() - hitsToFilter[idx].GetTime());
        double tof = (pos_i - pos_j).Mag() / fSoL;

        if (deltaT > tof) {
          keep = false;
          break;
        }
      }

      if (!keep) continue;
      filtered.push_back(hitsToFilter[idx]);
    }

  }

  return filtered;

}


//------------------------------------------------------------------------------
std::vector<MCHit> VertexLeastSquares::FilterHitsMC(std::vector<MCHit> hits)
{ 

  std::vector<MCHit> filtered;
  std::vector<MCHit> reliableHits;

  // (if applicable) apply a filter to remove unreliable PMTs
  if (fGoodTimes) {
    for (auto& h : hits) {
        int pmtid = h.GetTubeId();

        // look up timing std in map
        double timing_sigma = 1.0; // default
        auto it = ChannelKeyToTimingSigmaMap->find(pmtid);
        if (it != ChannelKeyToTimingSigmaMap->end()) {
            timing_sigma = it->second;
            logmessage = "VertexLeastSquares: PMTID " + std::to_string(pmtid) + ", timing std = " + std::to_string(timing_sigma) + "ns";
            Log(logmessage, v_debug, verbosity);
        } else {
            logmessage = "VertexLeastSquares: Didn't find timing std for PMTID " + std::to_string(pmtid) + ", using default = " + std::to_string(timing_sigma) + "ns";
            Log(logmessage, v_warning, verbosity);
        }

        // reject unreliable PMTs
        if (timing_sigma > fMinGoodTime) {
            logmessage = "VertexLeastSquares: Rejecting PMT " + std::to_string(pmtid) + ", with timing std = " + std::to_string(timing_sigma) + "ns (threshold " + std::to_string(fMinGoodTime) + "ns)";
            Log(logmessage, v_debug, verbosity);
            continue;
        }
        reliableHits.push_back(h);   // keep hit if PMT std is low enough
    }

    // bail if not enough reliable hits
    if (reliableHits.size() < 4) {
        Log("VertexLeastSquares: Not enough reliable hits (" 
            + std::to_string(reliableHits.size()) + ")", 
            v_warning, verbosity);
        return filtered;  // empty, RunLoop function will skip the reconstruction and assign -99 for the vertex 
    }
  }

  // pick which hits to use for the main filtering step
  std::vector<MCHit>& hitsToFilter = fGoodTimes ? reliableHits : hits;

  if (fCompatibleHits) {  // option 1

    // Sort the hits by time
    std::sort(hitsToFilter.begin(), hitsToFilter.end(),
        [](const MCHit &a, const MCHit &b) {return a.GetTime() < b.GetTime();});

    // Check for causally independent hits
    // time difference between the hits must be less than the relative time of flight between the PMTs
    std::map<int, std::set<int>> compatibleIds;
    int mostCompatibleIds = 0;
    int bestIdx = 0;
    for (uint idx = 0; idx < hitsToFilter.size(); ++idx) {
      Detector *det_i = fGeom->ChannelToDetector(hitsToFilter[idx].GetTubeId());
      Position pos_i = det_i->GetDetectorPosition();

      for (uint jdx = 0; jdx < hitsToFilter.size(); ++jdx) {
        Detector *det_j = fGeom->ChannelToDetector(hitsToFilter[jdx].GetTubeId());
        Position pos_j = det_j->GetDetectorPosition();
        
        double deltaT = abs(hitsToFilter[jdx].GetTime() - hitsToFilter[idx].GetTime());
        double tof = (pos_i - pos_j).Mag() / fSoL;
        
        if (deltaT <= tof) 
          compatibleIds[idx].insert(jdx);
      }

      if (compatibleIds[idx].size() > mostCompatibleIds) {
        mostCompatibleIds = compatibleIds[idx].size();
        bestIdx = idx;
      }
    }

    // Keep the set of most compatible ids if they're within 13 ns (light travel time across the inner structure) of the first hit
    int firstHitIdx = *compatibleIds[bestIdx].begin();
    double t0 = hitsToFilter[firstHitIdx].GetTime();
    for (auto idx : compatibleIds[bestIdx]) {
      if (abs(hitsToFilter[idx].GetTime() - t0) > 13) continue;
      filtered.push_back(hitsToFilter[idx]);
    }
  
  } else {

    // Sort the hits by time
    std::sort(hitsToFilter.begin(), hitsToFilter.end(),
        [](const MCHit &a, const MCHit &b) {return a.GetTime() < b.GetTime();});

    // Keep the first four and find the mean time
    double mean = 0;
    for (uint idx = 0; idx < 4; ++idx) {
      mean += hitsToFilter[idx].GetTime();
      filtered.push_back(hitsToFilter[idx]);
    }
    mean = mean/4.;

    // Look through the rest of the hits,
    // check that they are within 10 ns (for reflections),
    // and check that they could come from the same point
    for (uint idx = 4; idx < hitsToFilter.size(); ++idx) {
      if ((hitsToFilter[idx].GetTime() - mean) > 10)
        continue;

      bool keep = true;
      for (uint jdx = 0; jdx < 4; ++jdx) {
        Detector *det_i = fGeom->ChannelToDetector(hitsToFilter[idx].GetTubeId());
        Position pos_i = det_i->GetDetectorPosition();

        Detector *det_j = fGeom->ChannelToDetector(filtered[jdx].GetTubeId());
        Position pos_j = det_j->GetDetectorPosition();

        double deltaT = abs(filtered[jdx].GetTime() - hitsToFilter[idx].GetTime());
        double tof = (pos_i - pos_j).Mag() / fSoL;

        if (deltaT > tof) {
          keep = false;
          break;
        }
      }

      if (!keep) continue;
      filtered.push_back(hitsToFilter[idx]);
    }
  
  }

  return filtered;
}
