#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <limits>
#include <cstdlib>
using namespace std;

// GLOBAL PROBLEM DATA
const int NUM_CITIES = 20;

// 20-city dataset (x, y coordinates)
const double CITY_X[NUM_CITIES]= {12,35,60,80,15,45,70,25,55,90,10,40,65,85,20,50,75,30,60,95};

const double CITY_Y[NUM_CITIES]={ 20,50,10,70,80,30,60,45,85,15,55,25,75,40,65,90,20,35,50,80};

//  ACO HYPERPARAMETERS

// Common parameters shared by both variants
const int NUM_ANTS= 20;    // ants per iteration
const int NUM_ITERATIONS= 100;   // total iterations
const double ALPHA= 1.0;   // pheromone exponent
const double BETA=3.0;   // heuristic (1/dist) exponent
const double Q=100.0; // pheromone deposit constant
const double TAU_INIT= 1.0;   // initial pheromone on all edges

// ORIGINAL variant – fixed evaporation rate
const double RHO_FIXED=0.5;   // evaporation: trail *= (1 - rho)

// MODIFIED variant – adaptive evaporation
const double RHO_MIN= 0.3;   // lower bound for adaptive rho
const double RHO_MAX= 0.9;   // upper bound for adaptive rho
const int STAGNATION_LIMIT= 10;   // iterations with no improvement
                                       // before rho is bumped up

//  UTILITY: Euclidean distance matrix

vector<vector<double>> buildDistMatrix(){
    vector<vector<double>> dist(NUM_CITIES,vector<double>(NUM_CITIES, 0.0));
    for (int i=0;i<NUM_CITIES;i++){
        for(int j=0; j<NUM_CITIES;j++){
            if (i!=j){
                dist[i][j] = hypot(CITY_X[i] - CITY_X[j],CITY_Y[i] - CITY_Y[j]);
            }
        }
    }
    return dist;
}

//  UTILITY: Tour length

double tourLength(vector<int>& tour,const vector<vector<double>>& dist){
    double total= 0.0;
    for (int i=0; i<NUM_CITIES; i++){
        total += dist[tour[i]][tour[(i + 1) % NUM_CITIES]];
    }
    return total;
}

//  INITIALIZATION HELPERS

//Random start: ant begins at a uniformly random city.  Used by the ORIGINAL variant.                        

int randomStart() {
    return rand() % NUM_CITIES;
    }

/* Nearest-Neighbour start: We build a greedy tour from each city, then pick the ant's start city as the one whose greedy tour is shortest. Used by the MODIFIED variant. This gives ants a  departure point, which is better than the random approach. */

int nearestNeighbourStart(vector<vector<double>>& dist){
    int bestCity=0;
    double bestLen= numeric_limits<double>::max();
    for(int start=0; start < NUM_CITIES; start++){
        vector<bool> visited(NUM_CITIES, false);
        vector<int>  tour;
        int cur = start;
        visited[cur] = true;
        tour.push_back(cur);

        for (int step = 1; step < NUM_CITIES; ++step){
            double minD=numeric_limits<double>::max();
            int nearest = -1;
            for (int j=0; j<NUM_CITIES; j++){
            if (!visited[j] && dist[cur][j] < minD){
                minD=dist[cur][j]; 
                nearest=j;
            }
        }                        
        visited[nearest]=true;
        tour.push_back(nearest);
        cur=nearest;
        }
        double len = tourLength(tour, dist);
        if(len < bestLen){
            bestLen = len;
            bestCity = start;
        }
    }
    return bestCity;  // best greedy starting city
}

// CORE: Build one ant's tour using roulette- wheel selection on pheromone × heuristic

