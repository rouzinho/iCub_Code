#include "Node.hpp"
#include <vector>
#include <cstdlib>
#include <iostream>
#include <cmath>

using namespace std;


Node::Node(){
	x = 0;
	y = 0;
}

void Node::initNodeWithCoord()
{
    for (int k = 0; k < 2; ++k)
    {
        float r = (double)(rand()%1500);
        weight.push_back(r);
    }
}

void Node::initNodeHSV()
{
	for (int i = 0; i < 36;i++)
	{
		weight.push_back(float(rand() % 100) / 100);
	}
}

void Node::initNodeShape()
{
	for (int i = 0; i < 8; i++)
	{
		weight.push_back(float(rand() % 100) / 100);
	}
}

void Node::initNodeBody()
{
	for (int i = 0; i < 6; i++)
	{
		//weight.push_back(float(rand() % 100) / 100);		
		weight.push_back(float(-20 + float((2* 20+ 1)* 1.* rand()/ (RAND_MAX+ 1.))));
		//-20 + float((2* 20+ 1)* 1.* rand()/ (RAND_MAX+ 1.));
	}
}

void Node::initNodeWithRndColor()
{
    for (int k = 0; k < 3; ++k)
    {
        float r = (double)(rand()%255);
        weight.push_back(r);
    }
}

void Node::initNodeWithColor()
{
    int d = rand()%8;

    switch(d)
    {
        case 0:
            weight.push_back(255.0);
            weight.push_back(0.0);
            weight.push_back(0.0);
            break;
        case 1:
            weight.push_back(255.0);
            weight.push_back(0.0);
            weight.push_back(0.0);
            break;
        case 2:
            weight.push_back(0.0);
            weight.push_back(255.0);
            weight.push_back(0.0);
            break;
        case 3:
            weight.push_back(0.0);
            weight.push_back(0.0);
            weight.push_back(255.0);
            break;
        case 4:
            weight.push_back(255.0);
            weight.push_back(255.0);
            weight.push_back(0.0);
            break;
        case 5:
            weight.push_back(255.0);
            weight.push_back(0.0);
            weight.push_back(255.0);
            break;
        case 6:
            weight.push_back(0.0);
            weight.push_back(255.0);
            weight.push_back(255.0);
            break;
        case 7:
            weight.push_back(255.0);
            weight.push_back(255.0);
            weight.push_back(255.0);
            break;
        case 8:
            weight.push_back(155.0);
            weight.push_back(155.0);
            weight.push_back(155.0);
            break;
        default:break;
    }
}

void Node::PrintNode()
{
	cout<<"[";
	cout<<x<<" "<<y;
	cout<<"]";
}

double Node::getXofLattice()
{
	return x;
}

double Node::getYofLattice()
{
	return y;
}

double Node::getXCoord()
{
    return weight[0];
}

double Node::getYCoord()
{
    return weight[1];
}

void Node::PrintNodeVector()
{
    cout<<"[";

	for (std::vector<float>::const_iterator i = weight.begin(); i != weight.end(); ++i)
	{
        cout<<*i<<" ";
	}
    cout<<"]";

}

float Node::GetDistance(const vector<float> input)
{
	float distance = 0;

	for(int i = 0; i < weight.size(); ++i)
	{
		distance += (input[i] - weight[i]) * (input[i] - weight[i]);
	}


	return sqrt(distance);
}

std::vector<float> Node::GetVector ()
{
	return weight;
}

void Node::initNodeCoor( int i, int j)
{
	x = (double) i;
	y = (double) j;
}

void Node::AjustWeight(Node n, double lr, double infl)
{
	std::vector<float> v = n.GetVector();
	for (int i = 0; i < weight.size(); ++i)
	{
        weight[i] +=  lr * (v[i] - weight[i]) * infl;
	}
}

Node::~Node(){};
