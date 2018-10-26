#ifndef ANNIEGEOMETRY_HH
#define ANNIEGEOMETRY_HH

class ANNIEGeometry  {

 public:  
  
  typedef enum EGeoType {
   kUnknown  = -1,
   kCylinder = 0,
   kMailBox  = 1
  } GeoType_t; 

  typedef enum EGeoRegion {
   kTop     = 0,
   kSide    = 1,
   kBottom  = 2,
   kFront   = 10,
   kBack    = 12,
   kLeft    = 20,
   kRight   = 22
  } GeoRegion_t;

  //  const char* AsString(GeoConfiguration_t config);

  static ANNIEGeometry* Instance();
  static void BuildGeometry();
  static void PrintGeometry();
  static void WriteGeometry(const char* filename = "annie.geometry.root");
  static bool TouchGeometry();  
  static void Reset();

  void SetGeometry();
  void WriteToFile(const char* filename = "annie.geometry.root");

  // Lookup method for ANNIEGeometry
  // ===============================
  //  WCSimRootGeom* GetANNIEGeometry(){ return fWCSimRootGeom; }

  // Lookup method for Configuration
  // ===============================
  int GetGeoConfig()    { return fGeoConfig; }

  // Lookup methods for Geometry
  // ===========================
  int GetGeoType()      { return fGeoType; }

  bool IsCylinder()     { return fCylinder; }
  double GetCylRadius() { return fCylRadius; }
  double GetCylLength() { return fCylLength; }
  double GetCylFiducialRadius() { return fCylFiducialRadius; }
  double GetCylFiducialLength() { return fCylFiducialLength; }

  double GetArea()      { return fDetectorArea; }
  double GetVolume()    { return fDetectorVolume; }
  double GetFiducialVolume()    { return fDetectorFiducialVolume; }

  // Lookup methods for ss
  // =======================
  int GetNumPMTs()          { return fPMTs; }
  double GetPMTRadius()     { return fPMTRadius; }
  double GetPMTCoverage()   { return fPMTCoverage; }
  double GetPMTSeparation() { return fPMTSeparation; }

  int GetRegion( int tube );

  double GetX( int tube );
  double GetY( int tube );
  double GetZ( int tube );

  double GetNormX( int tube );
  double GetNormY( int tube );
  double GetNormZ( int tube );


  // Calculate Distance to Edge of Detector
  // ======================================
  bool InsideDetector(double x, double y, double z);
  bool InsideFiducialVolume(double x, double y, double z);
  bool InsideDetector(double vx, double vy, double vz,
                        double ex, double ey, double ez);

  double DistanceToEdge(double x, double y, double z);

  void ProjectToNearEdge(double x0, double y0, double z0, 
                         double px, double py, double pz, 
                         double& x, double& y, double&z, int& region);
  void ProjectToFarEdge(double x0, double y0, double z0, 
                        double px, double py, double pz, 
                        double& x, double& y, double&z, int& region);

  double ForwardProjectionToEdge(double x, double y, double z,
                                   double px, double py, double pz);
  double BackwardProjectionToEdge(double x, double y, double z,
                                    double px, double py, double pz);

  // Projection to Edge of Detector
  // ==============================
  // Input: A position (x0,y0,z0) and direction (px,py,pz)
  // Output: Projection on edge of detector (x,y,z,region)
  //  [Note: if (x0,y0,z0) is outside detector, then track 
  //    will pass through both edges of the detector. 
  //    If UseFarEdge=0, project forwards to nearest edge.
  //    If UseFarEdge=1, project forwards to furthest edge.]

  // Second attempt, using vectors to simplify algebra 
  void ProjectToEdge(bool useFarEdge,
                     double x0, double y0, double z0, 
                     double px, double py, double pz, 
                     double& x, double& y, double&z, int& region);
  
  // Coordinates for 'Roll-out' Display
  // ==================================
  // Input: A detector hit (x,y,z,region)
  // Output: Its position on the 'roll-out' display (u,v)
  void XYZtoUV(int region,
               double x, double y, double z,
               double& u, double& v);
 

  // Generic Geometry: Reconstruct a Circle from Three Hits
  // ======================================================
  // Input: Three hits (x,y,z) {0,1,2}
  // Output: Their common circle
  //  Centre=(rx,ry,rz), Normal=(nx,ny,nz), Radius=(r)
  static void FindCircle(double x0, double y0, double z0,
                         double x1, double y1, double z1,
                         double x2, double y2, double z2,
                         double& rx, double& ry, double& rz,
                         double& nx, double& ny, double& nz,
                         double& r);


