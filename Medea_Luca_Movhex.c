#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define SUP (100 * rows * cols + 1)
#define BUCKETS 101
#define HASH_SETS 128
#define HASH_WAYS 2
#define MAX_AIR_ROUTES 5

// STRUCT

typedef struct
{
    int cost_air_route;
    int x_dest;
    int y_dest;
} air_route;

typedef struct
{
    uint8_t cost;
    int num_air_routes;
    air_route *air_routes;

} block;

typedef struct
{
    int x;
    int y;
    int cost;
} node;

typedef struct
{
    int xp, yp, xd, yd;
    int val;
    uint8_t used;
} hash_line;

// allocazione variabili globali

block **map = NULL;
node *queue = NULL;
int *next = NULL;
int **visited = NULL;
int **distanza = NULL;
int rows = 0, cols = 0;
int testa[BUCKETS], coda[BUCKETS];

hash_line hash_table[HASH_SETS][HASH_WAYS];
int rr_replace[HASH_SETS]; // politica replacement di tipo round-robin

int dx_even[6] = {+1, 0, -1, -1, -1, 0};
int dy_even[6] = {0, +1, +1, 0, -1, -1};

int dx_odd[6] = {+1, +1, 0, -1, 0, +1};
int dy_odd[6] = {0, +1, +1, 0, -1, -1};

// FUNZIONI AUSILIARIE

void push(int dist, int x, int y, int *pool_top, int pool_cap)
{
    int b = dist % BUCKETS;
    int idx;
    if (*pool_top < pool_cap)
    {
        idx = (*pool_top)++;
        queue[idx].x = x;
        queue[idx].y = y;
        queue[idx].cost = dist;
        next[idx] = -1;
        if (testa[b] == -1)
        {
            testa[b] = coda[b] = idx;
        }
        else
        {
            next[coda[b]] = idx;
            coda[b] = idx;
        }
    }
}

int valid_pos(int x, int y) // funzione per controllare la validità della posizione
{
    if (x < 0 || y < 0 || x >= cols || y >= rows)
    {
        return 0;
    }
    return 1;
}

int new_cost(int val, int min, int max) // funzione per arrotondare il costo nell'intervallo [0,100]
{
    if (val > max)
    {
        return max;
    }
    else if (val < min)
    {
        return min;
    }
    return val;
}

// funzioni ausiliarie per le rotte aeree

int find_air_route(int x1, int y1, int x2, int y2)
{
    for (int i = 0; i < map[y1][x1].num_air_routes; i++)
    {
        if (map[y1][x1].air_routes[i].x_dest == x2 && map[y1][x1].air_routes[i].y_dest == y2)
        {
            return i;
        }
    }
    return -1;
}

int add_air_route(int x1, int y1, int x2, int y2, int cost)
{
    if (map[y1][x1].num_air_routes >= MAX_AIR_ROUTES)
    {
        return 0;
    }
    int n = map[y1][x1].num_air_routes + 1;
    air_route *new_routes = realloc(map[y1][x1].air_routes, n * sizeof(air_route));
    if (new_routes != NULL)
    {
        map[y1][x1].air_routes = new_routes;
        map[y1][x1].air_routes[map[y1][x1].num_air_routes] = (air_route){cost, x2, y2};
        map[y1][x1].num_air_routes = n;
        return 1;
    }
    else
    {
        return 0;
    }
}

void remove_air_route(int x1, int y1, int idx)
{
    int last = map[y1][x1].num_air_routes - 1;
    if (idx != last)
    {
        map[y1][x1].air_routes[idx] = map[y1][x1].air_routes[last];
    }
    map[y1][x1].num_air_routes = last;
    if (last == 0)
    {
        free(map[y1][x1].air_routes);
        map[y1][x1].air_routes = NULL;
    }
    else
    {
        air_route *new_routes = realloc(map[y1][x1].air_routes, last * sizeof(air_route));
        if (new_routes != NULL)
        {
            map[y1][x1].air_routes = new_routes;
        }
    }
}

void hash_clear() // pulisce la tabella hash
{
    for (int b = 0; b < HASH_SETS; b++)
    {
        hash_table[b][0].used = 0;
        hash_table[b][1].used = 0;
        rr_replace[b] = 0;
    }
}

