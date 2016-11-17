#ifndef SOM
#define SOM
#include <vector>
#include "Node.hpp"

class Som
{

private:

    Node **network;
    int width, height;
    Node bmu;
    int indexBMU;
    double map_radius;
    double lambda;
    double neighbour_rad;
    double influence;
    double learning_rate;
    int epoch;
    int current_time;
    const double start_lr = 0.1;
    int mode;

public:

    Som(int w, int h, int num_iter,int m);

    void PrintSOM();

    int GetWidth();

    int GetHeight();

    int GetBMU(std::vector<float> vect);

    std::vector<float> GetVectorBMU(std::vector<float> vect);

    void BestMatchUnit(Node n);

    void CalculateNewWeights(Node example);

    double NeighbourRadius(int iteration_count);

    void ReduceLR(int num_iter);

    Node GetNodeAt(int i, int j);

    void RunColorSOM();

    void RunHsvSOM();

    void RunPointSOM();

    void RunShapeSOM();

    void RunBodySOM();

	~Som();


};
#endif
