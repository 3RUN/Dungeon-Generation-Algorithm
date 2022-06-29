#include <acknex.h>
#include <default.c>

#define PRAGMA_POINTER

#include "vector2d.c"
#include "dynamic_array.h"

#define MIN(x, y) ifelse(x <= y, x, y)
#define MAX(x, y) ifelse(x >= y, x, y)
#define RANDOM_CHANCE(c) integer(random(c)) == 0
#define RANDOM_RANGE(min, max) (min + integer(random(max - min)))

#define MAP_WIDTH 15
#define MAP_HEIGHT 15
#define MAP_CELL_SIZE 32

#define ROOM_NONE 0
#define ROOM_NORMAL 1
#define ROOM_START 2
#define ROOM_BOSS 3
#define ROOM_SPECIAL 4
#define ROOM_LOCKED 5
#define ROOM_SECRET 6
#define ROOM_SUPER_SECRET 7

#define DEBUG_FONT_SCALE 0.5

#define MAX_ROOM_SPRITES 16
#define ROOM_TOP 0
#define ROOM_RIGHT 1
#define ROOM_BOTTOM 2
#define ROOM_LEFT 3
#define ROOM_TOP_N_RIGHT 4
#define ROOM_RIGHT_N_BOTTOM 5
#define ROOM_BOTTOM_N_LEFT 6
#define ROOM_LEFT_N_TOP 7
#define ROOM_LEFT_TOP_N_RIGHT 8
#define ROOM_TOP_RIGHT_N_BOTTOM 9
#define ROOM_RIGHT_BOTTOM_N_LEFT 10
#define ROOM_BOTTOM_LEFT_N_TOP 11
#define ROOM_TOP_N_BOTTOM 12
#define ROOM_RIGHT_N_LEFT 13
#define ROOM_ALL_SIDES 14
#define ROOM_VOID 15

#define CARDINAL_DIRECTIONS 4
#define CARDINAL_TOP 0
#define CARDINAL_RIGHT 1
#define CARDINAL_BOTTOM 2
#define CARDINAL_LEFT 3

// up, right, bottom, left
// up-right, right-bottom, bottom-left, left-up
// left-up-right, up-right-bottom, right-bottom-left, bottom-left-up
// up-bottom, left-right, all-sides, none
BMAP *rooms_pcx = "rooms.pcx";
BMAP *room[MAX_ROOM_SPRITES];

typedef struct Tile
{
    int region;
    int x;
    int y;

    int type;

    int doors;
    int top;
    int right;
    int bottom;
    int left;

    int secret_chance;
    int secret_doors;
    int secret_top;
    int secret_right;
    int secret_bottom;
    int secret_left;
} Tile;

int level_id = 2;
int max_level_id = 5;
int max_rooms = 0;
int max_secrets = 0;
int max_item_rooms = 0;
int created_item_rooms = 0;
int created_secret_rooms = 0;
int boss_room_found = false;
int shop_room_found = false;
int super_secret_created = false;

Array *rooms_queue_list;
Array *end_rooms_list;
Array *secret_positions_list;
Array *secret_rooms_list;
Array *super_positions_list;

Tile map[MAP_WIDTH][MAP_HEIGHT];

Vector2d cardinal_dir[CARDINAL_DIRECTIONS];

int is_in_map(int x, int y)
{
    return x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT;
}

void snap_to_grid(Vector2d *pos)
{
    if (!pos)
        return;

    pos->x += (MAP_CELL_SIZE / 2) * sign(pos->x);
    pos->x = (integer(pos->x / MAP_CELL_SIZE) * MAP_CELL_SIZE);
    pos->y += (MAP_CELL_SIZE / 2) * sign(pos->y);
    pos->y = (integer(pos->y / MAP_CELL_SIZE) * MAP_CELL_SIZE);
}

void get_map_center(int *x, int *y)
{
    *x = integer(MAP_WIDTH / 2);
    *y = integer(MAP_HEIGHT / 2);
}

