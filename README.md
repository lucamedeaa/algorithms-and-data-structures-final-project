Academic Year:2024-2025

Scope of the project: This project implements the final assignment for the Algorithms and Data Structures (API) course at Politecnico di Milano (2024–2025), providing a routing engine for the Movhex transportation system on a dynamically evolving hexagonal map, as specified in API_Final_Project_2025. The program supports map initialization, regional cost variations, air-route updates, and the computation of minimum travel costs between arbitrary tiles.
To achieve high efficiency, I developed a customised routing engine based on Dial’s algorithm, using a bucketed priority structure tailored to the problem’s bounded (≤100) edge weights. The solution also employs a set-associative hash table with a bit-mixed indexing scheme to store and retrieve previously computed travel costs, together with preallocated arrays, contiguous bucket storage, and an optimised strategy for handling hex-grid neighbour traversal, as realised in Medea_Luca_Movhex.

Final Grade: 30 Cum Laude / 30
