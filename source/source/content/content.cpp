/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Class representing a piece of game content.
 */

#include "content.h"

#include "../core/misc_structs.h"
#include "../util/string_utils.h"


/**
 * @brief Loads content metadata from a data node.
 *
 * @param node Data node to load from.
 */
void Content::loadMetadataFromDataNode(DataNode* node) {
    ReaderSetter rs(node);
    
    if(manifest) name = manifest->internalName;
    rs.set("name", name);
    rs.set("description", description);
    rs.set("tags", tags);
    rs.set("maker", maker);
    rs.set("version", version);
    rs.set("engine_version", engineVersion);
    rs.set("maker_notes", makerNotes);
    rs.set("notes", notes);
}


/**
 * @brief Resets the metadata.
 */
void Content::resetMetadata() {
    name.clear();
    description.clear();
    tags.clear();
    maker.clear();
    version.clear();
    engineVersion.clear();
    makerNotes.clear();
    notes.clear();
}


/**
 * @brief Saves content metadata to a data node.
 *
 * @param node Data node to save to.
 */
void Content::saveMetadataToDataNode(DataNode* node) const {
    GetterWriter gw(node);
    gw.get("name", name);

#define save_opt(n, v) if(!v.empty()) gw.get((n), (v))

    save_opt("description", description);
    save_opt("tags", tags);
    save_opt("maker", maker);
    save_opt("version", version);
    save_opt("engine_version", engineVersion);
    save_opt("maker_notes", makerNotes);
    save_opt("notes", notes);
    
#undef save_opt
}


/**
 * @brief Constructs a new content manifest object.
 */
ContentManifest::ContentManifest() {}


/**
 * @brief Constructs a new content manifest object.
 *
 * @param name Internal name. Basically file name sans extension or folder name.
 * @param path Path to the content, relative to the packs folder.
 * @param pack Pack it belongs to.
 */
ContentManifest::ContentManifest(const string &name, const string &path, const string &pack) :
    internalName(name),
    path(path),
    pack(pack) {
    
}


/**
 * @brief Clears all the information in a manifest.
 */
void ContentManifest::clear() {
    internalName.clear();
    path.clear();
    pack.clear();
}


/**
 * @brief Fills in the information using the provided path. It'll all be empty
 * if the path is not valid.
 */
void ContentManifest::fillFromPath(const string &path) {
    clear();
    
    vector<string> parts = split(path, "/");
    size_t game_data_idx = string::npos;
    for(size_t p = 0; p < parts.size(); p++) {
        if(parts[p] == FOLDER_NAMES::GAME_DATA) {
            game_data_idx = p;
            break;
        }
    }
    if(game_data_idx == string::npos) return;
    if((int) game_data_idx >= (int) parts.size() - 2) return;
    
    this->path = path;
    this->pack = parts[game_data_idx + 1];
    this->internalName = removeExtension(parts.back());
}