void reset_tile(Tile *tile, int x, int y)
{
    if (!tile)
        return;

    tile->region = -1;
    tile->x = x;
    tile->y = y;

    tile->type = ROOM_NONE;

    tile->doors = 0;
    tile->top = false;
    tile->right = false;
    tile->bottom = false;
    tile->left = false;

    tile->secret_chance = 0;
    tile->secret_doors = 0;
    tile->secret_top = false;
    tile->secret_right = false;
    tile->secret_bottom = false;
    tile->secret_left = false;
}

void map_reset()
{
    int x = 0, y = 0;
    for (y = 0; y < MAP_HEIGHT; y++)
        for (x = 0; x < MAP_WIDTH; x++)
            reset_tile(&map[x][y], x, y);
}

void room_create(Tile *tile, int type)
{
    if (!tile)
        return;

    array_add(rooms_queue_list, tile);

    tile->type = type;
    tile->region = array_size(rooms_queue_list);
}

int count_bordering_rooms(Tile *tile)
{
    if (!tile)
        return false;

    int x = tile->x;
    int y = tile->y;

    int n = 0, counter = 4;
    for (n = 0; n < CARDINAL_DIRECTIONS; n++)
    {
        int nx = x + cardinal_dir[n].x;
        int ny = y + cardinal_dir[n].y;
        if (!is_in_map(nx, ny))
            continue;

        if (map[nx][ny].type != ROOM_NONE)
            continue;

        counter--;
    }

    return counter;
}

int is_valid_neighbour(Tile *neighbour)
{
    if (!neighbour)
        return false;

    if (neighbour->type != ROOM_NONE)
        return false;

    if (count_bordering_rooms(neighbour) > 1)
        return false;

    if (array_size(rooms_queue_list) >= max_rooms)
        return false;

    if (RANDOM_CHANCE(2))
        return false;

    return true;
}

void map_add_rooms()
{
    // add starting room to the queue
    int start_x = -1, start_y = -1;
    get_map_center(&start_x, &start_y);
    if (!is_in_map(start_x, start_y))
        return;

    room_create(&map[start_x][start_y], ROOM_START);

    // cycle though the queue
    array_enumerate_begin(Tile *, rooms_queue_list, v)
    {
        if (!v)
            continue;

        int x = v->x;
        int y = v->y;

        int n = 0;
        for (n = 0; n < CARDINAL_DIRECTIONS; n++)
        {
            int nx = x + cardinal_dir[n].x;
            int ny = y + cardinal_dir[n].y;
            if (!is_in_map(nx, ny))
                continue;

            Tile *neighbour = &map[nx][ny];
            if (!neighbour)
                continue;

            if (!is_valid_neighbour(neighbour))
                continue;

            v->doors++;
            neighbour->doors++;

            switch (n)
            {
            case CARDINAL_TOP:
                v->top = true;
                neighbour->bottom = true;
                break;

            case CARDINAL_RIGHT:
                v->right = true;
                neighbour->left = true;
                break;

            case CARDINAL_BOTTOM:
                v->bottom = true;
                neighbour->top = true;
                break;

            case CARDINAL_LEFT:
                v->left = true;
                neighbour->right = true;
                break;
            }

            room_create(neighbour, ROOM_NORMAL);
        }
    }
    array_enumerate_end(rooms_queue_list);
}

void map_add_tile_to_array(Tile *tile, Array *array)
{
    if (!tile || !array)
        return;

    array_add(array, tile);
}

void map_find_end_rooms()
{
    array_enumerate_begin(Tile *, rooms_queue_list, v)
    {
        if (!v)
            continue;

        if (v->type != ROOM_NORMAL)
            continue;

        if (v->doors > 1)
            continue;

        if (count_bordering_rooms(v) > 1)
            continue;

        map_add_tile_to_array(v, end_rooms_list);
    }
    array_enumerate_end(rooms_queue_list);
}

var get_distance(int x1, int y1, int x2, int y2)
{
    VECTOR start, end;
    vec_set(&start, vector(x1 * MAP_CELL_SIZE, y1 * MAP_CELL_SIZE, 0));
    vec_set(&end, vector(x2 * MAP_CELL_SIZE, y2 * MAP_CELL_SIZE, 0));
    return vec_dist(&start, &end);
}

