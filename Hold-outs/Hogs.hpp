#ifndef HOGS_H
#define HOGS_H

#include <QtNetwork/QHostInfo>
#include <QString>
#include <QVector>
#include <QProcess>
#include <QThread>
#include <omp.h>
#include <yarp/os/all.h>
#include <yarp/sig/all.h>

using namespace yarp::os;
using namespace yarp::sig;
using namespace std;

class Hogs : public QThread {

    private :

        RpcClient world;
        RpcClient are;
        Network yarp_w;
        Network yarp_are;
        const char *client_world = "/worldcmd:o";
        const char *server_world = "/icubSim/world";
        const char *client_are = "/arecmd:o";
        const char *server_are = "/actionsRenderingEngine/cmd:io";

    public :

        Hogs();
        void init();
        void run();
        void sendMkBoxWorld();
        void placeBoxWorld();
        void sendMkModelWorld();
        void reachBoxAre(double x, double y, double z);
        void pointBoxAre(double x, double y, double z);
        void homeRobot();
        void graspBoxWorld(int g);
        bool openWorldPorts();
        bool openArePorts();
};

#endif HOGS_H