  // Generic Geometry: Calculate Positions of Cherenkov Hits 
  // =======================================================
  // Input: A point along a track
  //  Position of point along track: (x0,y0,z0)
  //  Projected position along track [edge of detector]: (xp,yp,zp)
  //  Cherenkov Angle: angle_degrees
  //  Azimuthal Angle: omega_degrees
  // Output: A point on the Cherenkov Ring
  //  Direction to point on Cherenkov ring: (nx,ny,nz)
  //  Rotated position: (rx,ry,rz); distance moved: r
  //   [The rotated position (rx,ry,rz) is calculated by rotating
  //    the input position (xp,yp,zp) through the two angles.]

  // Second attempt, using ROOT TVector3 for rotations
  static void FindCircle(double xp, double yp, double zp,
                         double x0, double y0, double z0,
                         double angle_degrees, double omega_degrees,
                         double& rx, double& ry, double& rz,
                         double& nx, double& ny, double& nz,
                         double& r);

  // First attempt, calculating all rotations by hand
  static void FindCircleOld(double xp, double yp, double zp,
                            double x0, double y0, double z0,
                            double angle_degrees, double omega_degrees,
                            double& rx, double& ry, double& rz,
                            double& nx, double& ny, double& nz,
                            double& r);


  // Generic Geometry: Reconstruct a Vertex from Four Hits
  // =====================================================
  // Input: Four hits (x,y,z,t) {0,1,2,3}
  // Output: Their common vertex (vx,vy,vz,vt) {m,p}
  //  [Note: this involves solving a quadratic, with two solutions.
  //    In some cases, both solutions appear to be valid.]

  // Use ROOT TMatrix to invert the 4x4 matrix
  //  [Take advantage of ROOT algorithm, as analytic solution is slow.]
  static void FindVertex(double x0, double y0, double z0, double t0,
                         double x1, double y1, double z1, double t1,
                         double x2, double y2, double z2, double t2,
                         double x3, double y3, double z3, double t3,
                         double& vxm, double& vym, double& vzm, double& vtm,
                         double& vxp, double& vyp, double& vzp, double& vtp);

  // Cherenkov Geometry: Project PMT Hit back to Point of Cherenkov Emission
  // ========================================================================
  // Input: PMT Hit      (x0,y0,z0), 
  //        Track Vertex (vx,vy,vz), 
  //        Track End    (ex,ey,ez)
  // Output: Intersection (x,y,z)
  //         Photon Distance L
  // [Note: use Sine Rule for Cherenkov Geometry]
  static void DistanceToIntersectLine(double x0, double y0, double z0, 
                                      double vx, double vy, double vz,
                                      double ex, double ey, double ez, 
                                      double& x, double& y, double& z, double& L);
  static double DistanceToIntersectLine(double x0, double y0, double z0, 
                                          double sx, double sy, double sz,
                                          double ex, double ey, double ez, 
                                          double& x, double& y, double& z);
  static double DistanceToIntersectLine(double* pos, double* start, double* end, double* intersection);

 private:

  ANNIEGeometry();
  //  ANNIEGeometry(WCSimRootGeom* geom);
  ~ANNIEGeometry();

  double fScale;

  int fPMTs;
  int fLAPPDs;
  double* fPmtX;
  double* fPmtY;
  double* fPmtZ;
  double* fPmtNormX;
  double* fPmtNormY;
  double* fPmtNormZ;
  int* fPmtRegion;

  double fPMTRadius;
  double fPMTCoverage;
  double fPMTSeparation;
  double fPMTSurfaceArea;

  int fGeoType;
  int fGeoTypeInput;
  int fGeoConfig;

  bool fCylinder;
  double fCylRadius;
  double fCylLength;
  double fCylFiducialRadius;
  double fCylFiducialLength;
  double fXoffset;
  double fYoffset;
  double fZoffset;

  bool fMailBox;
  double fMailBoxX;
  double fMailBoxY;
  double fMailBoxZ; 
  double fMailBoxFiducialX;
  double fMailBoxFiducialY;
  double fMailBoxFiducialZ;

  double fDetectorArea;
  double fDetectorVolume;
  double fDetectorFiducialVolume;

  int fTube;
  int fRegion;
  double fXpos;
  double fYpos;
  double fZpos; 
  double fXnorm;
  double fYnorm;
  double fZnorm;
};

#endif







