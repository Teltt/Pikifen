/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Area geometry-related functions.
 */

#define _USE_MATH_DEFINES

#include <algorithm>
#include <cmath>
#include <vector>

#include <allegro5/allegro_color.h>

#include "geometry.h"

#include "../../core/misc_functions.h"
#include "../../util/general_utils.h"
#include "../../util/geometry_utils.h"
#include "vertex.h"


using std::vector;


namespace GEOMETRY {

//Area active cells have this width and height.
const float AREA_CELL_SIZE = 128;

//Area blockmap blocks have this width and height.
const float BLOCKMAP_BLOCK_SIZE = 128;

//Default sector brightness.
const unsigned char DEF_SECTOR_BRIGHTNESS = 255;

//Liquids drain for this long.
const float LIQUID_DRAIN_DURATION = 2.0f;

//Auto wall shadow lengths are the sector height difference multiplied by this.
const float SHADOW_AUTO_LENGTH_MULT = 0.2f;

//Default color of wall shadows. This is the color at the edge.
const ALLEGRO_COLOR SHADOW_DEF_COLOR = {0.0f, 0.0f, 0.0f, 0.90f};

//Maximum length a wall shadow can be when the length is automatic.
const float SHADOW_MAX_AUTO_LENGTH = 50.0f;

//Maximum length a wall shadow can be.
const float SHADOW_MAX_LENGTH = 100.0f;

//Minimum length a wall shadow can be when the length is automatic.
const float SHADOW_MIN_AUTO_LENGTH = 8.0f;

//Minimum length a wall shadow can be.
const float SHADOW_MIN_LENGTH = 1.0f;

//Default color of the smoothing effect.
const ALLEGRO_COLOR SMOOTHING_DEF_COLOR = {0.0f, 0.0f, 0.0f, 0.70f};

//Maximum length the smoothing effect can be.
const float SMOOTHING_MAX_LENGTH = 100.0f;

//Mobs can walk up sectors that are, at the most,
//this high from the current one, as if climbing up steps.
const float STEP_HEIGHT = 50;

}


/**
 * @brief Constructs a new polygon object.
 */
Polygon::Polygon() {
}


/**
 * @brief Constructs a new polygon object.
 *
 * @param vertexes Vertexes that make up the polygon.
 */
Polygon::Polygon(const vector<Vertex*> &vertexes) :
    vertexes(vertexes) {
}


/**
 * @brief Cleans a polygon's vertexes.
 * This deletes 0-length edges, and 180-degree vertexes.
 *
 * @param recursive If true, clean the children polyon too recursively.
 */
void Polygon::clean(bool recursive) {
    for(size_t v = 0; v < vertexes.size();) {
        bool should_delete = false;
        Vertex* prev_v = getPrevInVector(vertexes, v);
        Vertex* cur_v = vertexes[v];
        Vertex* next_v = getNextInVector(vertexes, v);
        
        //If the distance between both vertexes is so small
        //that it's basically 0, delete this vertex from the list.
        if(
            fabs(prev_v->x - cur_v->x) < 0.00001 &&
            fabs(prev_v->y - cur_v->y) < 0.00001
        ) {
            should_delete = true;
        }
        
        //If the angle between this vertex and the next is the same, then
        //this is just a redundant point in the edge prev - next. Delete it.
        if(
            fabs(
                getAngle(v2p(cur_v), v2p(prev_v)) -
                getAngle(v2p(next_v), v2p(cur_v))
            ) < 0.000001
        ) {
            should_delete = true;
        }
        
        if(should_delete) {
            vertexes.erase(vertexes.begin() + v);
        } else {
            v++;
        }
    }
    
    if(recursive) {
        for(size_t c = 0; c < children.size(); c++) {
            children[c]->clean(true);
        }
    }
}


/**
 * @brief When this polygon has children polygons, a cut must be made between it
 * and the children polygons, as to make this one holeless.
 */