uint32_t hash_idx(int xp, int yp, int xd, int yd) // calcola l'indice di hash per la nuova entry della tabella rimescolando i bit delle coordinate
{
    uint32_t h = (uint32_t)xp;
    h ^= (uint32_t)(yp << 7);
    h ^= (uint32_t)(xd << 13);
    h ^= (uint32_t)(yd << 17);
    h = h ^ (h >> 16);
    return h & (HASH_SETS - 1);
}

int hash_get(int xp, int yp, int xd, int yd, int *out) // estrae una linea nella tabella hash
{
    uint32_t b = hash_idx(xp, yp, xd, yd);
    hash_line *L0, *L1;
    L0 = &hash_table[b][0];
    if (L0->used && L0->xp == xp && L0->yp == yp && L0->xd == xd && L0->yd == yd)
    {
        *out = L0->val;
        return 1;
    }
    L1 = &hash_table[b][1];
    if (L1->used && L1->xp == xp && L1->yp == yp && L1->xd == xd && L1->yd == yd)
    {
        *out = L1->val;
        return 1;
    }
    return 0;
}

void hash_put(int xp, int yp, int xd, int yd, int val) // inserisce una nuova linea nella tabella hash
{
    uint32_t b = hash_idx(xp, yp, xd, yd);
    int w;
    hash_line *L0, *L1, *L;
    L0 = &hash_table[b][0];

    if (L0->used && L0->xp == xp && L0->yp == yp && L0->xd == xd && L0->yd == yd)
    {
        L0->val = val;
        return;
    }
    L1 = &hash_table[b][1];

    if (L1->used && L1->xp == xp && L1->yp == yp && L1->xd == xd && L1->yd == yd)
    {
        L1->val = val;
        return;
    }

    if (!L0->used)
    {
        L0->used = 1;
        L0->xp = xp;
        L0->yp = yp;
        L0->xd = xd;
        L0->yd = yd;
        L0->val = val;
    }
    else if (!L1->used)
    {
        L1->used = 1;
        L1->xp = xp;
        L1->yp = yp;
        L1->xd = xd;
        L1->yd = yd;
        L1->val = val;
    }
    else
    {
        w = (rr_replace[b] ^= 1);
        L = &hash_table[b][w];
        L->used = 1;
        L->xp = xp;
        L->yp = yp;
        L->xd = xd;
        L->yd = yd;
        L->val = val;
    }
}

// FUNZIONI PRIMARIE

void init(int x, int y)
{
    if (x > 0 && y > 0)
    {
        if (map != NULL)
        {
            for (int i = 0; i < rows; i++)
            {
                free(map[i]);
                free(distanza[i]);
                free(visited[i]);
            }
            free(map);
            free(distanza);
            free(visited);
            free(queue);
            free(next);
            map = NULL;
            distanza = NULL;
            visited = NULL;
            queue = NULL;
            next = NULL;
        }

        cols = x;
        rows = y;

        queue = malloc((rows * cols) * sizeof(node));
        next = malloc((rows * cols) * sizeof(int));

        if (queue != NULL && next != NULL)
        {
            map = malloc(rows * sizeof(block *)); // allocazione righe
            distanza = malloc(rows * sizeof(int *));
            visited = malloc(rows * sizeof(int *));

            if (distanza != NULL && visited != NULL && map != NULL)
            {
                for (int i = 0; i < rows; i++)
                {
                    map[i] = malloc(cols * sizeof(block)); // allocazione colonne
                    distanza[i] = malloc(cols * sizeof(int));
                    visited[i] = malloc(cols * sizeof(int));
                    if (map[i] == NULL || distanza[i] == NULL || visited[i] == NULL)
                    {
                        for (int j = 0; j < i; j++)
                        {
                            free(map[j]);
                            free(distanza[j]);
                            free(visited[j]);
                        }
                        free(map);
                        free(distanza);
                        free(visited);
                        free(queue);
                        free(next);
                        map = NULL;
                        rows = 0;
                        cols = 0;
                        printf("KO\n");
                        break;
                    }
                }
                if (map != NULL)
                {
                    for (int i = 0; i < rows; i++)
                    {
                        for (int j = 0; j < cols; j++)
                        {
                            map[i][j].cost = 1;
                            map[i][j].num_air_routes = 0;
                            map[i][j].air_routes = NULL;
                        }
                    }
                    hash_clear();
                    for (int k = 0; k < rows * cols; k++)
                    {
                        next[k] = -1;
                    }
                    printf("OK\n");
                }
            }
            else
            {
                free(map);
                free(distanza);
                free(visited);
                free(queue);
                free(next);
                rows = 0;
                cols = 0;
                printf("KO\n");
            }
        }
        else
        {
            free(queue);
            free(next);
            printf("KO\n");
        }
    }
    else
    {
        rows = 0;
        cols = 0;
        printf("KO\n");
    }
}

