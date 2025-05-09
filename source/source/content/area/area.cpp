/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Area class and related functions.
 */

#include <algorithm>
#include <vector>

#include "area.h"

#include "../../core/misc_functions.h"
#include "../../core/game.h"
#include "../../util/allegro_utils.h"
#include "../../util/string_utils.h"


using std::size_t;
using std::vector;


namespace AREA {

//Default day time speed, in game-minutes per real-minutes.
const float DEF_DAY_TIME_SPEED = 120;

//Default day time at the start of gameplay, in minutes.
const size_t DEF_DAY_TIME_START = 7 * 60;

//Default difficulty.
const unsigned char DEF_DIFFICULTY = 0;

}


/**
 * @brief Checks to see if all indexes match their pointers,
 * for the various edges, vertexes, etc.
 *
 * This is merely a debugging tool. Aborts execution if any of the pointers
 * don't match.
 */
void Area::checkStability() {
    for(size_t v = 0; v < vertexes.size(); v++) {
        Vertex* v_ptr = vertexes[v];
        engineAssert(
            v_ptr->edges.size() == v_ptr->edgeIdxs.size(),
            i2s(v_ptr->edges.size()) + " " + i2s(v_ptr->edgeIdxs.size())
        );
        for(size_t e = 0; e < v_ptr->edges.size(); e++) {
            engineAssert(v_ptr->edges[e] == edges[v_ptr->edgeIdxs[e]], "");
        }
    }
    
    for(size_t e = 0; e < edges.size(); e++) {
        Edge* e_ptr = edges[e];
        for(size_t v = 0; v < 2; v++) {
            engineAssert(
                e_ptr->vertexes[v] == vertexes[e_ptr->vertexIdxs[v]], ""
            );
        }
        
        for(size_t s = 0; s < 2; s++) {
            Sector* s_ptr = e_ptr->sectors[s];
            if(
                s_ptr == nullptr &&
                e_ptr->sectorIdxs[s] == INVALID
            ) {
                continue;
            }
            engineAssert(s_ptr == sectors[e_ptr->sectorIdxs[s]], "");
        }
    }
    
    for(size_t s = 0; s < sectors.size(); s++) {
        Sector* s_ptr = sectors[s];
        engineAssert(
            s_ptr->edges.size() == s_ptr->edgeIdxs.size(),
            i2s(s_ptr->edges.size()) + " " + i2s(s_ptr->edgeIdxs.size())
        );
        for(size_t e = 0; e < s_ptr->edges.size(); e++) {
            engineAssert(s_ptr->edges[e] == edges[s_ptr->edgeIdxs[e]], "");
        }
    }
}


/**
 * @brief Clears the info of an area map.
 */
void Area::clear() {
    for(size_t v = 0; v < vertexes.size(); v++) {
        delete vertexes[v];
    }
    for(size_t e = 0; e < edges.size(); e++) {
        delete edges[e];
    }
    for(size_t s = 0; s < sectors.size(); s++) {
        delete sectors[s];
    }
    for(size_t m = 0; m < mobGenerators.size(); m++) {
        delete mobGenerators[m];
    }
    for(size_t s = 0; s < pathStops.size(); s++) {
        delete pathStops[s];
    }
    for(size_t s = 0; s < treeShadows.size(); s++) {
        delete treeShadows[s];
    }
    
    vertexes.clear();
    edges.clear();
    sectors.clear();
    mobGenerators.clear();
    pathStops.clear();
    treeShadows.clear();
    bmap.clear();
    
    if(bgBmp) {
        game.content.bitmaps.list.free(bgBmp);
        bgBmp = nullptr;
    }
    if(thumbnail) {
        thumbnail = nullptr;
    }
    
    resetMetadata();
    manifest = nullptr;
    name.clear();
    type = AREA_TYPE_SIMPLE;
    subtitle.clear();
    difficulty = AREA::DEF_DIFFICULTY;
    sprayAmounts.clear();
    songName.clear();
    weatherName.clear();
    dayTimeStart = AREA::DEF_DAY_TIME_START;
    dayTimeSpeed = AREA::DEF_DAY_TIME_SPEED;
    bgBmpName.clear();
    bgColor = COLOR_BLACK;
    bgDist = 2.0f;
    bgBmpZoom = 1.0f;
    mission = MissionData();
    
    problems.nonSimples.clear();
    problems.loneEdges.clear();
}


/**
 * @brief Cleans up redundant data and such.
 *
 * @param out_deleted_sectors If not nullptr, whether any sectors got deleted
 * is returned here.
 */
void Area::cleanup(bool* out_deleted_sectors) {
    //Get rid of unused sectors.
    bool deleted_sectors = false;
    for(size_t s = 0; s < sectors.size(); ) {
        if(sectors[s]->edges.empty()) {
            removeSector(s);
            deleted_sectors = true;
        } else {
            s++;
        }
    }
    if(out_deleted_sectors) *out_deleted_sectors = deleted_sectors;
    
    //And some other cleanup.
    if(songName == NONE_OPTION) {
        songName.clear();
    }
    if(weatherName == NONE_OPTION) {
        weatherName.clear();
    }
    engineVersion = getEngineVersionString();
}


/**
 * @brief Clones this area data into another Area object.
 *
 * @param other The area data object to clone to.
 */