void Polygon::cut() {
    if(vertexes.size() < 3) {
        //Some error happened.
        return;
    }
    
    //Start with the rightmost vertex.
    Vertex* rightmost = getRightmostVertex();
    
    //We have to make one cut for every inner.
    for(size_t c = 0; c < children.size(); c++) {
        Polygon* child_ptr = children[c];
        Vertex* closest_edge_v1 = nullptr;
        Vertex* closest_edge_v2 = nullptr;
        float closest_edge_r = FLT_MAX;
        Vertex* closest_vertex = nullptr;
        float closest_vertex_r = FLT_MAX;
        Vertex* best_vertex = nullptr;
        
        //Find the rightmost vertex on this inner.
        Vertex* start = child_ptr->getRightmostVertex();
        
        if(!start) {
            //Some error occured.
            continue;
        }
        
        //Imagine a line from this vertex to the right.
        //If any edge of the outer polygon intersects it,
        //we just find the best vertex on that edge, and make the cut.
        //This line stretching right is known as a ray.
        float ray_width = rightmost->x - start->x;
        
        //Let's also check the vertexes.
        //If the closest thing is a vertex, not an edge, then
        //we can skip a bunch of steps.
        Vertex* v1 = nullptr, *v2 = nullptr;
        for(size_t v = 0; v < vertexes.size(); v++) {
            v1 = vertexes[v];
            v2 = getNextInVector(vertexes, v);
            if(
                (v1->x >= start->x || v2->x >= start->x) &&
                (v1->x <= rightmost->x || v2->x <= rightmost->x)
            ) {
                float r;
                if(
                    lineSegsIntersect(
                        v2p(v1), v2p(v2),
                        v2p(start), Point(rightmost->x, start->y),
                        nullptr, &r
                    )
                ) {
                    if(!closest_edge_v1 || r < closest_edge_r) {
                        closest_edge_v1 = v1;
                        closest_edge_v2 = v2;
                        closest_edge_r = r;
                    }
                }
                
                if(v1->y == start->y && v1->x >= start->x) {
                    r = (v1->x - start->x) / ray_width;
                    if(!closest_vertex || r < closest_vertex_r) {
                        closest_vertex = v1;
                        closest_vertex_r = r;
                    }
                }
                
            }
        }
        
        if(!closest_vertex && !closest_edge_v1) {
            //Some error occured.
            continue;
        }
        
        //Which is closest, a vertex or an edge?
        if(closest_vertex_r <= closest_edge_r) {
            //If it's a vertex, done.
            best_vertex = closest_vertex;
        } else {
            if(!closest_edge_v1) continue;
            
            //If it's an edge, some more complicated steps need to be done.
            
            //We're on the edge closest to the vertex.
            //Go to the rightmost vertex of this edge.
            Vertex* vertex_to_compare =
                ::getRightmostVertex(closest_edge_v1, closest_edge_v2);
                
            //Now get a list of all vertexes inside the triangle
            //marked by the inner's vertex,
            //the point on the edge,
            //and the vertex we're comparing.
            vector<Vertex*> inside_triangle;
            for(size_t v = 0; v < vertexes.size(); v++) {
                Vertex* v_ptr = vertexes[v];
                if(
                    isPointInTriangle(
                        v2p(v_ptr), v2p(start),
                        Point(start->x + closest_edge_r * ray_width, start->y),
                        v2p(vertex_to_compare),
                        true) &&
                    v_ptr != vertex_to_compare
                ) {
                    inside_triangle.push_back(v_ptr);
                }
            }
            
            //Check which one makes the smallest angle compared to 0.
            best_vertex = vertex_to_compare;
            float closest_angle = FLT_MAX;
            
            for(size_t v = 0; v < inside_triangle.size(); v++) {
                Vertex* v_ptr = inside_triangle[v];
                float angle = getAngle(v2p(start), v2p(v_ptr));
                if(fabs(angle) < closest_angle) {
                    closest_angle = fabs(angle);
                    best_vertex = v_ptr;
                }
            }
        }
        
        //This is the final vertex. Make a bridge
        //from the start vertex to this.
        //First, we must find whether the outer vertex
        //already has bridges or not.
        //If so, we place the new bridge before or after,
        //depending on the angle.
        //We know a bridge exists if the same vertex
        //appears twice.
        vector<size_t> bridges;
        for(size_t v = 0; v < vertexes.size(); v++) {
            if(vertexes[v] == best_vertex) {
                bridges.push_back(v);
            }
        }
        
        //Insert the new bridge after this vertex.
        size_t insertion_vertex_idx;
        if(bridges.size() == 1) {
            //No bridges found, just use this vertex.
            insertion_vertex_idx = bridges[0];
        } else {
            //Find where to insert.
            insertion_vertex_idx = bridges.back();
            float new_bridge_angle =
                getAngleCwDiff(
                    getAngle(v2p(best_vertex), v2p(start)),
                    0.0f
                );
                
            for(size_t v = 0; v < bridges.size(); v++) {
                Vertex* v_ptr = vertexes[bridges[v]];
                Vertex* nv_ptr = getNextInVector(vertexes, bridges[v]);
                float a =
                    getAngleCwDiff(
                        getAngle(v2p(v_ptr), v2p(nv_ptr)),
                        0.0f
                    );
                if(a < new_bridge_angle) {
                    insertion_vertex_idx = bridges[v];
                    break;
                }
            }
        }
        
        //Now, make the bridge.
        //On the outer vertex, change the next vertex
        //to be the start of the inner, then
        //circle the inner, and go back to the outer vertex.
        //Let's just find where the start vertex is...
        size_t iv = 0;
        for(; iv < child_ptr->vertexes.size(); iv++) {
            if(child_ptr->vertexes[iv] == start) {
                break;
            }
        }
        
        auto it = child_ptr->vertexes.begin() + iv;
        size_t n_after = child_ptr->vertexes.size() - iv;
        //Finally, make the bridge.
        vertexes.insert(
            vertexes.begin() + insertion_vertex_idx + 1,
            it, child_ptr->vertexes.end()
        );
        vertexes.insert(
            vertexes.begin() + insertion_vertex_idx + 1 + n_after,
            child_ptr->vertexes.begin(), it
        );
        //This last one closes the inner polygon.
        vertexes.insert(
            vertexes.begin() + insertion_vertex_idx + 1 + n_after + iv,
            start
        );
        
        //Before we close the inner polygon, let's
        //check if the inner's rightmost and the outer best vertexes
        //are not the same.
        //This can happen if you have a square on the top-right
        //and one on the bottom-left, united by the central vertex.
        if(start != best_vertex) {
            vertexes.insert(
                vertexes.begin() + insertion_vertex_idx + 1 + n_after + iv + 1,
                best_vertex
            );
        }
    }
}


