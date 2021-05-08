
// tutorial/idea from https://www.boristhebrave.com/2020/09/12/dungeon-generation-in-binding-of-isaac/
// also some details are taken from 'Binding of Isaac: Explained!' videos from https://youtube.com/playlist?list=PLm_aCwRGMyAdCkytm9sp2-X7jfEU9ccXm

#include <acknex.h>
#include <default.c>
#include <windows.h>
#include <strio.c>

#include "console.h"

#define PRAGMA_POINTER

#include "list.h"

// max grid size
#define GRID_MAX_X 32
#define GRID_MAX_Y 32
#define ROOM_SIZE 32

// different room types
#define ROOM_NONE 0
#define ROOM_NORMAL 1
#define ROOM_START 2
#define ROOM_BOSS 3
#define ROOM_SHOP 4
#define ROOM_ITEM 5
#define ROOM_SECRET 6
#define ROOM_SUPER_SECRET 7

STRING *room_only_up_mdl = "room_only_up.mdl";
STRING *room_only_right_mdl = "room_only_right.mdl";
STRING *room_only_bottom_mdl = "room_only_bottom.mdl";
STRING *room_only_left_mdl = "room_only_left.mdl";

STRING *room_up_right_mdl = "room_up_right.mdl";
STRING *room_right_bottom_mdl = "room_right_bottom.mdl";
STRING *room_bottom_left_mdl = "room_bottom_left.mdl";
STRING *room_left_up_mdl = "room_left_up.mdl";

STRING *room_up_bottom_mdl = "room_up_bottom.mdl";
STRING *room_left_right_mdl = "room_left_right.mdl";

STRING *room_up_right_left_mdl = "room_up_right_left.mdl";
STRING *room_up_right_bottom_mdl = "room_up_right_bottom.mdl";
STRING *room_right_bottom_left_mdl = "room_right_bottom_left.mdl";
STRING *room_bottom_left_up_mdl = "room_bottom_left_up.mdl";

STRING *room_all_mdl = "room_all.mdl";
STRING *room_none_mdl = "room_none.mdl";

typedef struct TILE
{
    VECTOR pos;

    int id;
    int pos_x;
    int pos_y;

    int type;
    int secret_chance;

    int doors;
    int up;
    int right;
    int bottom;
    int left;

    int secret_doors;
    int secret_up;
    int secret_right;
    int secret_bottom;
    int secret_left;
} TILE;

TILE map[GRID_MAX_X][GRID_MAX_Y];
TILE *start_room_tile = NULL;        // starting room
TILE *boss_room_tile = NULL;         // boss room (1 per level)
TILE *shop_room_tile = NULL;         // shop room (1 per level)
TILE *super_secret_room_tile = NULL; // super secret room (1 per level)

// Lists which we are using for each dungeon generation !
List rooms_queue_list;     // queue of rooms for checking
List end_rooms_list;       // list of end rooms
List item_rooms_list;      // list of item rooms
List secret_rooms_list;    // list of secret rooms
List secret_position_list; // list of secret room potential positions
List super_position_list;  // list of super secret room potential positions

VECTOR start_room_pos; // position of the starting room

int level_id = 1;      // currently running level's ID (min 1 and max 10)
int max_rooms = 0;     // max amount of rooms to be created (changed in map_generate + increases with level_id)
int created_rooms = 0; // amount of already created rooms
int max_secrets = 0;   // max amount of secret rooms per level

// room function for visualisation
void room_fnc()
{
    set(my, PASSABLE | NOFILTER);
}

// span given position to the grid
void vec_to_grid(VECTOR *pos)
{
    if (!pos)
    {
        diag("\nERROR in vec_to_grid! Given vector pointer doesn't exist!");
        return;
    }

    pos->x += (ROOM_SIZE / 2) * sign(pos->x);
    pos->x = (integer(pos->x / ROOM_SIZE) * ROOM_SIZE);
    pos->y += (ROOM_SIZE / 2) * sign(pos->y);
    pos->y = (integer(pos->y / ROOM_SIZE) * ROOM_SIZE);
    pos->z = 0;
}

// return center of the grid
VECTOR *map_get_center_pos()
{
    VECTOR pos;
    vec_set(&pos, vector(-((GRID_MAX_X / 2) * ROOM_SIZE), -((GRID_MAX_Y / 2) * ROOM_SIZE), 0));
    return &pos;
}

// reset given tile
int map_reset_tile(TILE *tile)
{
    if (!tile)
    {
        diag("\nERROR in map_reset_tile! Tile doesn't exist");
        return false;
    }

    vec_set(&tile->pos, nullvector);

    tile->id = -1;
    tile->pos_x = -1;
    tile->pos_y = -1;

    tile->type = ROOM_NONE;
    tile->secret_chance = 0;

    tile->doors = 0;
    tile->up = false;
    tile->right = false;
    tile->bottom = false;
    tile->left = false;

    tile->secret_doors = 0;
    tile->secret_up = false;
    tile->secret_right = false;
    tile->secret_bottom = false;
    tile->secret_left = false;

    return true;
}

