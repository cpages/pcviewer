#include <stdexcept>
#include <string>
#include <stdio.h>
#include "3rdparty/rply/rply.h"
#include "shared.hpp"
#include "plyReader.hpp"

namespace
{
    struct Data
    {
        GLfloat *data;
        int idx;
    };

    int vertex_cb(p_ply_argument argument) {
        long eol;
        Data *pd;
        ply_get_argument_user_data(argument, (void **)&pd, &eol);
        pd->data[pd->idx++] = ply_get_argument_value(argument);
        //printf("%g", ply_get_argument_value(argument));
        //if (eol) printf("\n");
        //else printf(" ");
        return 1;
    }

    int face_cb(p_ply_argument argument) {
        return 1;
        long length, value_index;
        ply_get_argument_property(argument, NULL, &length, &value_index);
        switch (value_index) {
            case 0:
            case 1:
                printf("%g ", ply_get_argument_value(argument));
                break;
            case 2:
                printf("%g\n", ply_get_argument_value(argument));
                break;
            default:
                break;
        }
        return 1;
    }
}

void
readPLY(const char *file, std::vector<Vertex> &vertices)
{
    long nvertices, ntriangles;
    p_ply ply = ply_open(file, NULL, 0, NULL);
    if (!ply) throw std::runtime_error("cannot open ply");
    if (!ply_read_header(ply)) throw std::runtime_error("cannot read ply header");
    nvertices = ply_set_read_cb(ply, "vertex", "x", vertex_cb, 0, 0);
    vertices.resize(nvertices);
    Data gData;
    gData.data = (GLfloat *)&vertices[0];
    gData.idx = 0;
    ply_set_read_cb(ply, "vertex", "x", vertex_cb, &gData, 0);
    ply_set_read_cb(ply, "vertex", "y", vertex_cb, &gData, 0);
    ply_set_read_cb(ply, "vertex", "z", vertex_cb, &gData, 1);
    ntriangles = ply_set_read_cb(ply, "face", "vertex_indices", face_cb, NULL, 0);
    //printf("%ld\n%ld\n", nvertices, ntriangles);
    if (!ply_read(ply)) throw std::runtime_error("error parsing ply");
    ply_close(ply);
}
