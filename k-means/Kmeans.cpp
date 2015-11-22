#include <iostream>     // for cout
#include "cs1037lib-time.h" // for basic timing/pausing
#include <vector>
#include "Kmeans.h"
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <list>
#include <algorithm>
#include <functional>


using namespace std;


FeaturesGrid::FeaturesGrid()
: m_seeds()
, m_labeling()
, m_features()
, m_dim(0)
{}

void FeaturesGrid::reset(Table2D<RGB> & im, FEATURE_TYPE ft, float w) 
{
	cout << "resetting data... " << endl;
	int height = im.getHeight(), width =  im.getWidth();
	Point p;
	m_seeds.reset(width,height,NO_LABEL); 
	m_labeling.reset(width,height,NO_LABEL); // NOTE: width and height of m_labeling table 
	                                         // are used in functions to_index() and to_Point(), 

	m_dim=0;
	m_means.clear();
	m_features.clear();      // deleting old features
	m_features.resize(width*height); // creating a new array of feature vectors

	if (ft == color || ft == colorXY) {
		m_dim+=3;  // adding color features
		for (int y=0; y<height; y++) for (int x=0; x<width; x++) { 
			pix_id n = to_index(x,y);
			m_features[n].push_back((double) im[x][y].r);
			m_features[n].push_back((double) im[x][y].g);
			m_features[n].push_back((double) im[x][y].b);
		}
	}
	if (ft == colorXY) {
		m_dim+=2;  // adding location features (XY)
		for (int y=0; y<height; y++) for (int x=0; x<width; x++) { 
			pix_id n = to_index(x,y);
			m_features[n].push_back((double) w*x);
			m_features[n].push_back((double) w*y);
		}
	}

	cout << "done" << endl;
}

void FeaturesGrid::addFeature(Table2D<double> & im)
{
	m_dim++; // adding extra feature (increasing dimensionality of feature vectors
	int height = im.getHeight(), width =  im.getWidth();
	for (int y=0; y<height; y++) for (int x=0; x<width; x++) { 
		pix_id n = to_index(x,y);
		m_features[n].push_back(im[x][y]);
	}

}

// GUI calls this function when a user left- or right-clicks on an image pixel
void FeaturesGrid::addSeed(Point& p, Label seed_type) 
{
	if (!m_seeds.pointIn(p)) return;
	Label current_constraint = m_seeds[p];
	if (current_constraint==seed_type) return;
	m_seeds[p]=seed_type;
}

inline Label FeaturesGrid::what_label(Point& p)
{
	return m_labeling[p];
}



int FeaturesGrid::Kmeans(unsigned k, int mode, double w)
{
	init_means(k);
	int iter = 0; // iteration counter
	std::cout << "computing k-means for data..." << endl;

	// Initialize Variables
	double min_distance;
	int min_seed;
	double total_distance;
	bool finished_iterating = false;
	bool rgbxy = false;
	vector<int> label_count(k, 0);
	vector<vector<double>> adjusted_means(k, vector<double>(5, 0));
	double rval, gval, bval;

	// Set Mode
	if (mode == 1) {
		rgbxy = true;
	}
	else {
		w = 0;
	}


	// Begin iteration loop
	while (true) {
		iter += 1;
		cout << "Iterating...\n";

		// Scan through every pixel
		for (size_t y = 0; y < m_labeling.getHeight(); y++) {
			for (size_t x = 0; x < m_labeling.getWidth(); x++) {

				// Get RGB of Current Pixel
				pix_id n = to_index(x, y);
				rval = m_features[n][0];
				gval = m_features[n][1];
				bval = m_features[n][2];
				min_distance = 999;

				// Scan through every K cluster
				for (size_t z = 0; z < k; z++) {

					// If it is closest to a K Cluster, set that as the 'Min Seed'
					total_distance = (abs(rval - m_means[z][0]) + abs(gval - m_means[z][1]) + abs(bval - m_means[z][2] + x*w + y*w));
					if (total_distance < min_distance) {
						min_distance = total_distance;
						min_seed = z;
					}
				}

				// Label the points cluster after every K value has been checked
				m_labeling[x][y] = min_seed;

				// Add these values to the overall cluster statistics, so we can optimize the cluster at the end of the iteration
				label_count[min_seed] += 1;
				adjusted_means[min_seed][0] += rval;
				adjusted_means[min_seed][1] += gval;
				adjusted_means[min_seed][2] += bval;
				adjusted_means[min_seed][3] += w*x;
				adjusted_means[min_seed][4] += w*y;

			}
		}

		// Perform a check to see if we are finished
		finished_iterating = true;
		for (size_t z = 0; z < k; z++) {

			// Update the cluster means
			if (label_count[z] != 0) {
				adjusted_means[z][0] /= label_count[z];
				adjusted_means[z][1] /= label_count[z];
				adjusted_means[z][2] /= label_count[z];
				adjusted_means[z][3] /= label_count[z];
				adjusted_means[z][4] /= label_count[z];
			}
			label_count[z] = 0;

			// If there is a difference between the old and new means, we must iterate again
			// Otherwise, we are done
			if (int(adjusted_means[z][0]) != int(m_means[z][0]) || int(adjusted_means[z][1]) != int(m_means[z][1]) || int(adjusted_means[z][2]) != int(m_means[z][2]) || int(adjusted_means[z][3]) != int(m_means[z][3]) || int(adjusted_means[z][4]) != int(m_means[z][4])) finished_iterating = false;
			m_means[z] = adjusted_means[z];
		}

		if (finished_iterating) break;
	}

	return iter;
}


void FeaturesGrid::init_means(unsigned k) {
	m_means.clear();
	m_means.resize(k);
	
	for (Label ln = 0; ln<k; ln++) for (unsigned i=0; i<m_dim; i++) m_means[ln].push_back(0.0); // setting to zero all mean vector components

	// WRITE YOUR CODE FOR INITIALIZING MEANS (RANDOMLY OR FROM SEEDS IF ANY)
	size_t counter = 0;
	int w = 0;

	// If there are some seeds, use them first
	for (size_t y = 0; y < m_seeds.getHeight(); y++) for (size_t x = 0; x < m_seeds.getWidth(); x++) {
		if (m_seeds[x][y] != NO_LABEL) {
			pix_id n = to_index(x, y);
			m_means[counter].resize(5);
			m_means[counter][0] = m_features[n][0];
			m_means[counter][1] = m_features[n][1];
			m_means[counter][2] = m_features[n][2];
			m_means[counter][3] = x*w;
			m_means[counter][4] = y*w;
			counter += 1;
			if (counter == k) break;
		}
	}
	

	// Initialize all remaining seeds randomly
	for (counter = 0; counter < k; counter++) {
		m_means[counter].resize(5);
		m_means[counter][0] = rand() * 769 % 256; // multiply random by prime, and modulo for color (0-255)
		m_means[counter][1] = rand() * 769 % 256;
		m_means[counter][2] = rand() * 769 % 256;
		m_means[counter][3] = ((rand() * 769)*w) % m_seeds.getWidth();
		m_means[counter][4] = ((rand() * 769)*w) % m_seeds.getHeight();
	}
}