void Area::clone(Area &other) {
    other.clear();
    
    if(!other.bgBmpName.empty() && other.bgBmp) {
        game.content.bitmaps.list.free(other.bgBmpName);
    }
    other.bgBmpName = bgBmpName;
    if(other.bgBmpName.empty()) {
        other.bgBmp = nullptr;
    } else {
        other.bgBmp = game.content.bitmaps.list.get(bgBmpName, nullptr, false);
    }
    other.bgBmpZoom = bgBmpZoom;
    other.bgColor = bgColor;
    other.bgDist = bgDist;
    other.bmap = bmap;
    
    other.vertexes.reserve(vertexes.size());
    for(size_t v = 0; v < vertexes.size(); v++) {
        other.vertexes.push_back(new Vertex());
    }
    other.edges.reserve(edges.size());
    for(size_t e = 0; e < edges.size(); e++) {
        other.edges.push_back(new Edge());
    }
    other.sectors.reserve(sectors.size());
    for(size_t s = 0; s < sectors.size(); s++) {
        other.sectors.push_back(new Sector());
    }
    other.mobGenerators.reserve(mobGenerators.size());
    for(size_t m = 0; m < mobGenerators.size(); m++) {
        other.mobGenerators.push_back(new MobGen());
    }
    other.pathStops.reserve(pathStops.size());
    for(size_t s = 0; s < pathStops.size(); s++) {
        other.pathStops.push_back(new PathStop());
    }
    other.treeShadows.reserve(treeShadows.size());
    for(size_t t = 0; t < treeShadows.size(); t++) {
        other.treeShadows.push_back(new TreeShadow());
    }
    
    for(size_t v = 0; v < vertexes.size(); v++) {
        Vertex* v_ptr = vertexes[v];
        Vertex* ov_ptr = other.vertexes[v];
        ov_ptr->x = v_ptr->x;
        ov_ptr->y = v_ptr->y;
        ov_ptr->edges.reserve(v_ptr->edges.size());
        ov_ptr->edgeIdxs.reserve(v_ptr->edgeIdxs.size());
        for(size_t e = 0; e < v_ptr->edges.size(); e++) {
            size_t nr = v_ptr->edgeIdxs[e];
            ov_ptr->edges.push_back(other.edges[nr]);
            ov_ptr->edgeIdxs.push_back(nr);
        }
    }
    
    for(size_t e = 0; e < edges.size(); e++) {
        Edge* e_ptr = edges[e];
        Edge* oe_ptr = other.edges[e];
        oe_ptr->vertexes[0] = other.vertexes[e_ptr->vertexIdxs[0]];
        oe_ptr->vertexes[1] = other.vertexes[e_ptr->vertexIdxs[1]];
        oe_ptr->vertexIdxs[0] = e_ptr->vertexIdxs[0];
        oe_ptr->vertexIdxs[1] = e_ptr->vertexIdxs[1];
        if(e_ptr->sectorIdxs[0] == INVALID) {
            oe_ptr->sectors[0] = nullptr;
        } else {
            oe_ptr->sectors[0] = other.sectors[e_ptr->sectorIdxs[0]];
        }
        if(e_ptr->sectorIdxs[1] == INVALID) {
            oe_ptr->sectors[1] = nullptr;
        } else {
            oe_ptr->sectors[1] = other.sectors[e_ptr->sectorIdxs[1]];
        }
        oe_ptr->sectorIdxs[0] = e_ptr->sectorIdxs[0];
        oe_ptr->sectorIdxs[1] = e_ptr->sectorIdxs[1];
        e_ptr->clone(oe_ptr);
    }
    
    for(size_t s = 0; s < sectors.size(); s++) {
        Sector* s_ptr = sectors[s];
        Sector* os_ptr = other.sectors[s];
        s_ptr->clone(os_ptr);
        os_ptr->textureInfo.bmpName = s_ptr->textureInfo.bmpName;
        os_ptr->textureInfo.bitmap =
            game.content.bitmaps.list.get(s_ptr->textureInfo.bmpName, nullptr, false);
        os_ptr->edges.reserve(s_ptr->edges.size());
        os_ptr->edgeIdxs.reserve(s_ptr->edgeIdxs.size());
        for(size_t e = 0; e < s_ptr->edges.size(); e++) {
            size_t nr = s_ptr->edgeIdxs[e];
            os_ptr->edges.push_back(other.edges[nr]);
            os_ptr->edgeIdxs.push_back(nr);
        }
        os_ptr->triangles.reserve(s_ptr->triangles.size());
        for(size_t t = 0; t < s_ptr->triangles.size(); t++) {
            Triangle* t_ptr = &s_ptr->triangles[t];
            os_ptr->triangles.push_back(
                Triangle(
                    other.vertexes[findVertexIdx(t_ptr->points[0])],
                    other.vertexes[findVertexIdx(t_ptr->points[1])],
                    other.vertexes[findVertexIdx(t_ptr->points[2])]
                )
            );
        }
        os_ptr->bbox[0] = s_ptr->bbox[0];
        os_ptr->bbox[1] = s_ptr->bbox[1];
    }
    
    for(size_t m = 0; m < mobGenerators.size(); m++) {
        MobGen* m_ptr = mobGenerators[m];
        MobGen* om_ptr = other.mobGenerators[m];
        m_ptr->clone(om_ptr);
    }
    for(size_t m = 0; m < mobGenerators.size(); m++) {
        MobGen* om_ptr = other.mobGenerators[m];
        for(size_t l = 0; l < om_ptr->linkIdxs.size(); l++) {
            om_ptr->links.push_back(
                other.mobGenerators[om_ptr->linkIdxs[l]]
            );
        }
    }
    
    for(size_t s = 0; s < pathStops.size(); s++) {
        PathStop* s_ptr = pathStops[s];
        PathStop* os_ptr = other.pathStops[s];
        os_ptr->pos = s_ptr->pos;
        s_ptr->clone(os_ptr);
        os_ptr->links.reserve(s_ptr->links.size());
        for(size_t l = 0; l < s_ptr->links.size(); l++) {
            PathLink* new_link =
                new PathLink(
                os_ptr,
                other.pathStops[s_ptr->links[l]->endIdx],
                s_ptr->links[l]->endIdx
            );
            s_ptr->links[l]->clone(new_link);
            new_link->distance = s_ptr->links[l]->distance;
            os_ptr->links.push_back(new_link);
        }
    }
    
    for(size_t t = 0; t < treeShadows.size(); t++) {
        TreeShadow* t_ptr = treeShadows[t];
        TreeShadow* ot_ptr = other.treeShadows[t];
        ot_ptr->alpha = t_ptr->alpha;
        ot_ptr->angle = t_ptr->angle;
        ot_ptr->center = t_ptr->center;
        ot_ptr->bmpName = t_ptr->bmpName;
        ot_ptr->size = t_ptr->size;
        ot_ptr->sway = t_ptr->sway;
        ot_ptr->bitmap = game.content.bitmaps.list.get(t_ptr->bmpName, nullptr, false);
    }
    
    other.manifest = manifest;
    other.type = type;
    other.name = name;
    other.subtitle = subtitle;
    other.description = description;
    other.tags = tags;
    other.difficulty = difficulty;
    other.maker = maker;
    other.version = version;
    other.makerNotes = makerNotes;
    other.sprayAmounts = sprayAmounts;
    other.songName = songName;
    other.weatherName = weatherName;
    other.weatherCondition = weatherCondition;
    other.dayTimeStart = dayTimeStart;
    other.dayTimeSpeed = dayTimeSpeed;
    
    other.thumbnail = thumbnail;
    
    other.mission.goal = mission.goal;
    other.mission.goalAllMobs = mission.goalAllMobs;
    other.mission.goalMobIdxs = mission.goalMobIdxs;
    other.mission.goalAmount = mission.goalAmount;
    other.mission.goalExitCenter = mission.goalExitCenter;
    other.mission.goalExitSize = mission.goalExitSize;
    other.mission.failConditions = mission.failConditions;
    other.mission.failTooFewPikAmount = mission.failTooFewPikAmount;
    other.mission.failTooManyPikAmount = mission.failTooManyPikAmount;
    other.mission.failPikKilled = mission.failPikKilled;
    other.mission.failLeadersKod = mission.failLeadersKod;
    other.mission.failEnemiesKilled = mission.failEnemiesKilled;
    other.mission.failTimeLimit = mission.failTimeLimit;
    other.mission.gradingMode = mission.gradingMode;
    other.mission.pointsPerPikminBorn = mission.pointsPerPikminBorn;
    other.mission.pointsPerPikminDeath = mission.pointsPerPikminDeath;
    other.mission.pointsPerSecLeft = mission.pointsPerSecLeft;
    other.mission.pointsPerSecPassed = mission.pointsPerSecPassed;
    other.mission.pointsPerTreasurePoint = mission.pointsPerTreasurePoint;
    other.mission.pointsPerEnemyPoint = mission.pointsPerEnemyPoint;
    other.mission.enemyPointsOnCollection = mission.enemyPointsOnCollection;
    other.mission.pointLossData = mission.pointLossData;
    other.mission.pointHudData = mission.pointHudData;
    other.mission.startingPoints = mission.startingPoints;
    other.mission.bronzeReq = mission.bronzeReq;
    other.mission.silverReq = mission.silverReq;
    other.mission.goldReq = mission.goldReq;
    other.mission.platinumReq = mission.platinumReq;
    other.mission.makerRecord = mission.makerRecord;
    other.mission.makerRecordDate = mission.makerRecordDate;
    
    other.problems.nonSimples.clear();
    other.problems.loneEdges.clear();
    other.problems.loneEdges.reserve(problems.loneEdges.size());
    for(const auto &s : problems.nonSimples) {
        size_t nr = findSectorIdx(s.first);
        other.problems.nonSimples[other.sectors[nr]] = s.second;
    }
    for(const Edge* e : problems.loneEdges) {
        size_t nr = findEdgeIdx(e);
        other.problems.loneEdges.insert(other.edges[nr]);
    }
}


/**
 * @brief Connects an edge to a sector.
 *
 * This adds the sector and its index to the edge's
 * lists, and adds the edge and its index to the sector's.
 *
 * @param e_ptr Edge to connect.
 * @param s_ptr Sector to connect.
 * @param side Which of the sides of the edge the sector goes to.
 */
void Area::connectEdgeToSector(
    Edge* e_ptr, Sector* s_ptr, size_t side
) {
    if(e_ptr->sectors[side]) {
        e_ptr->sectors[side]->removeEdge(e_ptr);
    }
    e_ptr->sectors[side] = s_ptr;
    e_ptr->sectorIdxs[side] = findSectorIdx(s_ptr);
    if(s_ptr) {
        s_ptr->addEdge(e_ptr, findEdgeIdx(e_ptr));
    }
}


/**
 * @brief Connects an edge to a vertex.
 *
 * This adds the vertex and its index to the edge's
 * lists, and adds the edge and its index to the vertex's.
 *
 * @param e_ptr Edge to connect.
 * @param v_ptr Vertex to connect.
 * @param endpoint Which of the edge endpoints the vertex goes to.
 */
void Area::connectEdgeToVertex(
    Edge* e_ptr, Vertex* v_ptr, size_t endpoint
) {
    if(e_ptr->vertexes[endpoint]) {
        e_ptr->vertexes[endpoint]->removeEdge(e_ptr);
    }
    e_ptr->vertexes[endpoint] = v_ptr;
    e_ptr->vertexIdxs[endpoint] = findVertexIdx(v_ptr);
    v_ptr->addEdge(e_ptr, findEdgeIdx(e_ptr));
}



/**
 * @brief Connects the edges of a sector that link to it into the
 * edge_idxs vector.
 *
 * @param s_ptr The sector.
 */
void Area::connectSectorEdges(Sector* s_ptr) {
    s_ptr->edgeIdxs.clear();
    for(size_t e = 0; e < edges.size(); e++) {
        Edge* e_ptr = edges[e];
        if(e_ptr->sectors[0] == s_ptr || e_ptr->sectors[1] == s_ptr) {
            s_ptr->edgeIdxs.push_back(e);
        }
    }
    fixSectorPointers(s_ptr);
}


/**
 * @brief Connects the edges that link to it into the edge_idxs vector.
 *
 * @param v_ptr The vertex.
 */
void Area::connectVertexEdges(Vertex* v_ptr) {
    v_ptr->edgeIdxs.clear();
    for(size_t e = 0; e < edges.size(); e++) {
        Edge* e_ptr = edges[e];
        if(e_ptr->vertexes[0] == v_ptr || e_ptr->vertexes[1] == v_ptr) {
            v_ptr->edgeIdxs.push_back(e);
        }
    }
    fixVertexPointers(v_ptr);
}


/**
 * @brief Scans the list of edges and retrieves the index of
 * the specified edge.
 *
 * @param e_ptr Edge to find.
 * @return The index, or INVALID if not found.
 */
size_t Area::findEdgeIdx(const Edge* e_ptr) const {
    for(size_t e = 0; e < edges.size(); e++) {
        if(edges[e] == e_ptr) return e;
    }
    return INVALID;
}


/**
 * @brief Scans the list of mob generators and retrieves the index of
 * the specified mob generator.
 *
 * @param m_ptr Mob to find.
 * @return The index, or INVALID if not found.
 */
size_t Area::findMobGenIdx(const MobGen* m_ptr) const {
    for(size_t m = 0; m < mobGenerators.size(); m++) {
        if(mobGenerators[m] == m_ptr) return m;
    }
    return INVALID;
}


/**
 * @brief Scans the list of sectors and retrieves the index of
 * the specified sector.
 *
 * @param s_ptr Sector to find.
 * @return The index, or INVALID if not found.
 */
size_t Area::findSectorIdx(const Sector* s_ptr) const {
    for(size_t s = 0; s < sectors.size(); s++) {
        if(sectors[s] == s_ptr) return s;
    }
    return INVALID;
}