/**
 * @brief Cuts all children polygons, as the root of the polygon tree.
 */
void Polygon::cutAllAsRoot() {
    for(size_t o = 0; o < children.size(); o++) {
        //For each outer polygon...
        Polygon* outer_ptr = children[o];
        outer_ptr->cut();
        
        for(size_t i = 0; i < outer_ptr->children.size(); i++) {
            //For each inner polygon...
            Polygon* inner_ptr = outer_ptr->children[i];
            
            //An inner polygon's children are outer polygons again.
            //Now that we made the cut, we can move those back to the root list.
            children.insert(
                children.end(),
                inner_ptr->children.begin(),
                inner_ptr->children.end()
            );
            inner_ptr->children.clear();
        }
    }
}


/**
 * @brief Destroys the polygon, deleting from memory all children, recursively.
 */
void Polygon::destroy() {
    for(size_t c = 0; c < children.size(); c++) {
        children[c]->destroy();
        delete children[c];
    }
}


/**
 * @brief Returns the vertex farthest to the right in a polygon.
 *
 * @return The farthest right vertex.
 */
Vertex* Polygon::getRightmostVertex() const {
    Vertex* rightmost = nullptr;
    
    for(size_t v = 0; v < vertexes.size(); v++) {
        Vertex* v_ptr = vertexes[v];
        if(!rightmost) {
            rightmost = v_ptr;
        } else {
            rightmost = ::getRightmostVertex(v_ptr, rightmost);
        }
    }
    
    return rightmost;
}


/**
 * @brief Adds a polygon as a child of this polygon, or as a child of one of
 * its children, recursively.
 * It does this by checking if the polygon goes inside or not.
 *
 * @param p Polygon to insert.
 * @return Whether it got inserted.
 */
bool Polygon::insertChild(Polygon* p) {
    //First, check if it can be inserted in a child.
    for(size_t c = 0; c < children.size(); c++) {
        if(children[c]->insertChild(p)) return true;
    }
    
    //Check if it can be inserted in the polygon proper.
    if(!vertexes.empty()) {
        if(isPointInside(v2p(p->vertexes[0]))) {
            children.push_back(p);
            return true;
        }
    }
    
    //If this is the polygon tree root and nothing else worked, insert it here.
    if(vertexes.empty()) {
        children.push_back(p);
        return true;
    }
    
    //Can't insert.
    return false;
}


/**
 * @brief Returns whether a point is inside of the polygon.
 *
 * @param p Point to check.
 * @return Whether it is inside.
 */
bool Polygon::isPointInside(const Point &p) const {
    //http://paulbourke.net/geometry/polygonmesh/index.html#insidepoly
    
    Point p1 = v2p(vertexes[0]);
    Point p2;
    size_t nr_crossings = 0;
    
    for(size_t v = 1; v <= vertexes.size(); v++) {
        p2.x = vertexes[v % vertexes.size()]->x;
        p2.y = vertexes[v % vertexes.size()]->y;
        
        if(
            p.y > std::min(p1.y, p2.y) &&
            p.y <= std::max(p1.y, p2.y) &&
            p.x <= std::max(p1.x, p2.x) &&
            p1.y != p2.y
        ) {
            float x_inters =
                (p.y - p1.y) * (p2.x - p1.x) / (p2.y - p1.y) + p1.x;
            if(p1.x == p2.x || p.x <= x_inters) {
                nr_crossings++;
            }
        }
        
        p1 = p2;
    }
    
    return nr_crossings % 2 != 0;
}


/**
 * @brief Constructs a new triangle object.
 *
 * @param v1 First vertex.
 * @param v2 Second vertex.
 * @param v3 Third vertex.
 */
Triangle::Triangle(Vertex* v1, Vertex* v2, Vertex* v3) {
    points[0] = v1;
    points[1] = v2;
    points[2] = v3;
}


