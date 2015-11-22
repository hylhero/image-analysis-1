#include <iostream>     // for cout
#include "Graph2D.h"
#include "cs1037lib-time.h" // for basic timing/pausing
#include <algorithm>
using namespace std;

#define TERMINAL ( (arc *) 1 )		// COPY OF DEFINITION FROM maxflow.cpp
const int INFTY=100; // value of t-links corresponding to hard-constraints 


// function for printing error messages from the max-flow library
inline void mf_assert(char * msg) {cout << "Error from max-flow library: " << msg << endl; cin.ignore(); exit(1);}

// function for computing edge weights from intensity differences
int fn(const double dI, double sigma) {return (int) (5.0/(1.0+(dI*dI/(sigma*sigma))));}  // function used for setting "n-link costs" ("Sensitivity" sigme is a tuning parameter)

Graph2D::Graph2D()
: Graph<int,int,int>(0,0,mf_assert)
, m_nodeFlow() 
, m_seeds()
, m_box(0)
, m_labeling()
{}

void Graph2D::reset(Table2D<RGB> & im, double sigma, double r, bool eraze_seeds) 
{
	cout << "resetting segmentation" << endl;
	int x, y, wR, wD, height = im.getHeight(), width =  im.getWidth();
	Point p;
	m_nodeFlow.reset(width,height,0); // NOTE: width and height of m_nodeFlow table 
	                                  // are used in functions to_node() and to_Point(), 
	                                  // Thus, m_nodeFlow MUST BE RESET FIRST!!!!!!!!
	Graph<int,int,int>::reset();
	add_node(width*height);    // adding nodes
	for (y=0; y<(height-1); y++) for (x=0; x<(width-1); x++) { // adding edges (n-links)
		node_id n = to_node(x,y);
		wR=fn(dI(im[x][y],im[x+1][y]),sigma);   // function dI(...) is declared in Image2D.h
		wD=fn(dI(im[x][y],im[x  ][y+1]),sigma); // function fn(...) is declared at the top of this file
		add_edge( n, n+1,     wR, wR );
		add_edge( n, n+width, wD, wD );	
	}

	m_labeling.reset(width,height,NONE);
	if (eraze_seeds) { m_seeds.reset(width,height,NONE); m_box.clear();} // remove hard constraints and/or box
	else { // resets hard "constraints" (seeds)
		cout << "keeping seeds" << endl;
		for (p.y=0; p.y<height; p.y++) for (p.x=0; p.x<width; p.x++) { 
			if      (m_seeds[p]==OBJ) add_tweights( p, INFTY, 0   );
			else if (m_seeds[p]==BKG) add_tweights( p,   0, INFTY );
		}
	}
	m_r = r;
}

void Graph2D::boxReset(Table2D<RGB> & im)
{
	int x, y, wR, wD, height = im.getHeight(), width = im.getWidth();
	Point p;
	m_nodeFlow.reset(width, height, 0); // NOTE: width and height of m_nodeFlow table 
										// are used in functions to_node() and to_Point(), 
										// Thus, m_nodeFlow MUST BE RESET FIRST!!!!!!!!
	Graph<int, int, int>::reset();
	add_node(width*height);    // adding nodes
	for (y = 0; y<(height - 1); y++) for (x = 0; x<(width - 1); x++) { // adding edges (n-links)
		node_id n = to_node(x, y);
		wR = fn(dI(im[x][y], im[x + 1][y]), 8.0);   // function dI(...) is declared in Image2D.h
		wD = fn(dI(im[x][y], im[x][y + 1]), 8.0); // function fn(...) is declared at the top of this file
		add_edge(n, n + 1, wR, wR);
		add_edge(n, n + width, wD, wD);
	}

	m_labeling.reset(width, height, NONE);
	m_seeds.reset(width, height, NONE);
}

void Graph2D::graphReset(Table2D<RGB> & im)
{
	int x, y, wR, wD, height = im.getHeight(), width = im.getWidth();
	Point p;
	m_nodeFlow.reset(width, height, 0); // NOTE: width and height of m_nodeFlow table 
										// are used in functions to_node() and to_Point(), 
										// Thus, m_nodeFlow MUST BE RESET FIRST!!!!!!!!
	Graph<int, int, int>::reset();
	add_node(width*height);    // adding nodes
	for (y = 0; y<(height - 1); y++) for (x = 0; x<(width - 1); x++) { // adding edges (n-links)
		node_id n = to_node(x, y);
		wR = fn(dI(im[x][y], im[x + 1][y]), 8.0);   // function dI(...) is declared in Image2D.h
		wD = fn(dI(im[x][y], im[x][y + 1]), 8.0); // function fn(...) is declared at the top of this file
		add_edge(n, n + 1, wR, wR);
		add_edge(n, n + width, wD, wD);
	}

	m_labeling.reset(width, height, NONE);
	for (p.y = 0; p.y<height; p.y++) for (p.x = 0; p.x<width; p.x++) {
		if (m_seeds[p] == OBJ) add_tweights(p, INFTY, 0);
		else if (m_seeds[p] == BKG) add_tweights(p, 0, INFTY);
	}
}


