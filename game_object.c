/*
 * Copyright (c) 2011, David Reynolds <david@alwaysmovefast.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Tartarus nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <strings.h> /* strncasecmp */

#include "game_object.h"

bool object_matches_key(const game_object_t *obj, const char *key) {
    /* TODO: I don't like this. I'd prefer doing a hash table like the commands
     * hash table. */

    /* do a lookup over all object->keywords to find a match with key.
     * - true if match, false otherwise. */
    int i;
    bool res = false;
    if (obj) {
        for (i = 0; i < MAX_KEYWORDS; ++i) {
            if (obj->keywords[i]) {
                if (strncasecmp(obj->keywords[i], key, strlen(key)) == 0) {
                    res = true;
                    break;
                }
            }
        }
    }

    return res;
}

game_object_t *lookup_object_from_list(game_object_t *list, const char *key) {
    game_object_t *obj, *tmp;

    obj = NULL;

    for (tmp = list; tmp; tmp = tmp->next) {
        if (object_matches_key(tmp, key)) {
            obj = tmp;
            break;
        }
    }

    return obj;
}

int remove_game_object_from_list(game_object_t **list, game_object_t *obj) {
    game_object_t *cur, *prev;

    prev = NULL;

    for (cur = *list; cur; prev = cur, cur = cur->next) {
        if (cur == obj) {
            if (!prev)
                *list = cur->next;
            else
                prev->next = cur->next;
            return 0;
        }
    }

    /* object isn't in list */
    return -1;
}

void colorize_object_name(game_object_t *obj, char *writebuf) {
    /* transforms an object's name string into a colored string based
     * on its rarity. */
    char msg[MAX_NAME_LEN * 2];
    const char *color;

    switch(obj->rarity) {
    case COMMON:
        color = COMMON_COLOR;
        break;

    case LIMITED:
        color = LIMITED_COLOR;
        break;

    case RARE:
        color = RARE_COLOR;
        break;

    case ELITE:
        color = ELITE_COLOR;
        break;

    case LEGENDARY:
        color = LEGENDARY_COLOR;
        break;

    default:
        color = COMMON_COLOR;
        break;
    }

    sprintf(msg, "%s%s", color, obj->name);
    colorize_string(msg, writebuf);
}

int keywords_from_json(char out[MAX_KEYWORD_LEN][MAX_KEYWORDS], json_t *json) {
    json_t *keywords, *key;
    int numkeys, i;

    keywords = json_object_get(json, "keywords");
    numkeys = json_array_size(keywords);

    for (i = 0; i < numkeys; ++i) {
        key = json_array_get(keywords, i);
        sprintf(out[i], "%s", json_string_value(key));
    }
    return i;
}

game_object_t *game_object_from_json(json_t *json) {
    game_object_t *obj;

    obj = (game_object_t *)malloc(sizeof(game_object_t));
    obj->num_keywords = keywords_from_json(obj->keywords, json);

    obj->is_static = json_int_from_obj_key(json, "is_static");
    obj->rarity = json_int_from_obj_key(json, "rarity");
    obj->type = json_int_from_obj_key(json, "type");

    /* XXX: should I limit armor values to just wearable armor? */
    obj->armor = json_int_from_obj_key(json, "armor");
    obj->damage = json_int_from_obj_key(json, "damage");

    if (obj->type == KEY_TYPE) {
        obj->opens_area_id = json_int_from_obj_key(json, "opens_area_id");
        obj->opens_room_id = json_int_from_obj_key(json, "opens_room_id");
    }

    if (obj->type == ARMOR_TYPE)
        obj->wear_location = json_int_from_obj_key(json, "wear_location");
    else
        obj->wear_location = 0;

    snprintf(obj->name, MAX_NAME_LEN, "%s", json_str_from_obj_key(json, "name"));
    return obj;
}

json_t *game_object_to_json(game_object_t *obj) {
    json_t *json, *keywords, *objkey;
    int i;

    json = json_object();
    keywords = json_array();

    for (i = 0; i < obj->num_keywords; ++i) {
        if (!obj->keywords[i] || *obj->keywords[i] == '\0')
            break;

        objkey = json_string(obj->keywords[i]);
        json_array_append(keywords, objkey);
        json_decref(objkey);
    }

    json_object_set_new(json, "name", json_string(obj->name));
    json_object_set_new(json, "is_static", json_integer(obj->is_static));
    json_object_set_new(json, "rarity", json_integer(obj->rarity));
    json_object_set_new(json, "type", json_integer(obj->type));
    json_object_set_new(json, "armor", json_integer(obj->armor));
    json_object_set_new(json, "damage", json_integer(obj->damage));
    json_object_set_new(json, "wear_location", json_integer(obj->wear_location));

    if (obj->type == KEY_TYPE) {
        json_object_set_new(json, "opens_area_id", json_integer(obj->opens_area_id));
        json_object_set_new(json, "opens_room_id", json_integer(obj->opens_room_id));
    }

    json_object_set(json, "keywords", keywords);
    json_decref(keywords);

    return json;
}