/**
 * @brief Scans the list of vertexes and retrieves the index of
 * the specified vertex.
 *
 * @param v_ptr Vertex to find.
 * @return The index, or INVALID if not found.
 */
size_t Area::findVertexIdx(const Vertex* v_ptr) const {
    for(size_t v = 0; v < vertexes.size(); v++) {
        if(vertexes[v] == v_ptr) return v;
    }
    return INVALID;
}


/**
 * @brief Fixes the sector and vertex indexes in an edge,
 * making them match the correct sectors and vertexes,
 * based on the existing sector and vertex pointers.
 *
 * @param e_ptr Edge to fix the indexes of.
 */
void Area::fixEdgeIdxs(Edge* e_ptr) {
    for(size_t s = 0; s < 2; s++) {
        if(!e_ptr->sectors[s]) {
            e_ptr->sectorIdxs[s] = INVALID;
        } else {
            e_ptr->sectorIdxs[s] = findSectorIdx(e_ptr->sectors[s]);
        }
    }
    
    for(size_t v = 0; v < 2; v++) {
        if(!e_ptr->vertexes[v]) {
            e_ptr->vertexIdxs[v] = INVALID;
        } else {
            e_ptr->vertexIdxs[v] = findVertexIdx(e_ptr->vertexes[v]);
        }
    }
}


/**
 * @brief Fixes the sector and vertex pointers of an edge,
 * making them point to the correct sectors and vertexes,
 * based on the existing sector and vertex indexes.
 *
 * @param e_ptr Edge to fix the pointers of.
 */
void Area::fixEdgePointers(Edge* e_ptr) {
    e_ptr->sectors[0] = nullptr;
    e_ptr->sectors[1] = nullptr;
    for(size_t s = 0; s < 2; s++) {
        size_t s_idx = e_ptr->sectorIdxs[s];
        e_ptr->sectors[s] = (s_idx == INVALID ? nullptr : sectors[s_idx]);
    }
    
    e_ptr->vertexes[0] = nullptr;
    e_ptr->vertexes[1] = nullptr;
    for(size_t v = 0; v < 2; v++) {
        size_t v_idx = e_ptr->vertexIdxs[v];
        e_ptr->vertexes[v] = (v_idx == INVALID ? nullptr : vertexes[v_idx]);
    }
}


/**
 * @brief Fixes the path stop indexes in a path stop's links,
 * making them match the correct path stops,
 * based on the existing path stop pointers.
 *
 * @param s_ptr Path stop to fix the indexes of.
 */
void Area::fixPathStopIdxs(PathStop* s_ptr) {
    for(size_t l = 0; l < s_ptr->links.size(); l++) {
        PathLink* l_ptr = s_ptr->links[l];
        l_ptr->endIdx = INVALID;
        
        if(!l_ptr->endPtr) continue;
        
        for(size_t s = 0; s < pathStops.size(); s++) {
            if(l_ptr->endPtr == pathStops[s]) {
                l_ptr->endIdx = s;
                break;
            }
        }
    }
}


/**
 * @brief Fixes the path stop pointers in a path stop's links,
 * making them point to the correct path stops,
 * based on the existing path stop indexes.
 *
 * @param s_ptr Path stop to fix the pointers of.
 */
void Area::fixPathStopPointers(PathStop* s_ptr) {
    for(size_t l = 0; l < s_ptr->links.size(); l++) {
        PathLink* l_ptr = s_ptr->links[l];
        l_ptr->endPtr = nullptr;
        
        if(l_ptr->endIdx == INVALID) continue;
        if(l_ptr->endIdx >= pathStops.size()) continue;
        
        l_ptr->endPtr = pathStops[l_ptr->endIdx];
    }
}


/**
 * @brief Fixes the edge indexes in a sector,
 * making them match the correct edges,
 * based on the existing edge pointers.
 *
 * @param s_ptr Sector to fix the indexes of.
 */
void Area::fixSectorIdxs(Sector* s_ptr) {
    s_ptr->edgeIdxs.clear();
    for(size_t e = 0; e < s_ptr->edges.size(); e++) {
        s_ptr->edgeIdxs.push_back(findEdgeIdx(s_ptr->edges[e]));
    }
}


/**
 * @brief Fixes the edge pointers in a sector,
 * making them point to the correct edges,
 * based on the existing edge indexes.
 *
 * @param s_ptr Sector to fix the pointers of.
 */
void Area::fixSectorPointers(Sector* s_ptr) {
    s_ptr->edges.clear();
    for(size_t e = 0; e < s_ptr->edgeIdxs.size(); e++) {
        size_t e_idx = s_ptr->edgeIdxs[e];
        s_ptr->edges.push_back(e_idx == INVALID ? nullptr : edges[e_idx]);
    }
}


/**
 * @brief Fixes the edge indexes in a vertex,
 * making them match the correct edges,
 * based on the existing edge pointers.
 *
 * @param v_ptr Vertex to fix the indexes of.
 */
void Area::fixVertexIdxs(Vertex* v_ptr) {
    v_ptr->edgeIdxs.clear();
    for(size_t e = 0; e < v_ptr->edges.size(); e++) {
        v_ptr->edgeIdxs.push_back(findEdgeIdx(v_ptr->edges[e]));
    }
}


/**
 * @brief Fixes the edge pointers in a vertex,
 * making them point to the correct edges,
 * based on the existing edge indexes.
 *
 * @param v_ptr Vertex to fix the pointers of.
 */
void Area::fixVertexPointers(Vertex* v_ptr) {
    v_ptr->edges.clear();
    for(size_t e = 0; e < v_ptr->edgeIdxs.size(); e++) {
        size_t e_idx = v_ptr->edgeIdxs[e];
        v_ptr->edges.push_back(e_idx == INVALID ? nullptr : edges[e_idx]);
    }
}


/**
 * @brief Generates the blockmap for the area, given the current info.
 */
void Area::generateBlockmap() {
    bmap.clear();
    
    if(vertexes.empty()) return;
    
    //First, get the starting point and size of the blockmap.
    Point min_coords = v2p(vertexes[0]);
    Point max_coords = min_coords;
    
    for(size_t v = 0; v < vertexes.size(); v++) {
        Vertex* v_ptr = vertexes[v];
        updateMinMaxCoords(min_coords, max_coords, v2p(v_ptr));
    }
    
    bmap.topLeftCorner = min_coords;
    //Add one more to the cols/rows because, suppose there's an edge at y = 256.
    //The row would be 2. In reality, the row should be 3.
    bmap.nCols =
        ceil((max_coords.x - min_coords.x) / GEOMETRY::BLOCKMAP_BLOCK_SIZE) + 1;
    bmap.nRows =
        ceil((max_coords.y - min_coords.y) / GEOMETRY::BLOCKMAP_BLOCK_SIZE) + 1;
        
    bmap.edges.assign(
        bmap.nCols, vector<vector<Edge*> >(bmap.nRows, vector<Edge*>())
    );
    bmap.sectors.assign(
        bmap.nCols, vector<unordered_set<Sector*> >(
            bmap.nRows, unordered_set<Sector*>()
        )
    );
    
    
    //Now, add a list of edges to each block.
    generateEdgesBlockmap(edges);
    
    
    /* If at this point, there's any block that's missing a sector,
     * that means we couldn't figure out the sectors due to the edges it has
     * alone. But the block still has a sector (or nullptr). So we need another
     * way to figure it out.
     * We know the following things that can speed up the process:
     * * The blocks at the edges of the blockmap have the nullptr sector as the
     *     only candidate.
     * * If a block's neighbor only has one sector, then this block has that
     *     same sector.
     * If we can't figure out the sector the easy way, then we have to use the
     * triangle method to get the sector. Using the center of the blockmap is
     * just as good a checking spot as any.
     */
    for(size_t bx = 0; bx < bmap.nCols; bx++) {
        for(size_t by = 0; by < bmap.nRows; by++) {
        
            if(!bmap.sectors[bx][by].empty()) continue;
            
            if(
                bx == 0 || by == 0 ||
                bx == bmap.nCols - 1 || by == bmap.nRows - 1
            ) {
                bmap.sectors[bx][by].insert(nullptr);
                continue;
            }
            
            if(bmap.sectors[bx - 1][by].size() == 1) {
                bmap.sectors[bx][by].insert(*bmap.sectors[bx - 1][by].begin());
                continue;
            }
            if(bmap.sectors[bx + 1][by].size() == 1) {
                bmap.sectors[bx][by].insert(*bmap.sectors[bx + 1][by].begin());
                continue;
            }
            if(bmap.sectors[bx][by - 1].size() == 1) {
                bmap.sectors[bx][by].insert(*bmap.sectors[bx][by - 1].begin());
                continue;
            }
            if(bmap.sectors[bx][by + 1].size() == 1) {
                bmap.sectors[bx][by].insert(*bmap.sectors[bx][by + 1].begin());
                continue;
            }
            
            Point corner = bmap.getTopLeftCorner(bx, by);
            corner += GEOMETRY::BLOCKMAP_BLOCK_SIZE * 0.5;
            bmap.sectors[bx][by].insert(
                getSector(corner, nullptr, false)
            );
        }
    }
}


/**
 * @brief Generates the blockmap for a set of edges.
 *
 * @param edge_list Edges to generate the blockmap around.
 */