// GUI calls this function when a user left- or right-clicks on an image pixel while in "BRUSH" mode
void Graph2D::addSeed(Point& p, Label seed_type) 
{
	if (!m_seeds.pointIn(p)) return;
	Label current_constraint = m_seeds[p];
	if (current_constraint==seed_type) return;

	if (current_constraint==NONE) {
		if (seed_type==OBJ) add_tweights( p, INFTY, 0   );
		else                add_tweights( p,   0, INFTY );
	}
	else if (current_constraint==OBJ) {
		if (seed_type==BKG) add_tweights( p, -INFTY, INFTY );
		else                add_tweights( p, -INFTY,   0   );
	}
	else if (current_constraint==BKG) {
		if (seed_type==OBJ) add_tweights( p, INFTY, -INFTY );
		else                add_tweights( p,   0,   -INFTY );
	}
	m_seeds[p]=seed_type;
}

// GUI calls this function when a user left- or right-clicks and drags on image pixels while in "BOX" mode
// the first click defines the top-left corner (tl) of the box, while the unclick point defines the bottom-right (br) corner.
void Graph2D::setBox(Point& tl, Point& br)
{
	
	while (m_box.size()<2) m_box.push_back(Point(0,0));
	m_box[0] = tl;
	m_box[1] = br;
	

}


inline Label Graph2D::what_label(Point& p)
{
	node_id i = to_node(p);
	if (nodes[i].parent) return (nodes[i].is_sink) ? BKG : OBJ;
	else                 return NONE;
}

void Graph2D::setLabeling()
{	
	int width  = m_labeling.getWidth();
	int height = m_labeling.getHeight();
	Point p;
	for (p.y=0; p.y<height; p.y++) for (p.x=0; p.x<width; p.x++) m_labeling[p]=what_label(p);
}