void map_find_boss_room()
{
    int x = -1, y = -1;
    var farthest_distance = 0;

    int start_x = -1, start_y = -1;
    get_map_center(&start_x, &start_y);
    if (!is_in_map(start_x, start_y))
        return;

    array_enumerate_begin(Tile *, end_rooms_list, v)
    {
        if (!v)
            continue;

        var dist = get_distance(v->x, v->y, start_x, start_y);

        if (dist >= farthest_distance)
        {
            x = v->x;
            y = v->y;
            farthest_distance = dist;
        }
    }
    array_enumerate_end(end_rooms_list);

    if (x != -1 && y != -1)
        boss_room_found = true;

    if (boss_room_found == true)
        map[x][y].type = ROOM_BOSS;
}

void map_find_shop_room()
{
    int start_x = -1, start_y = -1;
    get_map_center(&start_x, &start_y);
    if (!is_in_map(start_x, start_y))
        return;

    int x = -1, y = -1;
    Tile *temp_tile = array_first(Tile *, end_rooms_list);
    if (!temp_tile)
        return;

    var closest_distance = get_distance(temp_tile->x, temp_tile->y, start_x, start_y);

    array_enumerate_begin(Tile *, end_rooms_list, v)
    {
        if (!v)
            continue;

        if (v->type != ROOM_NORMAL)
            continue;

        var dist = get_distance(v->x, v->y, start_x, start_y);
        if (dist <= closest_distance)
        {
            x = v->x;
            y = v->y;
            closest_distance = dist;
        }
    }
    array_enumerate_end(end_rooms_list);

    if (x != -1 && y != -1)
        shop_room_found = true;

    if (shop_room_found == true)
        map[x][y].type = ROOM_SPECIAL;
}

void map_find_item_rooms()
{
    array_enumerate_begin(Tile *, end_rooms_list, v)
    {
        if (!v)
            continue;

        if (v->type != ROOM_NORMAL)
            continue;

        v->type = ROOM_LOCKED;

        created_item_rooms++;
        if (created_item_rooms >= max_item_rooms)
            break;
    }
    array_enumerate_end(end_rooms_list);
}

int map_secret_position_contains(Tile *tile)
{
    if (!tile)
        return false;

    int found = false;
    array_enumerate_begin(Tile *, secret_positions_list, v)
    {
        if (!v)
            continue;

        if (found)
            break;

        if (v->x == tile->x && v->y == tile->y)
            found = true;
    }
    array_enumerate_end(secret_positions_list);
    return found;
}

int map_normal_neighbour_counter(int x, int y, int type)
{
    if (!is_in_map(x, y))
        return false;

    int n = 0, counter = 0;
    for (n = 0; n < CARDINAL_DIRECTIONS; n++)
    {
        int nx = x + cardinal_dir[n].x;
        int ny = y + cardinal_dir[n].y;
        if (!is_in_map(nx, ny))
            continue;

        Tile *neighbour = &map[nx][ny];
        if (!neighbour)
            continue;

        if (neighbour->type != type)
            continue;

        counter++;
    }

    return counter;
}

int map_valid_secret_position(Tile *tile)
{
    if (!tile)
        return false;

    if (tile->type != ROOM_NONE)
        return false;

    if (tile->secret_chance <= 0)
        return false;

    if (map_secret_position_contains(tile))
        return false;

    if (map_normal_neighbour_counter(tile->x, tile->y, ROOM_NORMAL) <= 0)
        return false;

    if (map_normal_neighbour_counter(tile->x, tile->y, ROOM_BOSS) > 0)
        return false;

    return true;
}

void map_find_secret_positions()
{
    array_enumerate_begin(Tile *, rooms_queue_list, v)
    {
        if (!v)
            continue;

        if (v->type == ROOM_START || v->type == ROOM_BOSS)
            continue;

        int x = v->x;
        int y = v->y;

        int n = 0;
        for (n = 0; n < CARDINAL_DIRECTIONS; n++)
        {
            int nx = x + cardinal_dir[n].x;
            int ny = y + cardinal_dir[n].y;
            if (!is_in_map(nx, ny))
                continue;

            Tile *neighbour = &map[nx][ny];
            if (!neighbour)
                continue;

            if (neighbour->type != ROOM_NONE)
                continue;

            neighbour->secret_chance++;

            if (!map_valid_secret_position(neighbour))
                continue;

            array_add(secret_positions_list, neighbour);
        }
    }
    array_enumerate_end(rooms_queue_list);
}

