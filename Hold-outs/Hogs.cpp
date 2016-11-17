#include "Hogs.hpp"
#include <omp.h>
#include <assert.h>
#include <iostream>
#include <unistd.h>
#include <yarp/sig/Image.h>
#include <yarp/sig/ImageFile.h>
#include <string>
#include <iCub/action/actionPrimitives.h>
//#include <Magick++.h>

using namespace yarp::os;
using namespace std;

Hogs::Hogs(){}

void Hogs::init()
{
    openWorldPorts();
    openArePorts();
}

void Hogs::run()
{
    //sendMkModelWorld();
    //sendMkBoxWorld();
    //Time::delay(2);
    /*
    printf("Attempt number 1\n");
    //reachBoxAre(-0.35,0.15,0);      //first attempt
    Time::delay(2);
    //graspBoxWorld(1);
    //printf("Grasping the box, START THE LEARNING\n");
    //Time::delay(2);
    //reachBoxAre(-0.35,-0.15,0);
    //Time::delay(2);
    //graspBoxWorld(0);
    pointBoxAre(-0.35,-0.15,0);
    Time::delay(2);
    printf("Not a successfull episode, NO REWARD\n");
    homeRobot();
    //printf("Not a successfull episode, NO REWARD\n");
    Time::delay(5);
    */
    printf("Attempt number 1\n");
    //placeBoxWorld();        //second attempt
    Time::delay(2);
    //reachBoxAre(-0.35,0.15,0);
    //Time::delay(2);
    //graspBoxWorld(1);
    //printf("Grasping the box, START THE LEARNING\n");
    printf("LEARNING TO POINT WITH TRANSFER LEARNING\n");
    //Time::delay(2);
    //reachBoxAre(-0.65,0.15,0);
    //Time::delay(2);
    //graspBoxWorld(0);
    pointBoxAre(-0.65,0.15,0);
    Time::delay(2);
    printf("Not a successfull episode, NO REWARD\n");
    homeRobot();
    //printf("Not a successfull episode, NO REWARD\n");
    Time::delay(5);

    printf("Attempt number 2\n");
    //placeBoxWorld();        //second attempt
    Time::delay(2);
    //reachBoxAre(-0.35,0.15,0);
    //Time::delay(2);
    //graspBoxWorld(1);
    //printf("Grasping the box, START THE LEARNING\n");
    printf("LEARNING TO POINT WITH TRANSFER LEARNING\n");
    //Time::delay(2);
    //reachBoxAre(-0.65,0.15,0);
    //Time::delay(2);
    //graspBoxWorld(0);
    pointBoxAre(-0.55,0.25,0);
    Time::delay(2);
    printf("Not a successfull episode, NO REWARD\n");
    homeRobot();
    //printf("Not a successfull episode, NO REWARD\n");
    Time::delay(5);

    printf("Attempt number 3\n");
    //placeBoxWorld();        //third attempt
    Time::delay(2);
    //reachBoxAre(-0.35,0.15,0);
    //Time::delay(2);
    //graspBoxWorld(1);
    //printf("Grasping the box, START THE LEARNING\n");
    printf("LEARNING TO POINT WITH TRANSFER LEARNING\n");
    //Time::delay(2);
    //reachBoxAre(-0.40,-0.15,0);
    //Time::delay(2);
    //graspBoxWorld(0);
    pointBoxAre(-0.40,-0.15,0);
    Time::delay(2);
    //homeRobot();
    printf("Successfull episode, REWARD !\n");
    homeRobot();
    Time::delay(5);

    printf("Attempt number 4\n");
    //placeBoxWorld();        //third attempt
    Time::delay(2);
    //reachBoxAre(-0.35,0.15,0);
    //Time::delay(2);
    //graspBoxWorld(1);
    //printf("Grasping the box, START THE LEARNING\n");
    printf("LEARNING TO POINT WITH TRANSFER LEARNING\n");
    //Time::delay(2);
    //reachBoxAre(-0.40,-0.15,0);
    //Time::delay(2);
    //graspBoxWorld(0);
    pointBoxAre(-0.45,-0.15,0);
    Time::delay(2);
    //homeRobot();
    printf("Successfull episode, REWARD !\n");
    homeRobot();
    Time::delay(5);

    //pointBoxAre();
    //placeBoxWorld();
}