/**
 * @brief Returns the next edge the trace algorithm should go to.
 *
 * Because at each vertex there can be multiple edges, including multiple
 * edges that belong to the sector we are looking for, we should pick
 * the next edge carefully.
 * Based on information from the previous edge, we should continue travelling
 * via the edge with the smallest angle difference (depending on the rotation
 * direction we're heading.
 *
 * @param v_ptr Vertex to check.
 * @param prev_v_ptr Vertex that we came from, if any.
 * Used to ensure we don't go backwards.
 * @param s_ptr Sector we are trying to trace.
 * @param prev_e_angle Angle of the previous edge.
 * This is the angle from the previous vertex to the current vertex,
 * so it's sent here cached for performance.
 * @param best_is_closest_cw True if we want the edge that is closest clockwise
 * from the previous edge. False for the closest counterclockwise.
 * @param next_e_ptr The next edge is returned here.
 * If there is none, nullptr is returned.
 * @param next_e_angle The next edge's angle is returned here.
 * This is used to feed the next iteration of the algorithm so it
 * doesn't need to re-calculate the angle.
 * @param next_v_ptr Opposing vertex of the next edge.
 * @param excluded_edges List of edges that must not be checked, if any.
 */
void findTraceEdge(
    Vertex* v_ptr, const Vertex* prev_v_ptr, const Sector* s_ptr,
    float prev_e_angle, bool best_is_closest_cw,
    Edge** next_e_ptr, float* next_e_angle, Vertex** next_v_ptr,
    unordered_set<Edge*>* excluded_edges
) {
    //Info about the best candidate edge, if any.
    Edge* best_e_ptr = nullptr;
    float best_e_angle = 0;
    float best_e_angle_cw_dif = 0;
    Vertex* best_v_ptr = nullptr;
    
    //Go through each edge to check for the best.
    for(size_t e = 0; e < v_ptr->edges.size(); e++) {
    
        Edge* e_ptr = v_ptr->edges[e];
        
        if(e_ptr->sectors[0] != s_ptr && e_ptr->sectors[1] != s_ptr) {
            //This edge is not related to our sector.
            continue;
        }
        if(excluded_edges && isInContainer(*excluded_edges, e_ptr)) {
            //This edge is not meant to be checked.
            continue;
        }
        
        Vertex* other_v_ptr = e_ptr->getOtherVertex(v_ptr);
        
        if(other_v_ptr == prev_v_ptr) {
            //This is where we came from.
            continue;
        }
        
        //Find this edge's angle,
        //between our vertex and this edge's other vertex.
        float e_angle =
            getAngle(v2p(v_ptr), v2p(other_v_ptr));
            
        float angle_cw_dif =
            getAngleCwDiff(prev_e_angle + TAU / 2.0f, e_angle);
            
        //Check if this is the best.
        if(
            !best_e_ptr ||
            (best_is_closest_cw && angle_cw_dif < best_e_angle_cw_dif) ||
            (!best_is_closest_cw && angle_cw_dif > best_e_angle_cw_dif)
        ) {
            best_e_ptr = e_ptr;
            best_e_angle_cw_dif = angle_cw_dif;
            best_e_angle = e_angle;
            best_v_ptr = other_v_ptr;
        }
    }
    
    //Return our result.
    *next_e_ptr = best_e_ptr;
    *next_e_angle = best_e_angle;
    *next_v_ptr = best_v_ptr;
}


/**
 * @brief Get the convex, concave and ear vertexes.
 *
 * @param vertexes_left List of vertexes left to be processed.
 * @param ears List of ears found.
 * @param convex_vertexes List of convex vertexes found.
 * @param concave_vertexes List of concave vertexes found.
 */
void getCCE(
    const vector<Vertex*> &vertexes_left, vector<size_t> &ears,
    vector<size_t> &convex_vertexes, vector<size_t> &concave_vertexes
) {
    ears.clear();
    convex_vertexes.clear();
    concave_vertexes.clear();
    for(size_t v = 0; v < vertexes_left.size(); v++) {
        bool is_convex = isVertexConvex(vertexes_left, v);
        if(is_convex) {
            convex_vertexes.push_back(v);
            
        } else {
            concave_vertexes.push_back(v);
        }
    }
    
    for(size_t c = 0; c < convex_vertexes.size(); c++) {
        if(isVertexEar(vertexes_left, concave_vertexes, convex_vertexes[c])) {
            ears.push_back(convex_vertexes[c]);
        }
    }
}


/**
 * @brief Returns all vertexes that are close enough to be merged with
 * the specified point, as well as their distances to said point.
 *
 * @param pos Coordinates of the point.
 * @param all_vertexes Vector with all of the vertexes in the area.
 * @param merge_radius Minimum radius to merge.
 * This does not take the camera zoom level into account.
 * @return The merge vertexes.
 */
vector<std::pair<Distance, Vertex*> > getMergeVertexes(
    const Point &pos, const vector<Vertex*> &all_vertexes,
    float merge_radius
) {

    vector<std::pair<Distance, Vertex*> > result;
    for(size_t v = 0; v < all_vertexes.size(); v++) {
        Vertex* v_ptr = all_vertexes[v];
        
        Distance d(pos, v2p(v_ptr));
        if(d <= merge_radius) {
            result.push_back(std::make_pair(d, v_ptr));
        }
    }
    
    return result;
}


