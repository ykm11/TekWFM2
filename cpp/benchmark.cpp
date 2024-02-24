#include <iostream>
#include <fstream>
#include <chrono>
#include <gmpxx.h>

#include "wfm_head.hpp"

#define N 100

void bench1() {
    const char* filename = "./Tek000_ch1_20230519130048037.wfm";
    std::vector<std::vector <double> > vec_waves_double;
    //std::vector<std::vector <mpf_class> > vec_waves_double;
    wfm_header meta_header;

    auto start_time = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < N; i++) {
        read_wfm_scaled(vec_waves_double, meta_header, filename);
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    std::cout << "Execution Time: " << (duration.count() / double(N) / 1.0e6) << " seconds" << std::endl;
}

void bench2() {
    const char* filename = "./Tek000_ch1_20230519130048037.wfm";
    char check_sum[8];
    //size_t samples, available_samples;
    char buffer[838]; // Meta header
    wfm_header meta_header;
    std::vector<std::vector <double> > vec_waves_double;

    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "The given file could not be read." << std::endl;
        return;
    }
    file.read(buffer, 838); // read header
    if (file.gcount() != 838) {
        std::cerr << "Reading a header failed." << std::endl;
        return;
    }
    decode_header(meta_header, buffer);

    char *p = (char *)malloc((24 + 30) * (meta_header.frames - 1));
    file.read(p, (24 + 30) * (meta_header.frames - 1));

    char *full_wave = (char *)malloc(2 * meta_header.available_values * meta_header.frames);
    file.read(full_wave, 2 * meta_header.available_values * meta_header.frames);
    file.read(check_sum, 8);
    file.close();

    // invoked once
    /*
    vec_waves_double.resize(meta_header.frames);
    for (size_t i = 0; i < meta_header.frames; i++) {
        vec_waves_double[i].resize(meta_header.samples);
    }
    */
    vec_waves_double.resize(meta_header.frames, 
            std::vector<double>(meta_header.samples, 0.0));

    auto start_time = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < N; i++) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "The given file could not be read." << std::endl;
            return;
        }
        file.read(buffer, 838); // read header
        if (file.gcount() != 838) {
            std::cerr << "Reading a header failed." << std::endl;
            return;
        }
        decode_header(meta_header, buffer);

        //char *p = (char *)malloc((24 + 30) * (meta_header.frames - 1));
        file.read(p, (24 + 30) * (meta_header.frames - 1));
        //char *full_wave = (char *)malloc(2 * meta_header.available_values * meta_header.frames);
        file.read(full_wave, 2 * meta_header.available_values * meta_header.frames);
        file.read(check_sum, 8);
        file.close();

        read_wfm_scaled_fast(vec_waves_double, meta_header, full_wave);
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    std::cout << "Execution Time: " << (duration.count() / double(N) / 1.0e6) << " seconds" << std::endl;
    free(p);
    free(full_wave);
}



int main() {
   bench1();
   bench2();
}