vector<int> buildTour(int startCity, vector<vector<double>>&dist,vector<vector<double>>& pheromone){
    vector<bool> visited(NUM_CITIES, false);
    vector<int>  tour;
    tour.reserve(NUM_CITIES);

    int cur=startCity;
    visited[cur]=true;
    tour.push_back(cur);
    for (int step=1;step<NUM_CITIES; step++){
// Compute desirability weights for all unvisited cities
        vector<double> prob(NUM_CITIES, 0.0);
        double total=0.0;
        for(int j=0; j<NUM_CITIES;j++){
            if(!visited[j]){
                double tau=pow(pheromone[cur][j], ALPHA);
                double eta=pow(1.0 / dist[cur][j], BETA); // heuristic
                prob[j]= tau * eta;
                total+= prob[j];
            }
        }
// Roulette-wheel selection
        double threshold= ((double)rand() / RAND_MAX) * total;
        double cumul= 0.0;
        int chosen=-1;
        for(int j=0; j<NUM_CITIES; j++){
            if(!visited[j]){
                cumul += prob[j];
                if(cumul >= threshold){
                    chosen = j; 
                    break; 
                }
            }
        }
// Fallback (floating-point safety)
        if(chosen == -1){
            for (int j = 0; j < NUM_CITIES; ++j){
                if (!visited[j]){ chosen = j; break; }
            }
        }                         
        visited[chosen] = true;
        tour.push_back(chosen);
        cur = chosen;
    }
    return tour;
}

//  PHEROMONE UPDATE (shared logic)
//  rho: evaporation rate passed in by caller

void updatePheromones(vector<vector<double>>& pheromone,vector<vector<int>>& allTours,vector<vector<double>>& dist,double rho){
// 1. Evaporate existing pheromone
    for (int i=0; i<NUM_CITIES; i++)
        for (int j=0; j<NUM_CITIES; j++)
            pheromone[i][j] *= (1.0-rho);

// 2. Deposit new pheromone proportional to tour quality
    for (auto& tour:allTours){
        double len= tourLength(tour, dist);
        double delta =Q /len;          // better tour → more pheromone
        for (int i=0;i<NUM_CITIES;i++){
            int a = tour[i];
            int b = tour[(i + 1) % NUM_CITIES];
            pheromone[a][b] += delta;     // directed deposit
            pheromone[b][a] += delta;     // symmetric (undirected graph)
        }
    }
}

// VARIANT 1 -ORIGINAL ACO
// Fixed rho, random start city per ant

void runOriginalACO(vector<vector<double>>& dist){
    cout<<" ORIGINAL ACO  (fixed rho = " << RHO_FIXED << ")\n";

// Initialise pheromone matrix uniformly
    vector<vector<double>> pheromone(NUM_CITIES, vector<double>(NUM_CITIES, TAU_INIT));

    vector<int> bestTour;
    double bestLen= numeric_limits<double>::max();

    for (int iter = 0; iter < NUM_ITERATIONS; ++iter){
        vector<vector<int>> allTours(NUM_ANTS);

// Each ant builds its tour from a random starting city
        for (int k=0; k<NUM_ANTS;k++){
            int start =randomStart();          // ORIGINAL: random start
            allTours[k]= buildTour(start, dist, pheromone);

            double len=tourLength(allTours[k], dist);
            if (len<bestLen){
                bestLen  = len;
                bestTour = allTours[k];
            }
        }

// Fixed evaporation rate — no adaptation
        updatePheromones(pheromone, allTours, dist, RHO_FIXED);

// Print progress every 10 iterations
        if((iter + 1) % 10 == 0)
            cout<<"Iter " <<setw(3)<<(iter + 1)<<"| Best= "<<fixed<<setprecision(2)<<bestLen<< "\n";
    }
    cout << "\n  >>> Final best tour length: " << bestLen << "\n";
    cout << ">>> Tour Path: ";

    for (int i = 0; i < bestTour.size(); i++) {
        cout << bestTour[i] << " -> ";
    }

    cout << bestTour[0];  // return to start
    cout << "\n";
}

// VARIANT 2 — MODIFIED ACO
// Adaptive rho + nearest-neighbour initialisation