/**
 * @brief Returns the polygons of a sector.
 *
 * Polygons can include child polygons.
 * Outer polygons are all the ones that contain the sector inside, and inner
 * polygons do not contain the sector inside. (In theory, since in practice
 * an inner polygon could contain another outer polygon inside.)
 * The vertexes are ordered counterclockwise for the outer polygons,
 * and clockwise for the inner ones.
 *
 * @param s_ptr Pointer to the sector.
 * @param polys Return the polygons here.
 * @return An error code.
 */
TRIANGULATION_ERROR getPolys(
    Sector* s_ptr, Polygon* polys
) {
    if(!s_ptr || !polys) return TRIANGULATION_ERROR_INVALID_ARGS;
    TRIANGULATION_ERROR result = TRIANGULATION_ERROR_NONE;
    
    bool doing_first_polygon = true;
    
    //First, compile a list of all edges related to this sector.
    unordered_set<Edge*> edges_left(s_ptr->edges.begin(), s_ptr->edges.end());
    unordered_set<Edge*> polygon_edges_so_far;
    
    //Now trace along the edges, vertex by vertex, until we have no more left.
    while(!edges_left.empty()) {
    
        //Start with the rightmost vertex.
        Vertex* first_v_ptr = getRightmostVertex(edges_left);
        
        //Figure out if the polygon we are going to trace is an outer polygon
        //or an inner one.
        Polygon* new_poly = new Polygon();
        bool is_outer =
            getPolysIsOuter(
                first_v_ptr, s_ptr, edges_left, doing_first_polygon
            );
            
        //Trace! For the outer poly, we're going counterclockwise,
        //while for the inner ones, it's clockwise.
        TRIANGULATION_ERROR trace_result =
            traceEdges(
                first_v_ptr, s_ptr, !is_outer,
                &new_poly->vertexes,
                &edges_left, &polygon_edges_so_far
            );
            
        //Add this polygon to the polygon tree.
        bool inserted = false;
        if(trace_result == TRIANGULATION_ERROR_NONE) {
            inserted = polys->insertChild(new_poly);
        } else {
            result = trace_result;
        }
        
        if(!inserted) {
            //Failed to insert... Clean up.
            delete new_poly;
        }
        
        doing_first_polygon = false;
    }
    
    return result;
}


/**
 * @brief Helper function that returns whether we are going to trace
 * an outer polygon or an inner polygon.
 *
 * @param v_ptr Pointer to the vertex the trace is starting on.
 * @param s_ptr Pointer to the sector we're working on.
 * @param edges_left Edges that are still remaining to get polygons from.
 * @param doing_first_polygon True if we're doing the first polygon of the
 * sector, false otherwise.
 * @return Whether it is an outer polygon.
 */
bool getPolysIsOuter(
    Vertex* v_ptr, const Sector* s_ptr, const unordered_set<Edge*> &edges_left,
    bool doing_first_polygon
) {
    if(doing_first_polygon) {
        //If we're working on the first polygon, then it's mandatorily an
        //outer polygon, since we always start with the rightmost vertex.
        return true;
    }
    
    //First, from the starting vertex (rightmost vertex available),
    //imagine an arrow pointing straight right. Obviously no other vertex of
    //the sector can be this way. But let's start rotating the arrow clockwise
    //along the vertex's edges and find the one closest.
    Edge* closest_edge_cw = nullptr;
    float closest_edge_cw_angle = FLT_MAX;
    
    for(size_t e = 0; e < v_ptr->edges.size(); e++) {
    
        Edge* e_ptr = v_ptr->edges[e];
        if(e_ptr->sectors[0] != s_ptr && e_ptr->sectors[1] != s_ptr) {
            //This edge is irrelevant to our sector.
            continue;
        }
        if(!isInContainer(edges_left, e_ptr)) {
            //This edge was already processed.
            continue;
        }
        
        Vertex* e_other_v_ptr = e_ptr->getOtherVertex(v_ptr);
        float edge_angle = getAngle(v2p(v_ptr), v2p(e_other_v_ptr));
        float edge_cw_angle = getAngleCwDiff(0.0f, edge_angle);
        if(!closest_edge_cw || edge_cw_angle < closest_edge_cw_angle) {
            closest_edge_cw = e_ptr;
            closest_edge_cw_angle = edge_cw_angle;
        }
        
    }
    
    if(!closest_edge_cw) return false;
    
    //With the closest clockwise edge, we just need to check to which side our
    //sector is. If we stand on our vertex and face the edge's other vertex,
    //our sector being on the right means it's inside the shape, so an outer
    //polygon. Otherwise, it's outside the shape and this is an inner polygon.
    if(closest_edge_cw->sectors[0] == s_ptr) {
        if(closest_edge_cw->vertexes[0] == v_ptr) {
            //Our sector's to the left.
            return false;
        } else {
            //Our sector's to the right.
            return true;
        }
    } else {
        if(closest_edge_cw->vertexes[0] == v_ptr) {
            //Our sector's to the right.
            return true;
        } else {
            //Our sector's to the left.
            return false;
        }
    }
}


/**
 * @brief Returns the vertex farthest to the right in a list of edges.
 *
 * @param edges Edges to check.
 * @return The vertex.
 */
