#ifndef ERA_H
#define ERA_H

#include <QtNetwork/QHostInfo>
#include <QString>
#include <QVector>
#include <QProcess>
#include <QThread>
#include <omp.h>
#include <yarp/os/all.h>
#include <yarp/sig/all.h>
#include "som.hpp"

using namespace yarp::os;
using namespace yarp::sig;
using namespace std;

class Era : public QThread
{

    private:

        BufferedPort<Bottle> speechOutPort;    //these attribute are for connecting through yarp
        BufferedPort<Bottle> speechInPort;
        BufferedPort<Bottle> attributePort;
        BufferedPort<Bottle> movePort;
        Port headInPort;
        Bottle praxResponse;
        QString speechOutPortName;
        QString speechInPortName;
        QString headInPortName;
        QString attributePortName;
        QString movePortName;
        QString praxiconPortName;
        QString serverName;
        QProcess *process;
        int simulationMode;
        int lookX;
        int lookY;

        std::vector<float> colorVector;    //attributes as inputs for differents SOM
        std::vector<float> shapeVector;
        std::vector<float> bodyVector;

        struct Image
        {
            BufferedPort<ImageOf<PixelRgb> > inputPort;
            BufferedPort<ImageOf<PixelRgb> > outputPort;
            QString inputPortName;
            QString outputPortName;
        }; Image leftCam;

        struct ObjectType
        {
            int X;
            int Y;
            int Xmin;
            int Xmax;
            int Ymin;
            int Ymax;
            int width;
            int height;
            int poolIdx;
            int shapeIdx;
            int unitIdx;
            int colourIdx;
        };

        struct WeightsType
        {
            int i;
            int j;
            float weight;
        };
        //Q_DECLARE_TYPEINFO( WeightsType, Q_MOVABLE_TYPE );

        struct ConnectionMatrixType
        {
            int connectMaps[2];
            QVector<WeightsType> weights;
        };
        //Q_DECLARE_TYPEINFO( ConnectionMatrixType, Q_MOVABLE_TYPE );

        struct PoolActivityType
        {
            float activity;
            float previousActivity;
            float previousPreviousActivity;
            float netInput;
            float extInput;
            int grammar;
            QString label;
            int X; //maybe these should be vectors allowing for multiple objects of the same type
            int Y;
        };
        //Q_DECLARE_TYPEINFO( PoolActivityType, Q_MOVABLE_TYPE );

        struct PoolType
        {
            QString kind;
            QString input;
            QVector<PoolActivityType> state;
            int currentWinner;
        };
        //Q_DECLARE_TYPEINFO( PoolType, Q_MOVABLE_TYPE );

        //Hebbian learning parameters for local units
        float internalBias;
        float externalBias;
        float iacMax;
        float iacMin;
        float decay;
        float rest;
        float landa;
        float landaDefault;
        float inhibition;
        float learningScalar;
        bool grammarOnOff;

        QVector<ObjectType> objects;
        QVector<QString> dictionary;
        QVector<ConnectionMatrixType> connectionMatrix;
        QVector<PoolType> pool;

        Som *color_som;
        Som *shape_som;
        Som *body_som;

        std::vector<int> bmu_shape_list;
        std::vector<float> last;



    public:

        bool pointFlag;
        bool pickUp;
        bool drop;
        int speechCount;
        int speechPool;
        int foviaBMU;        //attributes that win (only index of them)
        int shapeBMU;
        int bodyBMU;
        int speechWinner;
        int fixSomUnitBody;

        Era();
        //void initPorts(yarp::os::ResourceFinder &rf); Later for a smooth connection with Ports.h
        bool testLocalServer();
        bool openPorts(QString portPrefix);
        void setServerName(QString name);
        void clean();
        void init();
        void run();
        void setShapeAttribute(const Bottle& attribute);
        void getFovia(ImageOf<PixelRgb> *image, int xCenter, int yCenter, int foviaWidth, int foviaHeight);
        void RGBtoHSV(float r, float g, float b, float *h, float *s, float *v);
        void markImage(ImageOf<PixelRgb> *image, int target_x, int target_y, int r, int g, int b, int type);
        void setupPools();
        void setupConnectionMatrix(int hub, int maxIdx);
        void iacStep(int learningPause);
        void look(int x, int y, int maxPrimeIdx);
        bool isInBMU(int tmp);
        double GetDistance(vector<double> input, vector<double> comp);
        void setBodyInput();
        void setSpeechInput(const Bottle& command, int ignor);
        void activatePoolUnit(QString inputFrom, int unitIdx);
        void trackMotion();
        void lookDown();
        void forceGrammarOnOff();

        void setSimulationMode(bool simulation);
        void connectPorts();
        bool terminalMode;
};

#endif