void Hogs::sendMkBoxWorld()
{

    bool send_cmd = false;
    Bottle response;
    Bottle cmd;

    while (send_cmd == false)
    {
        if (world.getOutputCount() == 0)
        {
            printf("Trying to connect to %s\n", server_world);
            yarp_w.connect(client_world,server_world);
            printf("Wait\n");
        }
        else
        {
            cmd.addString("world");
            cmd.addString("mk");
            cmd.addString("box");
            cmd.addDouble(0.05);
            cmd.addDouble(0.05);
            cmd.addDouble(0.05);
            cmd.addDouble(-0.15);
            cmd.addDouble(0.553975);
            cmd.addDouble(0.35);
            cmd.addInt(0);
            cmd.addInt(0);
            cmd.addInt(80);
            //printf("Sending message... %s\n", cmd.toString().c_str());

            world.write(cmd,response);
            //printf("Got response: %s\n", response.toString().c_str());
        }
        std::string str (response.toString().c_str());
        std::string void_str ("");
        if (void_str.compare(str) != 0)
        {
            send_cmd = true;
        }
    //Time::delay(10);
    }
}

void Hogs::placeBoxWorld()
{

    bool send_cmd = false;
    Bottle response;
    Bottle cmd;

    while (send_cmd == false)
    {
        if (world.getOutputCount() == 0)
        {
            printf("Trying to connect to %s\n", server_world);
            yarp_w.connect(client_world,server_world);
            printf("Wait\n");
        }
        else
        {
            cmd.addString("world");
            cmd.addString("set");
            cmd.addString("box");
            cmd.addInt(1);
            cmd.addDouble(-0.15);
            cmd.addDouble(0.553975);
            cmd.addDouble(0.35);
            //printf("Sending message... %s\n", cmd.toString().c_str());

            world.write(cmd,response);
            //printf("Got response: %s\n", response.toString().c_str());
        }
        std::string str (response.toString().c_str());
        std::string void_str ("");
        if (void_str.compare(str) != 0)
        {
            send_cmd = true;
        }
    //Time::delay(10);
    }
}

void Hogs::sendMkModelWorld()
{
    bool send_cmd = false;
    Bottle response;
    Bottle cmd;

    while (send_cmd == false)
    {
        if (world.getOutputCount() == 0)
        {
            printf("Trying to connect to %s\n", server_world);
            yarp_w.connect(client_world,server_world);
            printf("Wait\n");
        }
        else
        {
            cmd.addString("world");
            cmd.addString("mk");
            cmd.addString("smodel");
            cmd.addString("att.x");
            cmd.addString("text.bmp");
            cmd.addDouble(0.15);
            cmd.addDouble(0.53);
            cmd.addDouble(0.35);
            //printf("Sending message... %s\n", cmd.toString().c_str());

            world.write(cmd,response);
            //printf("Got response: %s\n", response.toString().c_str());
        }
        std::string str (response.toString().c_str());
        std::string void_str ("");
        if (void_str.compare(str) != 0)
        {
            send_cmd = true;
        }
    //Time::delay(10);
    }
}

void Hogs::reachBoxAre(double x, double y, double z)
{
    bool send_cmd = false;
    Bottle response;
    Bottle b;

    while (send_cmd == false)
    {
        if (are.getOutputCount() == 0)
        {
            printf("Trying to connect to %s\n", server_world);
            yarp_are.connect(client_are,server_are);
            printf("Wait\n");
        }
        else
        {
            int cmd=yarp::os::Vocab::encode("touch");

            yarp::sig::Vector v(3,0.0);
            v[0] = x;
            v[1] = y;
            v[2] = z;

            b.addVocab(cmd);
            b.addList().read(v);
            b.addString("still");
            b.addString("left");
            //printf("Sending message... %s\n", b.toString().c_str());
            are.write(b,response);
            //printf("Got response: %s\n", response.toString().c_str());
        }
        std::string str (response.toString().c_str());
        std::string void_str ("");
        if (void_str.compare(str) != 0)
        {
            send_cmd = true;
        }
    //Time::delay(10);
        //cmd.clear();
    }
}

