#ifndef NODE
#define NODE
#include <vector>

class Node
{

private:
	std::vector<float> weight;
	double x,y;

public:

	Node();

	//Node(int nb_weight, int i, int j);

    void initNodeWithColor();

	void initNodeHSV();

    void initNodeWithRndColor();

    void initNodeWithCoord();

	void initNodeShape();

	void initNodeBody();

	void PrintNode();

	void PrintNodeVector();

    double getXofLattice();

    double getYofLattice();

    double getXCoord();

    double getYCoord();

	float GetDistance(std::vector<float> input);

	std::vector<float> GetVector();

	void initNodeCoor(int i, int j);

	void AjustWeight(Node n, double lr, double infl);

	~Node();


};
#endif