void Area::generateEdgesBlockmap(const vector<Edge*> &edge_list) {
    for(size_t e = 0; e < edge_list.size(); e++) {
    
        //Get which blocks this edge belongs to, via bounding-box,
        //and only then thoroughly test which it is inside of.
        
        Edge* e_ptr = edge_list[e];
        Point min_coords = v2p(e_ptr->vertexes[0]);
        Point max_coords = min_coords;
        updateMinMaxCoords(min_coords, max_coords, v2p(e_ptr->vertexes[1]));
        
        size_t b_min_x = bmap.getCol(min_coords.x);
        size_t b_max_x = bmap.getCol(max_coords.x);
        size_t b_min_y = bmap.getRow(min_coords.y);
        size_t b_max_y = bmap.getRow(max_coords.y);
        
        for(size_t bx = b_min_x; bx <= b_max_x; bx++) {
            for(size_t by = b_min_y; by <= b_max_y; by++) {
            
                //Get the block's coordinates.
                Point corner = bmap.getTopLeftCorner(bx, by);
                
                //Check if the edge is inside this blockmap.
                if(
                    lineSegIntersectsRectangle(
                        corner,
                        corner + GEOMETRY::BLOCKMAP_BLOCK_SIZE,
                        v2p(e_ptr->vertexes[0]), v2p(e_ptr->vertexes[1])
                    )
                ) {
                
                    //If it is, add it and the sectors to the list.
                    bool add_edge = true;
                    if(e_ptr->sectors[0] && e_ptr->sectors[1]) {
                        //If there's no change in height, why bother?
                        if(
                            (e_ptr->sectors[0]->z == e_ptr->sectors[1]->z) &&
                            e_ptr->sectors[0]->type != SECTOR_TYPE_BLOCKING &&
                            e_ptr->sectors[1]->type != SECTOR_TYPE_BLOCKING
                        ) {
                            add_edge = false;
                        }
                    }
                    
                    if(add_edge) bmap.edges[bx][by].push_back(e_ptr);
                    
                    if(e_ptr->sectors[0] || e_ptr->sectors[1]) {
                        bmap.sectors[bx][by].insert(e_ptr->sectors[0]);
                        bmap.sectors[bx][by].insert(e_ptr->sectors[1]);
                    }
                }
            }
        }
    }
}


/**
 * @brief Returns how many path links exist in the area.
 *
 * @return The number of path links.
 */
size_t Area::getNrPathLinks() {
    size_t one_ways_found = 0;
    size_t normals_found = 0;
    for(size_t s = 0; s < pathStops.size(); s++) {
        PathStop* s_ptr = pathStops[s];
        for(size_t l = 0; l < s_ptr->links.size(); l++) {
            PathLink* l_ptr = s_ptr->links[l];
            if(l_ptr->endPtr->get_link(s_ptr)) {
                //The other stop links to this one. So it's a two-way.
                normals_found++;
            } else {
                one_ways_found++;
            }
        }
    }
    return (normals_found / 2.0f) + one_ways_found;
}


/**
 * @brief Loads the area's main data from a data node.
 *
 * @param node Data node to load from.
 * @param level Level to load at.
 */
void Area::loadMainDataFromDataNode(
    DataNode* node, CONTENT_LOAD_LEVEL level
) {
    //Content metadata.
    loadMetadataFromDataNode(node);
    
    //Area configuration data.
    ReaderSetter rs(node);
    DataNode* weather_node = nullptr;
    DataNode* song_node = nullptr;
    
    rs.set("subtitle", subtitle);
    rs.set("difficulty", difficulty);
    rs.set("spray_amounts", sprayAmounts);
    rs.set("song", songName, &song_node);
    rs.set("weather", weatherName, &weather_node);
    rs.set("day_time_start", dayTimeStart);
    rs.set("day_time_speed", dayTimeSpeed);
    rs.set("bg_bmp", bgBmpName);
    rs.set("bg_color", bgColor);
    rs.set("bg_dist", bgDist);
    rs.set("bg_zoom", bgBmpZoom);
    
    //Weather.
    if(level > CONTENT_LOAD_LEVEL_BASIC) {
        if(weatherName.empty()) {
            weatherCondition = Weather();
            
        } else if(
            game.content.weatherConditions.list.find(weatherName) ==
            game.content.weatherConditions.list.end()
        ) {
            game.errors.report(
                "Unknown weather condition \"" + weatherName + "\"!",
                weather_node
            );
            weatherCondition = Weather();
            
        } else {
            weatherCondition =
                game.content.weatherConditions.list[weatherName];
                
        }
        
        //Song.
        if(
            !songName.empty() &&
            game.content.songs.list.find(songName) ==
            game.content.songs.list.end()
        ) {
            game.errors.report(
                "Unknown song \"" + songName + "\"!",
                song_node
            );
        }
    }
    
    if(level >= CONTENT_LOAD_LEVEL_FULL && !bgBmpName.empty()) {
        bgBmp = game.content.bitmaps.list.get(bgBmpName, node);
    }
}


/**
 * @brief Loads the area's mission data from a data node.
 *
 * @param node Data node to load from.
 */
void Area::loadMissionDataFromDataNode(DataNode* node) {
    mission.failHudPrimaryCond = INVALID;
    mission.failHudSecondaryCond = INVALID;
    
    ReaderSetter rs(node);
    string goal_str;
    string required_mobs_str;
    int mission_grading_mode_int = MISSION_GRADING_MODE_GOAL;
    
    rs.set("mission_goal", goal_str);
    rs.set("mission_goal_amount", mission.goalAmount);
    rs.set("mission_goal_all_mobs", mission.goalAllMobs);
    rs.set("mission_required_mobs", required_mobs_str);
    rs.set("mission_goal_exit_center", mission.goalExitCenter);
    rs.set("mission_goal_exit_size", mission.goalExitSize);
    rs.set("mission_fail_conditions", mission.failConditions);
    rs.set("mission_fail_too_few_pik_amount", mission.failTooFewPikAmount);
    rs.set("mission_fail_too_many_pik_amount", mission.failTooManyPikAmount);
    rs.set("mission_fail_pik_killed", mission.failPikKilled);
    rs.set("mission_fail_leaders_kod", mission.failLeadersKod);
    rs.set("mission_fail_enemies_killed", mission.failEnemiesKilled);
    rs.set("mission_fail_time_limit", mission.failTimeLimit);
    rs.set("mission_fail_hud_primary_cond", mission.failHudPrimaryCond);
    rs.set("mission_fail_hud_secondary_cond", mission.failHudSecondaryCond);
    rs.set("mission_grading_mode", mission_grading_mode_int);
    rs.set("mission_points_per_pikmin_born", mission.pointsPerPikminBorn);
    rs.set("mission_points_per_pikmin_death", mission.pointsPerPikminDeath);
    rs.set("mission_points_per_sec_left", mission.pointsPerSecLeft);
    rs.set("mission_points_per_sec_passed", mission.pointsPerSecPassed);
    rs.set("mission_points_per_treasure_point", mission.pointsPerTreasurePoint);
    rs.set("mission_points_per_enemy_point", mission.pointsPerEnemyPoint);
    rs.set("enemy_points_on_collection", mission.enemyPointsOnCollection);
    rs.set("mission_point_loss_data", mission.pointLossData);
    rs.set("mission_point_hud_data", mission.pointHudData);
    rs.set("mission_starting_points", mission.startingPoints);
    rs.set("mission_bronze_req", mission.bronzeReq);
    rs.set("mission_silver_req", mission.silverReq);
    rs.set("mission_gold_req", mission.goldReq);
    rs.set("mission_platinum_req", mission.platinumReq);
    rs.set("mission_maker_record", mission.makerRecord);
    rs.set("mission_maker_record_date", mission.makerRecordDate);
    
    mission.goal = MISSION_GOAL_END_MANUALLY;
    for(size_t g = 0; g < game.missionGoals.size(); g++) {
        if(game.missionGoals[g]->getName() == goal_str) {
            mission.goal = (MISSION_GOAL) g;
            break;
        }
    }
    vector<string> mission_required_mobs_strs =
        semicolonListToVector(required_mobs_str);
    mission.goalMobIdxs.reserve(
        mission_required_mobs_strs.size()
    );
    for(size_t m = 0; m < mission_required_mobs_strs.size(); m++) {
        mission.goalMobIdxs.insert(
            s2i(mission_required_mobs_strs[m])
        );
    }
    mission.gradingMode = (MISSION_GRADING_MODE) mission_grading_mode_int;
    
    //Automatically turn the pause menu fail condition on/off for convenience.
    if(mission.goal == MISSION_GOAL_END_MANUALLY) {
        disableFlag(
            mission.failConditions,
            getIdxBitmask(MISSION_FAIL_COND_PAUSE_MENU)
        );
    } else {
        enableFlag(
            mission.failConditions,
            getIdxBitmask(MISSION_FAIL_COND_PAUSE_MENU)
        );
    }
    
    //Automatically turn off the seconds left score criterion for convenience.
    if(
        !hasFlag(
            mission.failConditions,
            getIdxBitmask(MISSION_FAIL_COND_TIME_LIMIT)
        )
    ) {
        mission.pointsPerSecLeft = 0;
        disableFlag(
            mission.pointHudData,
            getIdxBitmask(MISSION_SCORE_CRITERIA_SEC_LEFT)
        );
        disableFlag(
            mission.pointLossData,
            getIdxBitmask(MISSION_SCORE_CRITERIA_SEC_LEFT)
        );
    }
}


/**
 * @brief Loads the area's geometry from a data node.
 *
 * @param node Data node to load from.
 * @param level Level to load at.
 */