void runModifiedACO(vector<vector<double>>& dist){
    cout<<"MODIFIED ACO(adaptive rho, NN init)\n";
// Initialise pheromone matrix uniformly
    vector<vector<double>> pheromone(NUM_CITIES, vector<double>(NUM_CITIES, TAU_INIT));

    vector<int>bestTour;
    double bestLen =numeric_limits<double>::max();

// MODIFICATION 1: heuristic starting city via nearest-neighbour
    int nnStart=nearestNeighbourStart(dist);

// MODIFICATION 2: adaptive evaporation variables
    double rho= RHO_MIN;       // start conservative
    int stagnation= 0;         // consecutive non-improving iters
    double prevBestLen= numeric_limits<double>::max();

    for (int iter = 0; iter < NUM_ITERATIONS; ++iter){
        vector<vector<int>> allTours(NUM_ANTS);
        bool improved = false;

        for (int k=0; k< NUM_ANTS; k++){
// MODIFICATION 1: all ants depart from the NN-heuristic city
            allTours[k] = buildTour(nnStart, dist, pheromone);
            double len = tourLength(allTours[k], dist);
            if (len< bestLen){
                bestLen=len;
                bestTour=allTours[k];
                improved=true;
            }
        }
// MODIFICATION 2: adjust rho based on stagnation
        if (!improved){
            ++stagnation;
            if (stagnation >= STAGNATION_LIMIT){
                // No improvement for too long → increase evaporation
                // to escape local optima by "forgetting" old trails
                rho=min(rho + 0.05, RHO_MAX);
                stagnation = 0;  // reset counter after bump
            }
        }
        else{
// Improvement found → reduce rho to preserve good trails
            rho =max(rho-0.02, RHO_MIN);
            stagnation= 0;
        }
        prevBestLen = bestLen;

        updatePheromones(pheromone, allTours, dist, rho);

        if((iter+1)%10 ==0){
            cout<<"Iter "<<setw(3)<<(iter+1)<< "| Best=" <<fixed<<setprecision(2)<<bestLen<<"|rho= "<<setprecision(2)<<rho<<"\n";
        }
    }

    cout<<" Final best tour length: "<< bestLen<< "\n";
    cout << ">>> Tour Path: ";

    for (int i = 0; i < bestTour.size(); i++) {
        cout << bestTour[i] << " -> ";
    }

    cout<<bestTour[0];  //return to start
    cout<<"\n";
}

int main(){
    srand(static_cast<unsigned>(42)); // fixed seed for reproducibility

    cout<<"TSP via Ant Colony Optimisation — C++\n";
    cout<<"Cities: "<< NUM_CITIES<< " | Ants: "<<NUM_ANTS<<" |Iterations: "<<NUM_ITERATIONS<<"\n";
    
// Build shared distance matrix
    auto dist = buildDistMatrix();

// Print city coordinates for reference
    cout<<"\nCity Coordinates:\n";
    cout<<left<<setw(6)<< "City"<<setw(10) <<"X" <<setw(10)<<"Y"<< "\n";
    for (int i = 0; i < NUM_CITIES; ++i){
        cout <<setw(6)<< i<< setw(10) << CITY_X[i] <<setw(10) << CITY_Y[i] << "\n";
    }

// Run both variants
    runOriginalACO(dist);
    runModifiedACO(dist);
    cout<<" Comparison Summary\n";
    cout<< "See report for detailed results table.\n";
    cout<<"Parameters: alpha=" <<ALPHA <<" beta=" <<BETA<< " Q=" <<Q<<" tau_init="<<TAU_INIT<<"\n";
    cout<<"Original :rho (fixed) = " <<RHO_FIXED<< "\n";
    cout<<"Modified :rho (adaptive) in ["<<RHO_MIN<< ", "<< RHO_MAX <<"]"<<" |NN heuristic init\n";

    return 0;
}