void toggle_air_route(int x1, int y1, int x2, int y2)
{
    if (valid_pos(x1, y1) && valid_pos(x2, y2))
    {
        int idx = find_air_route(x1, y1, x2, y2);
        if (idx != -1)
        {
            remove_air_route(x1, y1, idx);
            hash_clear();
            printf("OK\n");
        }
        else
        {
            if (add_air_route(x1, y1, x2, y2, map[y1][x1].cost))
            {
                hash_clear();
                printf("OK\n");
            }
            else
            {
                printf("KO\n");
            }
        }
    }
    else
    {
        printf("KO\n");
    }
}

void change_cost(int x, int y, int v, int raggio) // con visita in ampiezza(BFS)
{
    int delta, testa, coda, cx, cy, nx, ny;
    node curr;

    if (valid_pos(x, y) && raggio > 0 && (v >= -10 && v <= 10))
    {

        for (int i = 0; i < rows; i++)
        {
            for (int j = 0; j < cols; j++)
            {
                visited[i][j] = 0;
                distanza[i][j] = SUP;
            }
        }
        visited[y][x] = 1;
        distanza[y][x] = 0;
        testa = 0;
        coda = 0;
        queue[coda++] = (node){x, y, 0}; // enqueue sorgente
        while (testa < coda)             // not isEmpty(Queue)
        {
            curr = queue[testa++];
            cx = curr.x;
            cy = curr.y;

            if (distanza[cy][cx] >= raggio) // raggio-1
            {
                visited[cy][cx] = 2; // nero
                continue;
            }

            delta = v * (raggio - distanza[cy][cx]);
            if (delta >= 0)
            {
                delta /= raggio;
            }
            else
            {
                delta = -((-delta + raggio - 1) / raggio);
            }

            map[cy][cx].cost = new_cost(map[cy][cx].cost + delta, 0, 100); // modifico costo terrestre

            for (int i = 0; i < map[cy][cx].num_air_routes; i++) // modifico costo rotte aeree
            {
                map[cy][cx].air_routes[i].cost_air_route = new_cost(map[cy][cx].air_routes[i].cost_air_route + delta, 0, 100);
            }

            for (int neighbour = 0; neighbour < 6; neighbour++) // visito i vicini
            {
                if (cy % 2 == 0)
                {
                    nx = cx + dx_even[neighbour];
                    ny = cy + dy_even[neighbour];
                }
                else
                {
                    nx = cx + dx_odd[neighbour];
                    ny = cy + dy_odd[neighbour];
                }
                if (!valid_pos(nx, ny))
                {
                    continue;
                }
                if (visited[ny][nx] == 0)
                {
                    visited[ny][nx] = 1;
                    distanza[ny][nx] = distanza[cy][cx] + 1;
                    queue[coda++] = (node){nx, ny, distanza[ny][nx]};
                }
            }
            visited[cy][cx] = 2; // nero
        }
        hash_clear();
        printf("OK\n");
    }
    else
    {
        printf("KO\n");
    }
}