// reset the map
void map_reset_all()
{
    created_rooms = 0; // no rooms yet created

    start_room_tile = NULL;
    boss_room_tile = NULL;
    shop_room_tile = NULL;
    super_secret_room_tile = NULL;

    int i = 0, j = 0;
    for (i = 0; i < GRID_MAX_X; i++)
    {
        for (j = 0; j < GRID_MAX_Y; j++)
        {
            vec_set(&map[i][j].pos, vector(-ROOM_SIZE * i, -ROOM_SIZE * j, 0));

            map[i][j].id = -1;
            map[i][j].pos_x = i;
            map[i][j].pos_y = j;

            map[i][j].type = ROOM_NONE;
            map[i][j].secret_chance = 0;

            map[i][j].doors = 0;
            map[i][j].up = false;
            map[i][j].right = false;
            map[i][j].bottom = false;
            map[i][j].left = false;

            map[i][j].secret_doors = 0;
            map[i][j].secret_up = false;
            map[i][j].secret_right = false;
            map[i][j].secret_bottom = false;
            map[i][j].secret_left = false;
        }
    }
}

// visualize the map for debugging
void map_draw_debug()
{
    var tile_size_factor = 0.25;

    int i = 0, j = 0;
    for (i = 0; i < GRID_MAX_X; i++)
    {
        for (j = 0; j < GRID_MAX_Y; j++)
        {
            VECTOR pos;
            vec_set(&pos, &map[i][j].pos);

            int type = map[i][j].type;
            switch (type)
            {
            case ROOM_NONE:
                draw_point3d(&pos, vector(0, 0, 0), 25, ROOM_SIZE * tile_size_factor); // black
                break;

            case ROOM_NORMAL:
                draw_point3d(&pos, vector(255, 255, 255), 100, ROOM_SIZE * tile_size_factor); // white
                break;

            case ROOM_START:
                draw_point3d(&pos, vector(0, 255, 0), 100, ROOM_SIZE * tile_size_factor); // green
                break;

            case ROOM_BOSS:
                draw_point3d(&pos, vector(0, 0, 255), 100, ROOM_SIZE * tile_size_factor); // red
                break;

            case ROOM_SHOP:
                draw_point3d(&pos, vector(255, 0, 255), 100, ROOM_SIZE * tile_size_factor); // purple
                break;

            case ROOM_ITEM:
                draw_point3d(&pos, vector(255, 0, 0), 100, ROOM_SIZE * tile_size_factor); // blue
                break;

            case ROOM_SECRET:
                draw_point3d(&pos, vector(128, 128, 128), 100, ROOM_SIZE * tile_size_factor); // dark grey
                break;

            case ROOM_SUPER_SECRET:
                draw_point3d(&pos, vector(64, 128, 255), 100, ROOM_SIZE * tile_size_factor); // yellow
                break;
            }

            // show secret room spawning chances
            if (vec_to_screen(&pos, camera))
            {
                draw_text(str_for_int(NULL, map[i][j].secret_chance), pos.x - 4, pos.y - 4, COLOR_RED);
            }
        }
    }
}

// add given tile to the given list
int map_add_tile_to_list(TILE *tile, List *list)
{
    if (!tile)
    {
        diag("\nERROR in map_add_tile_to_list! Tile doesn't exist");
        return false;
    }

    if (!list)
    {
        diag("\nERROR in map_add_tile_to_list! List doesn't exist");
        return false;
    }

    if (list_contains(list, tile) == true)
    {
        diag("\nERROR in map_add_tile_to_list! Tile is already in the given list!");
        return false;
    }

    list_add(list, tile);
    return true;
}

// clear the given list
void map_clear_list(List *list)
{
    if (!list)
    {
        diag("\nERROR in map_clear_list! List doesn't exist");
        return;
    }

    list_clear(list);
}

// clear all lists
void map_clear_lists_all()
{
    map_clear_list(&rooms_queue_list);
    map_clear_list(&end_rooms_list);
    map_clear_list(&item_rooms_list);
    map_clear_list(&secret_rooms_list);
    map_clear_list(&secret_position_list);
    map_clear_list(&super_position_list);
}

// find and return starting room
TILE *map_get_starting_room()
{
    // find the starting room (at the center of the grid)
    VECTOR pos;
    vec_set(&pos, map_get_center_pos());

    // find tile position from the world3d position
    int i = 0, j = 0;
    i = floor(-pos.x / ROOM_SIZE);
    i = clamp(i, -1, GRID_MAX_X);
    j = floor(-pos.y / ROOM_SIZE);
    j = clamp(j, -1, GRID_MAX_Y);

    return &map[i][j];
}

// return pos_x and pos_y of the given tile
// pos_x and pos_y will receive the position
void map_return_pos_x_y(TILE *tile, int *pos_x, int *pos_y)
{
    if (!tile)
    {
        diag("\nERROR in map_return_pos_x_y! Tile doesn't exist!");
        return false;
    }

    int i = 0, j = 0;
    i = floor(-tile->pos.x / ROOM_SIZE);
    i = clamp(i, -1, GRID_MAX_X);
    j = floor(-tile->pos.y / ROOM_SIZE);
    j = clamp(j, -1, GRID_MAX_Y);

    *pos_x = i;
    *pos_y = j;
}

