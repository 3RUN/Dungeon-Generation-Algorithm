
typedef struct Vector2d
{
    int x;
    int y;
} Vector2d;

Vector2d *vector(int x, int y)
{
    Vector2d *vec2d = sys_malloc(sizeof(Vector2d));
    if (!vec2d)
        return NULL;

    vec2d->x = x;
    vec2d->y = y;
    return vec2d;
}

void vec2d_set(Vector2d *vec_a, Vector2d *vec_b)
{
    if (!vec_a || !vec_b)
        return;

    vec_a->x = vec_b->x;
    vec_a->y = vec_b->y;
}

void vec2d_set(Vector2d *vec2d, int x, int y)
{
    if (!vec2d)
        return;

    vec2d->x = x;
    vec2d->y = y;
}