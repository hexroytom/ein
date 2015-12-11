#include "gaussian_map.h"
#include "ein_words.h"
#include "ein.h"
#include "qtgui/einwindow.h"

void checkProb(string label, double prob) {
  if (prob > 1.0) {
    cout << label << " greater than 1: " << prob << endl;
  }
  if (prob < 0.0) {
    cout << label << " less than 0: " << prob << endl;
  }
}
void _GaussianMapChannel::zero() {
  counts = 0.0;
  squaredcounts = 0.0;
  mu = 0.0;
  sigmasquared = 0.0;
  samples = 0.0;
}
void _GaussianMapCell::zero() {
  red.zero();
  green.zero();
  blue.zero();
  z.zero();
}

double normal_pdf(double mu, double sigma, double x) {
  return 1 / (sigma * sqrt(2 * M_PI)) * exp(-pow(x - mu, 2) / (2 * sigma * sigma));
}

double computeInnerProduct(GaussianMapChannel & channel1, GaussianMapChannel & channel2, double * channel_term_out) {
  double normalizer = 0.0;				       
  double newsigmasquared = 1 / (1 / channel1.sigmasquared + 1 / channel2.sigmasquared); 
  double newmu = newsigmasquared * (channel1.mu / channel1.sigmasquared + channel2.mu / channel2.sigmasquared); 
  for (int i = 0; i < 256; i++) {
    normalizer += normal_pdf(newmu, sqrt(newsigmasquared), i);
  }
  double channel_term = exp(  -0.5*pow(channel1.mu-channel2.mu, 2)/( channel1.sigmasquared + channel2.sigmasquared )  ) / sqrt( 2.0*M_PI*(channel1.sigmasquared + channel2.sigmasquared) ); 
  channel_term = channel_term * normalizer * 0.5;
  *(channel_term_out) = channel_term;
}

double _GaussianMapCell::innerProduct(_GaussianMapCell * other, double * rterm_out, double * gterm_out, double * bterm_out) {
  double rterm, bterm, gterm; 
  computeInnerProduct(red, other->red, &rterm);
  computeInnerProduct(green, other->green, &gterm);
  computeInnerProduct(blue, other->blue, &bterm);
  return rterm * bterm * gterm;
}



void _GaussianMapCell::writeToFileStorage(FileStorage& fsvO) const {
  fsvO << "{:";
  fsvO << "rcounts" << red.counts;
  fsvO << "gcounts" << green.counts;
  fsvO << "bcounts" << blue.counts;
  fsvO << "rsquaredcounts" << red.squaredcounts;
  fsvO << "gsquaredcounts" << green.squaredcounts;
  fsvO << "bsquaredcounts" << blue.squaredcounts;
  fsvO << "rmu" << red.mu;
  fsvO << "gmu" << green.mu;
  fsvO << "bmu" << blue.mu;
  fsvO << "rsigmasquared" << red.sigmasquared;
  fsvO << "gsigmasquared" << green.sigmasquared;
  fsvO << "bsigmasquared" << blue.sigmasquared;
  fsvO << "rsamples" << red.samples;
  fsvO << "gsamples" << green.samples;
  fsvO << "bsamples" << blue.samples;
  fsvO << "zcounts" << z.counts;
  fsvO << "zsquaredcounts" << z.squaredcounts;
  fsvO << "zmu" << z.mu;
  fsvO << "zsigmasquared" << z.sigmasquared;
  fsvO << "zsamples" << z.samples;
  fsvO << "}";
}

void _GaussianMapCell::readFromFileNodeIterator(FileNodeIterator& it) {
  red.counts         =  (double)(*it)["rcounts"];         
  green.counts         =  (double)(*it)["gcounts"];         
  blue.counts         =  (double)(*it)["bcounts"];         
  red.squaredcounts  =  (double)(*it)["rsquaredcounts"];                
  green.squaredcounts  =  (double)(*it)["gsquaredcounts"];                
  blue.squaredcounts  =  (double)(*it)["bsquaredcounts"];                
  red.mu             =  (double)(*it)["rmu"];     
  green.mu             =  (double)(*it)["gmu"];     
  blue.mu             =  (double)(*it)["bmu"];     
  red.sigmasquared   =  (double)(*it)["rsigmasquared"];               
  green.sigmasquared   =  (double)(*it)["gsigmasquared"];               
  blue.sigmasquared   =  (double)(*it)["bsigmasquared"];               
  red.samples      =  (double)(*it)["rsamples"];            
  green.samples      =  (double)(*it)["gsamples"];            
  blue.samples      =  (double)(*it)["bsamples"];            
  z.counts         =  (double)(*it)["zcounts"];         
  z.squaredcounts  =  (double)(*it)["zsquaredcounts"];                
  z.mu             =  (double)(*it)["zmu"];     
  z.sigmasquared   =  (double)(*it)["zsigmasquared"];               
  z.samples        =  (double)(*it)["zsamples"];          
}