void Area::loadGeometryFromDataNode(
    DataNode* node, CONTENT_LOAD_LEVEL level
) {
    //Vertexes.
    if(game.perfMon) {
        game.perfMon->startMeasurement("Area -- Vertexes");
    }
    
    size_t n_vertexes =
        node->getChildByName(
            "vertexes"
        )->getNrOfChildrenByName("v");
    for(size_t v = 0; v < n_vertexes; v++) {
        DataNode* vertex_data =
            node->getChildByName(
                "vertexes"
            )->getChildByName("v", v);
        vector<string> words = split(vertex_data->value);
        if(words.size() == 2) {
            vertexes.push_back(
                new Vertex(s2f(words[0]), s2f(words[1]))
            );
        }
    }
    
    if(game.perfMon) {
        game.perfMon->finishMeasurement();
    }
    
    //Edges.
    if(game.perfMon) {
        game.perfMon->startMeasurement("Area -- Edges");
    }
    
    size_t n_edges =
        node->getChildByName(
            "edges"
        )->getNrOfChildrenByName("e");
    for(size_t e = 0; e < n_edges; e++) {
        DataNode* edge_data =
            node->getChildByName(
                "edges"
            )->getChildByName("e", e);
        Edge* new_edge = new Edge();
        
        vector<string> s_idxs = split(edge_data->getChildByName("s")->value);
        if(s_idxs.size() < 2) s_idxs.insert(s_idxs.end(), 2, "-1");
        for(size_t s = 0; s < 2; s++) {
            if(s_idxs[s] == "-1") new_edge->sectorIdxs[s] = INVALID;
            else new_edge->sectorIdxs[s] = s2i(s_idxs[s]);
        }
        
        vector<string> v_idxs = split(edge_data->getChildByName("v")->value);
        if(v_idxs.size() < 2) v_idxs.insert(v_idxs.end(), 2, "0");
        
        new_edge->vertexIdxs[0] = s2i(v_idxs[0]);
        new_edge->vertexIdxs[1] = s2i(v_idxs[1]);
        
        DataNode* shadow_length =
            edge_data->getChildByName("shadow_length");
        if(!shadow_length->value.empty()) {
            new_edge->wallShadowLength =
                s2f(shadow_length->value);
        }
        
        DataNode* shadow_color =
            edge_data->getChildByName("shadow_color");
        if(!shadow_color->value.empty()) {
            new_edge->wallShadowColor = s2c(shadow_color->value);
        }
        
        DataNode* smoothing_length =
            edge_data->getChildByName("smoothing_length");
        if(!smoothing_length->value.empty()) {
            new_edge->ledgeSmoothingLength =
                s2f(smoothing_length->value);
        }
        
        DataNode* smoothing_color =
            edge_data->getChildByName("smoothing_color");
        if(!smoothing_color->value.empty()) {
            new_edge->ledgeSmoothingColor =
                s2c(smoothing_color->value);
        }
        
        edges.push_back(new_edge);
    }
    
    if(game.perfMon) {
        game.perfMon->finishMeasurement();
    }
    
    //Sectors.
    if(game.perfMon) {
        game.perfMon->startMeasurement("Area -- Sectors");
    }
    
    size_t n_sectors =
        node->getChildByName(
            "sectors"
        )->getNrOfChildrenByName("s");
    for(size_t s = 0; s < n_sectors; s++) {
        DataNode* sector_data =
            node->getChildByName(
                "sectors"
            )->getChildByName("s", s);
        Sector* new_sector = new Sector();
        
        size_t new_type =
            game.sectorTypes.getIdx(
                sector_data->getChildByName("type")->value
            );
        if(new_type == INVALID) {
            new_type = SECTOR_TYPE_NORMAL;
        }
        new_sector->type = (SECTOR_TYPE) new_type;
        new_sector->isBottomlessPit =
            s2b(
                sector_data->getChildByName(
                    "is_bottomless_pit"
                )->getValueOrDefault("false")
            );
        new_sector->brightness =
            s2f(
                sector_data->getChildByName(
                    "brightness"
                )->getValueOrDefault(i2s(GEOMETRY::DEF_SECTOR_BRIGHTNESS))
            );
        new_sector->tag = sector_data->getChildByName("tag")->value;
        new_sector->z = s2f(sector_data->getChildByName("z")->value);
        new_sector->fade = s2b(sector_data->getChildByName("fade")->value);
        
        new_sector->textureInfo.bmpName =
            sector_data->getChildByName("texture")->value;
        new_sector->textureInfo.rot =
            s2f(sector_data->getChildByName("texture_rotate")->value);
            
        vector<string> scales =
            split(sector_data->getChildByName("texture_scale")->value);
        if(scales.size() >= 2) {
            new_sector->textureInfo.scale.x = s2f(scales[0]);
            new_sector->textureInfo.scale.y = s2f(scales[1]);
        }
        vector<string> translations =
            split(sector_data->getChildByName("texture_trans")->value);
        if(translations.size() >= 2) {
            new_sector->textureInfo.translation.x = s2f(translations[0]);
            new_sector->textureInfo.translation.y = s2f(translations[1]);
        }
        new_sector->textureInfo.tint =
            s2c(
                sector_data->getChildByName("texture_tint")->
                getValueOrDefault("255 255 255")
            );
            
        if(!new_sector->fade && !new_sector->isBottomlessPit) {
            new_sector->textureInfo.bitmap =
                game.content.bitmaps.list.get(new_sector->textureInfo.bmpName, nullptr);
        }
        
        DataNode* hazards_node = sector_data->getChildByName("hazards");
        vector<string> hazards_strs =
            semicolonListToVector(hazards_node->value);
        for(size_t h = 0; h < hazards_strs.size(); h++) {
            string hazard_name = hazards_strs[h];
            if(game.content.hazards.list.find(hazard_name) == game.content.hazards.list.end()) {
                game.errors.report(
                    "Unknown hazard \"" + hazard_name +
                    "\"!", hazards_node
                );
            } else {
                new_sector->hazards.push_back(&(game.content.hazards.list[hazard_name]));
            }
        }
        new_sector->hazardsStr = hazards_node->value;
        new_sector->hazardFloor =
            s2b(
                sector_data->getChildByName(
                    "hazards_floor"
                )->getValueOrDefault("true")
            );
            
        sectors.push_back(new_sector);
    }
    
    if(game.perfMon) {
        game.perfMon->finishMeasurement();
    }
    
    //Mobs.
    if(game.perfMon) {
        game.perfMon->startMeasurement("Area -- Object generators");
    }
    
    vector<std::pair<size_t, size_t> > mob_links_buffer;
    size_t n_mobs =
        node->getChildByName("mobs")->getNrOfChildren();
        
    for(size_t m = 0; m < n_mobs; m++) {
    
        DataNode* mob_node =
            node->getChildByName("mobs")->getChild(m);
            
        MobGen* mob_ptr = new MobGen();
        
        mob_ptr->pos = s2p(mob_node->getChildByName("p")->value);
        mob_ptr->angle =
            s2f(
                mob_node->getChildByName("angle")->getValueOrDefault("0")
            );
        mob_ptr->vars = mob_node->getChildByName("vars")->value;
        
        string category_name = mob_node->name;
        string type_name;
        MobCategory* category =
            game.mobCategories.getFromInternalName(category_name);
        if(category) {
            type_name = mob_node->getChildByName("type")->value;
            mob_ptr->type = category->getType(type_name);
        } else {
            category = game.mobCategories.get(MOB_CATEGORY_NONE);
            mob_ptr->type = nullptr;
        }
        
        vector<string> link_strs =
            split(mob_node->getChildByName("links")->value);
        for(size_t l = 0; l < link_strs.size(); l++) {
            mob_links_buffer.push_back(std::make_pair(m, s2i(link_strs[l])));
        }
        
        DataNode* stored_inside_node =
            mob_node->getChildByName("stored_inside");
        if(!stored_inside_node->value.empty()) {
            mob_ptr->storedInside = s2i(stored_inside_node->value);
        }
        
        bool valid =
            category && category->id != MOB_CATEGORY_NONE &&
            mob_ptr->type;
            
        if(!valid) {
            //Error.
            mob_ptr->type = nullptr;
            if(level >= CONTENT_LOAD_LEVEL_FULL) {
                game.errors.report(
                    "Unknown mob type \"" + type_name + "\" of category \"" +
                    mob_node->name + "\"!",
                    mob_node
                );
            }
        }
        
        mobGenerators.push_back(mob_ptr);
    }
    
    for(size_t l = 0; l < mob_links_buffer.size(); l++) {
        size_t f = mob_links_buffer[l].first;
        size_t s = mob_links_buffer[l].second;
        mobGenerators[f]->links.push_back(
            mobGenerators[s]
        );
        mobGenerators[f]->linkIdxs.push_back(s);
    }
    
    if(game.perfMon) {
        game.perfMon->finishMeasurement();
    }
    
    //Paths.
    if(game.perfMon) {
        game.perfMon->startMeasurement("Area -- Paths");
    }
    
    size_t n_stops =
        node->getChildByName("path_stops")->getNrOfChildren();
    for(size_t s = 0; s < n_stops; s++) {
    
        DataNode* path_stop_node =
            node->getChildByName("path_stops")->getChild(s);
            
        PathStop* s_ptr = new PathStop();
        
        s_ptr->pos = s2p(path_stop_node->getChildByName("pos")->value);
        s_ptr->radius = s2f(path_stop_node->getChildByName("radius")->value);
        s_ptr->flags = s2i(path_stop_node->getChildByName("flags")->value);
        s_ptr->label = path_stop_node->getChildByName("label")->value;
        DataNode* links_node = path_stop_node->getChildByName("links");
        size_t n_links = links_node->getNrOfChildren();
        
        for(size_t l = 0; l < n_links; l++) {
        
            string link_data = links_node->getChild(l)->value;
            vector<string> link_data_parts = split(link_data);
            
            PathLink* l_ptr =
                new PathLink(s_ptr, nullptr, s2i(link_data_parts[0]));
            if(link_data_parts.size() >= 2) {
                l_ptr->type = (PATH_LINK_TYPE) s2i(link_data_parts[1]);
            }
            
            s_ptr->links.push_back(l_ptr);
            
        }
        
        s_ptr->radius = std::max(s_ptr->radius, PATHS::MIN_STOP_RADIUS);
        
        pathStops.push_back(s_ptr);
    }
    
    if(game.perfMon) {
        game.perfMon->finishMeasurement();
    }
    
    //Tree shadows.
    if(game.perfMon) {
        game.perfMon->startMeasurement("Area -- Tree shadows");
    }
    
    size_t n_shadows =
        node->getChildByName("tree_shadows")->getNrOfChildren();
    for(size_t s = 0; s < n_shadows; s++) {
    
        DataNode* shadow_node =
            node->getChildByName("tree_shadows")->getChild(s);
            
        TreeShadow* s_ptr = new TreeShadow();
        
        vector<string> words =
            split(shadow_node->getChildByName("pos")->value);
        s_ptr->center.x = (words.size() >= 1 ? s2f(words[0]) : 0);
        s_ptr->center.y = (words.size() >= 2 ? s2f(words[1]) : 0);
        
        words = split(shadow_node->getChildByName("size")->value);
        s_ptr->size.x = (words.size() >= 1 ? s2f(words[0]) : 0);
        s_ptr->size.y = (words.size() >= 2 ? s2f(words[1]) : 0);
        
        s_ptr->angle =
            s2f(
                shadow_node->getChildByName(
                    "angle"
                )->getValueOrDefault("0")
            );
        s_ptr->alpha =
            s2i(
                shadow_node->getChildByName(
                    "alpha"
                )->getValueOrDefault("255")
            );
        s_ptr->bmpName = shadow_node->getChildByName("file")->value;
        s_ptr->bitmap = game.content.bitmaps.list.get(s_ptr->bmpName, nullptr);
        
        words = split(shadow_node->getChildByName("sway")->value);
        s_ptr->sway.x = (words.size() >= 1 ? s2f(words[0]) : 0);
        s_ptr->sway.y = (words.size() >= 2 ? s2f(words[1]) : 0);
        
        if(s_ptr->bitmap == game.bmpError && level >= CONTENT_LOAD_LEVEL_FULL) {
            game.errors.report(
                "Unknown tree shadow texture \"" + s_ptr->bmpName + "\"!",
                shadow_node
            );
        }
        
        treeShadows.push_back(s_ptr);
        
    }
    
    if(game.perfMon) {
        game.perfMon->finishMeasurement();
    }
    
    //Set up stuff.
    if(game.perfMon) {
        game.perfMon->startMeasurement("Area -- Geometry calculations");
    }
    
    for(size_t e = 0; e < edges.size(); e++) {
        fixEdgePointers(
            edges[e]
        );
    }
    for(size_t s = 0; s < sectors.size(); s++) {
        connectSectorEdges(
            sectors[s]
        );
    }
    for(size_t v = 0; v < vertexes.size(); v++) {
        connectVertexEdges(
            vertexes[v]
        );
    }
    for(size_t s = 0; s < pathStops.size(); s++) {
        fixPathStopPointers(
            pathStops[s]
        );
    }
    for(size_t s = 0; s < pathStops.size(); s++) {
        pathStops[s]->calculateDists();
    }
    if(level >= CONTENT_LOAD_LEVEL_FULL) {
        //Fade sectors that also fade brightness should be
        //at midway between the two neighbors.
        for(size_t s = 0; s < sectors.size(); s++) {
            Sector* s_ptr = sectors[s];
            if(s_ptr->fade) {
                Sector* n1 = nullptr;
                Sector* n2 = nullptr;
                s_ptr->getTextureMergeSectors(&n1, &n2);
                if(n1 && n2) {
                    s_ptr->brightness = (n1->brightness + n2->brightness) / 2;
                }
            }
        }
    }
    
    
    //Triangulate everything and save bounding boxes.
    set<Edge*> lone_edges;
    for(size_t s = 0; s < sectors.size(); s++) {
        Sector* s_ptr = sectors[s];
        s_ptr->triangles.clear();
        TRIANGULATION_ERROR res =
            triangulateSector(s_ptr, &lone_edges, false);
            
        if(res != TRIANGULATION_ERROR_NONE && level == CONTENT_LOAD_LEVEL_EDITOR) {
            problems.nonSimples[s_ptr] = res;
            problems.loneEdges.insert(
                lone_edges.begin(), lone_edges.end()
            );
        }
        
        s_ptr->calculateBoundingBox();
    }
    
    if(level >= CONTENT_LOAD_LEVEL_EDITOR) generateBlockmap();
    
    if(game.perfMon) {
        game.perfMon->finishMeasurement();
    }
}