Vertex* getRightmostVertex(const unordered_set<Edge*> &edges) {
    Vertex* rightmost = nullptr;
    
    for(auto &e : edges) {
        if(!rightmost) rightmost = e->vertexes[0];
        
        for(unsigned char v = 0; v < 2; v++) {
            rightmost = getRightmostVertex(e->vertexes[v], rightmost);
        }
    }
    
    return rightmost;
}


/**
 * @brief Returns the vertex farthest to the right between the two.
 *
 * In the case of a tie, the highest one is returned.
 * This is necessary because at one point, the rightmost
 * vertex was being decided kinda randomly.
 *
 * @param v1 First vertex to check.
 * @param v2 Second vertex to check.
 * @return The vertex.
 */
Vertex* getRightmostVertex(Vertex* v1, Vertex* v2) {
    if(v1->x > v2->x) return v1;
    if(v1->x == v2->x && v1->y < v2->y) return v1;
    return v2;
}


/**
 * @brief Returns whether a polygon was created clockwise or counterclockwise,
 * given the order of its vertexes.
 *
 * @param vertexes Vertexes to check.
 * @return Whether it is clockwise.
 */
bool isPolygonClockwise(vector<Vertex*> &vertexes) {
    //Solution by http://stackoverflow.com/a/1165943
    float sum = 0;
    for(size_t v = 0; v < vertexes.size(); v++) {
        Vertex* v_ptr = vertexes[v];
        Vertex* v2_ptr = getNextInVector(vertexes, v);
        sum += (v2_ptr->x - v_ptr->x) * (v2_ptr->y + v_ptr->y);
    }
    return sum < 0;
}


/**
 * @brief Returns whether this vertex is convex or not.
 *
 * @param vec List of all vertexes.
 * @param idx Index of the vertex to check.
 * @return Whether it is convex.
 */
bool isVertexConvex(const vector<Vertex*> &vec, size_t idx) {
    const Vertex* cur_v = vec[idx];
    const Vertex* prev_v = getPrevInVector(vec, idx);
    const Vertex* next_v = getNextInVector(vec, idx);
    float angle_prev = getAngle(v2p(cur_v), v2p(prev_v));
    float angle_next = getAngle(v2p(cur_v), v2p(next_v));
    return getAngleCwDiff(angle_prev, angle_next) < TAU / 2;
}


/**
 * @brief Returns whether this vertex is an ear or not.
 *
 * @param vec List of all vertexes.
 * @param concaves List of concave vertexes.
 * @param idx Index of the vertex to check.
 * @return Whether it is an ear.
 */
bool isVertexEar(
    const vector<Vertex*> &vec, const vector<size_t> &concaves, size_t idx
) {
    //A vertex is an ear if the triangle of it, the previous, and next vertexes
    //does not contain any other vertex inside. Also, if it has vertexes inside,
    //they mandatorily are concave, so only check those.
    const Vertex* v = vec[idx];
    const Vertex* pv = getPrevInVector(vec, idx);
    const Vertex* nv = getNextInVector(vec, idx);
    
    for(size_t c = 0; c < concaves.size(); c++) {
        const Vertex* v_to_check = vec[concaves[c]];
        if(v_to_check == v || v_to_check == pv || v_to_check == nv) continue;
        if(
            isPointInTriangle(
                v2p(v_to_check), v2p(pv), v2p(v), v2p(nv), true
            )
        ) return false;
    }
    
    return true;
}


/**
 * @brief Traces edges until it returns to the start, at which point it
 * closes a polygon.
 *
 * @param start_v_ptr Vertex to start on.
 * @param s_ptr Sector to trace around.
 * @param going_cw True if the travel direction should be clockwise,
 * false for counterclockwise.
 * @param vertexes The final list of vertexes is returned here.
 * @param unvisited_edges List of edges that have not been visited,
 * so the algorithm can remove them from the list as it visits them.
 * @param polygon_edges_so_far List of edges that have already been added
 * to the polygon so far.
 * @return An error code.
 */
