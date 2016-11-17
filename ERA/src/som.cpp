#include "som.hpp"
#include "Node.hpp"
#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>

using namespace std;

Som::Som(int w, int h, int ep, int m)
{
    srand(time(NULL));
    epoch = ep;
    width = w;
    height = h;
    map_radius = max(width, height)/2;
    lambda = epoch/log(map_radius);
    learning_rate = start_lr;
    current_time = 0;
    mode = m;

    network = new Node* [height];
    for (int k = 0; k < height; ++k)
    {
        network [k] = new Node[width];
    }
    for (int i = 0; i < height; ++i)
    {
        for (int j = 0; j < width; ++j)
        {
            network [i][j].initNodeCoor(i,j);
            if(mode == 0)
            {
                network [i][j].initNodeWithRndColor();
            }
            if(mode == 1)
            {
                network[i][j].initNodeWithCoord();
            }
            if(mode == 2)
            {
                network[i][j].initNodeHSV();
            }
            if(mode == 3)
            {
                network[i][j].initNodeShape();
            }
            if(mode == 4)
            {
                network[i][j].initNodeBody();
            }
        }
    }
}

void Som::PrintSOM()
{
	for (int i = 0; i < height; ++i)
	{
            for (int j = 0; j < width; ++j)
            {
                network [i][j].PrintNodeVector();
            }
            cout<<endl;
	}
}

void Som::BestMatchUnit(Node n)
{
	float best = 1000;
	float last = 1000;
	int i_tmp, j_tmp;
    int ind_tmp = 0;
	for (int i = 0; i < height; ++i)
	{
		for (int j = 0; j < width; ++j)
		{
            ind_tmp++;
			last = network [i][j].GetDistance(n.GetVector());
			if (last < best)
			{
				best = last;
                indexBMU = ind_tmp;
				i_tmp = i;
				j_tmp = j;
			}
		}
	}

	bmu = network[i_tmp][j_tmp];
}

int Som::GetBMU(std::vector<float> vect)
{
    float best = 1000;
	float last = 1000;
	int i_tmp, j_tmp;
    int ind_tmp = 0;
	for (int i = 0; i < height; ++i)
	{
		for (int j = 0; j < width; ++j)
		{
            ind_tmp++;
			last = network [i][j].GetDistance(vect);
			if (last < best)
			{
				best = last;
                indexBMU = ind_tmp;
			}
		}
	}
    //cout<<"distance BMU    "<<best<<"\n";
    //cout<<"index BMU    "<<indexBMU<<"\n";
    return indexBMU;
}

std::vector<float> Som::GetVectorBMU(std::vector<float> vect)
{
    float best = 1000;
	float last = 1000;
	int i_tmp, j_tmp;
    int ind_tmp = 0;
	for (int i = 0; i < height; ++i)
	{
		for (int j = 0; j < width; ++j)
		{
            ind_tmp++;
			last = network [i][j].GetDistance(vect);
			if (last < best)
			{
				best = last;
                indexBMU = ind_tmp;
                i_tmp = i;
				j_tmp = j;
			}
		}
	}
    //cout<<"distance BMU    "<<best<<"\n";
    //cout<<"index BMU    "<<indexBMU<<"\n";
    return network[i_tmp][j_tmp].GetVector();
}

double Som::NeighbourRadius(int iteration_count)
{
    neighbour_rad = map_radius * exp(-(double)iteration_count/lambda);
}

void Som::CalculateNewWeights(Node example)
{
    for (int i = 0; i < height; ++i)
    {
        for (int j = 0; j < width; ++j)
        {
            double dist_node = (bmu.getXofLattice()-network[i][j].getXofLattice())*
                               (bmu.getXofLattice()-network[i][j].getXofLattice())+
                               (bmu.getYofLattice()-network[i][j].getYofLattice())*
                               (bmu.getYofLattice()-network[i][j].getYofLattice());

                double widthsq = neighbour_rad * neighbour_rad;
                if (dist_node < (neighbour_rad * neighbour_rad))
                {
                    influence = exp(-(dist_node) / (2*widthsq));
                    //influence = exp(-sqrt((dist_node)) / (2*100));

                    network[i][j].AjustWeight(example,learning_rate,influence);
                }
        }
    }
}

void Som::ReduceLR(int iteration_count)
{
    learning_rate = start_lr * exp(-(double)iteration_count/lambda);
}

int Som::GetWidth()
{
    return width;
}

int Som::GetHeight()
{
    return height;
}

Node Som::GetNodeAt(int i, int j)
{
    return network[i][j];
}

void Som::RunColorSOM()
{
    while (current_time < epoch)
    {
        float t = 0.0;
        Node n;
        n.initNodeWithColor();
        BestMatchUnit(n);
        CalculateNewWeights(n);
        NeighbourRadius(current_time);
        ReduceLR(current_time);
        ++current_time;
        //cout<<"epoch : "<<current_time<<"\n";
    }
}

void Som::RunHsvSOM()
{
    while(current_time < epoch)
    {
        float t = 0.0;
        Node n;
        n.initNodeHSV();
        BestMatchUnit(n);
        CalculateNewWeights(n);
        NeighbourRadius(current_time);
        ReduceLR(current_time);
        ++current_time;
    }
}

void Som::RunShapeSOM()
{
    while(current_time < epoch)
    {
        float t = 0.0;
        Node n;
        n.initNodeShape();
        BestMatchUnit(n);
        CalculateNewWeights(n);
        NeighbourRadius(current_time);
        ReduceLR(current_time);
        ++current_time;
    }
}

void Som::RunBodySOM()
{
    while(current_time < epoch)
    {
        float t = 0.0;
        Node n;
        n.initNodeBody();
        BestMatchUnit(n);
        CalculateNewWeights(n);
        NeighbourRadius(current_time);
        ReduceLR(current_time);
        ++current_time;
    }
}

void Som::RunPointSOM()
{
    while(current_time < epoch)
    {
        float t = 0.0;
        Node n;
        n.initNodeWithCoord();
        BestMatchUnit(n);
        CalculateNewWeights(n);
        NeighbourRadius(current_time);
        ReduceLR(current_time);
        ++current_time;
    }
}

Som::~Som(){

	for(int i = 0; i<height; i++)
	{
    	delete [] network[i];
	}

	delete [] network;
}