/**
 * @brief Loads the thumbnail image from the disk and updates the
 * thumbnail class member.
 *
 * @param thumbnail_path Path to the bitmap.
 */
void Area::loadThumbnail(const string &thumbnail_path) {
    if(thumbnail) {
        thumbnail = nullptr;
    }
    
    if(al_filename_exists(thumbnail_path.c_str())) {
        thumbnail =
            std::shared_ptr<ALLEGRO_BITMAP>(
                al_load_bitmap(thumbnail_path.c_str()),
        [](ALLEGRO_BITMAP * b) {
            al_destroy_bitmap(b);
        }
            );
    }
}


/**
 * @brief Adds a new edge to the list.
 *
 * @return The new edge's pointer.
 */
Edge* Area::newEdge() {
    Edge* e_ptr = new Edge();
    edges.push_back(e_ptr);
    return e_ptr;
}


/**
 * @brief Adds a new sector to the list.
 *
 * @return The new sector's pointer.
 */
Sector* Area::newSector() {
    Sector* s_ptr = new Sector();
    sectors.push_back(s_ptr);
    return s_ptr;
}


/**
 * @brief Adds a new vertex to the list.
 *
 * @return The new vertex's pointer.
 */
Vertex* Area::newVertex() {
    Vertex* v_ptr = new Vertex();
    vertexes.push_back(v_ptr);
    return v_ptr;
}


/**
 * @brief Removes an edge from the list, and updates all indexes after it.
 *
 * @param e_idx Index number of the edge to remove.
 */
void Area::removeEdge(size_t e_idx) {
    edges.erase(edges.begin() + e_idx);
    for(size_t v = 0; v < vertexes.size(); v++) {
        Vertex* v_ptr = vertexes[v];
        for(size_t e = 0; e < v_ptr->edges.size(); e++) {
            if(
                v_ptr->edgeIdxs[e] != INVALID &&
                v_ptr->edgeIdxs[e] > e_idx
            ) {
                v_ptr->edgeIdxs[e]--;
            } else {
                //This should never happen.
                engineAssert(
                    v_ptr->edgeIdxs[e] != e_idx,
                    i2s(v_ptr->edgeIdxs[e]) + " " + i2s(e_idx)
                );
            }
        }
    }
    for(size_t s = 0; s < sectors.size(); s++) {
        Sector* s_ptr = sectors[s];
        for(size_t e = 0; e < s_ptr->edges.size(); e++) {
            if(
                s_ptr->edgeIdxs[e] != INVALID &&
                s_ptr->edgeIdxs[e] > e_idx
            ) {
                s_ptr->edgeIdxs[e]--;
            } else {
                //This should never happen.
                engineAssert(
                    s_ptr->edgeIdxs[e] != e_idx,
                    i2s(s_ptr->edgeIdxs[e]) + " " + i2s(e_idx)
                );
            }
        }
    }
}


/**
 * @brief Removes an edge from the list, and updates all indexes after it.
 *
 * @param e_ptr Pointer of the edge to remove.
 */
void Area::removeEdge(const Edge* e_ptr) {
    for(size_t e = 0; e < edges.size(); e++) {
        if(edges[e] == e_ptr) {
            removeEdge(e);
            return;
        }
    }
}


/**
 * @brief Removes a sector from the list, and updates all indexes after it.
 *
 * @param s_idx Index number of the sector to remove.
 */
void Area::removeSector(size_t s_idx) {
    sectors.erase(sectors.begin() + s_idx);
    for(size_t e = 0; e < edges.size(); e++) {
        Edge* e_ptr = edges[e];
        for(size_t s = 0; s < 2; s++) {
            if(
                e_ptr->sectorIdxs[s] != INVALID &&
                e_ptr->sectorIdxs[s] > s_idx
            ) {
                e_ptr->sectorIdxs[s]--;
            } else {
                //This should never happen.
                engineAssert(
                    e_ptr->sectorIdxs[s] != s_idx,
                    i2s(e_ptr->sectorIdxs[s]) + " " + i2s(s_idx)
                );
            }
        }
    }
}


/**
 * @brief Removes a sector from the list, and updates all indexes after it.
 *
 * @param s_ptr Pointer of the sector to remove.
 */
void Area::removeSector(const Sector* s_ptr) {
    for(size_t s = 0; s < sectors.size(); s++) {
        if(sectors[s] == s_ptr) {
            removeSector(s);
            return;
        }
    }
}


/**
 * @brief Removes a vertex from the list, and updates all indexes after it.
 *
 * @param v_idx Index number of the vertex to remove.
 */
void Area::removeVertex(size_t v_idx) {
    vertexes.erase(vertexes.begin() + v_idx);
    for(size_t e = 0; e < edges.size(); e++) {
        Edge* e_ptr = edges[e];
        for(size_t v = 0; v < 2; v++) {
            if(
                e_ptr->vertexIdxs[v] != INVALID &&
                e_ptr->vertexIdxs[v] > v_idx
            ) {
                e_ptr->vertexIdxs[v]--;
            } else {
                //This should never happen.
                engineAssert(
                    e_ptr->vertexIdxs[v] != v_idx,
                    i2s(e_ptr->vertexIdxs[v]) + " " + i2s(v_idx)
                );
            }
        }
    }
}