void _GaussianMapCell::readFromFileNode(FileNode& it) {
  red.counts         =  (double)(it)["rcounts"];         
  green.counts         =  (double)(it)["gcounts"];         
  blue.counts         =  (double)(it)["bcounts"];         
  red.squaredcounts  =  (double)(it)["rsquaredcounts"];                
  green.squaredcounts  =  (double)(it)["gsquaredcounts"];                
  blue.squaredcounts  =  (double)(it)["bsquaredcounts"];                
  red.mu             =  (double)(it)["rmu"];     
  green.mu             =  (double)(it)["gmu"];     
  blue.mu             =  (double)(it)["bmu"];     
  red.sigmasquared   =  (double)(it)["rsigmasquared"];               
  green.sigmasquared   =  (double)(it)["gsigmasquared"];               
  blue.sigmasquared   =  (double)(it)["bsigmasquared"];               
  red.samples      =  (double)(it)["rgbsamples"];
  green.samples      =  (double)(it)["rgbsamples"];            
  blue.samples      =  (double)(it)["rgbsamples"];            
  z.counts         =  (double)(it)["zcounts"];         
  z.squaredcounts  =  (double)(it)["zsquaredcounts"];                
  z.mu             =  (double)(it)["zmu"];     
  z.sigmasquared   =  (double)(it)["zsigmasquared"];               
  z.samples        =  (double)(it)["zsamples"];          
}

void _GaussianMapCell::newObservation(Vec3b vec) {
  red.counts += vec[2];
  green.counts += vec[1];
  blue.counts += vec[0];
  red.squaredcounts += pow(vec[2], 2);
  green.squaredcounts += pow(vec[1], 2);
  blue.squaredcounts += pow(vec[0], 2);
  red.samples += 1;
  green.samples += 1;
  blue.samples += 1;
}

void GaussianMap::reallocate() {
  if (width <= 0 || height <= 0) {
    cout << "GaussianMap area error: tried to allocate width, height: " << width << " " << height << endl;
  } else if ((width % 2 == 0) || (height % 2 == 0)) {
    cout << "GaussianMap parity error: tried to allocate width, height: " << width << " " << height << endl;
  } else {
    if (cells == NULL) {
      cells = new GaussianMapCell[width*height];
    } else {
      delete cells;
      cells = new GaussianMapCell[width*height];
    }
  }
}

GaussianMap::GaussianMap(int w, int h, double cw) {
  width = w;
  height = h;
  x_center_cell = (width-1)/2;
  y_center_cell = (height-1)/2;
  cell_width = cw;
  cells = NULL;
  reallocate();
}

GaussianMap::~GaussianMap() {
  if (cells != NULL) {
    delete cells;
    cells = NULL;
  } else {
  }
}

int GaussianMap::safeAt(int x, int y) {
  return ( (cells != NULL) && (x >= 0) && (x < width) && (y >= 0) && (y < height) );
}

GaussianMapCell *GaussianMap::refAtCell(int x, int y) {
  return (cells + x + width*y);
}

GaussianMapCell GaussianMap::valAtCell(int x, int y) {
  return *(cells + x + width*y);
}

#define BILIN_MAPCELL(field) \
  toReturn.field = wx0*wy0*refAtCell(x0,y0)->field + wx1*wy0*refAtCell(x1,y0)->field + wx0*wy1*refAtCell(x0,y1)->field + wx1*wy1*refAtCell(x1,y1)->field;

GaussianMapCell GaussianMap::bilinValAtCell(double _x, double _y) {

  double x = min( max(0.0, _x), double(width-1));
  double y = min( max(0.0, _y), double(height-1));

  // -2 makes for appropriate behavior on the upper boundary
  double x0 = std::min( std::max(0.0, floor(x)), double(width-2));
  double x1 = x0+1;

  double y0 = std::min( std::max(0.0, floor(y)), double(height-2));
  double y1 = y0+1;

  double wx0 = x1-x;
  double wx1 = x-x0;

  double wy0 = y1-y;
  double wy1 = y-y0;

  GaussianMapCell toReturn;

  // wx0*wy0*val(x0,y0) + wx1*wy0*val(x1,y0) + wx1*wy1*val(x1,y1) + wx0*wy1*val(x0,y1) ~sub 1 for vals =~
  // wx0*(wy0+wy1) + wx1*(wy0 + wy1) = wx0 + wx1 = 1

  BILIN_MAPCELL(red.counts);
  BILIN_MAPCELL(green.counts);
  BILIN_MAPCELL(blue.counts);
  BILIN_MAPCELL(red.squaredcounts);
  BILIN_MAPCELL(green.squaredcounts);
  BILIN_MAPCELL(blue.squaredcounts);
  BILIN_MAPCELL(red.mu);
  BILIN_MAPCELL(green.mu);
  BILIN_MAPCELL(blue.mu);
  BILIN_MAPCELL(red.sigmasquared);
  BILIN_MAPCELL(green.sigmasquared);
  BILIN_MAPCELL(blue.sigmasquared);
  BILIN_MAPCELL(red.samples);
  BILIN_MAPCELL(green.samples);
  BILIN_MAPCELL(blue.samples);
  BILIN_MAPCELL(z.counts);
  BILIN_MAPCELL(z.squaredcounts);
  BILIN_MAPCELL(z.mu);
  BILIN_MAPCELL(z.sigmasquared);
  BILIN_MAPCELL(z.samples);

  return toReturn;
}