// create room at the given tile with the given params
int map_create_room(TILE *tile, int type)
{
    if (!tile)
    {
        diag("\nERROR in map_create_room! Tile doesn't exist!");
        return false;
    }

    tile->type = type;
    tile->id = created_rooms;
    created_rooms++;
    map_add_tile_to_list(tile, &rooms_queue_list);
}

// return amount of rooms currently given tile is bordering with !
// this is differnet from 'doors' counter, because 'doors' counter shows connections with other rooms
// while this one looks for bordering with rooms, even if they aren't connected (f.e. to find end rooms)
int count_bordering_rooms(TILE *tile)
{
    if (!tile)
    {
        diag("\nERROR in count_bordering_rooms! Tile doesn't exist!");
        return false;
    }

    // given tile's position on the grid
    int pos_x = 0, pos_y = 0;
    map_return_pos_x_y(tile, &pos_x, &pos_y);

    // from start we think that all 4 sides are occupied with other rooms
    int counter = 4;
    if (map[pos_x - 1][pos_y].type == ROOM_NONE)
    {
        counter--;
    }
    if (map[pos_x][pos_y + 1].type == ROOM_NONE)
    {
        counter--;
    }
    if (map[pos_x + 1][pos_y].type == ROOM_NONE)
    {
        counter--;
    }
    if (map[pos_x][pos_y - 1].type == ROOM_NONE)
    {
        counter--;
    }

    return counter;
}

// check if room has same neighbours or not (ignoring ROOM_NONE)
int map_room_all_same_neighbours(TILE *tile)
{
    if (!tile)
    {
        diag("\nERROR in map_room_all_same_neighbours! Tile doesn't exist");
        return false;
    }

    int pos_x = 0, pos_y = 0;
    map_return_pos_x_y(tile, &pos_x, &pos_y);

    int i = 0;
    int type[4];
    int counter = 0;

    for (i = 0; i < 4; i++)
    {
        type[i] = 0;
    }

    // up
    if (map[pos_x - 1][pos_y].type != ROOM_NONE)
    {
        type[counter] = map[pos_x - 1][pos_y].type;
        counter++;
    }

    // right
    if (map[pos_x][pos_y + 1].type != ROOM_NONE)
    {
        type[counter] = map[pos_x][pos_y + 1].type;
        counter++;
    }

    // bottom
    if (map[pos_x + 1][pos_y].type != ROOM_NONE)
    {
        type[counter] = map[pos_x + 1][pos_y].type;
        counter++;
    }

    // left
    if (map[pos_x][pos_y - 1].type != ROOM_NONE)
    {
        type[counter] = map[pos_x][pos_y - 1].type;
        counter++;
    }

    // not enough neighbours to compare...
    if (counter <= 1)
    {
        return false;
    }

    int is_type_same = true;
    int old_type = type[0];

    // check if tiles are all the same
    for (i = 0; i < counter; i++)
    {
        if (old_type != type[i])
        {
            is_type_same = false;
        }
    }

    // so above is true ?
    // that means that all tiles are the same...
    // but we need to make sure, that they are not normal rooms !
    if (old_type == ROOM_NORMAL)
    {
        is_type_same = false;
    }

    return is_type_same;
}

// can this room be added into the queue?
int is_valid_room(TILE *tile)
{
    if (!tile)
    {
        diag("\nERROR in is_valid_room! Tile doesn't exist!");
        return false;
    }

    if (tile->type != ROOM_NONE)
    {
        return false;
    }

    if (count_bordering_rooms(tile) > 1)
    {
        return false;
    }

    if (created_rooms >= max_rooms)
    {
        return false;
    }

    if (random(1) > 0.5)
    {
        return false;
    }

    return true;
}

// find all end rooms (bordering with one 1 room + have only 1 door)
int map_find_end_rooms()
{
    // cycle though the queue of rooms
    TILE *next_room = NULL;
    ListIterator *it = list_begin_iterate(&rooms_queue_list);
    for (next_room = list_iterate(it); it->hasNext; next_room = list_iterate(it))
    {
        // not a room at all ?
        if (next_room->type != ROOM_NORMAL)
        {
            continue;
        }

        // too many doors ?
        if (next_room->doors > 1)
        {
            continue;
        }

        // make sure that there is no bordering rooms around 3 sides
        // if so, then we've found our item room (single neighbour room)
        if (count_bordering_rooms(next_room) <= 1)
        {
            map_add_tile_to_list(next_room, &end_rooms_list);
        }
    }
    list_end_iterate(it);

    // not enough end rooms were created ?
    if (end_rooms_list.count < 3)
    {
        return false;
    }
    else
    {
        return true;
    }
}

// find the boss fight room
// return false is non was found, otherwise - true
int map_find_boss_room()
{
    int pos_x = -1, pos_y = -1;
    var farthest_distance = 0;

    // cycle though the queue of rooms
    TILE *next_room = NULL;
    ListIterator *it = list_begin_iterate(&end_rooms_list);
    for (next_room = list_iterate(it); it->hasNext; next_room = list_iterate(it))
    {
        // distance to the center of the starting room
        var distance = vec_dist(&next_room->pos, &start_room_pos);

        // far enough ?
        if (distance > farthest_distance)
        {
            map_return_pos_x_y(next_room, &pos_x, &pos_y);
            farthest_distance = distance; // save distance as the farthest one
        }
    }
    list_end_iterate(it);

    // boss room was found ?
    if (pos_x != -1 && pos_y != -1)
    {
        map[pos_x][pos_y].type = ROOM_BOSS;
        boss_room_tile = &map[pos_x][pos_y];
    }

    // make sure to return int
    if (boss_room_tile == NULL)
    {
        return false;
    }
    else
    {
        return true;
    }
}

