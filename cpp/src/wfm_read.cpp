#include "wfm_head.hpp"
#include <fstream>
#include <string>
#include <vector>

void dump_header(const wfm_header &meta) {
    printf("byte order %04x\n", meta.byte_order);
    printf("wfm version %8s\n", meta.version);
    printf("imp_dim_count %x\n", meta.imp_dim_count);
    printf("exp_dim_count %x\n", meta.exp_dim_count);
    printf("record_type %x\n", meta.record_type);
    printf("exp_dim_1_type %x\n", meta.exp_dim_1_type);
    printf("time_base_1 %x\n", meta.time_base_1);
    printf("fastfram %x\n", meta.fastframe);
    printf("frames %d\n", meta.frames);
    printf("summary_frame %d\n", meta.summary_frame);
    printf("curve_offset %d\n", meta.curve_offset);

    printf("vslace %.16f\n", meta.vscale);
    printf("voffset %.16f\n", meta.voffset);

    printf("tstart %.16f\n", meta.tstart);
    printf("tslace %.16f\n", meta.tscale);
    printf("tfrac %.16f\n", meta.tfrac);
    printf("tdatefrac %.16f\n", meta.tdatefrac);
    printf("tdate %d\n", meta.tdate);

    printf("tdsize %d\n", meta.dsize);
    printf("code %d\n", meta.code);
    printf("bps %d\n", meta.bps);

    printf("samples %d\n", meta.samples);
    printf("available_values %d\n", meta.available_values);
    printf("pre_values %d\n", meta.pre_values);
    printf("post_values %d\n", meta.post_values);
}


int decode_header(wfm_header &meta, const char header_buf[838]) {
    size_t v1_offset = 0;

    // Endian
    meta.byte_order = *(uint16_t *)header_buf; // 0F0F: little, F0F0: big
    if ((meta.byte_order != 0xF0F0) && (meta.byte_order != 0x0F0F)) {
        puts("Endianness could not be parsed");
        return 1;
    }

    // Version
    memcpy(meta.version, header_buf + 2, 8);
    memset(meta.version + 8, 0, 1); //meta.version[8] = 0;
    if (strncmp(":WFM#001", (const char *)meta.version, 8) == 0) {
        v1_offset = 2;
    } else if (strncmp(":WFM#002", (const char *)meta.version, 8) == 0 ||
            strncmp(":WFM#003", (const char *)meta.version, 8) == 0) {
        v1_offset = 0;
    } else {
        puts("only version 1, 2 or 3 wfms supported in this version");
        return 1;
    }

    meta.imp_dim_count = (*(uint32_t *)(header_buf + 114));
    meta.exp_dim_count = (*(uint32_t *)(header_buf + 118));
    meta.record_type = (*(uint32_t *)(header_buf + 122));
    meta.exp_dim_1_type = (*(uint32_t *)(header_buf + 244-v1_offset));
    meta.time_base_1 = (*(uint32_t *)(header_buf + 768-v1_offset));
    meta.fastframe = (*(uint32_t *)(header_buf + 78));
    meta.frames = (*(uint32_t *)(header_buf + 72)) + 1;
    meta.summary_frame = (*(short *)(header_buf + 154));
    meta.curve_offset = (*(int *)(header_buf + 16));

    // Scaling factors
    meta.vscale = (*(double *)(header_buf + 168-v1_offset));
    meta.voffset = (*(double *)(header_buf + 176-v1_offset));

    if (strncmp(":WFM#003", (const char *)meta.version, 8) == 0) {
        meta.tstart = (*(double *)(header_buf + 496-v1_offset));
        meta.tscale = (*(double *)(header_buf + 488-v1_offset));
    } else {
        meta.tstart = (*(double *)(header_buf + 488-v1_offset));
        meta.tscale = (*(double *)(header_buf + 536-v1_offset));
    }

    meta.tfrac = (*(double *)(header_buf + 788-v1_offset));
    meta.tdatefrac = (*(double *)(header_buf + 796));
    meta.tdate = (*(uint32_t *)(header_buf + 804-v1_offset));

    // Data offsets
    // frames are same size, only first frame offsets are used
    if (strncmp(":WFM#003", (const char *)meta.version, 8) == 0) {
        meta.dsize = (*(uint32_t *)(header_buf + 822));
        meta.dpost = (*(uint32_t *)(header_buf + 826));
        meta.readbytes = meta.dpost - meta.dsize; // dpost > dsize ?
        meta.allbytes = (*(uint32_t *)(header_buf + 830));
    } else {
        meta.dsize = (*(uint32_t *)(header_buf + 818-v1_offset));
    }

    // sample data type detection
    // waveform is determined by code and bps
    meta.code = (*(int *)(header_buf + 240-v1_offset));
    meta.bps = *(header_buf + 15);

    uint32_t samples;
    if (meta.code == 7 && meta.bps == 1) {
        // format is i1
        samples = meta.readbytes;
    } else if (meta.code == 0 && meta.bps == 2) {
        // format is i2
        samples = meta.readbytes / 2;
    } else if (meta.code == 4 && meta.bps == 4) {
        // format is f32
        samples = meta.readbytes / 4;
    } else {
        puts("data type code or bytes-per-sample not understood");
        return 1;
    }

    meta.samples = samples;
    meta.available_values = meta.allbytes / meta.bps;
    meta.pre_values = meta.dsize / meta.bps;
    meta.post_values = (meta.allbytes - meta.dpost) / meta.bps;

    return 0;
}

