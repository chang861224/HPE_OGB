#ifndef WARP_H
#define WARP_H

#include "../../smore/src/proNet.h"

/*****
 * WARP
 * **************************************************************/

class WARP {

    public:
        
        WARP();
        ~WARP();
        
        proNet pnet;

        // parameters
        int dim;                // representation dimensions
        vector< vector<double> > w_vertex;

        // data function
        void LoadEdgeList(string, bool);
        void SaveWeights(string);
        
        // model function
        void Init(int, string, bool);
        void Train(int, int, double, double, int);

};


#endif
