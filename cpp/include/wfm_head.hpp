#pragma once
#include <iostream>

struct wfm_header {
    uint16_t byte_order;
    uint8_t version[9];

    uint32_t imp_dim_count;
    uint32_t exp_dim_count;
    uint32_t record_type;
    uint32_t exp_dim_1_type;
    uint32_t time_base_1;
    uint32_t fastframe;
    uint32_t frames;
    short summary_frame;
    int curve_offset;

    // scale
    double vscale, voffset;
    double tscale, tstart;

    double tfrac, tdatefrac;
    uint32_t tdate;

    uint32_t dsize, dpost, readbytes, allbytes;

    int code;
    char bps;
    uint32_t samples, available_values, pre_values, post_values;

};

void dump_header(const wfm_header&);
int decode_header(wfm_header&, const char[838]);
int read_wfm(std::vector< std::vector<short> >&, wfm_header&, const char *);
int read_wfm_scaled(std::vector< std::vector<double> >&, wfm_header&, const char *);