int read_wfm(std::vector<std::vector<short> >& vec_waves, wfm_header& meta_header, const char *filepath) {
    //char *p, *full_wave;
    char check_sum[8];
    size_t samples, available_samples;
    char buffer[838]; // Meta header

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "The given file could not be read." << std::endl;
        return 1;
    }

    file.read(buffer, 838); // read header
    if ( file.gcount() != 838) {
        std::cerr << "Reading a header failed." << std::endl;
        return 1;
    }
 
    decode_header(meta_header, buffer);
#ifdef DEBUG
    puts("[*] Debug mode");
    dump_header(meta_header); return 1;
#endif
    
    // (Frames - 1) * (24 + 30) bytes
    std::shared_ptr<char> p(new char((24 + 30) * (meta_header.frames - 1)));
    //p = (char *)malloc((24 + 30) * (meta_header.frames - 1));
    file.read(p.get(), (24 + 30) * (meta_header.frames - 1));

    samples = meta_header.samples;
    available_samples = meta_header.available_values;

    std::shared_ptr<char> full_wave(new char(2 * available_samples * meta_header.frames));
    //full_wave = (char *)malloc(2 * available_samples * meta_header.frames);
    file.read(full_wave.get(), 2 * available_samples * meta_header.frames);
    file.read(check_sum, 8);
    file.close();

    vec_waves.resize(meta_header.frames);
    for (size_t frame = 0; frame < meta_header.frames; frame++) {
        vec_waves[frame].resize(meta_header.samples);
        size_t frame_offset = frame * meta_header.available_values;

        vec_waves[frame].assign(
                (short *)full_wave.get() + frame_offset + meta_header.pre_values,
                (short *)full_wave.get() + frame_offset + meta_header.pre_values + 2*samples
                );
    }

    return 0;
}

int read_wfm_scaled(std::vector<std::vector<double> >& vec_waves, wfm_header& meta_header, const char *filepath) {
    //char *p, *full_wave;
    short val;
    char check_sum[8];
    size_t samples, available_samples;
    char buffer[838]; // Meta header

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "The given file could not be read." << std::endl;
        return 1;
    }

    file.read(buffer, 838); // read header
    if ( file.gcount() != 838) {
        std::cerr << "Reading a header failed." << std::endl;
        return 1;
    }
 
    decode_header(meta_header, buffer);
#ifdef DEBUG
    puts("[*] Debug mode");
    dump_header(meta_header); return 1;
#endif
    
    // (Frames - 1) * (24 + 30) bytes
    std::shared_ptr<char> p(new char[(24 + 30) * (meta_header.frames - 1)]);
    file.read(p.get(), (24 + 30) * (meta_header.frames - 1));

    samples = meta_header.samples;
    available_samples = meta_header.available_values;

    std::shared_ptr<char> full_wave(new char[2 * available_samples * meta_header.frames]);
    file.read(full_wave.get(), 2 * available_samples * meta_header.frames);
    //char *full_wave = (char *)malloc(2 * available_samples * meta_header.frames);
    //file.read(full_wave, 2 * available_samples * meta_header.frames);
    file.read(check_sum, 8);
    file.close();

    vec_waves.resize(meta_header.frames);
    for (size_t frame = 0; frame < meta_header.frames; frame++) {
        vec_waves[frame].resize(meta_header.samples);
        size_t frame_offset = frame * meta_header.available_values;

        for (size_t i = 0; i < samples; i++ ) {
            //val = *((short *)full_wave + i + frame_offset + meta_header.pre_values),
            val = *((short *)full_wave.get() + i + frame_offset + meta_header.pre_values),
            vec_waves[frame][i] = double(val) * meta_header.vscale + meta_header.voffset;
        }
    }

    return 0;
}
