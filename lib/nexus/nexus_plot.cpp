/*
   Copyright (C) 2017 Statoil ASA, Norway.

   The file 'nexus_plot.cpp' is part of ERT - Ensemble based Reservoir Tool.

   ERT is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   ERT is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.

   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
   for more details.
*/

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <sstream>


#include <ert/util/build_config.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#elif defined(HAVE_ARPA_INET_H)
#include <arpa/inet.h>
#elif defined(HAVE_WINSOCK2_H)
#include <winsock2.h>
#endif

#include <ert/nexus/nexus_plot.hpp>
#include <ert/ecl/ecl_sum.h>
#include <map>


const std::string NEXUS_PLOT_TYPE_HEADER = "PLOT  BIN ";

namespace {

    float int32_t_bits_to_float(int32_t i) {
        int32_t sign_bit  = (i & 0x80000000L) >> 31;
        float   sign      = sign_bit ? -1.0 : 1.0;
        int32_t exp_bits  = (i & 0x7F800000L) >> 23;
        int32_t exp       = exp_bits ? exp_bits - 0x0000007FL : -126L;
        int32_t frac_bits = i & 0x007FFFFFL;
        double significand = (exp_bits ? 1.0 : 0.0) + frac_bits / double(0x007FFFFFL);

        if (exp_bits == 0x000000FFL)
            return frac_bits ? NAN : (sign_bit ? -INFINITY : INFINITY);
        else if (frac_bits | exp_bits)
            return float(sign * significand * std::pow(2, exp));
        else
            return 0.0f;
    }

template< int N >
std::string read_str(std::istream& stream) {
    std::array< char, N> buf;
    stream.read( buf.data(), N );
    return std::string( buf.data(), N );
}

struct hinfo {
    int32_t num_classes;
    int32_t day, month, year;
    int32_t nx, ny, nz;
    int32_t ncomp;
    std::vector<std::string> class_names;
    std::map< std::string, int32_t> vars_in_class;
    std::map< std::string, std::vector<std::string> > var_names;
};

hinfo headerinfo( std::istream& stream ) {
    stream.seekg(4 + 10 + 562 + 264, std::ios::beg);

    std::array< int32_t, 8 > buf {};
    stream.read( (char*)buf.data(), buf.max_size() * 4 );
    if ( !stream.good() ) throw nex::unexpected_eof("");

    auto ntoh = []( int32_t x ) { return ntohl( x ); };
    std::transform( buf.begin(), buf.end(), buf.begin(), ntoh );
    auto negative = []( int32_t x ) { return x < 0; };
    if ( std::any_of( buf.begin(), buf.end(), negative ) )
        throw nex::bad_header("Negative value, corrupted file");

    hinfo h = {
        buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
        {},
        std::map< std::string, int32_t >(),
        std::map< std::string, std::vector<std::string> >(),
    };

    stream.seekg(8, std::ios::cur);
    std::array< char, 8 > class_name;
    for (int i = 0; i < h.num_classes; i++) {
        stream.read(class_name.data(), 8);
        h.class_names.push_back(std::string( class_name.data(), 8 ));
    }

    stream.seekg(8, std::ios::cur);
    std::vector< int32_t > vars_in_class( h.num_classes );
    stream.read((char*) vars_in_class.data(), h.num_classes * 4);
    std::transform( vars_in_class.begin(),
                    vars_in_class.end(),
                    vars_in_class.begin(),
                    ntoh );
    if (std::any_of( vars_in_class.begin(), vars_in_class.end(), negative))
        throw nex::bad_header("Negative value, corrupted file");
    for (int i = 0; i < h.num_classes; i++)
        h.vars_in_class[h.class_names[i]] = vars_in_class[i];

    stream.seekg(8, std::ios::cur);
    for (int i = 0; i < h.num_classes; ++i) {
        stream.seekg(4, std::ios::cur);
        std::vector< char > var_names( h.vars_in_class[h.class_names[i]] * 4, 0 );
        stream.read( var_names.data(), h.vars_in_class[h.class_names[i]] * 4 );
        for (int k = 0; k < h.vars_in_class[h.class_names[i]]; ++k)
            h.var_names[h.class_names[i]]
                .push_back(std::string( var_names.data() + k*4 ,4 ));
        stream.seekg(8, std::ios::cur);
    }

    return h;
}

std::map< float, nex::NexusPlot::timestep_data > nexus_classes( hinfo h, std::istream& stream ) {
    std::cout << "STREAM POSITION = " << stream.tellg() << std::endl;
    std::map< float, nex::NexusPlot::timestep_data > data;

    std::string class_name = read_str<8>(stream);
    int counter = 0;
    while (class_name.compare("STOP    ") != 0 && (counter++ < 2)) {
        //Skipped
        stream.seekg(8, std::ios::cur);

        std::array< int32_t, 5 > buf {};
        stream.read( (char*)buf.data(), buf.max_size() * 4 );
        if ( !stream.good() ) throw nex::unexpected_eof("");
        auto ntoh = []( int32_t x ) { return ntohl( x ); };
        std::transform( buf.begin(), buf.end(), buf.begin(), ntoh );

        float timestep = int32_t_bits_to_float(buf[0]);
        int num_items = (int) int32_t_bits_to_float(buf[2]);
        if (data.find(timestep) == data.end())
            data[timestep] = nex::NexusPlot::timestep_data {};
        if (data[timestep].find(class_name) == data[timestep].end())
            data[timestep][class_name] = std::vector<nex::NexusPlot::NexusClassItem> {};


        for (int i = 0; i < num_items && i < 2; i++) {
            nex::NexusPlot::NexusClassItem nc = {
                    class_name,
                    int32_t_bits_to_float(buf[0]),
                    int32_t_bits_to_float(buf[1]),
                    int32_t_bits_to_float(buf[2]),
                    int32_t_bits_to_float(buf[3]),
                    int32_t_bits_to_float(buf[4]),
                    std::map< std::string, float >()
            };

            stream.seekg(72 + 8, std::ios::cur);

            std::vector< int32_t > values( (size_t) h.vars_in_class[class_name] );
            stream.read( (char*)values.data(), values.size() * 4 );
            std::transform( values.begin(), values.end(), values.begin(), ntoh );
            for (int i = 0; i < h.vars_in_class[class_name]; i++) {
                nc.var[h.var_names[class_name][i]] = int32_t_bits_to_float(values[i]);
            }

            data[timestep][class_name].push_back(nc);



            std::vector< std::string > VAR_NAMES = {
                    "COP ", "CGP ", "CWP ", "CGI ", "CWI ", "QOP ", "QGP ", "QWP ",
                    "QGI ", "QWI ", "BHP ", "WPH ", "WKH ", "WPAV", "THP ", "COWP",
                    "QOWP", "GOR ", "WCUT", "WOR ", "QGLG", "CGLG", "DRDN", "DRMX",
                    "CROP", "CRGP", "CRWP", "CROI", "CRGI", "CRWI", "ROP ", "RGP ",
                    "RWP ", "ROI ", "RGI ", "RWI ", "ONTM", "ALQ ", "API ", "QCDP",
                    "CCDP", "YCDP", "ACTV", "STAT", "Q1P ", "Q1I ", "C1P ", "C1I ",
                    "X1P ", "Y1P ", "Q2P ", "Q2I ", "C2P ", "C2I ", "X2P ", "Y2P " };

            std::cout << "\tname      = " << nc.name      << std::endl;
            std::cout << "\ttimestep  = " << nc.timestep  << std::endl;
            std::cout << "\ttime      = " << nc.time      << std::endl;
            std::cout << "\tnum_items = " << nc.num_items << std::endl;
            std::cout << "\tmax_items = " << nc.max_items << std::endl;
            std::cout << "\tmax_perfs = " << nc.max_perfs << std::endl;
            std::cout << "\tvar       = {" << std::endl;
            for (auto s : VAR_NAMES) {
                std::cout << "\t\t" << s << " -> " << nc.var[s] << std::endl;
            }
            std::cout << "\t}" << std::endl;
            std::cout << "}" << std::endl;
        }


        // Mangler skip
        //stream.seekg(8, std::ios::cur);
        class_name = read_str<8>(stream);
    }

    return data;
}

}


