A clean, modular C++ implementation of the Ant Colony Optimization meta-heuristic algorithm used to solve the Traveling Salesperson Problem (TSP) on a 20-city coordinate space.

This project compares two variants:
1. **Original ACO:** Uses fixed evaporation ($\rho = 0.5$) and uniform random starting cities.
2. **Modified ACO:** Introduces a **Nearest-Neighbour (NN)** starting heuristic and **Adaptive Evaporation** ($\rho \in [0.3, 0.9]$) to escape local stagnation.

## Features

- **Adaptive Pheromone Evaporation:** Dynamically increases evaporation when stagnation is detected to prevent early convergence to local minima.
- **Nearest-Neighbour Initialization:** Gives ants an optimal heuristic starting city baseline.
- **Zero Heavy Dependencies:** Uses modern standard C++ (`<vector>`, `<cmath>`, `<random>`) with no external libraries required.

## How to Build and Run

### Direct Compilation (GCC / Clang)
```bash
g++ -O3 -std=c++17 ACO_Code.cpp -o aco_tsp
./aco_tsp