GaussianMapCell GaussianMap::bilinValAtMeters(double x, double y) {
  int cell_x;
  int cell_y;
  metersToCell(x, y, &cell_x, &cell_y);
  return bilinValAtCell(cell_x, cell_y);
}

void GaussianMap::metersToCell(double xm, double ym, int * xc, int * yc) {
  (*xc) = round(xm / cell_width) + x_center_cell;
  (*yc) = round(ym / cell_width) + y_center_cell;
} 

void GaussianMap::cellToMeters(int xc, int yc, double * xm, double * ym) {
  (*xm) = (xc - x_center_cell) * cell_width; 
  (*ym) = (yc - y_center_cell) * cell_width; 
} 

void GaussianMap::writeToFileStorage(FileStorage& fsvO) {
  fsvO << "{";
  {
    fsvO << "width" << width;
    fsvO << "height" << height;
    fsvO << "x_center_cell" << x_center_cell;
    fsvO << "y_center_cell" << y_center_cell;
    fsvO << "cell_width" << cell_width;

    fsvO << "cells" << "[" ;

    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
	refAtCell(x,y)->writeToFileStorage(fsvO);
      }
    }
    fsvO << "]";
  }
  fsvO << "}";
}

void GaussianMap::saveToFile(string filename) {
  FileStorage fsvO;
  cout << "GaussianMap::saveToFile writing: " << filename << endl;
  fsvO.open(filename, FileStorage::WRITE);
  fsvO << "GaussianMap";
  writeToFileStorage(fsvO);
  fsvO.release();
}

void GaussianMap::readFromFileNodeIterator(FileNodeIterator& it) {
  {
    (*it)["width"] >> width;
    (*it)["height"] >> height;
    (*it)["x_center_cell"] >> x_center_cell;
    (*it)["y_center_cell"] >> y_center_cell;
    (*it)["cell_width"] >> cell_width;
  }
  reallocate();
  {
    FileNode bnode = (*it)["cells"];

    int numLoadedCells= 0;
    FileNodeIterator itc = bnode.begin(), itc_end = bnode.end();
    for ( ; (itc != itc_end) && (numLoadedCells < width*height); itc++, numLoadedCells++) {
      cells[numLoadedCells].readFromFileNodeIterator(itc);
    }

    if (numLoadedCells != width*height) {
      ROS_ERROR_STREAM("Error, GaussianMap loaded " << numLoadedCells << " but expected " << width*height << endl);
    } else {
      cout << "successfully loaded " << numLoadedCells << " GaussianMapCells." << endl;
    }
  }
}

void GaussianMap::readFromFileNode(FileNode& it) {
  {
    it["width"] >> width;
    it["height"] >> height;
    it["x_center_cell"] >> x_center_cell;
    it["y_center_cell"] >> y_center_cell;
    it["cell_width"] >> cell_width;
  }
  reallocate();
  {
    FileNode bnode = it["cells"];

    int numLoadedCells= 0;
    FileNodeIterator itc = bnode.begin(), itc_end = bnode.end();
    for ( ; (itc != itc_end) && (numLoadedCells < width*height); itc++, numLoadedCells++) {
      cells[numLoadedCells].readFromFileNodeIterator(itc);
    }

    if (numLoadedCells != width*height) {
      ROS_ERROR_STREAM("Error, GaussianMap loaded " << numLoadedCells << " but expected " << width*height << endl);
    } else {
      cout << "successfully loaded " << numLoadedCells << " GaussianMapCells." << endl;
    }
  }
}

void GaussianMap::loadFromFile(string filename) {
  FileStorage fsvI;
  cout << "GaussianMap::loadFromFile reading: " << filename<< " ..." << endl;
  fsvI.open(filename, FileStorage::READ);

  FileNode anode = fsvI["GaussianMap"];
  readFromFileNode(anode);

  cout << "done." << endl;
}


void GaussianMapChannel::recalculateMusAndSigmas() {
  double safe_samples = max(samples, 1.0);
  mu = counts / samples;			
  sigmasquared = (squaredcounts / samples) - (mu * mu); 
}

void GaussianMap::recalculateMusAndSigmas() {
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {

      refAtCell(x, y)->red.recalculateMusAndSigmas();
      refAtCell(x, y)->green.recalculateMusAndSigmas();
      refAtCell(x, y)->blue.recalculateMusAndSigmas();
      refAtCell(x, y)->z.recalculateMusAndSigmas();
      
    }
  }
}