int is_secret_already_added(Tile *tile)
{
    if (!tile)
        return false;

    int found = false;
    array_enumerate_begin(Tile *, secret_rooms_list, v)
    {
        if (!v)
            continue;

        if (found)
            break;

        if (v->x == tile->x && v->y == tile->y)
            found = true;
    }
    array_enumerate_end(secret_rooms_list);

    return found;
}

int is_valid_secret_neighbour(Tile *tile)
{
    if (!tile)
        return false;

    if (tile->type == ROOM_NONE || tile->type == ROOM_BOSS || tile->type == ROOM_SECRET || tile->type == ROOM_SUPER_SECRET)
        return false;

    return true;
}

void map_add_secret_rooms()
{
    var lifespan = 1;
    while (lifespan > 0)
    {
        lifespan -= time_frame / 16;

        int highest_chance = 0, x = -1, y = -1;
        array_enumerate_begin(Tile *, secret_positions_list, v)
        {
            if (!v)
                continue;

            if (v->type != ROOM_NONE)
                continue;

            if (v->secret_chance < highest_chance)
                continue;

            if (is_secret_already_added(v))
                continue;

            x = v->x;
            y = v->y;
            highest_chance = v->secret_chance;
        }
        array_enumerate_end(secret_positions_list);

        if (x != -1 && y != -1)
            array_add(secret_rooms_list, &map[x][y]);

        if (array_size(secret_rooms_list) >= max_secrets)
            break;
    }

    array_enumerate_begin(Tile *, secret_rooms_list, v)
    {
        if (!v)
            continue;

        v->type = ROOM_SECRET;
        v->region = 0;

        int x = v->x;
        int y = v->y;

        int n = 0;
        for (n = 0; n < CARDINAL_DIRECTIONS; n++)
        {
            int nx = x + cardinal_dir[n].x;
            int ny = y + cardinal_dir[n].y;
            if (!is_in_map(nx, ny))
                continue;

            Tile *neighbour = &map[nx][ny];
            if (!neighbour)
                continue;

            if (!is_valid_secret_neighbour(neighbour))
                continue;

            v->doors++;
            neighbour->secret_doors++;

            switch (n)
            {
            case CARDINAL_TOP:
                neighbour->secret_bottom = true;
                v->top = true;
                break;

            case CARDINAL_RIGHT:
                neighbour->secret_left = true;
                v->right = true;
                break;

            case CARDINAL_BOTTOM:
                neighbour->secret_top = true;
                v->bottom = true;
                break;

            case CARDINAL_LEFT:
                neighbour->secret_right = true;
                v->left = true;
                break;
            }
        }

        created_secret_rooms++;
        if (created_secret_rooms >= max_secrets)
            break;
    }
    array_enumerate_end(secret_rooms_list);
}

int is_valid_super_secret_neighbour(Tile *tile)
{
    if (!tile)
        return false;
}

void map_add_super_secret_positions()
{
    array_enumerate_begin(Tile *, secret_positions_list, v)
    {
        if (!v)
            continue;

        if (v->secret_chance != 1)
            continue;

        if (count_bordering_rooms(v) > 1)
            continue;

        int x = v->x;
        int y = v->y;

        int n = 0, counter = 0;
        for (n = 0; n < CARDINAL_DIRECTIONS; n++)
        {
            int nx = x + cardinal_dir[n].x;
            int ny = y + cardinal_dir[n].y;
            if (!is_in_map(nx, ny))
                continue;

            Tile *neighbour = &map[nx][ny];
            if (!neighbour)
                continue;

            if (neighbour->type == ROOM_NONE)
                continue;

            if (neighbour->type != ROOM_NORMAL)
                continue;

            counter++;
        }

        if (counter <= 0)
            continue;

        array_add(super_positions_list, v);
    }
    array_enumerate_end(secret_positions_list);
}