// find shop room
// return false is non was found, otherwise - true
int map_find_shop_room()
{
    // cycle though the queue of rooms
    TILE *next_room = NULL;

    while (!shop_room_tile)
    {
        int index = integer(random(end_rooms_list.count));
        next_room = list_item_at(&end_rooms_list, index);
        if (boss_room_tile != next_room)
        {
            next_room->type = ROOM_SHOP;
            shop_room_tile = next_room;
        }
    }

    // make sure to return int
    if (shop_room_tile == NULL)
    {
        return false;
    }
    else
    {
        return true;
    }
}

// we cycle though the end rooms and first or all define 1 boss room, then 1 shop room
// and then all the rest of end rooms are going to become item rooms !
// return false is non was found or not all end rooms are filled, otherwise - true
int map_find_item_rooms()
{
    // cycle though the queue of rooms
    TILE *next_room = NULL;
    ListIterator *it = list_begin_iterate(&end_rooms_list);
    for (next_room = list_iterate(it); it->hasNext; next_room = list_iterate(it))
    {
        // ignore boss room !
        if (next_room == boss_room_tile)
        {
            continue;
        }

        // ignore shop room !
        if (next_room == shop_room_tile)
        {
            continue;
        }

        next_room->type = ROOM_ITEM;
        map_add_tile_to_list(next_room, &item_rooms_list);
    }
    list_end_iterate(it);

    // make sure to return int
    if (item_rooms_list.count != (end_rooms_list.count - 2))
    {
        return false;
    }
    else
    {
        return true;
    }
}

// check if given tile should be added into the secret level spawning position
int is_valid_secret_pos(TILE *tile)
{
    if (!tile)
    {
        diag("\nERROR in is_valid_secret_pos! Given tile doesn't exist");
        return false;
    }

    // already occupied ?
    if (tile->type != ROOM_NONE)
    {
        return false;
    }

    // not enough chances
    if (tile->secret_chance <= 0)
    {
        return false;
    }

    // was already added into the list ?
    if (list_contains(&secret_position_list, tile) == true)
    {
        return false;
    }

    return true;
}

// calculate chances for all empty tiles to spawn secret on the best tiles
// bordered with 3 rooms, or with at least 2
void map_calculate_secret_pos_chance()
{
    // cycle though the queue
    TILE *next_room = NULL;
    ListIterator *it = list_begin_iterate(&rooms_queue_list);
    for (next_room = list_iterate(it); it->hasNext; next_room = list_iterate(it))
    {
        int pos_x = -1, pos_y = -1;
        map_return_pos_x_y(next_room, &pos_x, &pos_y);

        // up
        if (map[pos_x - 1][pos_y].type == ROOM_NONE)
        {
            map[pos_x - 1][pos_y].secret_chance++;
            if (is_valid_secret_pos(&map[pos_x - 1][pos_y]) == true)
            {
                map_add_tile_to_list(&map[pos_x - 1][pos_y], &secret_position_list);
            }
        }

        // right
        if (map[pos_x][pos_y + 1].type == ROOM_NONE)
        {
            map[pos_x][pos_y + 1].secret_chance++;
            if (is_valid_secret_pos(&map[pos_x][pos_y + 1]) == true)
            {
                map_add_tile_to_list(&map[pos_x][pos_y + 1], &secret_position_list);
            }
        }

        // bottom
        if (map[pos_x + 1][pos_y].type == ROOM_NONE)
        {
            map[pos_x + 1][pos_y].secret_chance++;
            if (is_valid_secret_pos(&map[pos_x + 1][pos_y]) == true)
            {
                map_add_tile_to_list(&map[pos_x + 1][pos_y], &secret_position_list);
            }
        }

        // left
        if (map[pos_x][pos_y - 1].type == ROOM_NONE)
        {
            map[pos_x][pos_y - 1].secret_chance++;
            if (is_valid_secret_pos(&map[pos_x][pos_y - 1]) == true)
            {
                map_add_tile_to_list(&map[pos_x][pos_y - 1], &secret_position_list);
            }
        }
    }
    list_end_iterate(it);
}

// check for the secret room's neighbour to see if we can connect to it or not
int is_valid_secret_neighbours(TILE *tile)
{
    if (!tile)
    {
        diag("\nERROR in is_valid_secret_neighbours! Tile doesn't exist");
        return false;
    }

    // don't point into the void...
    if (tile->type == ROOM_NONE)
    {
        return false;
    }

    // don't point into the boss room
    // ignore boss room
    if (tile->type == ROOM_BOSS)
    {
        return false;
    }

    // or into the super secret !
    if (tile->type == ROOM_SUPER_SECRET)
    {
        return false;
    }

    return true;
}