void GaussianMap::rgbMuToMat(Mat& out) {
  //out = Mat(height, width, CV_64FC3);
  Mat big = Mat(height, width, CV_8UC3);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {

      if (refAtCell(x, y)->red.samples > 0) {
	big.at<Vec3b>(y,x)[0] = uchar(refAtCell(x,y)->blue.mu);
	big.at<Vec3b>(y,x)[1] = uchar(refAtCell(x,y)->green.mu);
	big.at<Vec3b>(y,x)[2] = uchar(refAtCell(x,y)->red.mu);
      } else {
	big.at<Vec3b>(y,x)[0] = 0;
	big.at<Vec3b>(y,x)[1] = 128;
	big.at<Vec3b>(y,x)[2] = 128;
      }
    }
  }

  cv::resize(big, out, cv::Size(301, 301), 2, 2);

}

void GaussianMap::rgbDiscrepancyMuToMat(Mat& out) {
  //out = Mat(height, width, CV_64FC3);
  out = Mat(height, width, CV_8UC3);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      out.at<Vec3b>(y,x)[0] = refAtCell(x,y)->blue.mu;
      out.at<Vec3b>(y,x)[1] = refAtCell(x,y)->green.mu;
      out.at<Vec3b>(y,x)[2] = refAtCell(x,y)->red.mu;
    }
  }
}


void GaussianMap::rgbSigmaSquaredToMat(Mat& out) {
  out = Mat(height, width, CV_64FC3);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      out.at<Vec3d>(y,x)[0] = refAtCell(x,y)->blue.sigmasquared;
      out.at<Vec3d>(y,x)[1] = refAtCell(x,y)->green.sigmasquared;
      out.at<Vec3d>(y,x)[2] = refAtCell(x,y)->red.sigmasquared;
    }
  }
}

void GaussianMap::rgbCountsToMat(Mat& out) {
  out = Mat(height, width, CV_64FC3);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      out.at<Vec3d>(y,x)[0] = refAtCell(x,y)->blue.counts;
      out.at<Vec3d>(y,x)[1] = refAtCell(x,y)->green.counts;
      out.at<Vec3d>(y,x)[2] = refAtCell(x,y)->red.counts;
    }
  }
}

void GaussianMap::zMuToMat(Mat& out) {
  out = Mat(height, width, CV_64F);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      out.at<double>(y,x) = refAtCell(x,y)->z.mu;
    }
  }
}

void GaussianMap::zSigmaSquaredToMat(Mat& out) {
  out = Mat(height, width, CV_64F);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      out.at<double>(y,x) = refAtCell(x,y)->z.sigmasquared;
    }
  }
}

void GaussianMap::zCountsToMat(Mat& out) {
  out = Mat(height, width, CV_64F);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      out.at<double>(y,x) = refAtCell(x,y)->z.counts;
    }
  }
}


// extract a bounding box at specified corners top left (x1,y1) bottom right (x2,y2)
shared_ptr<GaussianMap> GaussianMap::copyBox(int _x1, int _y1, int _x2, int _y2) {
  int x1 = min(_x1, _x2);
  int x2 = max(_x1, _x2);
  int y1 = min(_y1, _y2);
  int y2 = max(_y1, _y2);

  x1 = min( max(0,x1), width-1);
  x2 = min( max(0,x2), width-1);
  y1 = min( max(0,y1), height-1);
  y2 = min( max(0,y2), height-1);

  shared_ptr<GaussianMap> toReturn = std::make_shared<GaussianMap>(x2-x1, y2-y1, cell_width);
  for (int y = y1; y <= y2; y++) {
    for (int x = x1; x <= x2; x++) {
      *(toReturn->refAtCell(x-x1,y-y1)) = *(refAtCell(x,y));
    }
  }
  
  return toReturn;
}

void GaussianMap::zeroBox(int _x1, int _y1, int _x2, int _y2) {
  int x1 = min( max(0,_x1), width-1);
  int x2 = min( max(0,_x2), width-1);
  int y1 = min( max(0,_y1), height-1);
  int y2 = min( max(0,_y2), height-1);
  
  for (int y = y1; y <= y2; y++) {
    for (int x = x1; x <= x2; x++) {
      refAtCell(x,y)->zero();
    }
  }
}

void GaussianMap::zero() {
  zeroBox(0,0,width-1,height-1);
}



void SceneObject::writeToFileStorage(FileStorage& fsvO) {
// XXX
}

void SceneObject::readFromFileNodeIterator(FileNodeIterator& it) {
// XXX
}

void SceneObject::readFromFileNode(FileNode& it) {
// XXX
}