void map_add_super_secret_room()
{
    int index = RANDOM_RANGE(0, array_size(super_positions_list));
    Tile *super_secret_room = array_get_at(Tile *, super_positions_list, index);
    if (!super_secret_room)
        return;

    super_secret_room->type = ROOM_SUPER_SECRET;
    super_secret_room->region = 0;

    int x = super_secret_room->x;
    int y = super_secret_room->y;

    int n = 0, counter = 0;
    for (n = 0; n < CARDINAL_DIRECTIONS; n++)
    {
        int nx = x + cardinal_dir[n].x;
        int ny = y + cardinal_dir[n].y;
        if (!is_in_map(nx, ny))
            continue;

        Tile *neighbour = &map[nx][ny];
        if (!neighbour)
            continue;

        if (neighbour->type != ROOM_NORMAL)
            continue;

        super_secret_room->doors++;
        neighbour->secret_doors++;

        switch (n)
        {
        case CARDINAL_TOP:
            neighbour->secret_bottom = true;
            super_secret_room->top = true;
            break;

        case CARDINAL_RIGHT:
            neighbour->secret_left = true;
            super_secret_room->right = true;
            break;

        case CARDINAL_BOTTOM:
            neighbour->secret_top = true;
            super_secret_room->bottom = true;
            break;

        case CARDINAL_LEFT:
            neighbour->secret_right = true;
            super_secret_room->left = true;
            break;
        }
    }
}

void map_generate()
{
    if ((MAP_WIDTH % 2) == false || (MAP_HEIGHT % 2) == false)
        return;

    max_rooms = RANDOM_RANGE(0, 2) + 5 + minv(level_id, max_level_id) * 2.6;

    if (level_id == 0)
    {
        max_secrets = 1;
        max_item_rooms = 1;
    }
    else
    {
        max_secrets = RANDOM_RANGE(0, 2) + 1;
        max_item_rooms = RANDOM_RANGE(0, 2) + 1;
    }

    created_item_rooms = 0;
    created_secret_rooms = 0;

    boss_room_found = false;
    shop_room_found = false;
    super_secret_created = false;

    map_reset();

    array_destroy(rooms_queue_list);
    array_destroy(end_rooms_list);
    array_destroy(secret_positions_list);
    array_destroy(secret_rooms_list);
    array_destroy(super_positions_list);

    rooms_queue_list = array_create(Tile *);
    end_rooms_list = array_create(Tile *);
    secret_positions_list = array_create(Tile *);
    secret_rooms_list = array_create(Tile *);
    super_positions_list = array_create(Tile *);

    level_load("");

    map_add_rooms();
    wait_for(map_add_rooms);
    if (array_size(rooms_queue_list) != max_rooms)
    {
        map_generate();
        return;
    }

    map_find_end_rooms();
    wait_for(map_find_end_rooms);
    if (array_size(end_rooms_list) < 2 + max_item_rooms)
    {
        map_generate();
        return;
    }

    map_find_boss_room();
    wait_for(map_find_boss_room);
    if (!boss_room_found)
    {
        map_generate();
        return;
    }

    map_find_shop_room();
    wait_for(map_find_shop_room);
    if (!shop_room_found)
    {
        map_generate();
        return;
    }

    map_find_item_rooms();
    wait_for(map_find_item_rooms);

    map_find_secret_positions();
    wait_for(map_find_secret_positions);
    if (array_size(secret_positions_list) < max_secrets)
    {
        map_generate();
        return;
    }

    map_add_secret_rooms();
    wait_for(map_add_secret_rooms);
    if (created_secret_rooms != max_secrets)
    {
        map_generate();
        return;
    }

    map_add_super_secret_positions();
    wait_for(map_add_super_secret_positions);
    if (array_size(super_positions_list) <= 0)
    {
        map_generate();
        return;
    }

    map_add_super_secret_room();
    wait_for(map_add_super_secret_room);

    beep();
}

