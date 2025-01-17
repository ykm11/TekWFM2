# -*- coding: utf-8 -*-
"""
Modified on Thu Jun 16 01:08:31 2023
Created on Mon Mar  1 15:34:22 2021

@author: Pavel Gostev, ykm11

WFM v.1, v.2, v.3 reader
https://www.tek.com/sample-license
reads volts vs. time records (including fastframes) from little- and big-endian version 1 & 2 WFM files


See Also
Performance Oscilloscope Reference Waveform File Format
https://download.tek.com/manual/Waveform-File-Format-Manual-077022011.pdf

Based on
https://forum.tek.com/viewtopic.php?t=141081

Tested on Python 3.6+


This module is tested on only little-endian version 3 WFM files.
I also provide fast-frame mode, but not tested for version both 1 and 2.
"""


import struct
import numpy as np


class WfmReadError(Exception):
    """error for unexpected things"""
    pass


def read_wfm(path):
    """return sample data from path WFM file"""
    with open(path, 'rb') as f:
        hbytes = f.read(838)
        meta = decode_header(path, hbytes)

        # file signature checks
        if meta['imp_dim_count'] != 1:
            raise WfmReadError(path, 'imp dim count not 1')
        if meta['exp_dim_count'] != 1:
            raise WfmReadError(path, 'exp dim count not 1')
        if meta['record_type'] != 2:
            raise WfmReadError(path, 'not WFMDATA_VECTOR')
        # if meta['exp_dim_1_type'] != 0:
        #    raise WfmReadError(path, 'not EXPLICIT_SAMPLE')
        if meta['time_base_1'] != 0:
            raise WfmReadError(path, 'not BASE_TIME')

        tfrac_array = np.zeros(meta['Frames'], dtype=np.double)
        tdatefrac_array = np.zeros(meta['Frames'], dtype=np.double)
        tdate_array = np.zeros(meta['Frames'], dtype=np.int32)
        tfrac_array[0] = meta['tfrac']
        tdatefrac_array[0] = meta['tdatefrac']
        tdate_array[0] = meta['tdate']

        # if fastframe, read fastframe table
        if meta['fastframe'] == 1:
            if meta['version'] == b":WFM#003":
                WUSp = np.fromfile(f, dtype='i4,f8,f8,i4', count=(meta['Frames'] - 1))
                # merge first frame trigger infos with frames > 1
                tfrac_array[1:] = WUSp['f1']
                tdatefrac_array[1:] = WUSp['f2']
                tdate_array[1:] = WUSp['f3']
            else:
                raise WfmReadError(path, 'Fast Frames are supported only for version 3')

        # read curve block
        bin_wave = np.memmap(filename=f,
                            dtype=meta['dformat'],
                            mode='r',
                            offset=meta['curve_offset'],
                            shape = (meta['avilable_values'], meta['Frames']),
                            order='F')

    # slice out buffer values 
    bin_wave = bin_wave[meta['pre_values'] : \
            meta['avilable_values'] - meta['post_values'],:] 
    scaled_array = bin_wave * meta['vscale'] + meta['voffset']

    # Convert arrays from (samples, frames) to (frames, samples).
    scaled_array = scaled_array.transpose()

    #print("len(bin_wave):", len(bin_wave))
    #print("Fastrame:", meta['fastframe'])
    #print("Frames:", meta['Frames'])
    #print("avilable_values:", meta['avilable_values'])
    #print("offset:", meta['curve_offset'])
    #print("vscale:", meta["vscale"])
    #print("voffset:", meta["voffset"])

    return scaled_array, meta