Scene::Scene(shared_ptr<MachineState> _ms, int w, int h, double cw) {
  ms = _ms;
  width = w;
  height = h;
  x_center_cell = (width-1)/2;
  y_center_cell = (height-1)/2;
  cell_width = cw;
  background_pose = eePose::zero();
  score = 0;
  predicted_objects.resize(0);
  reallocate();
}

void Scene::reallocate() {
  background_map = make_shared<GaussianMap>(width, height, cell_width);
  predicted_map = make_shared<GaussianMap>(width, height, cell_width);
  predicted_segmentation = Mat(height, width, CV_64F);
  observed_map = make_shared<GaussianMap>(width, height, cell_width);
  discrepancy = make_shared<GaussianMap>(width, height, cell_width);

  discrepancy_magnitude = Mat(height, width, CV_64F);
  discrepancy_density = Mat(height, width, CV_64F);
}

void Scene::composePredictedMap() {
  // choose the argMAP distribution
  //   assign that color to the predicted map
  //   assign the source to the segmentation
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      *(predicted_map->refAtCell(x,y)) = *(background_map->refAtCell(x,y));
    }
  }
  // XXX  currently only incorporates background
}

void Scene::measureDiscrepancy() {
  // close to kl-divergence
  // for now this only does rgb
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      if ((predicted_map->refAtCell(x,y)->red.samples > 0) && (observed_map->refAtCell(x,y)->red.samples > 0)) {

/*
	double rmu_diff = (predicted_map->refAtCell(x,y)->rmu - observed_map->refAtCell(x,y)->rmu);
	double gmu_diff = (predicted_map->refAtCell(x,y)->gmu - observed_map->refAtCell(x,y)->gmu);
	double bmu_diff = (predicted_map->refAtCell(x,y)->bmu - observed_map->refAtCell(x,y)->bmu);
	
	double oos = 1.0 / predicted_map->refAtCell(x,y)->rgbsamples;

	double rd_observed = max(oos, observed_map->refAtCell(x,y)->rsigmasquared);
	double rd_predicted = max(oos, predicted_map->refAtCell(x,y)->rsigmasquared);
	double rvar_quot = (predicted_map->refAtCell(x,y)->rsigmasquared / rd_observed) + 
			   (observed_map->refAtCell(x,y)->rsigmasquared / rd_predicted);

	double gd_observed = max(oos, observed_map->refAtCell(x,y)->gsigmasquared);
	double gd_predicted = max(oos, predicted_map->refAtCell(x,y)->gsigmasquared);
	double gvar_quot = (predicted_map->refAtCell(x,y)->gsigmasquared / gd_observed) + 
			   (observed_map->refAtCell(x,y)->gsigmasquared / gd_predicted);

	double bd_observed = max(oos, observed_map->refAtCell(x,y)->bsigmasquared);
	double bd_predicted = max(oos, predicted_map->refAtCell(x,y)->bsigmasquared);
	double bvar_quot = (predicted_map->refAtCell(x,y)->bsigmasquared / bd_observed) + 
			   (observed_map->refAtCell(x,y)->bsigmasquared / bd_predicted);

*/
	double p_pgb = 0.5;

	double rmu_diff = 0.0;
	double gmu_diff = 0.0;
	double bmu_diff = 0.0;

	double total_discrepancy = predicted_map->refAtCell(x,y)->innerProduct(observed_map->refAtCell(x,y), &rmu_diff, &gmu_diff, &bmu_diff);
	checkProb("rmu_diff", rmu_diff);
	checkProb("bmu_diff", bmu_diff);
	checkProb("gmu_diff", gmu_diff);

	discrepancy->refAtCell(x,y)->red.samples = observed_map->refAtCell(x,y)->red.samples;
	discrepancy->refAtCell(x,y)->green.samples = observed_map->refAtCell(x,y)->green.samples;
	discrepancy->refAtCell(x,y)->blue.samples = observed_map->refAtCell(x,y)->blue.samples;
	discrepancy->refAtCell(x,y)->red.mu = rmu_diff; //(1.0 - rmu_diff * 256.0 * p_pgb);
	discrepancy->refAtCell(x,y)->green.mu = gmu_diff; //(1.0 - gmu_diff * 256.0 * p_pgb);
	discrepancy->refAtCell(x,y)->blue.mu = bmu_diff; //(1.0 - bmu_diff * 256.0 * p_pgb);
	discrepancy->refAtCell(x,y)->red.sigmasquared = 0;
	discrepancy->refAtCell(x,y)->green.sigmasquared = 0;
	discrepancy->refAtCell(x,y)->blue.sigmasquared = 0;
  
	discrepancy_magnitude.at<double>(y,x) = (1.0 - total_discrepancy * pow(256.0,3.0) * p_pgb);

	checkProb("total_discrepancy", total_discrepancy);
	checkProb("rmu", discrepancy->refAtCell(x,y)->red.mu);
	checkProb("bmu", discrepancy->refAtCell(x,y)->blue.mu);
	checkProb("gmu", discrepancy->refAtCell(x,y)->green.mu);
	//rmu_diff*rmu_diff + gmu_diff*gmu_diff + bmu_diff*bmu_diff ;
	//+ rvar_quot + gvar_quot + bvar_quot;

	discrepancy_density.at<double>(y,x) = discrepancy_magnitude.at<double>(y,x);
	//sqrt(discrepancy_magnitude.at<double>(y,x) / 3.0) / 255.0; 

      } else {
	discrepancy->refAtCell(x,y)->zero();
  
	discrepancy_magnitude.at<double>(y,x) = 0.0;
	discrepancy_density.at<double>(y,x) = 0.0;
      }
    }
  }
}