// find secret rooms
// return false is non was found, otherwise - true
int map_find_secret_rooms()
{
    // we sort this like this (by hand)...
    // ugly but works and not super slow, so it's kinda ok
    // first we try to occupy all tiles with secret chance 4
    int i = 0;
    for (i = 0; i < 4; i++)
    {
        // enough secret rooms ?
        if (secret_rooms_list.count >= max_secrets)
        {
            break;
        }

        TILE *next_room = NULL;
        ListIterator *it = list_begin_iterate(&secret_position_list);
        for (next_room = list_iterate(it); it->hasNext; next_room = list_iterate(it))
        {
            // enough secret rooms ?
            if (secret_rooms_list.count >= max_secrets)
            {
                break;
            }

            // make sure that we don't spawn between all same kind of tiles (f.e. only between item rooms)
            // maybe this isn't really needed, but we'll leave this like this for now
            if (map_room_all_same_neighbours(next_room) == true)
            {
                continue;
            }

            // not enough chance ?
            // 4, 3, 2
            if (next_room->secret_chance != 4 - i)
            {
                continue;
            }

            // add into the secret list
            map_add_tile_to_list(next_room, &secret_rooms_list);
        }
        list_end_iterate(it);
    }

    // cycle though the secret rooms and set their connections
    TILE *next_room = NULL;
    ListIterator *it = list_begin_iterate(&secret_rooms_list);
    for (next_room = list_iterate(it); it->hasNext; next_room = list_iterate(it))
    {
        int pos_x = 0, pos_y = 0;
        map_return_pos_x_y(next_room, &pos_x, &pos_y);

        // we finally found the secret room...
        next_room->type = ROOM_SECRET;

        // up neighbour
        if (is_valid_secret_neighbours(&map[pos_x - 1][pos_y]) == true)
        {
            map[pos_x - 1][pos_y].secret_bottom = true;
            map[pos_x - 1][pos_y].secret_doors++;

            next_room->up = true;
            next_room->doors++;
        }

        // right neighbour
        if (is_valid_secret_neighbours(&map[pos_x][pos_y + 1]) == true)
        {
            map[pos_x][pos_y + 1].secret_left = true;
            map[pos_x][pos_y + 1].secret_doors++;

            next_room->right = true;
            next_room->doors++;
        }

        // bottom neighbour
        if (is_valid_secret_neighbours(&map[pos_x + 1][pos_y]) == true)
        {
            map[pos_x + 1][pos_y].secret_up = true;
            map[pos_x + 1][pos_y].secret_doors++;

            next_room->bottom = true;
            next_room->doors++;
        }

        // left neighbour
        if (is_valid_secret_neighbours(&map[pos_x][pos_y - 1]) == true)
        {
            map[pos_x][pos_y - 1].secret_right = true;
            map[pos_x][pos_y - 1].secret_doors++;

            next_room->left = true;
            next_room->doors++;
        }
    }
    list_end_iterate(it);

    // make sure to return int
    if (secret_rooms_list.count < max_secrets)
    {
        return false;
    }
    else
    {
        return true;
    }
}

// check if neighbours of the given tile are normal rooms
// false - if they are not, otherwise - true
int is_valid_super_secret_neighbour(TILE *tile)
{
    if (!tile)
    {
        diag("\nERROR in is_valid_super_secret_neighbour! Tile doesn't exist");
        return false;
    }

    int pos_x = 0, pos_y = 0;
    map_return_pos_x_y(tile, &pos_x, &pos_y);

    int i = 0;
    int type[4];
    int counter = 0;

    for (i = 0; i < 4; i++)
    {
        type[i] = 0;
    }

    // up
    if (map[pos_x - 1][pos_y].type != ROOM_NONE)
    {
        type[counter] = map[pos_x - 1][pos_y].type;
        counter++;
    }

    // right
    if (map[pos_x][pos_y + 1].type != ROOM_NONE)
    {
        type[counter] = map[pos_x][pos_y + 1].type;
        counter++;
    }

    // bottom
    if (map[pos_x + 1][pos_y].type != ROOM_NONE)
    {
        type[counter] = map[pos_x + 1][pos_y].type;
        counter++;
    }

    // left
    if (map[pos_x][pos_y - 1].type != ROOM_NONE)
    {
        type[counter] = map[pos_x][pos_y - 1].type;
        counter++;
    }

    int is_type_same = true;
    int old_type = ROOM_NORMAL;

    // check if tiles are all the same
    for (i = 0; i < counter; i++)
    {
        if (old_type != type[i])
        {
            is_type_same = false;
        }
    }

    return is_type_same;
}