/**
 * @brief Removes a vertex from the list, and updates all indexes after it.
 *
 * @param v_ptr Pointer of the vertex to remove.
 */
void Area::removeVertex(const Vertex* v_ptr) {
    for(size_t v = 0; v < vertexes.size(); v++) {
        if(vertexes[v] == v_ptr) {
            removeVertex(v);
            return;
        }
    }
}


/**
 * @brief Saves the area's geometry to a data node.
 *
 * @param node Data node to save to.
 */
void Area::saveGeometryToDataNode(DataNode* node) {
    //Vertexes.
    DataNode* vertexes_node = node->addNew("vertexes");
    for(size_t v = 0; v < vertexes.size(); v++) {
    
        //Vertex.
        Vertex* v_ptr = vertexes[v];
        vertexes_node->addNew("v", p2s(v2p(v_ptr)));
    }
    
    //Edges.
    DataNode* edges_node = node->addNew("edges");
    for(size_t e = 0; e < edges.size(); e++) {
    
        //Edge.
        Edge* e_ptr = edges[e];
        DataNode* edge_node = edges_node->addNew("e");
        GetterWriter egw(edge_node);
        
        string s_str;
        for(size_t s = 0; s < 2; s++) {
            if(e_ptr->sectorIdxs[s] == INVALID) s_str += "-1";
            else s_str += i2s(e_ptr->sectorIdxs[s]);
            s_str += " ";
        }
        s_str.erase(s_str.size() - 1);
        string v_str =
            i2s(e_ptr->vertexIdxs[0]) + " " + i2s(e_ptr->vertexIdxs[1]);
            
        egw.get("s", s_str);
        egw.get("v", v_str);
        
        if(e_ptr->wallShadowLength != LARGE_FLOAT) {
            egw.get("shadow_length", e_ptr->wallShadowLength);
        }
        
        if(e_ptr->wallShadowColor != GEOMETRY::SHADOW_DEF_COLOR) {
            egw.get("shadow_color", e_ptr->wallShadowColor);
        }
        
        if(e_ptr->ledgeSmoothingLength != 0.0f) {
            egw.get("smoothing_length", e_ptr->ledgeSmoothingLength);
        }
        
        if(e_ptr->ledgeSmoothingColor != GEOMETRY::SMOOTHING_DEF_COLOR) {
            egw.get("smoothing_color", e_ptr->ledgeSmoothingColor);
        }
    }
    
    //Sectors.
    DataNode* sectors_node = node->addNew("sectors");
    for(size_t s = 0; s < sectors.size(); s++) {
    
        //Sector.
        Sector* s_ptr = sectors[s];
        DataNode* sector_node = sectors_node->addNew("s");
        GetterWriter sgw(sector_node);
        
        if(s_ptr->type != SECTOR_TYPE_NORMAL) {
            sgw.get("type", game.sectorTypes.getName(s_ptr->type));
        }
        if(s_ptr->isBottomlessPit) {
            sgw.get("is_bottomless_pit", true);
        }
        sgw.get("z", s_ptr->z);
        if(s_ptr->brightness != GEOMETRY::DEF_SECTOR_BRIGHTNESS) {
            sgw.get("brightness", s_ptr->brightness);
        }
        if(!s_ptr->tag.empty()) {
            sgw.get("tag", s_ptr->tag);
        }
        if(s_ptr->fade) {
            sgw.get("fade", s_ptr->fade);
        }
        if(!s_ptr->hazardsStr.empty()) {
            sgw.get("hazards", s_ptr->hazardsStr);
            sgw.get("hazards_floor", s_ptr->hazardFloor);
        }
        
        if(!s_ptr->textureInfo.bmpName.empty()) {
            sgw.get("texture", s_ptr->textureInfo.bmpName);
        }
        
        if(s_ptr->textureInfo.rot != 0) {
            sgw.get("texture_rotate", s_ptr->textureInfo.rot);
        }
        if(
            s_ptr->textureInfo.scale.x != 1 ||
            s_ptr->textureInfo.scale.y != 1
        ) {
            sgw.get("texture_scale", s_ptr->textureInfo.scale);
        }
        if(
            s_ptr->textureInfo.translation.x != 0 ||
            s_ptr->textureInfo.translation.y != 0
        ) {
            sgw.get("texture_trans", s_ptr->textureInfo.translation);
        }
        if(
            s_ptr->textureInfo.tint.r != 1.0 ||
            s_ptr->textureInfo.tint.g != 1.0 ||
            s_ptr->textureInfo.tint.b != 1.0 ||
            s_ptr->textureInfo.tint.a != 1.0
        ) {
            sgw.get("texture_tint", s_ptr->textureInfo.tint);
        }
        
    }
    
    //Mobs.
    DataNode* mobs_node = node->addNew("mobs");
    for(size_t m = 0; m < mobGenerators.size(); m++) {
    
        //Mob.
        MobGen* m_ptr = mobGenerators[m];
        string cat_name = "unknown";
        if(m_ptr->type && m_ptr->type->category) {
            cat_name = m_ptr->type->category->internalName;
        }
        DataNode* mob_node = mobs_node->addNew(cat_name);
        GetterWriter mgw(mob_node);
        
        if(m_ptr->type) {
            mgw.get("type", m_ptr->type->manifest->internalName);
        }
        mgw.get("p", m_ptr->pos);
        if(m_ptr->angle != 0) {
            mgw.get("angle", m_ptr->angle);
        }
        if(!m_ptr->vars.empty()) {
            mgw.get("vars", m_ptr->vars);
        }
        
        string links_str;
        for(size_t l = 0; l < m_ptr->linkIdxs.size(); l++) {
            if(l > 0) links_str += " ";
            links_str += i2s(m_ptr->linkIdxs[l]);
        }
        
        if(!links_str.empty()) {
            mgw.get("links", links_str);
        }
        
        if(m_ptr->storedInside != INVALID) {
            mgw.get("stored_inside", m_ptr->storedInside);
        }
    }
    
    //Path stops.
    DataNode* path_stops_node = node->addNew("path_stops");
    for(size_t s = 0; s < pathStops.size(); s++) {
    
        //Path stop.
        PathStop* s_ptr = pathStops[s];
        DataNode* path_stop_node = path_stops_node->addNew("s");
        GetterWriter sgw(path_stop_node);
        
        sgw.get("pos", s_ptr->pos);
        if(s_ptr->radius != PATHS::MIN_STOP_RADIUS) {
            sgw.get("radius", s_ptr->radius);
        }
        if(s_ptr->flags != 0) {
            sgw.get("flags", s_ptr->flags);
        }
        if(!s_ptr->label.empty()) {
            sgw.get("label", s_ptr->label);
        }
        
        DataNode* links_node = path_stop_node->addNew("links");
        for(size_t l = 0; l < s_ptr->links.size(); l++) {
            PathLink* l_ptr = s_ptr->links[l];
            string link_data = i2s(l_ptr->endIdx);
            if(l_ptr->type != PATH_LINK_TYPE_NORMAL) {
                link_data += " " + i2s(l_ptr->type);
            }
            links_node->addNew("l", link_data);
        }
        
    }
    
    //Tree shadows.
    DataNode* shadows_node = node->addNew("tree_shadows");
    for(size_t s = 0; s < treeShadows.size(); s++) {
    
        //Tree shadow.
        TreeShadow* s_ptr = treeShadows[s];
        DataNode* shadow_node = shadows_node->addNew("shadow");
        GetterWriter sgw(shadow_node);
        
        sgw.get("pos", s_ptr->center);
        sgw.get("size", s_ptr->size);
        sgw.get("file", s_ptr->bmpName);
        sgw.get("sway", s_ptr->sway);
        if(s_ptr->angle != 0) {
            sgw.get("angle", s_ptr->angle);
        }
        if(s_ptr->alpha != 255) {
            sgw.get("alpha", s_ptr->alpha);
        }
        
    }
}


/**
 * @brief Saves the area's main data to a data node.
 *
 * @param node Data node to save to.
 */
void Area::saveMainDataToDataNode(DataNode* node) {
    //Content metadata.
    saveMetadataToDataNode(node);
    
    GetterWriter gw(node);
    
    //Main data.
    gw.get("subtitle", subtitle);
    gw.get("difficulty", difficulty);
    gw.get("bg_bmp", bgBmpName);
    gw.get("bg_color", bgColor);
    gw.get("bg_dist", bgDist);
    gw.get("bg_zoom", bgBmpZoom);
    gw.get("song", songName);
    gw.get("weather", weatherName);
    gw.get("day_time_start", dayTimeStart);
    gw.get("day_time_speed", dayTimeSpeed);
    gw.get("spray_amounts", sprayAmounts);
}


/**
 * @brief Saves the area's mission data to a data node.
 *
 * @param node Data node to save to.
 */