void map_draw(int pos_x, int pos_y)
{
    int x = 0, y = 0;
    for (y = 0; y < MAP_HEIGHT; y++)
    {
        for (x = 0; x < MAP_WIDTH; x++)
        {
            VECTOR pos;
            vec_set(&pos, vector(pos_x + (x * MAP_CELL_SIZE), pos_y + (y * MAP_CELL_SIZE), 0));

            VECTOR size;
            vec_set(&size, vector(MAP_CELL_SIZE * DEBUG_FONT_SCALE, MAP_CELL_SIZE * DEBUG_FONT_SCALE, 0));

            int type = map[x][y].type;

            int top_room = map[x][y].top;
            int right_room = map[x][y].right;
            int bottom_room = map[x][y].bottom;
            int left_room = map[x][y].left;

            VECTOR color;
            vec_set(&color, COLOR_WHITE);

            var bmap_scale = 2;
            var bmap_alpha = 100;

            if (type == ROOM_SECRET || type == ROOM_SUPER_SECRET)
                vec_set(&color, COLOR_GREY);

            if (top_room && !right_room && !bottom_room && !left_room)
                draw_quad(room[ROOM_TOP], &pos, NULL, NULL, vector(bmap_scale, bmap_scale, 0), &color, bmap_alpha, 0);
            else if (!top_room && right_room && !bottom_room && !left_room)
                draw_quad(room[ROOM_RIGHT], &pos, NULL, NULL, vector(bmap_scale, bmap_scale, 0), &color, bmap_alpha, 0);
            else if (!top_room && !right_room && bottom_room && !left_room)
                draw_quad(room[ROOM_BOTTOM], &pos, NULL, NULL, vector(bmap_scale, bmap_scale, 0), &color, bmap_alpha, 0);
            else if (!top_room && !right_room && !bottom_room && left_room)
                draw_quad(room[ROOM_LEFT], &pos, NULL, NULL, vector(bmap_scale, bmap_scale, 0), &color, bmap_alpha, 0);
            else if (top_room && right_room && !bottom_room && !left_room)
                draw_quad(room[ROOM_TOP_N_RIGHT], &pos, NULL, NULL, vector(bmap_scale, bmap_scale, 0), &color, bmap_alpha, 0);
            else if (!top_room && right_room && bottom_room && !left_room)
                draw_quad(room[ROOM_RIGHT_N_BOTTOM], &pos, NULL, NULL, vector(bmap_scale, bmap_scale, 0), &color, bmap_alpha, 0);
            else if (!top_room && !right_room && bottom_room && left_room)
                draw_quad(room[ROOM_BOTTOM_N_LEFT], &pos, NULL, NULL, vector(bmap_scale, bmap_scale, 0), &color, bmap_alpha, 0);
            else if (top_room && !right_room && !bottom_room && left_room)
                draw_quad(room[ROOM_LEFT_N_TOP], &pos, NULL, NULL, vector(bmap_scale, bmap_scale, 0), &color, bmap_alpha, 0);
            else if (top_room && right_room && !bottom_room && left_room)
                draw_quad(room[ROOM_LEFT_TOP_N_RIGHT], &pos, NULL, NULL, vector(bmap_scale, bmap_scale, 0), &color, bmap_alpha, 0);
            else if (top_room && right_room && bottom_room && !left_room)
                draw_quad(room[ROOM_TOP_RIGHT_N_BOTTOM], &pos, NULL, NULL, vector(bmap_scale, bmap_scale, 0), &color, bmap_alpha, 0);
            else if (!top_room && right_room && bottom_room && left_room)
                draw_quad(room[ROOM_RIGHT_BOTTOM_N_LEFT], &pos, NULL, NULL, vector(bmap_scale, bmap_scale, 0), &color, bmap_alpha, 0);
            else if (top_room && !right_room && bottom_room && left_room)
                draw_quad(room[ROOM_BOTTOM_LEFT_N_TOP], &pos, NULL, NULL, vector(bmap_scale, bmap_scale, 0), &color, bmap_alpha, 0);
            else if (top_room && !right_room && bottom_room && !left_room)
                draw_quad(room[ROOM_TOP_N_BOTTOM], &pos, NULL, NULL, vector(bmap_scale, bmap_scale, 0), &color, bmap_alpha, 0);
            else if (!top_room && right_room && !bottom_room && left_room)
                draw_quad(room[ROOM_RIGHT_N_LEFT], &pos, NULL, NULL, vector(bmap_scale, bmap_scale, 0), &color, bmap_alpha, 0);
            else if (top_room && right_room && bottom_room && left_room)
                draw_quad(room[ROOM_ALL_SIDES], &pos, NULL, NULL, vector(bmap_scale, bmap_scale, 0), &color, bmap_alpha, 0);
            else if (!top_room && !right_room && !bottom_room && !left_room)
                draw_quad(room[ROOM_VOID], &pos, NULL, NULL, vector(bmap_scale, bmap_scale, 0), &color, bmap_alpha, 0);

            if (key_enter)
            {
                var temp_x = pos.x + (size.x / 2);
                var temp_y = pos.y + (size.y / 2);

                switch (type)
                {
                case ROOM_NONE:
                    draw_quad(NULL, vector(temp_x, temp_y, 0), NULL, &size, NULL, vector(8, 8, 8), 100, 0);
                    break;

                case ROOM_NORMAL:
                    draw_quad(NULL, vector(temp_x, temp_y, 0), NULL, &size, NULL, vector(128, 128, 128), 100, 0);
                    break;

                case ROOM_START:
                    draw_quad(NULL, vector(temp_x, temp_y, 0), NULL, &size, NULL, vector(0, 128, 0), 100, 0);
                    break;

                case ROOM_BOSS:
                    draw_quad(NULL, vector(temp_x, temp_y, 0), NULL, &size, NULL, vector(0, 0, 128), 100, 0);
                    break;

                case ROOM_SPECIAL:
                    draw_quad(NULL, vector(temp_x, temp_y, 0), NULL, &size, NULL, vector(128, 128, 0), 100, 0);
                    break;

                case ROOM_LOCKED:
                    draw_quad(NULL, vector(temp_x, temp_y, 0), NULL, &size, NULL, vector(128, 0, 0), 100, 0);
                    break;

                case ROOM_SECRET:
                    draw_quad(NULL, vector(temp_x, temp_y, 0), NULL, &size, NULL, vector(64, 64, 64), 100, 0);
                    break;

                case ROOM_SUPER_SECRET:
                    draw_quad(NULL, vector(temp_x, temp_y, 0), NULL, &size, NULL, vector(32, 64, 128), 100, 0);
                    break;
                }

                if (map[x][y].type == ROOM_NONE)
                    draw_text(str_for_num(NULL, map[x][y].secret_chance), temp_x, temp_y, COLOR_WHITE);
                else
                    draw_text(str_for_num(NULL, map[x][y].region), temp_x, temp_y, COLOR_WHITE);
            }
        }
    }

    if (key_enter)
    {
        VECTOR size;
        vec_set(&size, vector(MAP_CELL_SIZE * DEBUG_FONT_SCALE, MAP_CELL_SIZE * DEBUG_FONT_SCALE, 0));

        var icon_pos_x = 10;
        var icon_pos_y = 128;
        var text_offset_x = 22;

        draw_quad(NULL, vector(icon_pos_x - 4, icon_pos_y - 4, 0), NULL, vector(120, 164, 0), NULL, vector(0, 0, 0), 50, 0);
        draw_text("Rooms:", icon_pos_x, icon_pos_y - MAP_CELL_SIZE * 1.5 * DEBUG_FONT_SCALE, COLOR_RED);

        draw_quad(NULL, vector(icon_pos_x, icon_pos_y, 0), NULL, &size, NULL, vector(8, 8, 8), 100, 0);
        draw_text("none", icon_pos_x + text_offset_x, icon_pos_y, COLOR_RED);

        icon_pos_y += 20;
        draw_quad(NULL, vector(icon_pos_x, icon_pos_y, 0), NULL, &size, NULL, vector(128, 128, 128), 100, 0);
        draw_text("normal", icon_pos_x + text_offset_x, icon_pos_y, COLOR_RED);

        icon_pos_y += 20;
        draw_quad(NULL, vector(icon_pos_x, icon_pos_y, 0), NULL, &size, NULL, vector(0, 128, 0), 100, 0);
        draw_text("start", icon_pos_x + text_offset_x, icon_pos_y, COLOR_RED);

        icon_pos_y += 20;
        draw_quad(NULL, vector(icon_pos_x, icon_pos_y, 0), NULL, &size, NULL, vector(0, 0, 128), 100, 0);
        draw_text("finish (boss)", icon_pos_x + text_offset_x, icon_pos_y, COLOR_RED);

        icon_pos_y += 20;
        draw_quad(NULL, vector(icon_pos_x, icon_pos_y, 0), NULL, &size, NULL, vector(128, 128, 0), 100, 0);
        draw_text("special", icon_pos_x + text_offset_x, icon_pos_y, COLOR_RED);

        icon_pos_y += 20;
        draw_quad(NULL, vector(icon_pos_x, icon_pos_y, 0), NULL, &size, NULL, vector(128, 0, 0), 100, 0);
        draw_text("locked", icon_pos_x + text_offset_x, icon_pos_y, COLOR_RED);

        icon_pos_y += 20;
        draw_quad(NULL, vector(icon_pos_x, icon_pos_y, 0), NULL, &size, NULL, vector(64, 64, 64), 100, 0);
        draw_text("secret", icon_pos_x + text_offset_x, icon_pos_y, COLOR_RED);

        icon_pos_y += 20;
        draw_quad(NULL, vector(icon_pos_x, icon_pos_y, 0), NULL, &size, NULL, vector(32, 64, 128), 100, 0);
        draw_text("super secret", icon_pos_x + text_offset_x, icon_pos_y, COLOR_RED);
    }
}