// find super secret room !
int map_find_super_secret_room()
{
    TILE *next_room = NULL;
    ListIterator *it = list_begin_iterate(&secret_position_list);
    for (next_room = list_iterate(it); it->hasNext; next_room = list_iterate(it))
    {
        // make sure to find places with only one secret chance !
        if (next_room->secret_chance != 1)
        {
            continue;
        }

        // make sure that it's bordering with only one room
        if (count_bordering_rooms(next_room) > 1)
        {
            continue;
        }

        // also make sure that that single room we are next to is a normal room !
        if (is_valid_super_secret_neighbour(next_room) == false)
        {
            continue;
        }

        // add into the secret list
        map_add_tile_to_list(next_room, &super_position_list);
    }
    list_end_iterate(it);

    // no positions were found ?
    if (super_position_list.count <= 0)
    {
        return false;
    }

    // random index from the available super secret room positions
    int index = integer(random(super_position_list.count));

    // find the proper tile from the list
    TILE *tile = list_item_at(&super_position_list, index);
    tile->type = ROOM_SUPER_SECRET;

    int pos_x = 0, pos_y = 0;
    map_return_pos_x_y(tile, &pos_x, &pos_y);

    // update connection with the map
    // up neighbour
    if (is_valid_secret_neighbours(&map[pos_x - 1][pos_y]) == true)
    {
        map[pos_x - 1][pos_y].secret_bottom = true;
        map[pos_x - 1][pos_y].secret_doors++;

        tile->up = true;
        tile->doors++;
    }

    // right neighbour
    if (is_valid_secret_neighbours(&map[pos_x][pos_y + 1]) == true)
    {
        map[pos_x][pos_y + 1].secret_left = true;
        map[pos_x][pos_y + 1].secret_doors++;

        tile->right = true;
        tile->doors++;
    }

    // bottom neighbour
    if (is_valid_secret_neighbours(&map[pos_x + 1][pos_y]) == true)
    {
        map[pos_x + 1][pos_y].secret_up = true;
        map[pos_x + 1][pos_y].secret_doors++;

        tile->bottom = true;
        tile->doors++;
    }

    // left neighbour
    if (is_valid_secret_neighbours(&map[pos_x][pos_y - 1]) == true)
    {
        map[pos_x][pos_y - 1].secret_right = true;
        map[pos_x][pos_y - 1].secret_doors++;

        tile->left = true;
        tile->doors++;
    }

    return true;
}