TRIANGULATION_ERROR traceEdges(
    Vertex* start_v_ptr, const Sector* s_ptr, bool going_cw,
    vector<Vertex*>* vertexes,
    unordered_set<Edge*>* unvisited_edges,
    unordered_set<Edge*>* polygon_edges_so_far
) {
    if(!start_v_ptr || !s_ptr) return TRIANGULATION_ERROR_INVALID_ARGS;
    
    Vertex* v_ptr = start_v_ptr;
    unordered_set<Edge*> polygon_edges;
    
    //At the start, no need to check if we're going to the previous vertex.
    Vertex* prev_v_ptr = nullptr;
    //At the start, assume the angle is left.
    float prev_e_angle = TAU / 2.0f;
    
    Edge* next_e_ptr = nullptr;
    Vertex* next_v_ptr = nullptr;
    float next_e_angle = 0.0f;
    
    Edge* first_e_ptr = nullptr;
    
    TRIANGULATION_ERROR result = TRIANGULATION_ERROR_NONE;
    bool poly_done = false;
    
    //Trace around, vertex by vertex, until we're done.
    while(!poly_done) {
    
        //Find the next edge to go to.
        //For cases where the vertex only has two edges of our sector,
        //it's pretty obvious -- just go to the edge that isn't the one we
        //came from. But if the vertex has more edges, we need to pick based
        //on the angle and what we're trying to do. There are two approaches:
        //
        //            Turn inward           |           Turn outward
        //----------------------------------+----------------------------------
        //You can think of it like you're   |Same, but the cane must stay on
        //walking on top of the lines, and  |the sector/s that are outside of
        //you have a cane dragging on the   |the trace direction.
        //floor to your left or right.      |
        //This cane must stay on the        |
        //sector/s that are inside of the   |
        //trace direction.                  |
        //----------------------------------+----------------------------------
        //With this you traverse the shape  |With this you traverse the shape
        //as deeply as possible, and enter  |as broadly as possible, and you
        //and close loops as soon as        |don't enter any loop.
        //possible.                         |
        //----------------------------------+----------------------------------
        //For outer polygons, this works    |For outer polygons, this will
        //fine and can even be used to      |skip out on geometry inside shared
        //detect islands.                   |vertexes (see fig. A).
        //----------------------------------+----------------------------------
        //For inner polygons, this works,   |For inner polygons the broad shape
        //but the other method is better.   |is more convenient.
        //----------------------------------+----------------------------------
        //So basically this is the best     |Basically best suited for inner
        //option for outer polygons. This   |polygons since when we land on
        //way when we land on a shared      |a reused vertex, we can skip over
        //vertex, we don't skip over parts  |loops and just get the final shape,
        //of the outer polygon's geometry.  |instead of obtaining multiple
        //e.g. see fig. A.                  |inner polygons.
        //                                  |e.g. see fig. B.
        //----------------------------------+----------------------------------
        //You want the edge closest to you  |You want the edge closest to you
        //in the opposite orientation to    |in the same orientation as
        //your direction of travel (from the|your direction of travel (from the
        //shared vertex). i.e. closest      |shared vertex). i.e. closest
        //clockwise if you're traversing the|clockwise if you're traversing the
        //edges counterclockwise, closest   |edges clockwise, closest
        //counterclockwise if you're        |counterclockwise if you're
        //traversing clockwise.             |traversing counterclockwise.
        //
        //Fig. A.
        //  +--------+
        //  |   +--+ |  1 = Sector 1
        //  |   |   \|  2 = Sector 2
        //  | 1 | 2  +
        //  |   |   /|
        //  |   +--+ |
        //  +--------+
        //
        //Fig. B.
        //  +---------+
        //  |    +--+ |  1 = Sector 1
        //  | 1  |2/  |  2 = Sector 2
        //  |    |/   |  3 = Sector 3
        //  | +--+    |
        //  |  \3|    |
        //  |   \|    |
        //  |    +    |
        //  +---------+
        //
        //With all of that said, the first iteration, where we find the first
        //edge, needs to be selected according to what sort of polygon we're
        //tracing. Counterclockwise for outer, clockwise for inner. The edge
        //we pick from the starting vertex will dictate the direction of travel.
        //So for outer polygons, we want to start by picking the closest
        //counterclockwise edge, so we can set the trace orientation to
        //counterclockwise, and then swap over to picking the closest
        //clockwise so we can turn inward.
        //For inner polygons, start with the closest clockwise edge so we
        //trace clockwise, then continue that way so we turn outward.
        
        bool best_is_closest_cw = going_cw;
        if(prev_v_ptr != nullptr) best_is_closest_cw = true;
        
        findTraceEdge(
            v_ptr, prev_v_ptr, s_ptr, prev_e_angle, best_is_closest_cw,
            &next_e_ptr, &next_e_angle, &next_v_ptr, polygon_edges_so_far
        );
        
        //Now that we have the edge, what do we do?
        if(!next_e_ptr) {
            //If there is no edge to go to next, this sector is not closed.
            result = TRIANGULATION_ERROR_NOT_CLOSED;
            poly_done = true;
            
        } else if(next_e_ptr == first_e_ptr) {
            //If we already did this edge, that's it, polygon closed.
            poly_done = true;
            
        } else {
            //Part of the trace.
            vertexes->push_back(v_ptr);
            prev_e_angle = next_e_angle;
            prev_v_ptr = v_ptr;
            v_ptr = next_v_ptr;
            
        }
        
        //Finishing setup before the next iteration.
        if(!first_e_ptr) {
            first_e_ptr = next_e_ptr;
        }
        if(next_e_ptr) {
            unvisited_edges->erase(next_e_ptr);
            polygon_edges.insert(next_e_ptr);
        }
        
    }
    
    polygon_edges_so_far->insert(polygon_edges.begin(), polygon_edges.end());
    
    return result;
}


/**
 * @brief Triangulates a polygon via the Triangulation by Ear Clipping
 * algorithm.
 *
 * http://www.geometrictools.com/Documentation/TriangulationByEarClipping.pdf
 *
 * @param poly The polygon to triangulate.
 * @param triangles The final list of triangles is returned here.
 * @return An error code.
 */