void room_bmaps_create()
{
    int width = 16;
    int height = 16;

    int i = 0, x = 0, y = 0;
    for (i = 0; i < MAX_ROOM_SPRITES; i++)
    {
        room[i] = bmap_createblack(MAP_CELL_SIZE, MAP_CELL_SIZE, 8888);
        bmap_blitpart(room[i], rooms_pcx, NULL, NULL, vector(x * height, y * width, 0), vector(height, width, 0));

        x++;
        if (x >= 4)
        {
            x = 0;
            y++;
        }
    }
}

void room_bmaps_destroy()
{
    int i = 0;
    for (i = 0; i < MAX_ROOM_SPRITES; i++)
        safe_remove(room[i]);
}

void on_exit_event()
{
    array_destroy(rooms_queue_list);
    array_destroy(end_rooms_list);
    array_destroy(secret_positions_list);
    array_destroy(secret_rooms_list);
    array_destroy(super_positions_list);

    room_bmaps_destroy();
}

void main()
{
    on_exit = on_exit_event;
    on_space = map_generate;

    random_seed(0);

    fps_max = 60;
    warn_level = 6;
    video_mode = 9;

    wait(1);

    draw_textmode("Arial", 0, MAP_CELL_SIZE * DEBUG_FONT_SCALE, 100);

    room_bmaps_create();

    vec2d_set(&cardinal_dir[CARDINAL_TOP], 0, -1);
    vec2d_set(&cardinal_dir[CARDINAL_RIGHT], 1, 0);
    vec2d_set(&cardinal_dir[CARDINAL_BOTTOM], 0, 1);
    vec2d_set(&cardinal_dir[CARDINAL_LEFT], -1, 0);

    map_generate();

    while (!key_esc)
    {
        draw_text(str_printf(NULL,
                             "level=%d;\nrooms=%d/%d;\nsecrets=%d/%d;\nlocked=%d/%d;\ntotal endrooms=%d;",
                             (long)level_id, (long)array_size(rooms_queue_list), (long)max_rooms, (long)created_secret_rooms,
                             (long)max_secrets, (long)created_item_rooms, (long)max_item_rooms, (long)array_size(end_rooms_list)),
                  10, 10, COLOR_RED);

        map_draw(384, 128);
        wait(1);
    }
}