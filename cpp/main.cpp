#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <string.h>

#include "wfm_head.hpp"

int main() {
    const char* filename = "./Tek000_ch1_20230519130048037.wfm";
    std::vector<std::vector <short> > vec_waves;
    std::vector<std::vector <double> > vec_waves_double;
    wfm_header meta_header;

    //read_wfm(vec_waves, meta_header, filename);
    read_wfm_scaled(vec_waves_double, meta_header, filename);

    for (size_t i = 0; i < meta_header.samples; i++) { 
        //printf("%d,", vec_waves[0][i]);
        printf("%f,", vec_waves_double[1][i]);
    }

    return 0;
}