void Area::saveMissionDataToDataNode(DataNode* node) {
    GetterWriter gw(node);
    
    if(mission.goal != MISSION_GOAL_END_MANUALLY) {
        string goal_name = game.missionGoals[mission.goal]->getName();
        gw.get("mission_goal", goal_name);
    }
    if(
        mission.goal == MISSION_GOAL_TIMED_SURVIVAL ||
        mission.goal == MISSION_GOAL_GROW_PIKMIN
    ) {
        gw.get("mission_goal_amount", mission.goalAmount);
    }
    if(
        mission.goal == MISSION_GOAL_COLLECT_TREASURE ||
        mission.goal == MISSION_GOAL_BATTLE_ENEMIES ||
        mission.goal == MISSION_GOAL_GET_TO_EXIT
    ) {
        gw.get("mission_goal_all_mobs", mission.goalAllMobs);
        vector<string> mission_mob_idx_strs;
        for(auto m : mission.goalMobIdxs) {
            mission_mob_idx_strs.push_back(i2s(m));
        }
        string mission_mob_idx_str = join(mission_mob_idx_strs, ";");
        if(!mission_mob_idx_str.empty()) {
            gw.get("mission_required_mobs", mission_mob_idx_str);
        }
    }
    if(mission.goal == MISSION_GOAL_GET_TO_EXIT) {
        gw.get("mission_goal_exit_center", mission.goalExitCenter);
        gw.get("mission_goal_exit_size", mission.goalExitSize);
    }
    if(mission.failConditions > 0) {
        gw.get("mission_fail_conditions", mission.failConditions);
    }
    if(
        hasFlag(
            mission.failConditions,
            getIdxBitmask(MISSION_FAIL_COND_TOO_FEW_PIKMIN)
        )
    ) {
        gw.get("mission_fail_too_few_pik_amount", mission.failTooFewPikAmount);
    }
    if(
        hasFlag(
            mission.failConditions,
            getIdxBitmask(MISSION_FAIL_COND_TOO_MANY_PIKMIN)
        )
    ) {
        gw.get("mission_fail_too_many_pik_amount", mission.failTooManyPikAmount);
    }
    if(
        hasFlag(
            mission.failConditions,
            getIdxBitmask(MISSION_FAIL_COND_LOSE_PIKMIN)
        )
    ) {
        gw.get("mission_fail_pik_killed", mission.failPikKilled);
    }
    if(
        hasFlag(
            mission.failConditions,
            getIdxBitmask(MISSION_FAIL_COND_LOSE_LEADERS)
        )
    ) {
        gw.get("mission_fail_leaders_kod", mission.failLeadersKod);
    }
    if(
        hasFlag(
            mission.failConditions,
            getIdxBitmask(MISSION_FAIL_COND_KILL_ENEMIES)
        )
    ) {
        gw.get("mission_fail_enemies_killed", mission.failEnemiesKilled);
    }
    if(
        hasFlag(
            mission.failConditions,
            getIdxBitmask(MISSION_FAIL_COND_TIME_LIMIT)
        )
    ) {
        gw.get("mission_fail_time_limit", mission.failTimeLimit);
    }
    if(mission.failHudPrimaryCond != INVALID) {
        gw.get("mission_fail_hud_primary_cond", mission.failHudPrimaryCond);
    }
    if(mission.failHudSecondaryCond != INVALID) {
        gw.get("mission_fail_hud_secondary_cond", mission.failHudSecondaryCond);
    }
    gw.get("mission_grading_mode", mission.gradingMode);
    if(mission.gradingMode == MISSION_GRADING_MODE_POINTS) {
        if(mission.pointsPerPikminBorn != 0) {
            gw.get("mission_points_per_pikmin_born", mission.pointsPerPikminBorn);
        }
        if(mission.pointsPerPikminDeath != 0) {
            gw.get("mission_points_per_pikmin_death", mission.pointsPerPikminDeath);
        }
        if(mission.pointsPerSecLeft != 0) {
            gw.get("mission_points_per_sec_left", mission.pointsPerSecLeft);
        }
        if(mission.pointsPerSecPassed != 0) {
            gw.get("mission_points_per_sec_passed", mission.pointsPerSecPassed);
        }
        if(mission.pointsPerTreasurePoint != 0) {
            gw.get("mission_points_per_treasure_point", mission.pointsPerTreasurePoint);
        }
        if(mission.pointsPerEnemyPoint != 0) {
            gw.get("mission_points_per_enemy_point", mission.pointsPerEnemyPoint);
        }
        if(mission.enemyPointsOnCollection) {
            gw.get("enemy_points_on_collection", mission.enemyPointsOnCollection);
        }
        if(mission.pointLossData > 0) {
            gw.get("mission_point_loss_data", mission.pointLossData);
        }
        if(mission.pointHudData != 255) {
            gw.get("mission_point_hud_data", mission.pointHudData);
        }
        if(mission.startingPoints != 0) {
            gw.get("mission_starting_points", mission.startingPoints);
        }
        gw.get("mission_bronze_req", mission.bronzeReq);
        gw.get("mission_silver_req", mission.silverReq);
        gw.get("mission_gold_req", mission.goldReq);
        gw.get("mission_platinum_req", mission.platinumReq);
        if(!mission.makerRecordDate.empty()) {
            gw.get("mission_maker_record", mission.makerRecord);
            gw.get("mission_maker_record_date", mission.makerRecordDate);
        }
    }
}


/**
 * @brief Saves the area's thumbnail to the disk, or deletes it from the disk
 * if it's meant to not exist.
 *
 * @param to_backup Whether it's to save to the area backup.
 */
void Area::saveThumbnail(bool to_backup) {
    string thumb_path =
        (to_backup ? userDataPath : manifest->path) +
        "/" + FILE_NAMES::AREA_THUMBNAIL;
    if(thumbnail) {
        al_save_bitmap(thumb_path.c_str(), thumbnail.get());
    } else {
        al_remove_filename(thumb_path.c_str());
    }
}


/**
 * @brief Clears the info of the blockmap.
 */
void Blockmap::clear() {
    topLeftCorner = Point();
    edges.clear();
    sectors.clear();
    nCols = 0;
    nRows = 0;
}


/**
 * @brief Returns the block column in which an X coordinate is contained.
 *
 * @param x X coordinate.
 * @return The column, or INVALID on error.
 */
size_t Blockmap::getCol(float x) const {
    if(x < topLeftCorner.x) return INVALID;
    float final_x = (x - topLeftCorner.x) / GEOMETRY::BLOCKMAP_BLOCK_SIZE;
    if(final_x >= nCols) return INVALID;
    return final_x;
}


/**
 * @brief Obtains a list of edges that are within the specified
 * rectangular region.
 *
 * @param tl Top-left coordinates of the region.
 * @param br Bottom-right coordinates of the region.
 * @param out_edges Set to fill the edges into.
 * @return Whether it succeeded.
 */
bool Blockmap::getEdgesInRegion(
    const Point &tl, const Point &br, set<Edge*> &out_edges
) const {

    size_t bx1 = getCol(tl.x);
    size_t bx2 = getCol(br.x);
    size_t by1 = getRow(tl.y);
    size_t by2 = getRow(br.y);
    
    if(
        bx1 == INVALID || bx2 == INVALID ||
        by1 == INVALID || by2 == INVALID
    ) {
        //Out of bounds.
        return false;
    }
    
    for(size_t bx = bx1; bx <= bx2; bx++) {
        for(size_t by = by1; by <= by2; by++) {
        
            const vector<Edge*> &block_edges = edges[bx][by];
            
            for(size_t e = 0; e < block_edges.size(); e++) {
                out_edges.insert(block_edges[e]);
            }
        }
    }
    
    return true;
}


/**
 * @brief Returns the block row in which a Y coordinate is contained.
 *
 * @param y Y coordinate.
 * @return The row, or INVALID on error.
 */
size_t Blockmap::getRow(float y) const {
    if(y < topLeftCorner.y) return INVALID;
    float final_y = (y - topLeftCorner.y) / GEOMETRY::BLOCKMAP_BLOCK_SIZE;
    if(final_y >= nRows) return INVALID;
    return final_y;
}


/**
 * @brief Returns the top-left coordinates for the specified column and row.
 *
 * @param col Column to check.
 * @param row Row to check.
 * @return The top-left coordinates.
 */
Point Blockmap::getTopLeftCorner(size_t col, size_t row) const {
    return
        Point(
            col * GEOMETRY::BLOCKMAP_BLOCK_SIZE + topLeftCorner.x,
            row * GEOMETRY::BLOCKMAP_BLOCK_SIZE + topLeftCorner.y
        );
}


/**
 * @brief Constructs a new mob generator object.
 *
 * @param pos Coordinates.
 * @param type The mob type.
 * @param angle Angle it is facing.
 * @param vars String representation of the script vars.
 */
MobGen::MobGen(
    const Point &pos, MobType* type, float angle, const string &vars
) :
    type(type),
    pos(pos),
    angle(angle),
    vars(vars) {
    
}


/**
 * @brief Clones the properties of this mob generator onto another
 * mob generator.
 *
 * @param destination Mob generator to clone the data into.
 * @param include_position If true, the position is included too.
 */
void MobGen::clone(MobGen* destination, bool include_position) const {
    destination->angle = angle;
    if(include_position) destination->pos = pos;
    destination->type = type;
    destination->vars = vars;
    destination->linkIdxs = linkIdxs;
    destination->storedInside = storedInside;
}


/**
 * @brief Constructs a new tree shadow object.
 *
 * @param center Center coordinates.
 * @param size Width and height.
 * @param angle Angle it is rotated by.
 * @param alpha How opaque it is [0-255].
 * @param bmp_name Internal name of the tree shadow texture's bitmap.
 * @param sway Multiply the sway distance by this much, horizontally and
 * vertically.
 */
TreeShadow::TreeShadow(
    const Point &center, const Point &size, float angle,
    unsigned char alpha, const string &bmp_name, const Point &sway
) :
    bmpName(bmp_name),
    bitmap(nullptr),
    center(center),
    size(size),
    angle(angle),
    alpha(alpha),
    sway(sway) {
    
}


/**
 * @brief Destroys the tree shadow object.
 *
 */
TreeShadow::~TreeShadow() {
    game.content.bitmaps.list.free(bmpName);
}
