#include "HOPREC.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <omp.h>

HOPREC::HOPREC() {
    char method[15] = "no_degrees";
    pnet.SetNegativeMethod(method);
}
HOPREC::~HOPREC() {
}

void HOPREC::LoadEdgeList(string filename, bool undirect) {
    pnet.LoadEdgeList(filename, undirect);
}

void HOPREC::LoadFieldMeta(string filename) {
    pnet.LoadFieldMeta(filename);
}

void HOPREC::SaveWeights(string model_name){
    
    cout << "Save Model:" << endl;
    ofstream model(model_name);
    if (model)
    {
        model << pnet.MAX_vid << " " << dim << endl;
        for (long vid=0; vid!=pnet.MAX_vid; vid++)
        {
            model << pnet.vertex_hash.keys[vid];
            for (int d=0; d<dim; ++d)
                model << " " << w_vertex[vid][d];
            model << endl;
        }
        cout << "\tSave to <" << model_name << ">" << endl;
    }
    else
    {
        cout << "\tfail to open file" << endl;
    }
}

void HOPREC::Init(int dim, string embed_path, bool directed) {
    vector< vector<double> > vec;
    vec.resize(2927963);
    ifstream infile;

    cout << "Pre-Trained Embedding:\t< " << embed_path << " >" << endl;
    infile.open(embed_path);
    
    for(long i = 0 ; i < 2927963 ; i++){
        vec[i].resize(128);

        for(int j = 0 ; j < 128 ; j++){
            infile >> vec[i][j];
        }
    }

    infile.close();
   
    this->dim = dim;
    cout << "Model Setting:" << endl;
    cout << "\tdimension:\t\t" << dim << endl;

    w_vertex.resize(pnet.MAX_vid);
    w_context.resize(pnet.MAX_vid);

    if(directed == true){
        for (long vid = 0 ; vid < pnet.MAX_vid ; ++vid){
            w_vertex[vid].resize(dim);

            char* vertex = strtok(pnet.vertex_hash.keys[vid], "ST");
            //cout << pnet.vertex_hash.keys[vid] << " " << vertex << endl;

            for (int d = 0 ; d < dim ; ++d){
                //w_vertex[vid][d] = (rand()/(double)RAND_MAX - 0.5) / dim;
                w_vertex[vid][d] = vec[stol(vertex)][d];
            }

            w_context[vid].resize(dim);
            
            for (int d = 0 ; d < dim ; ++d)
                w_context[vid][d] = 0.0;
        }
    }
    else{
        for (long vid = 0 ; vid < pnet.MAX_vid ; ++vid){
            w_vertex[vid].resize(dim);

            for (int d = 0 ; d < dim ; ++d){
                //w_vertex[vid][d] = (rand()/(double)RAND_MAX - 0.5) / dim;
                w_vertex[vid][d] = vec[stol(pnet.vertex_hash.keys[vid])][d];
            }

            w_context[vid].resize(dim);
            
            for (int d=0; d<dim;++d)
                w_context[vid][d] = 0.0;
        }
    }
}


void HOPREC::Train(int sample_times, int walk_steps, double alpha, int workers){
    
    omp_set_num_threads(workers);

    cout << "Model:" << endl;
    cout << "\t[HOP REC]" << endl;

    cout << "Learning Parameters:" << endl;
    cout << "\tsample_times:\t\t" << sample_times << endl;
    cout << "\talpha:\t\t\t" << alpha << endl;
    cout << "\twalk steps:\t\t" << walk_steps << endl;
    cout << "\tworkers:\t\t" << workers << endl;

    cout << "Start Training:" << endl;

    unsigned long long total_sample_times = (unsigned long long)sample_times*1000000;
    double alpha_min = alpha * 0.0001;
    double alpha_last;
    
    unsigned long long current_sample = 0;
    unsigned long long jobs = total_sample_times/workers;

    #pragma omp parallel for
    for (int worker=0; worker<workers; ++worker)
    {
        
        long vid, cid1, cid2, cid3, cid4, cid5, cid6, nid;
        unsigned long long count = 0;
        double _alpha = alpha;
        double margin;
        
        while (count<jobs)
        {          

            vid = pnet.SourceSample();
            while (pnet.field[vid].fields[0]!=0)
                vid = pnet.SourceSample();
            cid1 = pnet.TargetSample(vid);
           
            margin = 1.0;            
            for (int w=1; w<=walk_steps; w++)
            {
                if (w!=1)
                {
                    cid1 = pnet.TargetSample(cid1);
                    cid1 = pnet.TargetSample(cid1);
                }
                nid = pnet.NegativeSample();
                while (pnet.field[nid].fields[0]!=pnet.field[cid1].fields[0])
                    nid = pnet.NegativeSample();
                pnet.UpdateFBPRPair(w_vertex, w_vertex, vid, cid1, nid, dim, _alpha/w, margin/w);
            }

            count ++;
            if (count % MONITOR == 0)
            {
                _alpha = alpha* ( 1.0 - (double)(current_sample)/total_sample_times );
                current_sample += MONITOR;
                if (_alpha < alpha_min) _alpha = alpha_min;
                alpha_last = _alpha;
                printf("\tAlpha: %.6f\tProgress: %.3f %%%c", _alpha, (double)(current_sample)/total_sample_times * 100, 13);
                fflush(stdout);
            }
        }

    }
    printf("\tAlpha: %.6f\tProgress: 100.00 %%\n", alpha_last);

}