nex::NexusPlot::NexusPlot( const std::string& filename ) {
    std::ifstream stream(filename, std::ios::binary);
    if ( !stream.good() )
        throw nex::read_error("Could not open file " + filename);
    this->load(stream);
}

nex::NexusPlot::NexusPlot( std::istream& stream ) {
    this->load(stream);
}

void nex::NexusPlot::load(std::istream& stream) {
    struct stream_guard {
        stream_guard(std::istream &stream) :
                mask(stream.exceptions()),
                s(stream) {}

        ~stream_guard() { this->s.exceptions(this->mask); }

        std::ios::iostate mask;
        std::istream &s;
    } g{stream};
    stream.exceptions(std::ios::goodbit);

    stream.seekg(4, std::ios::beg); // skip 4 bytes
    auto type_header = read_str<10>(stream);

    if (type_header.compare(NEXUS_PLOT_TYPE_HEADER) != 0 || !stream.good())
        throw nex::bad_header("Could not verify file type");

    auto header = headerinfo(stream);
    this->num_classes = header.num_classes;
    this->day = header.day;
    this->month = header.month;
    this->year = header.year;
    this->nx = header.nx;
    this->ny = header.ny;
    this->nz = header.nz;
    this->ncomp = header.ncomp;
    this->class_names = header.class_names;
    this->vars_in_class = header.vars_in_class;
    this->var_names = header.var_names;

    if (stream.eof() || !stream.good())
        return;

    this->data= nexus_classes(header, stream);
}


ecl_sum_type* nex::NexusPlot::ecl_summary( const std::string& ecl_case ) {
    bool unified = true;
    bool fmt_output = false;
    const char* key_join_string = ":";
    time_t sim_start = 0;
    bool time_in_days = true;

    ecl_sum_type * ecl_sum = ecl_sum_alloc_writer( ecl_case.c_str(),
        fmt_output,
        unified,
        key_join_string,
        sim_start,
        time_in_days,
        this->nx, this->ny, this->nz);

    return ecl_sum;
}