double Scene::assignScore() {
  score = measureScoreRegion(0,0,width-1,height-1);
  return score;
}

double Scene::measureScoreRegion(int _x1, int _y1, int _x2, int _y2) {
  int x1 = min(_x1, _x2);
  int x2 = max(_x1, _x2);
  int y1 = min(_y1, _y2);
  int y2 = max(_y1, _y2);

  x1 = min( max(0,x1), width-1);
  x2 = min( max(0,x2), width-1);
  y1 = min( max(0,y1), height-1);
  y2 = min( max(0,y2), height-1);

  double totalScore = 0.0;
  for (int y = y1; y <= y2; y++) {
    for (int x = x1; x <= x2; x++) {
      if (discrepancy->refAtCell(x,y)->red.samples > 0) {
	totalScore += discrepancy_magnitude.at<double>(y,x); 
      } else {
      }
    }
  }
  
  return totalScore;
}

// XXX 
void Scene::proposeRegion() {
}
// XXX 
void Scene::proposeObject() {
}

// XXX 
void Scene::tryToAddObjectToScene() {
}

// XXX 
void Scene::removeObjectFromPredictedMap() {
}

// XXX 
void Scene::addObjectToPredictedMap() {
}

// XXX 
void Scene::removeSpaceObjects() {
}

// XXX 
void Scene::addSpaceObjects() {
}

// XXX 
void Scene::reregisterBackground() {
}

// XXX 
void Scene::reregisterObject(int i) {
}

void Scene::writeToFileStorage(FileStorage& fsvO) {
// XXX 
}

void Scene::readFromFileNodeIterator(FileNodeIterator& it) {
// XXX 
}

void Scene::readFromFileNode(FileNode& it) {
// XXX
}

void Scene::saveToFile(string filename) {
// XXX
}

void Scene::loadFromFile(string filename) {
// XXX
}





// XXX word
void TransitionTable::setPrescene(shared_ptr<Scene> s) {
}

// XXX word
void TransitionTable::setPostscene(shared_ptr<Scene> s) {
}

// XXX word
void TransitionTable::setPerformedAction() {
}

// XXX word
void TransitionTable::recordTransitionSceneObject() {
}
// XXX word
void setStateLabelsFromClassLabels() {
}

// XXX word
// XXX word setter
// XXX word getter
//   the word should take a compound word of words, which may be compound words, 
//   and stores their reprs as strings; this makes it easier to serialize
void TransitionTable::setActions(std::vector<string> * actions) {
}

// XXX word
// XXX word setter
// XXX word getter
void TransitionTable::setActionProbabilities(std::vector<double> * actions) {
}

// XXX 
void TransitionTable::initCounts() {
}

void TransitionTable::writeToFileStorage(FileStorage& fsvO) {
// XXX 
}

void TransitionTable::readFromFileNodeIterator(FileNodeIterator& it) {
// XXX 
}

void TransitionTable::readFromFileNode(FileNode& it) {
// XXX
}

void TransitionTable::saveToFile(string filename) {
// XXX
}

void TransitionTable::loadFromFile(string filename) {
// XXX
}




/*

// XXX Scene Viewer
// Observed    Predicted    Segmentation
// Background  Discrepancy  Discrepancy Density
// update with words


// XXX accessors for sceneObjects: number, pose...

// XXX word to add objects to scene until evidence is accounted for

// XXX words to save and load current background and current object maps
// XXX word to remove object from scene and zero region around it
// XXX word to zero entire observedMap
// XXX word to search clearance area for the cell with lowest number of counts, push its eePose




// XXX sample action word and put string word onto stack
// XXX evaluate string word 
// XXX use string word to set current action 

// XXX calls to init class maps
//  XXX add calls to writeClassToFolder

// XXX word to update observed_map based on current wrist camera image
// XXX map the table by using image stream buffer 
//   XXX  word to update observed_map  based on stream buffer, images and IR
// XXX when training from crops, render from the stream buffer into the observed_map then crop out

// XXX word to fly on a path over all the sceneObjects, streaming images
// XXX word to densely explore map streaming images and IR 
// XXX init checker MACRO for Scene and TransitionTable 
*/





