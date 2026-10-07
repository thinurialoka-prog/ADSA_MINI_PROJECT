#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <time.h>
#include <string.h>

#define X 10
#define MAX_EDGES 30
#define MAX_STOPS 6
#define HOURS 24
#define MAX_PASSENGERS 1000
#define QUEUE_SIZE MAX_PASSENGERS
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
typedef struct {
    int id;
    int origin;
    int destination;
    int departureHour;
    double distance;
    double travelTime;
    double waitingTime;
    int transfers;
    int successful;
    int capacityRejected;
} Passenger1;

typedef struct {
    int data[QUEUE_SIZE];
    int front;
    int rear;
    int count;
} PassengerQueue;
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
/*total passengers */
int demand[HOURS] = {
    10, 8, 6, 6, 20, 30,
    40, 100, 150, 100, 70, 60,
    80, 90, 80, 90, 100, 150,
    90, 80, 70, 20, 12, 10
};

Passenger1 passengers[MAX_PASSENGERS];
int passengerCount = 0;

/* One departure per route per simulated hour. */
int routeLoad[MAX_STOPS * 2];
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
    for (i = 0; i < COUNT_OF_TRAIN; i++) {
        for (j = 0; j < trainRoute[i].stopCount - 1; j++) {
            addEdge(g, trainRoute[i].stops[j], trainRoute[i].stops[j + 1],
                    trainRoute[i].distance[j], TRAIN, i);
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
/*FIFO Queue*/

void initQueue(PassengerQueue *q)
{
    q->front = 0;
    q->rear = 0;
    q->count = 0;
}

int enqueue(PassengerQueue *q, int passengerIndex)
{
    if (q->count >= QUEUE_SIZE)
        return 0;

    q->data[q->rear] = passengerIndex;
    q->rear = (q->rear + 1) % QUEUE_SIZE;
    q->count++;
    return 1;
}

int dequeue(PassengerQueue *q, int *passengerIndex)
{
    if (q->count == 0)
        return 0;

    *passengerIndex = q->data[q->front];
    q->front = (q->front + 1) % QUEUE_SIZE;
    q->count--;
    return 1;
}
/*Passengers*/

static unsigned long passengerSeed = 123456789UL;

int passengerRandom(int max)
{
    if (max <= 0)
        return 0;

    passengerSeed = passengerSeed * 1103515245UL + 12345UL;
    return (int)((passengerSeed / 65536UL) % (unsigned long)max);
}

void choosePassengerTrip(int hour, int *origin, int *destination)
{
    do {
        if (hour >= 7 && hour <= 9) {
            /* Morning: inner city -> outer/work/study locations. */
            *origin = passengerRandom(4);
            *destination = 4 + passengerRandom(6);
        } else if (hour >= 17 && hour <= 19) {
            /* Evening: outer/work/study locations -> inner city. */
            *origin = 4 + passengerRandom(6);
            *destination = passengerRandom(4);
        } else {
            *origin = passengerRandom(X);
            *destination = passengerRandom(X);
        }
    } while (*origin == *destination);
}

/*Capacity Check*/
int reserveRouteCapacity(const int path[], int pathLength, Graph1 *g)
{
    int usedRoute[X];
    int usedCount = 0;
    int i, j;

    for (i = 1; i < pathLength; i++) {
        int from = path[i - 1];
        int to = path[i];
        int edgeIndex = -1;

        /* Find the edge selected by Dijkstra between these vertices. */
        for (j = 0; j < g->count[from]; j++) {
            if (g->edges[from][j].to == to) {
                edgeIndex = j;
                break;
            }
        }

        if (edgeIndex >= 0) {
            int route = g->edges[from][edgeIndex].routeIndex;
            int k, already = 0;

            for (k = 0; k < usedCount; k++) {
                if (usedRoute[k] == route &&
                    g->edges[from][edgeIndex].mode == BUS) {
                    already = 1;
                    break;
                }
            }

            if (!already) {
                int capacity = routeCapacity(g->edges[from][edgeIndex].mode);

                /* Route1 index spaces are separate for bus/train, so the
                   load index uses a mode offset. */
                int loadIndex = route;
                if (g->edges[from][edgeIndex].mode == TRAIN)
                    loadIndex = COUNT_OF_BUS + route;

                if (routeLoad[loadIndex] >= capacity)
                    return 0;

                routeLoad[loadIndex]++;
                usedRoute[usedCount++] = route;
            }
        }
    }

    return 1;
}

/*CSV Export*/
void exportJourneyRecords(const char *filename)
{
    FILE *file = fopen(filename, "w");
    int i;

    if (file == NULL) {
        printf("\nUnable to create CSV file: %s\n", filename);
        return;
    }

    fprintf(file,
            "PassengerID,Hour,Origin,Destination,DistanceKM,"
            "TravelTimeMin,WaitingTimeMin,Transfers,Status,CapacityRejected\n");

    for (i = 0; i < passengerCount; i++) {
        Passenger1 *p = &passengers[i];

        fprintf(file,
                "P%03d,%02d:00,%s,%s,%.2f,%.2f,%.2f,%d,%s,%s\n",
                p->id,
                p->departureHour,
                location[p->origin],
                location[p->destination],
                p->distance,
                p->travelTime,
                p->waitingTime,
                p->transfers,
                p->successful ? "SUCCESS" : "FAILED",
                p->capacityRejected ? "YES" : "NO");
    }

    fclose(file);
    printf("\nJourney records exported to %s\n", filename);
}

/*Mulmode Demostration*/

void demonstrateMultimodalJourney(Graph1 *g)
{
    int path[] = {0, 1, 2, 4, 8};
    int pathLength = 5;
    int i;
    double distance = 0.0;
    double travelTime = 0.0;
    double waitTime = 0.0;
    int transfers = 0;

    (void)g;

    printf("______________________________________________\n\n");
    printf("  SPECIFIC MULTIMODAL JOURNEY AT 07:00 \n");
    printf("______________________________________________\n\n");
    printf("  Passenger X: ");

    for (i = 0; i < pathLength; i++) {
        if (i > 0)
            printf(" -> ");
        printf("%s", location[path[i]]);
    }

    printf("\n  Route Used : T01 (Train) -> T02 (Train) -> B02 (Bus)\n");

    distance = 2.0 + 8.0 + 7.0 + 6.0;
    travelTime = (10.0 / 60.0 * 60.0)
               + (7.0 / 60.0 * 60.0)
               + (6.0 / 25.0 * 60.0);
    transfers = 2;
    waitTime = expectedWaitingTime(TRAIN, 7)
             + expectedWaitingTime(BUS, 7);

    printf("  Travel Time : %.1f mins\n", travelTime);
    printf("  Waiting Time: %.1f mins\n", waitTime);
    printf("  Total Time  : %.1f mins\n", travelTime + waitTime);
    printf("  Distance    : %.1f km\n", distance);
    printf("  Transfers   : %d\n", transfers);
}

/*Passenger Simulation*/

void simulatePassengers(Graph1 *g)
{
    int hour;
    int i;
    PassengerQueue queue;

    passengerCount = 0;
    passengerSeed = 123456789UL;
    initQueue(&queue);

    printf("______________________________________________\n\n");
    printf("  PASSENGER DEMAND & JOURNEY SIMULATION \n");
    printf("______________________________________________\n\n");

    for (hour = 0; hour < HOURS; hour++) {
        int queuedThisHour = 0;
        int passengerIndex;

        /* One service departure per route is assumed per hour. */
        for (i = 0; i < COUNT_OF_BUS + COUNT_OF_TRAIN; i++)
            routeLoad[i] = 0;

        printf("\n%02d:00 - %d passengers\n",
               hour, demand[hour]);

        /* Generate passengers in arrival order and put them in FIFO queue. */
        for (i = 0; i < demand[hour]; i++) {
            Passenger1 *p;

            if (passengerCount >= MAX_PASSENGERS)
                break;

            p = &passengers[passengerCount];
            p->id = passengerCount + 1;
            p->departureHour = hour;
            p->distance = 0.0;
            p->travelTime = 0.0;
            p->waitingTime = 0.0;
            p->transfers = 0;
            p->successful = 0;
            p->capacityRejected = 0;

            choosePassengerTrip(hour, &p->origin, &p->destination);

            enqueue(&queue, passengerCount);
            passengerCount++;
            queuedThisHour++;
        }

        /* FIFO boarding order. */
        while (queuedThisHour > 0 && dequeue(&queue, &passengerIndex)) {
            Passenger1 *p = &passengers[passengerIndex];
            int path[X], pathRoute[X], pathLength;

            if (getPathDetails(g,
                               p->origin,
                               p->destination,
                               hour,
                               &p->distance,
                               &p->travelTime,
                               &p->waitingTime,
                               &p->transfers,
                               path,
                               pathRoute,
                               &pathLength)) {

                if (reserveRouteCapacity(path, pathLength, g)) {
                    p->successful = 1;
                } else {
                    p->successful = 0;
                    p->capacityRejected = 1;
                    p->distance = 0.0;
                    p->travelTime = 0.0;
                    p->waitingTime = 0.0;
                    p->transfers = 0;
                }
            }

            queuedThisHour--;
        }
    }

    printf("\nTotal individual passengers generated: %d\n",
           passengerCount);
}

/*Profiling*/

void profiling(void)
{
    int i;
    int totalPassengers = passengerCount;
    int successful = 0;
    int failed = 0;
    int capacityFailed = 0;
    int directTrips = 0;
    int oneTransferTrips = 0;
    int multiTransferTrips = 0;
    double totalTravelTime = 0.0;
    double totalWaitingTime = 0.0;
    double totalDistance = 0.0;
    clock_t start = clock();

    for (i = 0; i < passengerCount; i++) {
        Passenger1 *p = &passengers[i];

        if (p->successful) {
            successful++;
            totalTravelTime += p->travelTime;
            totalWaitingTime += p->waitingTime;
            totalDistance += p->distance;

            if (p->transfers == 0)
                directTrips++;
            else if (p->transfers == 1)
                oneTransferTrips++;
            else
                multiTransferTrips++;
        } else {
            failed++;
            if (p->capacityRejected)
                capacityFailed++;
        }
    }

    clock_t end = clock();

    printf("______________________________________________\n\n");
    printf("  SYSTEM EFFICIENCY PROFILING \n");
    printf("______________________________________________\n\n");

    printf("  Total Passengers Simulated : %d\n", totalPassengers);
    printf("  Successful Journeys        : %d\n", successful);
    printf("  Failed Journeys            : %d\n", failed);
    printf("  Capacity-Rejected Journeys : %d\n", capacityFailed);
    printf("  Success Completion Rate    : %.1f%%\n",
           totalPassengers > 0 ? successful * 100.0 / totalPassengers : 0.0);
    printf("  Average Travel Time        : %.1f mins\n",
           successful > 0 ? totalTravelTime / successful : 0.0);
    printf("  Average Waiting Time       : %.1f mins\n",
           successful > 0 ? totalWaitingTime / successful : 0.0);
    printf("  Average Total Journey Time : %.1f mins\n",
           successful > 0 ? (totalTravelTime + totalWaitingTime) / successful : 0.0);
    printf("  Average Journey Distance   : %.1f km\n",
           successful > 0 ? totalDistance / successful : 0.0);
    printf("  Direct Trips               : %.1f%%\n",
           successful > 0 ? directTrips * 100.0 / successful : 0.0);
    printf("  1 Transfer Trips           : %.1f%%\n",
           successful > 0 ? oneTransferTrips * 100.0 / successful : 0.0);
    printf("  1+ Transfer Trips          : %.1f%%\n",
           successful > 0 ? multiTransferTrips * 100.0 / successful : 0.0);
    printf("  Execution Time             : %.6f seconds\n",
           (double)(end - start) / CLOCKS_PER_SEC);
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
    printf("  TRAIN ROUTES AND TRAIN NETWORK \n");
    printf("______________________________________________\n\n");
    printRoutes(TRAIN);

    demonstrateMultimodalJourney(&city);
    simulatePassengers(&city);
    
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

    profiling();
    exportJourneyRecords("journey_records.csv");

    return 0;
}