int Graph2D::compute_mincut(Table2D<RGB> & im, int mode, int brush_index)
{

	// Reset Graph Labels
	graphReset(im);

	// Initialize Variables
	int bins[2][512];
	int full_bins[2][512];
	double binp1, binp2, fullbinp1, fullbinp2;
	int bint1 = 0, bint2 = 0, fullbint1 = 0, fullbint2 = 0;
	int foreground_total = 0;
	int background_total = 0;
	Table2D<double> foreground_chance = im;
	Table2D<double> background_chance = im;
	Table2D<double> actual_chance = im;
	int height = im.getHeight(), width = im.getWidth();
	int rval, gval, bval;
	int x, y;
	bool flag = false;
	int temp = 0;



	/********************/
	//  BOX BRUSH MODE  //
	/********************/
	if (brush_index == 5) {
		Point p;
		boxReset(im);
		
		// Set area outside of box to be background
		for (y = 0; y < (height - 1); y++) for (x = 0; x < (width - 1); x++) {
			p = Point(x, y);
			if (x<m_box[0].x - 10 || x>m_box[1].x + 10 || y<m_box[0].y - 10 || y>m_box[1].y + 10) {
				addSeed(p, BKG);
			}
		}

		// Create an ellipse inside the box
		int xrad = (int)((m_box[1].x - m_box[0].x) / 4);
		int yrad = (int)((m_box[1].y - m_box[0].y) / 4);
		int xmid = (int)((m_box[1].x - m_box[0].x) / 2) + m_box[0].x;
		int ymid = (int)((m_box[1].y - m_box[0].y) / 2) + m_box[0].y;
		double xdist, ydist;

		// Set this ellipse to be foreground, and then it will automatically calculate the remaining area
		for (y = 0; y < (height - 1); y++) for (x = 0; x < (width - 1); x++) {
			p = Point(x, y);
			xdist = abs(x - xmid);
			ydist = abs(y - ymid);
			if ((xdist*xdist) / (xrad*xrad) + (ydist*ydist) / (yrad*yrad) < 1) {
				addSeed(p, OBJ);
			}
		}
	}


	/*******************/
	// INITIAL SET UP  //
	/*******************/

	// Initialize bins
	for (y = 0; y < 512; y++) for (x = 0; x < 2; x++) {
		bins[x][y] = 0;
		full_bins[x][y] = 0;
	}

	/******************/
	//  COLOR_F MODE  //
	/******************/
	if (mode == 1) {

		// Initialize probabilities
		for (y = 0; y < (height - 1); y++) for (x = 0; x < (width - 1); x++) {
			foreground_chance[x][y] = 0;
			background_chance[x][y] = 0;
		}

		// Set bin values based on seeds
		for (y = 0; y < (height - 1); y++) for (x = 0; x < (width - 1); x++) {
			if (m_seeds[x][y] != 0) {
				bins[m_seeds[x][y] - 1][(im[x][y].r + im[x][y].g * 8 + im[x][y].b * 8 * 8) % 8] += 1;
				m_seeds[x][y] == 1 ? foreground_total += 1 : background_total += 1;
			}
		}

		// Set probabilities based on bins
		for (y = 0; y < (height - 1); y++) for (x = 0; x < (width - 1); x++) {
			foreground_total == 0 ? foreground_chance[x][y] = -INFTY : foreground_chance[x][y] = -log((double)bins[0][(im[x][y].r + im[x][y].g * 8 + im[x][y].b * 8 * 8) % 8] / foreground_total);
			background_total == 0 ? background_chance[x][y] = -INFTY : background_chance[x][y] = -log((double)bins[1][(im[x][y].r + im[x][y].g * 8 + im[x][y].b * 8 * 8) % 8] / background_total);
		}

		// Set TWeights based on the probabilities that we calculated
		for (y = 0; y < (height - 1); y++) for (x = 0; x < (width - 1); x++) {
			Point p(x, y);
			add_tweights(p, m_r * foreground_chance[x][y], m_r * background_chance[x][y]);
		}
	}

	/******************/
	//  COLOR_E MODE  //
	/******************/
	if (mode == 2) {
		while (true) {

			// Iterate, reset flag
			cout << "Iterating";
			flag = true;

			// Reset bin totals
			bint1 = 0;
			bint2 = 0;

			// Count bin size
			for (y = 0; y < 512; y++) {
				bint1 += bins[0][y];
				bint2 += bins[1][y];
			}

			// Set probabilities based on bins
			for (y = 0; y < (height - 1); y++) for (x = 0; x < (width - 1); x++) {
				bint1 == 0 ? foreground_chance[x][y] = -INFTY : foreground_chance[x][y] = -log((double)bins[0][(im[x][y].r + im[x][y].g * 8 + im[x][y].b * 8 * 8) % 8] / bint1);
				bint2 == 0 ? background_chance[x][y] = -INFTY : background_chance[x][y] = -log((double)bins[1][(im[x][y].r + im[x][y].g * 8 + im[x][y].b * 8 * 8) % 8] / bint2);
			}

			// Set TWeights based on the probabilities that we calculated
			for (y = 0; y < (height - 1); y++) for (x = 0; x < (width - 1); x++) {
				Point p(x, y);
				add_tweights(p, m_r * foreground_chance[x][y], m_r * background_chance[x][y]);
			}

			// Calculate labels
			setLabeling();

			// Update full bins based on new labels
			for (y = 0; y < (height - 1); y++) for (x = 0; x < (width - 1); x++) { // adding edges (t-links)
				Point p(x, y);
				Label current_label = what_label(p);
				int current_int = current_label == OBJ ? 0 : 1;
				current_label == OBJ ? fullbint1 += 1 : fullbint2 += 1;
				full_bins[current_int][(im[x][y].r + im[x][y].g * 8 + im[x][y].b * 8 * 8) % 8] += 1;
			}

			// Clear labels
			m_labeling.reset(width, height, NONE);

			// Compare our bins to the 
			for (y = 0; y < 512; y++){
				binp1 = bint1 == 0 ? 0 : (double)(bins[0][y] / bint1);
				binp2 = bint2 == 0 ? 0 : (double)(bins[1][y] / bint2);
				fullbinp1 = fullbint1 == 0 ? 0 : (double)(full_bins[0][y] / fullbint1);
				fullbinp2 = fullbint2 == 0 ? 0 : (double)(full_bins[1][y] / fullbint2);

				// If it is not optimized, set the flag to false
				if (abs(fullbinp1 - binp1) > 0.05 || abs(fullbinp2 - binp2) > 0.05) flag = false;

				 // Update the bins to match the new iteration
				 bins[0][y] = full_bins[0][y];
				 bins[1][y] = full_bins[1][y];
			}

			// Break if it everything is optimized.
		    if (flag == true) break;
		}
	}

	int flow = Graph<int,int,int>::maxflow();
	setLabeling();

	// Hide seeds if using bounding box brush
	if (brush_index == 5) m_seeds.reset(width, height, NONE);
	return flow;
}