int travel_cost(int xp, int yp, int xd, int yd)
{
    int nx, ny, len, scan, pool_cap = rows * cols, cur_b = 0, pool_top = 0, idx, hashed;
    int *dx, *dy;
    node aux;

    if (!valid_pos(xp, yp) || !valid_pos(xd, yd))
    {
        return -1;
    }
    else if (xp == xd && yp == yd)
    {
        return 0;
    }

    if (hash_get(xp, yp, xd, yd, &hashed)) // check hash table
    {
        return hashed;
    }

    for (int i = 0; i < rows; i++) // inizializzo distanza e visited
    {
        for (int j = 0; j < cols; j++)
        {
            distanza[i][j] = SUP;
            visited[i][j] = 0;
        }
    }

    distanza[yp][xp] = 0;

    for (int i = 0; i < BUCKETS; ++i)
    {
        testa[i] = -1;
        coda[i] = -1;
    }

    // push della sorgente nel bucket 0
    if (pool_top < pool_cap)
    {
        queue[pool_top] = (node){xp, yp, 0};
        next[pool_top] = -1;
        testa[0] = coda[0] = pool_top++;
    }

    while (1)
    {
        scan = 0;
        while (testa[cur_b] == -1 && scan < BUCKETS)
        {
            cur_b++;
            if (cur_b == BUCKETS)
            {
                cur_b = 0; // rotazione
            }
            scan++;
        }

        if (scan == BUCKETS && testa[cur_b] == -1)
        {
            break; // tutti vuoti
        }

        idx = testa[cur_b];
        aux = queue[idx];
        testa[cur_b] = next[idx];

        if (testa[cur_b] == -1)
        {
            coda[cur_b] = -1;
        }

        if (!visited[aux.y][aux.x])
        {
            if (aux.x == xd && aux.y == yd)
            {
                break;
            }
            visited[aux.y][aux.x] = 1;

            // espansione solo se cost>0
            if (map[aux.y][aux.x].cost > 0)
            {
                if (aux.y % 2 == 0)
                {
                    dx = dx_even;
                    dy = dy_even;
                }
                else
                {
                    dx = dx_odd;
                    dy = dy_odd;
                }

                for (int k = 0; k < 6; k++)
                {
                    nx = aux.x + dx[k];
                    ny = aux.y + dy[k];
                    if (valid_pos(nx, ny) && !visited[ny][nx])
                    {
                        if ((map[ny][nx].cost == 0) && !(nx == xd && ny == yd))
                        {
                            continue;
                        }

                        len = distanza[aux.y][aux.x] + map[aux.y][aux.x].cost;

                        if (len < distanza[ny][nx])
                        {
                            distanza[ny][nx] = len;
                            push(len, nx, ny, &pool_top, pool_cap);
                        }
                    }
                }

                for (int i = 0; i < map[aux.y][aux.x].num_air_routes; i++)
                {
                    if (map[aux.y][aux.x].air_routes[i].cost_air_route > 0)
                    {
                        nx = map[aux.y][aux.x].air_routes[i].x_dest;
                        ny = map[aux.y][aux.x].air_routes[i].y_dest;

                        if (!visited[ny][nx])
                        {
                            len = distanza[aux.y][aux.x] + map[aux.y][aux.x].air_routes[i].cost_air_route;

                            if (len < distanza[ny][nx])
                            {
                                distanza[ny][nx] = len;
                                push(len, nx, ny, &pool_top, pool_cap);
                            }
                        }
                    }
                }
            }
        }
    }
    if (distanza[yd][xd] == SUP)
    {
        return -1;
    }
    else
    {
        hash_put(xp, yp, xd, yd, distanza[yd][xd]);
        return distanza[yd][xd];
    }
}

int main()
{
    int xp, yp, v, raggio, xd, yd, ncols, nrows;
    char cmd[17];
    while (scanf("%s", cmd) != EOF)
    {
        if (!strcmp(cmd, "init"))
        {
            if (scanf("%d %d", &ncols, &nrows) == 2)
            {
                init(ncols, nrows);
            }
            else
            {
                printf("KO\n");
            }
        }
        else if (!strcmp(cmd, "change_cost"))
        {
            if (scanf("%d %d %d %d", &xp, &yp, &v, &raggio) == 4)
            {
                change_cost(xp, yp, v, raggio);
            }
            else
            {
                printf("KO\n");
            }
        }
        else if (!strcmp(cmd, "toggle_air_route"))
        {
            if (scanf("%d %d %d %d", &xp, &yp, &xd, &yd) == 4)
            {
                toggle_air_route(xp, yp, xd, yd);
            }
            else
            {
                printf("KO\n");
            }
        }
        else if (!strcmp(cmd, "travel_cost"))
        {
            if (scanf("%d %d %d %d", &xp, &yp, &xd, &yd) == 4)
            {
                printf("%d\n", travel_cost(xp, yp, xd, yd));
            }
            else
            {
                printf("KO\n");
            }
        }
    }
    if (map != NULL)
    {
        for (int i = 0; i < rows; i++)
        {
            free(map[i]);
            free(distanza[i]);
            free(visited[i]);
        }
        free(queue);
        free(map);
        free(distanza);
        free(visited);
        free(next);
    }
    return 0;
}