void Hogs::pointBoxAre(double x, double y, double z)
{
    bool send_cmd = false;
    Bottle response;
    Bottle b;

    while (send_cmd == false)
    {
        if (are.getOutputCount() == 0)
        {
            printf("Trying to connect to %s\n", server_world);
            yarp_are.connect(client_are,server_are);
            printf("Wait\n");
        }
        else
        {
            int cmd=yarp::os::Vocab::encode("point");

            yarp::sig::Vector v(3,0.0);
            v[0] = x;
            v[1] = y;
            v[2] = z;

            b.addVocab(cmd);
            b.addList().read(v);
            b.addString("still");
            b.addString("left");
            //printf("Sending message... %s\n", b.toString().c_str());
            are.write(b,response);
            //printf("Got response: %s\n", response.toString().c_str());
        }
        std::string str (response.toString().c_str());
        std::string void_str ("");
        if (void_str.compare(str) != 0)
        {
            send_cmd = true;
        }
    Time::delay(10);
        //cmd.clear();
    }
}

void Hogs::homeRobot()
{

    bool send_cmd = false;
    Bottle response;
    Bottle b;

    while (send_cmd == false)
    {
        if (are.getOutputCount() == 0)
        {
            printf("Trying to connect to %s\n", server_world);
            yarp_are.connect(client_are,server_are);
            printf("Wait\n");
        }
        else
        {
            int cmd=yarp::os::Vocab::encode("home");
            b.addVocab(cmd);
            //printf("Sending message... %s\n", b.toString().c_str());
            are.write(b,response);
            //printf("Got response: %s\n", response.toString().c_str());
        }
        std::string str (response.toString().c_str());
        std::string void_str ("");
        if (void_str.compare(str) != 0)
        {
            send_cmd = true;
        }
    //Time::delay(5);
        //cmd.clear();
    }
}

void Hogs::graspBoxWorld(int g)
{

    bool send_cmd = false;
    Bottle response;
    Bottle cmd;

    while (send_cmd == false)
    {
        if (world.getOutputCount() == 0)
        {
            printf("Trying to connect to %s\n", server_world);
            yarp_w.connect(client_world,server_world);
            printf("Wait\n");
        }
        else
        {
            if(g == 1)
            {
                cmd.addString("world");
                cmd.addString("grab");
                cmd.addString("box");
                cmd.addInt(1);
                cmd.addString("left");
                cmd.addInt(1);
            }
            else
            {
                cmd.addString("world");
                cmd.addString("grab");
                cmd.addString("box");
                cmd.addInt(1);
                cmd.addString("left");
                cmd.addInt(0);
            }
            //printf("Sending message... %s\n", cmd.toString().c_str());

            world.write(cmd,response);
            //printf("Got response: %s\n", response.toString().c_str());
        }
        std::string str (response.toString().c_str());
        std::string void_str ("");
        if (void_str.compare(str) != 0)
        {
            send_cmd = true;
        }
    //Time::delay(10);
    }
}

bool Hogs::openWorldPorts()
{
    bool test = false;
    world.open(client_world);
    yarp_w.connect(client_world,server_world);
    while(test == false)
    {
        if (world.getOutputCount() == 0)
        {
            printf("Trying to connect to %s\n", server_world);
            yarp_w.connect(client_world,server_world);
        }
        else
        {
            test = true;
        }
    }
    return test;
}

bool Hogs::openArePorts()
{
    bool test = false;
    are.open(client_are);
    yarp_are.connect(client_are,server_are);
    while(test == false)
    {
        if (are.getOutputCount() == 0)
        {
            printf("Trying to connect to %s\n", server_are);
            yarp_are.connect(client_are,server_are);
        }
        else
        {
            printf("Connected\n");
            test = true;
        }
    }

    return test;
}