def decode_header(path, header_bytes):
    """returns a dict of wfm metadata"""
    wfm_info = {}
    if len(header_bytes) != 838:
        raise WfmReadError(path, 'wfm header bytes not 838')

    wfm_info['byte_order'] = struct.unpack_from('H', header_bytes, offset=0)[0]

    if wfm_info['byte_order'] == 0x0f0f:
        endianness = "<"
    elif wfm_info['byte_order'] == 0xf0f0:
        endianness = ">"
    else:
        raise WfmReadError(path, 'endianness could not be parsed')

    wfm_info['version'] = struct.unpack_from('8s', header_bytes, offset=2)[0]

    if wfm_info['version'] == b':WFM#001':
        v1_offset = 2
    elif wfm_info['version'] == b':WFM#002' or wfm_info['version'] == b':WFM#003':
        v1_offset = 0
    else:
        raise WfmReadError(
             path, 'only version 1 or 2 wfms supported in this version')

    wfm_info['imp_dim_count'] = struct.unpack_from(endianness+'I', header_bytes, offset=114)[0]
    wfm_info['exp_dim_count'] = struct.unpack_from(endianness+'I', header_bytes, offset=118)[0]
    wfm_info['record_type'] = struct.unpack_from(endianness+'I', header_bytes, offset=122)[0]
    wfm_info['exp_dim_1_type'] = struct.unpack_from(endianness+'I', header_bytes, offset=244-v1_offset)[0]
    wfm_info['time_base_1'] = struct.unpack_from(endianness+'I', header_bytes, offset=768-v1_offset)[0]
    wfm_info['fastframe'] = struct.unpack_from(endianness+'I', header_bytes, offset=78)[0]
    wfm_info['Frames'] = struct.unpack_from(endianness+'I', header_bytes, offset=72)[0] + 1
    wfm_info['summary_frame'] = struct.unpack_from(endianness+'h', header_bytes, offset=154)[0]
    wfm_info['curve_offset'] = struct.unpack_from(endianness+'i', header_bytes, offset=16)[0]  # 838 + ((frames - 1) * 54)

    # scaling factors
    wfm_info['vscale'] = struct.unpack_from(endianness+'d', header_bytes, offset=168-v1_offset)[0]
    wfm_info['voffset'] = struct.unpack_from(endianness+'d', header_bytes, offset=176-v1_offset)[0]

    if wfm_info['version'] == b":WFM#003":
        wfm_info['tstart'] = struct.unpack_from('d', header_bytes, offset=496)[0]
        wfm_info['tscale'] = struct.unpack_from('d', header_bytes, offset=488)[0]
    else:
        wfm_info['tstart'] = struct.unpack_from(endianness+'d', 
                header_bytes, offset=488-v1_offset)[0]
        wfm_info['tscale'] = struct.unpack_from(endianness+'d', 
                header_bytes, offset=536-v1_offset)[0]

    # trigger detail
    wfm_info['tfrac'] = struct.unpack_from(endianness+'d', header_bytes, offset=788-v1_offset)[
        0]  # frame index 0
    wfm_info['tdatefrac'] = struct.unpack_from(endianness+'d', header_bytes, offset=796)[0]  # frame index 0
    wfm_info['tdate'] = struct.unpack_from(endianness+'I', header_bytes, offset=804-v1_offset)[
        0]  # frame index 0

    # data offsets
    # frames are same size, only first frame offsets are used
    if wfm_info['version'] == b":WFM#003":
        dsize = struct.unpack_from('I', header_bytes, offset=822)[0]

        # data offsets
        # frames are same size, only first frame offsets are used
        wfm_info['dsize'] = dsize
        dpost = struct.unpack_from('I', header_bytes, offset=826)[0]
        wfm_info['dpost'] = dpost
        readbytes = dpost - dsize
        wfm_info['readbytes'] = readbytes
        allbytes = struct.unpack_from('I', header_bytes, offset=830)[0]
        wfm_info['allbytes'] = allbytes

    else:
        dsize = struct.unpack_from(endianness+'I', header_bytes, offset=818-v1_offset)[0]
        wfm_info['dsize'] = dsize

    # sample data type detection
    code = struct.unpack_from(endianness+'i', header_bytes, offset=240-v1_offset)[0]
    wfm_info['code'] = code
    bps = struct.unpack_from(endianness+'b', header_bytes, offset=15)[0]  # bytes-per-sample
    wfm_info['bps'] = bps

    if code == 7 and bps == 1:
        dformat = endianness+'i1'
        samples = readbytes
    elif code == 0 and bps == 2:
        dformat = endianness+'i2'
        samples = readbytes // 2
    elif code == 4 and bps == 4:
        dformat = endianness+'f32'
        samples = readbytes // 4
    else:
        raise WfmReadError(path, 'data type code or bytes-per-sample not understood')

    wfm_info['dformat'] = dformat
    wfm_info['samples'] = samples
    wfm_info['avilable_values'] = allbytes // bps
    wfm_info['pre_values'] = dsize // bps
    wfm_info['post_values'] = (allbytes - dpost) // bps

    return wfm_info


class ScopeData:

    def __init__(self, path):
        """
        Class for WFM oscillogram data from old Tek Oscilloscopes
        5 Series MSO
        6 Series MSO

        Parameters
        ----------
        path : str
            Full or relative path to oscillogram WFM file.

        """
        try:
            self.y, meta = read_wfm(path)
        except Exception as E:
            raise WfmReadError(path, E)

        for k, v in meta.items():
            setattr(self, k, v)

        samples = len(self.y)
        self.tstop = samples * self.tscale + self.tstart
        self.x = np.linspace(self.tstart, self.tstop, samples, endpoint=False)
        self.horizInterval = self.x[1] - self.x[0]
