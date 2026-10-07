#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <time.h>
#include <string.h>

#define X 10
#define MAX_EDGES 30
#define MAX_STOPS 6



#define BUS_CAPACITY 40

#define COUNT_OF_BUS ((int)(sizeof(busRoute) / sizeof(busRoute[0])))


typedef enum { BUS, TRAIN } Mode;

const char *location[X] = {
    "Pettah", "Maradana", "Nugegoda", "Maharagama", "Kottawa",
    "Dehiwala", "Wellawatte", "Moratuwa", "Homagama", "Avissawella"
};

typedef struct {
    char id[10];
    char name[30];
    Mode mode;
    int stopCount;
    int stops[MAX_STOPS];
    double distance[MAX_STOPS - 1];
} Route1;

typedef struct {
    int to;
    double distance;
    Mode mode;
    int routeIndex;
} Edge1;

typedef struct {
    Edge1 edges[X][MAX_EDGES];
    int count[X];
} Graph1;

/*Routes*/

Route1 busRoute[] = {
    {"B01", "Bus 01", BUS, 4, {0, 1, 2, 3}, {2.0, 4.0, 5.0}},
    {"B02", "Bus 02", BUS, 4, {2, 4, 8, 9}, {5.0, 6.0, 12.0}},
    {"B03", "Bus 03", BUS, 4, {5, 6, 7, 4}, {8.0, 3.0, 20.0}},
    {"B04", "Bus 04", BUS, 3, {3, 8, 9}, {7.0, 20.0}},
    {"B05", "Bus 05", BUS, 4, {0, 5, 6, 7}, {9.0, 10.0, 8.0}}
};

/*Graph*/

void addEdge(Graph1 *g, int from, int to, double distance,
             Mode mode, int routeIndex)
{
    if (g->count[from] < MAX_EDGES) {
        g->edges[from][g->count[from]].to = to;
        g->edges[from][g->count[from]].distance = distance;
        g->edges[from][g->count[from]].mode = mode;
        g->edges[from][g->count[from]].routeIndex = routeIndex;
        g->count[from]++;
    }

    if (g->count[to] < MAX_EDGES) {
        g->edges[to][g->count[to]].to = from;
        g->edges[to][g->count[to]].distance = distance;
        g->edges[to][g->count[to]].mode = mode;
        g->edges[to][g->count[to]].routeIndex = routeIndex;
        g->count[to]++;
    }
}

void buildNetwork(Graph1 *g)
{
    int i, j;

    for (i = 0; i < X; i++)
        g->count[i] = 0;

    for (i = 0; i < COUNT_OF_BUS; i++) {
        for (j = 0; j < busRoute[i].stopCount - 1; j++) {
            addEdge(g, busRoute[i].stops[j], busRoute[i].stops[j + 1],
                    busRoute[i].distance[j], BUS, i);
        }
    }
}

const char *routeName(Mode mode, int routeIndex)
{
    return mode == BUS ? busRoute[routeIndex].name
                       : trainRoute[routeIndex].name;
}

const char *routeId(Mode mode, int routeIndex)
{
    return mode == BUS ? busRoute[routeIndex].id
                       : trainRoute[routeIndex].id;
}

double edgeTime(Edge1 e)
{
    double speed = (e.mode == TRAIN) ? 60.0 : 25.0;
    return (e.distance / speed) * 60.0;
}

void printRoutes(Mode mode)
{
    Route1 *routes;
    int count;
    int i, j;

    if (mode == BUS) {
        routes = busRoute;
        count = COUNT_OF_BUS;
    } else {
        routes = trainRoute;
        count = COUNT_OF_TRAIN;
    }

    for (i = 0; i < count; i++) {
        printf("  %-4s %-10s : ", routes[i].id, routes[i].name);
        for (j = 0; j < routes[i].stopCount; j++) {
            if (j > 0)
                printf(" -> ");
            printf("%s", location[routes[i].stops[j]]);
        }
        printf("\n");
    }
}

/*Bus Connectivity*/
int bfsConnected(Graph1 *g, int start, int end)
{
    int visited[X] = {0};
    int queue[X];
    int front = 0, rear = 0;
    int i;

    if (start == end)
        return 1;

    visited[start] = 1;
    queue[rear++] = start;

    while (front < rear) {
        int u = queue[front++];

        for (i = 0; i < g->count[u]; i++) {
            int v = g->edges[u][i].to;

            if (!visited[v]) {
                if (v == end)
                    return 1;
                visited[v] = 1;
                queue[rear++] = v;
            }
        }
    }

    return 0;
}

/*Main Function*/

int main(void)
{
    Graph1 city;

    buildNetwork(&city);

    printf("______________________________________________\n\n");
    printf("       SMART CITY PUBLIC TRANSPORT SYSTEM\n");
    printf("______________________________________________\n\n");

    printf("______________________________________________\n\n");
    printf("  BUS ROUTES AND BUS NETWORK \n");
    printf("______________________________________________\n\n");
    printRoutes(BUS);

    printf("______________________________________________\n\n");
    printf("  GRAPH CONNECTIVITY TEST (BFS) \n\n");

    printf("  Pettah -> Avissawella : %s\n",
           bfsConnected(&city, 0, 9) ? "CONNECTED" : "NOT CONNECTED");
    printf("  Pettah -> Homagama   : %s\n",
           bfsConnected(&city, 0, 8) ? "CONNECTED" : "NOT CONNECTED");
}