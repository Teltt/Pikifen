/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Header for area geometry-related functions.
 */

#ifndef GEOMETRY_INCLUDED
#define GEOMETRY_INCLUDED

#include <map>
#include <set>
#include <unordered_set>
#include <vector>

#include "../utils/geometry_utils.h"

using std::map;
using std::set;
using std::unordered_set;
using std::vector;

struct edge;
struct sector;
struct vertex;


namespace GEOMETRY {
extern const float BLOCKMAP_BLOCK_SIZE;
extern const unsigned char DEF_SECTOR_BRIGHTNESS;
extern const float STEP_HEIGHT;
extern const float LIQUID_DRAIN_DURATION;
extern const float SHADOW_AUTO_LENGTH_MULT;
extern const ALLEGRO_COLOR SHADOW_DEF_COLOR;
extern const float SHADOW_MAX_AUTO_LENGTH;
extern const float SHADOW_MAX_LENGTH;
extern const float SHADOW_MIN_AUTO_LENGTH;
extern const float SHADOW_MIN_LENGTH;
extern const ALLEGRO_COLOR SMOOTHING_DEF_COLOR;
extern const float SMOOTHING_MAX_LENGTH;
}


//Possible errors after a triangulation operation.
enum TRIANGULATION_ERRORS {
    //No error occured.
    TRIANGULATION_NO_ERROR,
    //Invalid arguments provided.
    TRIANGULATION_ERROR_INVALID_ARGS,
    //Non-simple sector: Lone edges break the sector.
    TRIANGULATION_ERROR_LONE_EDGES,
    //Non-simple sector: Vertexes are used multiple times.
    TRIANGULATION_ERROR_VERTEXES_REUSED,
    //Non-simple sector: Ran out of ears while triangulating.
    TRIANGULATION_ERROR_NO_EARS,
};


/* ----------------------------------------------------------------------------
 * A triangle. Sectors (essentially polygons) are made out of triangles.
 * These are used to detect whether a point is inside a sector,
 * and to draw, seeing as OpenGL cannot draw concave polygons.
 */
struct triangle {
    //Points that make up this triangle.
    vertex* points[3];
    
    triangle(vertex* v1, vertex* v2, vertex* v3);
};


/* ----------------------------------------------------------------------------
 * Represents a series of vertexes that make up a polygon.
 */
struct polygon {
    //Ordered list of vertexes that represent the polygon.
    vector<vertex*> vertexes;
    
    void clean();
    void cut(vector<polygon>* inners);
    vertex* get_rightmost_vertex() const;
    
    polygon();
    explicit polygon(const vector<vertex*> &vertexes);
};


/* ----------------------------------------------------------------------------
 * A structure holding info on the geometry problems the area currently has.
 */
struct geometry_problems {
    //Non-simple sectors found, and their reason for being broken.
    map<sector*, TRIANGULATION_ERRORS> non_simples;
    //List of lone edges found.
    unordered_set<edge*> lone_edges;
};



void get_cce(
    vector<vertex> &vertexes_left, vector<size_t> &ears,
    vector<size_t> &convex_vertexes, vector<size_t> &concave_vertexes
);
vector<std::pair<dist, vertex*> > get_merge_vertexes(
    const point &p, vector<vertex*> &all_vertexes, const float merge_radius
);
TRIANGULATION_ERRORS get_polys(
    sector* s, polygon* outer, vector<polygon>* inners,
    set<edge*>* lone_edges, const bool check_vertex_reuse
);
vertex* get_rightmost_vertex(const map<edge*, bool> &edges);
vertex* get_rightmost_vertex(vertex* v1, vertex* v2);
bool is_polygon_clockwise(vector<vertex*> &vertexes);
bool is_vertex_convex(const vector<vertex*> &vec, const size_t nr);
bool is_vertex_ear(
    const vector<vertex*> &vec, const vector<size_t> &concaves, const size_t nr
);
TRIANGULATION_ERRORS triangulate(
    sector* s_ptr, set<edge*>* lone_edges, const bool check_vertex_reuse,
    const bool clear_lone_edges
);

#endif //ifndef GEOMETRY_INCLUDED