namespace ein_words {

WORD(SceneSaveBackgroundMap)
virtual void execute(std::shared_ptr<MachineState> ms) {
  string message;
  GET_STRING_ARG(ms, message);

  stringstream ss;
  ss << ms->config.data_directory + "/maps/" + message + ".yml";
  stringstream ss_dir;
  ss_dir << ms->config.data_directory + "/maps/";
  mkdir(ss_dir.str().c_str(), 0777);

  ms->config.scene->background_map->saveToFile(ss.str());

}
END_WORD
REGISTER_WORD(SceneSaveBackgroundMap)

WORD(SceneLoadBackgroundMap)
virtual void execute(std::shared_ptr<MachineState> ms) {
  string message;
  GET_STRING_ARG(ms, message);

  stringstream ss;
  ss << ms->config.data_directory + "/maps/" + message + ".yml";
  stringstream ss_dir;
  ss_dir << ms->config.data_directory + "/maps/";
  mkdir(ss_dir.str().c_str(), 0777);

  ms->config.scene->background_map->loadFromFile(ss.str());

  Mat backgroundImage;
  ms->config.scene->background_map->rgbMuToMat(backgroundImage);
  Mat rgb = backgroundImage.clone();  
  cvtColor(backgroundImage, rgb, CV_YCrCb2BGR);
  ms->config.backgroundWindow->updateImage(rgb);

}
END_WORD
REGISTER_WORD(SceneLoadBackgroundMap)

WORD(SceneSaveObservedMap)
virtual void execute(std::shared_ptr<MachineState> ms) {
  string message;
  GET_STRING_ARG(ms, message);

  stringstream ss;
  ss << ms->config.data_directory + "/maps/" + message + ".yml";
  stringstream ss_dir;
  ss_dir << ms->config.data_directory + "/maps/";
  mkdir(ss_dir.str().c_str(), 0777);

  ms->config.scene->observed_map->saveToFile(ss.str());

}
END_WORD
REGISTER_WORD(SceneSaveObservedMap)

WORD(SceneLoadObservedMap)
virtual void execute(std::shared_ptr<MachineState> ms) {
  string message;
  GET_STRING_ARG(ms, message);

  stringstream ss;
  ss << ms->config.data_directory + "/maps/" + message + ".yml";
  stringstream ss_dir;
  ss_dir << ms->config.data_directory + "/maps/";
  mkdir(ss_dir.str().c_str(), 0777);

  ms->config.scene->observed_map->loadFromFile(ss.str());

}
END_WORD
REGISTER_WORD(SceneLoadObservedMap)

WORD(SceneSaveDiscrepancyMap)
virtual void execute(std::shared_ptr<MachineState> ms) {
  string message;
  GET_STRING_ARG(ms, message);

  stringstream ss;
  ss << ms->config.data_directory + "/maps/" + message + ".yml";
  stringstream ss_dir;
  ss_dir << ms->config.data_directory + "/maps/";
  mkdir(ss_dir.str().c_str(), 0777);

  ms->config.scene->discrepancy->saveToFile(ss.str());
}
END_WORD
REGISTER_WORD(SceneSaveDiscrepancyMap)

WORD(SceneLoadDiscrepancyMap)
virtual void execute(std::shared_ptr<MachineState> ms) {
  string message;
  GET_STRING_ARG(ms, message);

  stringstream ss;
  ss << ms->config.data_directory + "/maps/" + message + ".yml";
  stringstream ss_dir;
  ss_dir << ms->config.data_directory + "/maps/";
  mkdir(ss_dir.str().c_str(), 0777);

  ms->config.scene->discrepancy->loadFromFile(ss.str());
}
END_WORD
REGISTER_WORD(SceneLoadDiscrepancyMap)

WORD(SceneClearPredictedObjects)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.scene->predicted_objects.resize(0);
}
END_WORD
REGISTER_WORD(SceneClearPredictedObjects)

WORD(SceneInit)
virtual void execute(std::shared_ptr<MachineState> ms) {
  double p_cell_width = 0.005; //0.01;
  int p_width = 601;
  int p_height = 601;
  ms->config.scene = make_shared<Scene>(ms, p_width, p_height, p_cell_width);
}
END_WORD
REGISTER_WORD(SceneInit)

WORD(SceneClearObservedMap)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.scene->observed_map->zero();
  Mat observedImage;
  ms->config.scene->observed_map->rgbMuToMat(observedImage);
  //observedImage = observedImage / 255.0;
  Mat rgb = observedImage.clone();  
  cvtColor(observedImage, rgb, CV_YCrCb2BGR);
  ms->config.observedWindow->updateImage(rgb);
}
END_WORD
REGISTER_WORD(SceneClearObservedMap)

WORD(SceneSetBackgroundFromObserved)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.scene->background_map->zero();
}
END_WORD
REGISTER_WORD(SceneSetBackgroundFromObserved)

