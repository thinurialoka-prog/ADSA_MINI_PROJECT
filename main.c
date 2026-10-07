#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <time.h>
#include <string.h>

#define X 10
#define MAX_EDGES 30
#define MAX_STOPS 6



#define BUS_CAPACITY 40
#define TRAIN_CAPACITY 120
#define COUNT_OF_BUS ((int)(sizeof(busRoute) / sizeof(busRoute[0])))
#define COUNT_OF_TRAIN ((int)(sizeof(trainRoute) / sizeof(trainRoute[0])))

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

Route1 trainRoute[] = {
    {"T01", "Line 1", TRAIN, 3, {0, 1, 2}, {2.0, 8.0}},
    {"T02", "Line 2", TRAIN, 3, {2, 4, 3}, {7.0, 5.0}},
    {"T03", "Line 3", TRAIN, 3, {4, 8, 9}, {6.0, 15.0}}
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


int serviceInterval(Mode mode, int hour)
{
    if (hour == 7 || hour == 8 || hour == 17 || hour == 18)
        return mode == TRAIN ? 5 : 10;

    if (hour >= 9 && hour <= 16)
        return mode == TRAIN ? 10 : 15;

    return mode == TRAIN ? 15 : 20;
}

double expectedWaitingTime(Mode mode, int hour)
{
    return serviceInterval(mode, hour) / 2.0;
}

int routeCapacity(Mode mode)
{
    return mode == TRAIN ? TRAIN_CAPACITY : BUS_CAPACITY;
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


/*Dijkstra*/

int dijkstra(Graph1 *g, int start, int end,
             int parent[], int parentEdge[])
{
    double dist[X];
    int visited[X] = {0};
    int i, j;

    for (i = 0; i < X; i++) {
        dist[i] = DBL_MAX;
        parent[i] = -1;
        parentEdge[i] = -1;
    }

    dist[start] = 0.0;

    for (i = 0; i < X; i++) {
        int u = -1;
        double best = DBL_MAX;

        for (j = 0; j < X; j++) {
            if (!visited[j] && dist[j] < best) {
                best = dist[j];
                u = j;
            }
        }

        if (u == -1)
            break;

        visited[u] = 1;

        if (u == end)
            break;

        for (j = 0; j < g->count[u]; j++) {
            Edge1 e = g->edges[u][j];
            double newDist = dist[u] + edgeTime(e);

            if (!visited[e.to] && newDist < dist[e.to]) {
                dist[e.to] = newDist;
                parent[e.to] = u;
                parentEdge[e.to] = j;
            }
        }
    }

    return dist[end] != DBL_MAX;
}

/*Path Details*/

int getPathDetails(Graph1 *g, int start, int end, int hour,
                   double *distance, double *travelTime,
                   double *waitTime, int *transfers,
                   int path[], int pathRoute[], int *pathLength)
{
    int parent[X], parentEdge[X];
    int reversePath[X], reverseEdge[X];
    int length = 0, reverseLength = 0;
    int current, i;
    Mode previousMode = BUS;
    int previousRoute = -1;
    int first = 1;

    *distance = 0.0;
    *travelTime = 0.0;
    *waitTime = 0.0;
    *transfers = 0;
    *pathLength = 0;

    if (start == end)
        return 1;

    if (!dijkstra(g, start, end, parent, parentEdge))
        return 0;

    current = end;
    while (current != -1) {
        reversePath[reverseLength] = current;
        if (current != start)
            reverseEdge[reverseLength] = parentEdge[current];
        reverseLength++;
        current = parent[current];
    }

    for (i = reverseLength - 1; i >= 0; i--)
        path[length++] = reversePath[i];

    for (i = 1; i < length; i++) {
        int from = path[i - 1];
        int edgeIndex = reverseEdge[length - i - 1];
        Edge1 e = g->edges[from][edgeIndex];

        pathRoute[i] = e.routeIndex;
        *distance += e.distance;
        *travelTime += edgeTime(e);

        if (first) {
        *waitTime += expectedWaitingTime(e.mode, hour);
         previousMode = e.mode;
         previousRoute = e.routeIndex;
         first = 0;
        }

        else if (e.mode != previousMode || e.routeIndex != previousRoute) {
        /* A transfer occurs when changing from one vehicle/route
        to another, even if the transport mode is the same. */
        (*transfers)++;

        *waitTime += expectedWaitingTime(e.mode, hour);

        previousMode = e.mode;
        previousRoute = e.routeIndex;
    }
    
    }

    pathRoute[0] = -1;
    *pathLength = length;
    return 1;
}

void printPath(Graph1 *g, int start, int end)
{
    int path[X], pathRoute[X], pathLength;
    double distance, travelTime, waitTime;
    int transfers;
    int i;

    if (!getPathDetails(g, start, end, 7,
                        &distance, &travelTime, &waitTime,
                        &transfers, path, pathRoute, &pathLength)) {
        printf("  No route available.\n");
        return;
    }

    printf("  Route : ");
    for (i = 0; i < pathLength; i++) {
        if (i > 0)
            printf(" -> ");
        printf("%s", location[path[i]]);
    }

    printf("\n  Travel Time : %.1f mins", travelTime);
    printf("\n  Waiting Time: %.1f mins", waitTime);
    printf("\n  Total Time  : %.1f mins", travelTime + waitTime);
    printf("\n  Distance    : %.1f km", distance);
    printf("\n  Transfers   : %d\n", transfers);
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

            printf("______________________________________________\n\n");
    printf("  DIJKSTRA FASTEST ROUTE EXAMPLE \n\n");

    printf("  Pettah -> Homagama\n");
    printPath(&city, 0, 8);
}