// generate new map
void map_generate()
{
    // random amount of max_room per each generation
    max_rooms = (integer(random(3)) + 4 + level_id * 2); // * 2;

    // max 1 - 2 secret room per level
    max_secrets = 1 + integer(random(2));

    // clear all previous maps
    map_reset_all();

    // clear all previous data from the lists
    map_clear_lists_all();

    // this part is needed for removing created room tiles
    level_load("");
    wait(1);
    VECTOR pos;
    vec_set(&pos, map_get_center_pos());
    vec_set(&camera->x, vector(pos.x, pos.y, 1000));
    vec_set(&camera->pan, vector(0, -90, 0));

    // add starting room to the queue
    TILE *starting_room = map_get_starting_room();
    vec_set(&start_room_pos, &starting_room->pos);
    map_create_room(starting_room, ROOM_START);
    start_room_tile = starting_room;

    // cycle though the queue
    TILE *next_room = NULL;
    ListIterator *it = list_begin_iterate(&rooms_queue_list);
    for (next_room = list_iterate(it); it->hasNext; next_room = list_iterate(it))
    {
        int pos_x = 0, pos_y = 0;
        map_return_pos_x_y(next_room, &pos_x, &pos_y);

        // up neighbour
        if (is_valid_room(&map[pos_x - 1][pos_y]) == true)
        {
            map_create_room(&map[pos_x - 1][pos_y], ROOM_NORMAL);
            map[pos_x - 1][pos_y].bottom = true;
            map[pos_x - 1][pos_y].doors++;

            next_room->up = true;
            next_room->doors++;
        }

        // right neighbour
        if (is_valid_room(&map[pos_x][pos_y + 1]) == true)
        {
            map_create_room(&map[pos_x][pos_y + 1], ROOM_NORMAL);
            map[pos_x][pos_y + 1].left = true;
            map[pos_x][pos_y + 1].doors++;

            next_room->right = true;
            next_room->doors++;
        }

        // bottom neighbour
        if (is_valid_room(&map[pos_x + 1][pos_y]) == true)
        {
            map_create_room(&map[pos_x + 1][pos_y], ROOM_NORMAL);
            map[pos_x + 1][pos_y].up = true;
            map[pos_x + 1][pos_y].doors++;

            next_room->bottom = true;
            next_room->doors++;
        }

        // left neighbour
        if (is_valid_room(&map[pos_x][pos_y - 1]) == true)
        {
            map_create_room(&map[pos_x][pos_y - 1], ROOM_NORMAL);
            map[pos_x][pos_y - 1].right = true;
            map[pos_x][pos_y - 1].doors++;

            next_room->left = true;
            next_room->doors++;
        }
    }
    list_end_iterate(it);

    // make sure we have proper amount of rooms
    if (created_rooms != max_rooms)
    {
        map_generate();
        return;
    }

    // if our map is accepted, then find end rooms
    // we need to have at least 3 end rooms per level (item, shop, boss)
    if (map_find_end_rooms() == NULL)
    {
        map_generate();
        diag("\nERROR in dungeon generation! Not enough of end rooms were created!");
        return;
    }

    // find boss fight room
    // no boss room found (for some reason...)
    if (map_find_boss_room() == NULL)
    {
        map_generate();
        diag("\nERROR in dungeon generation! Boss fight room wasn't created!");
        return;
    }

    // no shop room found ?
    if (map_find_shop_room() == NULL)
    {
        map_generate();
        diag("\nERROR in dungeon generation! Shop room wasn't created!");
        return;
    }

    // no item rooms or not all of the rest end rooms occupied ?
    if (map_find_item_rooms() == NULL)
    {
        map_generate();
        diag("\nERROR in dungeon generation! No item rooms were created!");
        return;
    }

    // calculate positions for the secrets to spawn at
    map_calculate_secret_pos_chance();

    // no secret rooms ?
    if (map_find_secret_rooms() == NULL)
    {
        map_generate();
        diag("\nERROR in dungeon generation! No secret rooms were created!");
        return;
    }

    // no super secret room ?
    if (map_find_super_secret_room() == NULL)
    {
        map_generate();
        diag("\nERROR in dungeon generation! No super secret room was created!");
        return;
    }

    // create all 3d rooms
    // currently only for visualisation/debugging
    // this is very ugly... and will be removed !
    int i = 0, j = 0;
    for (i = 0; i < GRID_MAX_X; i++)
    {
        for (j = 0; j < GRID_MAX_Y; j++)
        {
            int type = map[i][j].type;

            // room doesn't exist ?
            if (type == ROOM_NONE)
            {
                continue;
            }

            VECTOR pos;
            vec_set(&pos, &map[i][j].pos);

            int room_up = map[i][j].up;
            int room_right = map[i][j].right;
            int room_bottom = map[i][j].bottom;
            int room_left = map[i][j].left;

            if (room_up == true && room_right == false && room_bottom == false && room_left == false) // only up / right / bottom / left
            {
                ENTITY *tile_ent = ent_create(room_only_up_mdl, &pos, room_fnc);
                if (type == ROOM_SECRET || type == ROOM_SUPER_SECRET)
                {
                    if (tile_ent)
                    {
                        set(tile_ent, TRANSLUCENT);
                    }
                }
            }
            else if (room_up == false && room_right == true && room_bottom == false && room_left == false)
            {
                ENTITY *tile_ent = ent_create(room_only_right_mdl, &pos, room_fnc);
                if (type == ROOM_SECRET || type == ROOM_SUPER_SECRET)
                {
                    if (tile_ent)
                    {
                        set(tile_ent, TRANSLUCENT);
                    }
                }
            }
            else if (room_up == false && room_right == false && room_bottom == true && room_left == false)
            {
                ENTITY *tile_ent = ent_create(room_only_bottom_mdl, &pos, room_fnc);
                if (type == ROOM_SECRET || type == ROOM_SUPER_SECRET)
                {
                    if (tile_ent)
                    {
                        set(tile_ent, TRANSLUCENT);
                    }
                }
            }
            else if (room_up == false && room_right == false && room_bottom == false && room_left == true)
            {
                ENTITY *tile_ent = ent_create(room_only_left_mdl, &pos, room_fnc);
                if (type == ROOM_SECRET || type == ROOM_SUPER_SECRET)
                {
                    if (tile_ent)
                    {
                        set(tile_ent, TRANSLUCENT);
                    }
                }
            }
            else if (room_up == true && room_right == true && room_bottom == false && room_left == false) // two sides at the same time
            {
                ENTITY *tile_ent = ent_create(room_up_right_mdl, &pos, room_fnc);
                if (type == ROOM_SECRET || type == ROOM_SUPER_SECRET)
                {
                    if (tile_ent)
                    {
                        set(tile_ent, TRANSLUCENT);
                    }
                }
            }
            else if (room_up == false && room_right == true && room_bottom == true && room_left == false)
            {
                ENTITY *tile_ent = ent_create(room_right_bottom_mdl, &pos, room_fnc);
                if (type == ROOM_SECRET || type == ROOM_SUPER_SECRET)
                {
                    if (tile_ent)
                    {
                        set(tile_ent, TRANSLUCENT);
                    }
                }
            }
            else if (room_up == false && room_right == false && room_bottom == true && room_left == true)
            {
                ENTITY *tile_ent = ent_create(room_bottom_left_mdl, &pos, room_fnc);
                if (type == ROOM_SECRET || type == ROOM_SUPER_SECRET)
                {
                    if (tile_ent)
                    {
                        set(tile_ent, TRANSLUCENT);
                    }
                }
            }
            else if (room_up == true && room_right == false && room_bottom == false && room_left == true)
            {
                ENTITY *tile_ent = ent_create(room_left_up_mdl, &pos, room_fnc);
                if (type == ROOM_SECRET || type == ROOM_SUPER_SECRET)
                {
                    if (tile_ent)
                    {
                        set(tile_ent, TRANSLUCENT);
                    }
                }
            }
            else if (room_up == true && room_right == false && room_bottom == true && room_left == false) // parallel sides ?
            {
                ENTITY *tile_ent = ent_create(room_up_bottom_mdl, &pos, room_fnc);
                if (type == ROOM_SECRET || type == ROOM_SUPER_SECRET)
                {
                    if (tile_ent)
                    {
                        set(tile_ent, TRANSLUCENT);
                    }
                }
            }
            else if (room_up == false && room_right == true && room_bottom == false && room_left == true)
            {
                ENTITY *tile_ent = ent_create(room_left_right_mdl, &pos, room_fnc);
                if (type == ROOM_SECRET || type == ROOM_SUPER_SECRET)
                {
                    if (tile_ent)
                    {
                        set(tile_ent, TRANSLUCENT);
                    }
                }
            }
            else if (room_up == true && room_right == true && room_bottom == false && room_left == true) // three sides ?
            {
                ENTITY *tile_ent = ent_create(room_up_right_left_mdl, &pos, room_fnc);
                if (type == ROOM_SECRET || type == ROOM_SUPER_SECRET)
                {
                    if (tile_ent)
                    {
                        set(tile_ent, TRANSLUCENT);
                    }
                }
            }
            else if (room_up == true && room_right == true && room_bottom == true && room_left == false)
            {
                ENTITY *tile_ent = ent_create(room_up_right_bottom_mdl, &pos, room_fnc);
                if (type == ROOM_SECRET || type == ROOM_SUPER_SECRET)
                {
                    if (tile_ent)
                    {
                        set(tile_ent, TRANSLUCENT);
                    }
                }
            }
            else if (room_up == false && room_right == true && room_bottom == true && room_left == true)
            {
                ENTITY *tile_ent = ent_create(room_right_bottom_left_mdl, &pos, room_fnc);
                if (type == ROOM_SECRET || type == ROOM_SUPER_SECRET)
                {
                    if (tile_ent)
                    {
                        set(tile_ent, TRANSLUCENT);
                    }
                }
            }
            else if (room_up == true && room_right == false && room_bottom == true && room_left == true)
            {
                ENTITY *tile_ent = ent_create(room_bottom_left_up_mdl, &pos, room_fnc);
                if (type == ROOM_SECRET || type == ROOM_SUPER_SECRET)
                {
                    if (tile_ent)
                    {
                        set(tile_ent, TRANSLUCENT);
                    }
                }
            }
            else if (room_up == true && room_right == true && room_bottom == true && room_left == true) // all 4 doors ?
            {
                ENTITY *tile_ent = ent_create(room_all_mdl, &pos, room_fnc);
                if (type == ROOM_SECRET || type == ROOM_SUPER_SECRET)
                {
                    if (tile_ent)
                    {
                        set(tile_ent, TRANSLUCENT);
                    }
                }
            }
        }
    }
}