WORD(SceneUpdateObservedFromSnout)
virtual void execute(std::shared_ptr<MachineState> ms) {
//XXX 
}
END_WORD
REGISTER_WORD(SceneUpdateObservedFromSnout)

WORD(SceneUpdateObservedFromWrist)
virtual void execute(std::shared_ptr<MachineState> ms) {
  Mat wristViewYCbCr = ms->config.wristCamImage.clone();  
  cvtColor(ms->config.wristCamImage, wristViewYCbCr, CV_BGR2YCrCb);
    
  for (int px = ms->config.grayTop.x+ms->config.mapGrayBoxPixelSkirtCols; px < ms->config.grayBot.x-ms->config.mapGrayBoxPixelSkirtCols; px++) {
    for (int py = ms->config.grayTop.y+ms->config.mapGrayBoxPixelSkirtRows; py < ms->config.grayBot.y-ms->config.mapGrayBoxPixelSkirtRows; py++) {
      if (isInGripperMask(ms, px, py)) {
	continue;
      }
      double x, y;
      double z = ms->config.trueEEPose.position.z + ms->config.currentTableZ;
      pixelToGlobal(ms, px, py, z, &x, &y);
      int i, j;
      ms->config.scene->observed_map->metersToCell(x, y, &i, &j);
      GaussianMapCell * cell = ms->config.scene->observed_map->refAtCell(i, j);
      Vec3b pixel = wristViewYCbCr.at<Vec3b>(py, px);
      cell->newObservation(pixel);
    }
  }  
  ms->config.scene->observed_map->recalculateMusAndSigmas();
  Mat image;
  ms->config.scene->observed_map->rgbMuToMat(image);
  Mat rgb = image.clone();  
  cvtColor(image, rgb, CV_YCrCb2BGR);
  ms->config.observedWindow->updateImage(rgb);
}
END_WORD
REGISTER_WORD(SceneUpdateObservedFromWrist)

WORD(SceneComposePredictedMap)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.scene->composePredictedMap();

  Mat image;
  ms->config.scene->predicted_map->rgbMuToMat(image);
  Mat rgb = image.clone();  
  cvtColor(image, rgb, CV_YCrCb2BGR);
  ms->config.predictedWindow->updateImage(rgb);
}
END_WORD
REGISTER_WORD(SceneComposePredictedMap)

WORD(SceneUpdateDiscrepancy)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->config.scene->measureDiscrepancy();
  Mat image;
  ms->config.scene->discrepancy->rgbDiscrepancyMuToMat(image);
  image = image * 255.0;

  ms->config.discrepancyWindow->updateImage(image);
}
END_WORD
REGISTER_WORD(SceneUpdateDiscrepancy)

WORD(SceneGrabCenterCropAsClass)
virtual void execute(std::shared_ptr<MachineState> ms) {
//XXX 
  //shared_ptr<GaussianMap> camera_frame = copyBox(ul_cell_x, ul_cell_y, br_cell_x, br_cell_y);
}
END_WORD
REGISTER_WORD(SceneGrabCenterCropAsClass)

WORD(SceneDensityFromDiscrepancy)
virtual void execute(std::shared_ptr<MachineState> ms) {
// this enables denisty based models to use the new channel
// XXX this does not take the rotation of the wrist into account
  Size sz = ms->config.wristCamImage.size();
  int imW = sz.width;
  int imH = sz.height;

  for (int y = 0; y < imH; y++) {
    for (int x = 0; x < imW; x++) {
      double meter_x = 0;
      double meter_y = 0;
      double zToUse = ms->config.currentEEPose.pz+ms->config.currentTableZ;
      pixelToGlobal(ms, x, y, zToUse, &meter_x, &meter_y, ms->config.currentEEPose);
      int cell_x = 0;
      int cell_y = 0;
      ms->config.scene->discrepancy->metersToCell(meter_x, meter_y, &cell_x, &cell_y);
      ms->config.density[y*imW+x] = ms->config.scene->discrepancy_density.at<double>(cell_y,cell_x);
    }
  }
  drawDensity(ms, 1);
}
END_WORD
REGISTER_WORD(SceneDensityFromDiscrepancy)

/* 
WORD()
virtual void execute(std::shared_ptr<MachineState> ms) {
}
END_WORD
REGISTER_WORD()

WORD(TransitionTableInit)
virtual void execute(std::shared_ptr<MachineState> ms) {
// zero it out
}
END_WORD
REGISTER_WORD(TransitionTableInit)

WORD(TransitionTableCount)
virtual void execute(std::shared_ptr<MachineState> ms) {
}
END_WORD
REGISTER_WORD(TransitionTableCount)


WORD(PlanWithTransitionTable)
virtual void execute(std::shared_ptr<MachineState> ms) {

// takes a desired state and outputs the action that best takes the prescene to that state

}
END_WORD
REGISTER_WORD(PlanWithTransitionTable)

*/

}




