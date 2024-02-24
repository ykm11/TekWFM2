#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <gmpxx.h>

#include "wfm_head.hpp"

void test() {
    const char* filename = "./Tek000_ch1_20230519130048037.wfm";
    char check_sum[8];
    char buffer[838]; // Meta header
    wfm_header meta_header;
    std::vector<std::vector <double> > vec_waves_double;

    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        puts("The given file could not be read.");
        return;
    }
    file.read(buffer, 838); // read header
    if ( file.gcount() != 838) {
        puts("Reading a header failed.");
        return;
    }
    decode_header(meta_header, buffer);

    //std::shared_ptr<char> p(new char((24 + 30) * (meta_header.frames - 1)));
    char *p = (char *)malloc((24 + 30) * (meta_header.frames - 1));
    file.read(p, (24 + 30) * (meta_header.frames - 1));

    char *full_wave_buf = (char *)malloc(2 * meta_header.available_values * meta_header.frames);
    file.read(full_wave_buf, 2 * meta_header.available_values * meta_header.frames);
    file.read(check_sum, 8);
    file.close();

    // invoke once and re-use it
    vec_waves_double.resize(meta_header.frames, 
            std::vector<double>(meta_header.samples, 0.0));

    read_wfm_scaled_fast(vec_waves_double, meta_header, full_wave_buf);

    for (size_t i = 0; i < meta_header.samples; i++) { 
        printf("%f,", vec_waves_double[0][i]);
    }
}


int main() {
    const char* filename = "./Tek000_ch1_20230519130048037.wfm";
    std::vector<std::vector <short> > vec_waves;
    std::vector<std::vector <double> > vec_waves_double;
    wfm_header meta_header;

    test();

    //read_wfm(vec_waves, meta_header, filename);
    //read_wfm_scaled(vec_waves_double, meta_header, filename);
    return 0;
}