TRIANGULATION_ERROR triangulatePolygon(
    Polygon* poly, vector<Triangle>* triangles
) {

    TRIANGULATION_ERROR result = TRIANGULATION_ERROR_NONE;
    vector<Vertex*> vertexes_left = poly->vertexes;
    vector<size_t> ears;
    vector<size_t> convex_vertexes;
    vector<size_t> concave_vertexes;
    
    if(vertexes_left.size() > 3 && triangles->empty()) {
        triangles->reserve(vertexes_left.size() - 2);
    }
    
    //Begin by making a list of all concave, convex, and ear vertexes.
    getCCE(vertexes_left, ears, convex_vertexes, concave_vertexes);
    
    //We do the triangulation until we're left
    //with three vertexes -- the final triangle.
    while(vertexes_left.size() > 3) {
    
        if(ears.empty()) {
            //Something went wrong, the polygon mightn't be simple.
            result = TRIANGULATION_ERROR_NO_EARS;
            break;
            
        } else {
            //The ear, the previous, and the next vertexes make a triangle.
            triangles->push_back(
                Triangle(
                    vertexes_left[ears[0]],
                    getPrevInVector(vertexes_left, ears[0]),
                    getNextInVector(vertexes_left, ears[0])
                )
            );
            
            //Remove the ear.
            vertexes_left.erase(vertexes_left.begin() + ears[0]);
            
            //Recalculate the ears, concave, and convex vertexes.
            getCCE(vertexes_left, ears, convex_vertexes, concave_vertexes);
        }
    }
    
    //Finally, add the final triangle.
    if(vertexes_left.size() == 3) {
        triangles->push_back(
            Triangle(
                vertexes_left[1], vertexes_left[0], vertexes_left[2]
            )
        );
    }
    
    return result;
}


/**
 * @brief Triangulates (turns into triangles) a sector.
 *
 * We need to do this because drawing concave polygons is not possible.
 *
 * @param s_ptr Pointer to the sector.
 * @param lone_edges Return lone edges found here.
 * @param clear_lone_edges Clear this sector's edges from the list of
 * lone edges, if they are there.
 * @return An error code.
 */
TRIANGULATION_ERROR triangulateSector(
    Sector* s_ptr, set<Edge*>* lone_edges, bool clear_lone_edges
) {
    //Root of the polygon tree.
    Polygon root;
    
    //Let's clear any "lone" edges here.
    if(clear_lone_edges) {
        for(size_t e = 0; e < s_ptr->edges.size(); e++) {
            auto it = lone_edges->find(s_ptr->edges[e]);
            if(it != lone_edges->end()) {
                lone_edges->erase(it);
            }
        }
    }
    
    
    //------------------------------
    //Step 1. Get polygons.
    //We need to know what vertexes mark the outermost polygon,
    //and what vertexes mark the inner ones.
    //Because there can be islands or polygons of our sector inside some inner
    //ones, we need a polygon tree to know what's inside of what.
    //Example of a sector's polygons:
    /*
     * +-------+     +-----------+  +-----+
     * | OUTER  \    |           |  |OUTER \
     * |         +---+           |  +-------+
     * |   +----+                |
     * |  /INNER|   +----------+ |
     * | +------+   |          | |
     * +---+    +---+  +-----+ | |
     *     |   /INNER  |OUTER| | |
     *     |  /        +-----+ | |
     *     | +-----------------+ |
     *     +---------------------+
     */
    
    TRIANGULATION_ERROR result = getPolys(s_ptr, &root);
    if(result != TRIANGULATION_ERROR_NONE) {
        root.destroy();
        return result;
    }
    
    //Get rid of 0-length edges and 180-degree vertexes,
    //as they're redundant. Do this recursively for all.
    root.clean(true);
    
    
    //------------------------------
    //Step 2. Make cuts.
    //Make cuts on the outer polygons between where it and inner polygons exist,
    //as to make each outer polygon one big holeless polygon.
    //Example of the process:
    /*
     * +-----------+    +-----------+
     * | OUTER     |    |           |
     * |           |    |           |
     * |  +-----+  |    |  +-----+--+ <--- 0-width gap
     * |  |INNER|  | -> |  |     +--+ <-´
     * |  |     |  |    |  |     |  |
     * |  +-----+  |    |  +-----+  |
     * |           |    |           |
     * +-----------+    +-----------+
     */
    root.cutAllAsRoot();
    
    
    //------------------------------
    //Step 3. Triangulate the polygons.
    //Transforming the polygons into triangles.
    s_ptr->triangles.clear();
    for(size_t p = 0; p < root.children.size(); p++) {
        TRIANGULATION_ERROR poly_result =
            triangulatePolygon(root.children[p], &s_ptr->triangles);
        if(poly_result != TRIANGULATION_ERROR_NONE) {
            result = poly_result;
        }
    }
    
    //Done!
    root.destroy();
    return result;
}