void on_exit_event()
{
    map_clear_lists_all();
}

void main()
{
    on_exit = on_exit_event;
    on_space = map_generate;

    fps_max = 60;
    warn_level = 6;

    random_seed(0);

    video_mode = 9;
    video_switch(video_mode, 0, 0);

    level_load("");

    map_generate();

    VECTOR pos;
    vec_set(&pos, map_get_center_pos());
    vec_set(&camera->x, vector(pos.x, pos.y, 1000));
    vec_set(&camera->pan, vector(0, -90, 0));

    mouse_pointer = 2;
    mouse_mode = 4;

    while (!key_esc)
    {
        DEBUG_VAR(item_rooms_list.count, 0);
        DEBUG_VAR(end_rooms_list.count, 20);
        DEBUG_VAR((end_rooms_list.count - 2), 40);

        draw_text(
            str_printf(NULL,
                       "created room: %d\nmax rooms: %d\nqueue count: %d\nend rooms: %d\nsecret rooms: %d\nmax secrets: %d\nitem rooms: %d",
                       (long)created_rooms, (long)max_rooms, (long)rooms_queue_list.count, (long)end_rooms_list.count, (long)secret_rooms_list.count, (long)max_secrets, (long)item_rooms_list.count),
            10, 240, COLOR_RED);

        VECTOR mouse_3d_pos;
        vec_set(&mouse_3d_pos, vector(mouse_pos.x, mouse_pos.y, camera->z));
        vec_for_screen(&mouse_3d_pos, camera);
        vec_to_grid(&mouse_3d_pos);
        draw_point3d(vector(mouse_3d_pos.x, mouse_3d_pos.y, 0), COLOR_RED, 100, ROOM_SIZE * 0.5);

        int i = 0, j = 0;
        i = floor(-mouse_3d_pos.x / ROOM_SIZE);
        i = clamp(i, -1, GRID_MAX_X);
        j = floor(-mouse_3d_pos.y / ROOM_SIZE);
        j = clamp(j, -1, GRID_MAX_Y);

        if (i >= 0 && i < GRID_MAX_X && j >= 0 && j < GRID_MAX_Y)
        {
            DEBUG_VAR(map_room_all_same_neighbours(&map[i][j]), 80);

            draw_text(
                str_printf(NULL,
                           "type: %d\ni: %d\nj: %d\nid: %d\ndoors: %d\nup: %d\nright: %d\nbottom: %d\nleft: %d\nsecret up: %d\nsecret right: %d\nsecret bottom: %d\nsecret left: %d",
                           (long)map[i][j].type, (long)map[i][j].pos_x, (long)map[i][j].pos_y, (long)map[i][j].id, (long)map[i][j].doors, (long)map[i][j].up, (long)map[i][j].right, (long)map[i][j].bottom, (long)map[i][j].left, (long)map[i][j].secret_up, (long)map[i][j].secret_right, (long)map[i][j].secret_bottom, (long)map[i][j].secret_left),
                10, 420, COLOR_RED);
        }

        if (key_enter)
        {
            map_draw_debug();
        }

        wait(1);
    }
    sys_exit(NULL);
}