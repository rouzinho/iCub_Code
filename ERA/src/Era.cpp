#include "Era.h"
#include <omp.h>
#include <assert.h>
#include <iostream>
#include <unistd.h>
#include <yarp/sig/Image.h>
#include <yarp/sig/ImageFile.h>
//#include <Magick++.h>

using namespace yarp::os;
using namespace std;

Era::Era(){}

bool Era::openPorts(QString portPrefix)
{
    //assign port names

    Network yarp;
    leftCam.inputPortName = portPrefix + QString("/cam/left:i");
    leftCam.outputPortName = portPrefix + QString("/cam/left:o");
    speechInPortName = portPrefix + QString("/speech:i");
    speechOutPortName = portPrefix + QString("/speech:o");
    //speechInPortName = QString("/speech:i");
    //speechOutPortName = QString("/speech:o");
    headInPortName = portPrefix + QString("/head:i");
    attributePortName = portPrefix + QString("/attribute:i");
    movePortName = portPrefix + QString("/move:o");

    //open ports
    return  (
            leftCam.inputPort.open(leftCam.inputPortName.toStdString().c_str()) &&
            leftCam.outputPort.open(leftCam.outputPortName.toStdString().c_str()) &&
            speechInPort.open(speechInPortName.toStdString().c_str()) &&
            speechOutPort.open(speechOutPortName.toStdString().c_str()) &&
            attributePort.open(attributePortName.toStdString().c_str()) &&
            headInPort.open(headInPortName.toStdString().c_str()) &&
            movePort.open(movePortName.toStdString().c_str()));
}

bool Era::testLocalServer()
{
    qDebug("testLocalServer() CALLED");
    if(yarp::os::Network::exists(serverName.toStdString().c_str(), true)) //server detected
    {
        qDebug("local server was found on %s", serverName.toStdString().c_str());
        return true;
    }
    else //server not detected - try to launch it
    {
        QString program = "yarprun";
        QStringList arguments = QStringList()<<"--server"<<serverName.toStdString().c_str();
        process = new QProcess(this);
        process->start(program, arguments);
        process->waitForStarted();
        if(testLocalServer())
        {
            qDebug("local server was started on %s", serverName.toStdString().c_str());
            return true;
        }
        else
        {
            qCritical("local server was not found on %s and attempts to launch it failed", serverName.toStdString().c_str());
            return false;
        }
    }
}

void Era::setServerName(QString name)   //set the server name.
{
    serverName = name;
}

void Era::init()        //init Era by pretraining the 2 SOM (color,shape,body?) and the other parameters.
{
    //srand(time(NULL));
    speechPool = 3;
    pointFlag = false;
    bool suc = false;
    while(suc == false)
    {
        color_som = new Som(32,32,1000,2);
        shape_som = new Som(32,32,1000,3);
        body_som = new Som(32,32,1000,4);
        color_som->RunHsvSOM();
        //color_som->PrintSOM();
        //cout<<"----------------------------------------------------\n";
        shape_som->RunShapeSOM();
        //shape_som->PrintSOM();
        //cout<<"----------------------------------------------------\n";
        body_som->RunBodySOM();
        //body_som->PrintSOM();

        //init the vector that will receive the color information from the camera
        for (int i = 0; i < 36;i++) {
    		colorVector.push_back(float(0));
    	}
        //init the vector of the shape SOM
        for (int i = 0; i < 8; i++) {
    		shapeVector.push_back(float(0));
    	}
        //init the vector of the body SOM
        for (int i = 0; i < 7; i++) {
		bodyVector.push_back(float(0));    		
		//bodyVector.push_back(float(-20 + float((2* 20+ 1)* 1.* rand()/ (RAND_MAX+ 1.))));
    	}
        suc = true;
    }
    cout<<"ready\n";

    internalBias    = 0.1f;      //bias for internal activity 0-1 (typically .1)
    externalBias    = 0.5f;      //bias for external activity 0-1 (typically 0.8)
    iacMax          = 1.0f;      //the maximum unit activity (typically 1)
    iacMin          = -0.2f;     //the minimum unit activity (typically -0.2)
    decay           = 0.5f;      //the rate of decay of activity relative to the distance from rest (typically 0.5)
    rest            = -0.01f;    //the resting level of activity for any unit (typically -0.01)
    landa           = 0.15f; //05f;     //the IAC learning rate //was 0.05
    landaDefault    = 0.15f; //05f;     //the IAC learning rate //was 0.05
    inhibition      = -0.8f;     //the level of constant inhibition within each pool (typically -0.8)
    learningScalar  = 0.01f; //25f; //0.01f; //0.0001f;   //used to subdue and amplify learning with events such as speech input

    fixSomUnitBody = 10;

    //set up some pools
    setupPools();

    //intialize IAC connection matrix
    setupConnectionMatrix(speechPool, 4); // (hub, maxIdx)

}

void Era::run()
{
    int X,Y;
    lookX = 160;
    lookY = 120;
    int maxPrimeIdx;
    bool running = true;
    bool lookFlag = false;
    bool learnFlag = false;
    int lookPause = 0;
    float primed;
    int speechFlag = 0;
    int learningPause = 0;
    float maxPrime;
    vector<float> newtmp;

    ImageOf<PixelRgb> *leftImage = NULL;
    Bottle *speech = NULL;
    Bottle *attribute = NULL;

    while(running)
    {
        //read the attributes of the object;

        leftImage = leftCam.inputPort.read(false);
        if(leftImage != NULL)
        {
            //cout<<"Got left Image \n";
            speech = speechInPort.read(false);

            if(speech!=NULL)
            {
                cout<<"speech not null\n";
                setSpeechInput(*speech, 0);
                speechFlag = 1;
            }
	    

            if(!learningPause)
            {
                attribute = attributePort.read(false);
                if(attribute!=NULL)
                {
                    //cout<<"Got attributes \n";
                    setShapeAttribute(*attribute);
                    //cout<<"number of objects "<<objects.size()<<"\n";
                }
		
                //read the color of the object
                maxPrime = 0;
                maxPrimeIdx = -1;
		
		if(objects.size() < 2)
		{
                	look(lookX,lookY,0);
		}
		setBodyInput();
		
                for(int objectNumber = 0; objectNumber < objects.size(); objectNumber++)
                {
                    foviaBMU = -1;
                    X = objects.at(objectNumber).X;
                    Y = objects.at(objectNumber).Y;
                    getFovia(leftImage, X, Y, objects.at(objectNumber).width, objects.at(objectNumber).height);


                    //usleep(1000000);
                    foviaBMU = color_som->GetBMU(colorVector);
                    objects[objectNumber].colourIdx = foviaBMU;
                    //cout<<"BMU color"<<foviaBMU<<"\n";
                    //newtmp = color_som->GetVectorBMU(colorVector);
                    /*cout<<"BMU  Vector \n";
                    for(int i=0; i<36; i++)
                    {
                        cout<<newtmp[i]<<"  ";
                    }
                    /*cout<<"\n";
                    cout<<"Color  Vector \n";
                    for(int i=0; i<36; i++)
                    {
                        cout<<colorVector[i]<<"  ";
                    }

                    cout<<"BMU  Index "<<foviaBMU<<"\n";
                    cout<<"\n-------------------------------\n";

                    //cout<<"number of objects : "<<objects.size()<<"\n";
                    //objects[objectNumber].colourIdx = foviaBMU;
                    //objects.resize(0);
                    usleep(1000000);*/
                    if(foviaBMU == pool[0].currentWinner)
                    {
                        lookX = X;
                        lookY = Y;
                        lookFlag = true;
                    }
                    if(pointFlag)
                    {
                        //cout<<"point flag\n";
                        //*******************************************************************
                        //** look at each object in turn and tell me what words are primed **
                        //*******************************************************************
                        //zero all activity except the current focus object
                        for(int i=0; i<pool.size(); i++)
                        {
                            for(int j=0; j<pool.at(i).state.size(); j++) //pool.at(i).state.size()
                            {
                                pool[i].state[j].activity = -0.2;
                                pool[i].state[j].extInput = 0.0;
                            }
                        }


                        if(objects[objectNumber].colourIdx > -1) pool[0].state[objects[objectNumber].colourIdx].extInput = 1;
                        if(objects[objectNumber].shapeIdx > -1)  pool[1].state[objects[objectNumber].shapeIdx].extInput = 1;
                            //step the iac network WITH NO LEARNING

                        for(int steps = 0; steps < 10; steps ++) iacStep(1);

                        //now tell me what the word activity is for any words recieving external input
                        if(speechWinner > -1 && speechWinner < pool.at(speechPool).state.size())
                        {
                            printf("Object %d activates external word %d at: %f\n", objectNumber, speechWinner, pool.at(speechPool).state.at(speechWinner).activity);
                            //cout<<"Object "<<objectNumber<<" activates external word "<<speechWinner<<" at "<<pool.at(speechPool).state.at(speechWinner).activity<<"\n";
                            //emit messageSet(message);
                            if(maxPrime < pool.at(speechPool).state.at(speechWinner).activity)
                            {
                                maxPrime = pool.at(speechPool).state.at(speechWinner).activity;
                                maxPrimeIdx = objectNumber;
                            }
                        }
                            //do actual pointing after considering each object
			printf("maxPrimeIdx : %d\n",maxPrimeIdx);
                    }
                    else if(learningScalar > 0.05)
                    {
                        learnFlag = false;
                        //scan each word
                        for(int word=0; word<dictionary.size(); word++)
                        {
        //                       messageSet("looking for primed object");
                            //is this word recieving external input?
                            if(pool[speechPool].state[word].extInput > 0.05)
                            {
                                learnFlag = true;
                                printf("word %d has external input\n",word);
                                //cout<<"word "<<word<<" has external input\n";
                                //messageSet(message);
                                primed = 0;
                                //and is there a positive link between this object and word?
                                for(int connected=0; connected<connectionMatrix.size(); connected++)
                                {
                                    int poolIdxA = connectionMatrix.at(connected).connectMaps[1];
                                    int poolIdxB = connectionMatrix.at(connected).connectMaps[0];
                                        //Find pools that are connected to the speechPool
                                    if((poolIdxA < 2 && poolIdxB == speechPool) || (poolIdxB < 2 && poolIdxA == speechPool))
                                    {	
                                        for(int connection=0; connection<connectionMatrix.at(connected).weights.size(); connection++)   //for each connection between those pools
                                        {
                                            int i = connectionMatrix.at(connected).weights.at(connection).i;
                                            int j = connectionMatrix.at(connected).weights.at(connection).j;
                                            if((i == foviaBMU || i == shapeBMU) && j == word)                                     //is this a connection between a winning colour / shape and an external word
                                            {
                                                primed += connectionMatrix.at(connected).weights.at(connection).weight;
                                                if(primed > maxPrime)
                                                {
						    printf("affectation maxPrimeIdx\n");
                                                    maxPrime = primed;
                                                    maxPrimeIdx = objectNumber;
                                                    lookX = X;
                                                    lookY = Y;
                                                    lookFlag = true;
                                                    lookPause = 0;
                                                }
                                            }
                                        }
                                    }
                                }
                                printf("Object %d is primed by word %d at a level of %f **************************\n", objectNumber, word, primed);
                                //cout<<"Object "<<objectNumber<<" is primed by word "<<word<<" at a level of "<<primed<<" **************************\n";
                                //emit messageSet(message);
                            }
                        }
                    }
                }
                

                if(pointFlag)
                {
                    printf("Point to Object %d \n", maxPrimeIdx);
                    //emit messageSet(message);
                    if(maxPrimeIdx > -1)
                    {
                        lookX = objects.at(maxPrimeIdx).X;
                        //if(lookX < 120) lookX += 17;
                        //else if(lookX < 175) lookX += 13;
                        lookY = objects.at(maxPrimeIdx).Y;
                        look(lookX, lookY, maxPrimeIdx);
                        //yarp::os::Time::delay(0.1);
			usleep(10000000);
                        //point(lookX, lookY, maxPrimeIdx);       //note: point(...) resets the pointFlag to false but takes too long to do it
                    }
                    pointFlag = false;
                }

                if(!learnFlag) learningScalar = 0;

                if(!pointFlag)
                {
                    maxPrimeIdx = -1;
                }

                if(maxPrimeIdx > -1)
                {
                    for(int objectNo=0; objectNo<objects.size(); objectNo++)
                    {
                        if(objectNo != maxPrimeIdx && objects[objectNo].colourIdx > -1 && objects[objectNo].colourIdx < 100)
                        {
                            pool[0].state[objects[objectNo].colourIdx].activity = -0.2;
                            pool[0].state[objects[objectNo].colourIdx].extInput = 0.0;
                            pool[1].state[objects[objectNo].shapeIdx].activity = -0.2;
                            pool[1].state[objects[objectNo].shapeIdx].extInput = 0.0;
                        }
                        else
                        {
                            //sprintf(message, "external input to single object %d at colourIdx %d, shapeIdx %d", maxPrimeIdx, objects[maxPrimeIdx].colourIdx, objects[maxPrimeIdx].shapeIdx);
                            cout<<"external input to single object "<<maxPrimeIdx<<" at colourIdx "<<objects[maxPrimeIdx].colourIdx<<", shapeIdx "<<objects[maxPrimeIdx].shapeIdx<<"\n";
                            //emit messageSet(message);
                            pool[0].state[objects[maxPrimeIdx].colourIdx].extInput = 1.0;
                            pool[1].state[objects[maxPrimeIdx].shapeIdx].extInput = 1.0;
                        }
                    }
                }
                else
                {
                    for(int objectNo=0; objectNo<objects.size(); objectNo++)
                    {
                        if(objects[objectNo].colourIdx > -1 && objects[objectNo].colourIdx < 100)
                        {
                            pool[0].state[objects[objectNo].colourIdx].extInput = 1.0;
                        }
                        if(objects[objectNo].shapeIdx > -1 && objects[objectNo].shapeIdx < 100)
                        {
                            pool[1].state[objects[objectNo].shapeIdx].extInput = 1.0;
                        }
                    }
                }
                learningPause = speechFlag * 2 * objects.size();
                speechFlag = 0;
                objects.resize(0);
            }
            else
            {
                learningPause --;
            }

            if(lookFlag && lookPause == 0)
            {
                if(pointFlag)
                {
                    //look(lookX, lookY, maxPrimeIdx);          //removed for cogsci experiment, only look when asked to point
                }
                markImage(leftImage, lookX, lookY, 255,255,255, 1);

                lookFlag = false;
                if(maxPrimeIdx > -1) {
                    //emit messageSet("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
                    lookPause = 50;
                }
            }

            if(lookPause > 0)
            {
                //sprintf(message, "lookPause is %d - triger actions at 40", lookPause);
                //messageSet(message);
                lookPause --;

                if(lookPause == 40 && pointFlag)
                {
                    //point(lookX, lookY, maxPrimeIdx);
                }
            }
            //emit messageSet("POINT FLAG IS TRUE!!!!");

            //field inputs are automatically updated as field input streams in through yarp ports and is managed in interface.cpp with calls to era.cpp functions

            //***************
            //run IAC network
            //***************
            iacStep(learningPause);

            //*******************
            //work out what to do
            //*******************
            //say what you see
            if(speechCount > 0)
            {
                //speechOutput();
                speechCount --;
                //usleep(2000000);
            }

            //display the modified image
            leftCam.outputPort.prepare().copy(*leftImage);
            leftCam.outputPort.write();
        }
    }
}

/*!
 * \brief Gets attribute info... The content of the port is something like this:
 *   Affordance object descriptor output port \n
 *   The message is a Bottle containing several values.
 *   The first value is an integer indicating the number of objects (N).
 *   The consecutive N values (one per object) are lists (Bottles) containing
 *   the objects descriptors, with the following order:
 * - 0 (double) - normalized x coordinate of the center of the enclosing rectangle (between -1 and 1 w.r.t. the image size).
 * - 1 (double) - normalized y coordinate of the center of the enclosing rectangle (between -1 and 1 w.r.t. the image size).
 * - 2 (double) - normalized width of the enclosing rectangle (between 0 and 1 w.r.t. the the image size)
 * - 3 (double) - normalized height of the enclosing rectangle (between 0 and 1 w.r.t. the the image size)
 * - 4 (double) - angle (orientation) of the object's enclosing rectangle, according to OpenCV definition (see CvBox2D).
 * - 5 (double) - reserved for future usage, set to 0.
 * - 6 (double) - reserved for future usage, set to 0.
 * - 7 (double) - value of the 1st bin of the Hue normalized color histogram of the pixels inside the object's region.
 * - ...
 * - 22 (double) - value of the 16th (last) bin of the Hue normalized color histogram of the pixels inside the object's region.
 * - 23 (double) - area
 * - 24 (double) - convexity - ratio between the perimeter of the object's convex hull and the perimeter of the object's contour.
 * - 25 (double) - eccentricity - ratio between the minor and major axis of the minimum area enclosing rectangle
 * - 26 (double) - compactness - ratio between the object area and its squared perimeter.
 * - 27 (double) - circleness - ratio between the object area and the area of its enclosing circle.
 * - 28 (double) - squareness - ratio between the object area and the area of its minimum-area enclosing rectangle
 */

void Era::setShapeAttribute(const Bottle& attribute)
{
    double dist;
    vector<float> new_tmp;
    //for(int j = 0; j < 5;j++){


    for(int i = 0; i < attribute.get(0).asInt(); i++)
    {
        shapeBMU = -1;
        shapeVector[0] = 0; //width
        shapeVector[1] = 0; //height
        shapeVector[2] = attribute.get(i+1).asList()->get(24).asDouble(); //convexity
        shapeVector[3] = attribute.get(i+1).asList()->get(25).asDouble(); //eccentricity
        shapeVector[4] = attribute.get(i+1).asList()->get(26).asDouble(); //compactness
        shapeVector[5] = attribute.get(i+1).asList()->get(27).asDouble(); //circleness
        shapeVector[6] = attribute.get(i+1).asList()->get(28).asDouble(); //squareness
        shapeVector[7] = attribute.get(i+1).asList()->get(29).asDouble(); //elongatedness

        shapeBMU = shape_som->GetBMU(shapeVector);
        //cout<<"BMU shape --"<<shapeBMU<<"   ";
        //if(isInBMU(shapeBMU) == false)
        //{
            //bmu_shape_list.push_back(shapeBMU);
            ObjectType object;
            object.width = int(attribute.get(i+1).asList()->get(2).asDouble());
            //new_tmp.push_back(object.width);
            //cout<<object.width;
            //cout<<"----";
            object.height = int(attribute.get(i+1).asList()->get(3).asDouble());
            //new_tmp.push_back(object.height);
            //cout<<object.height;
            //cout<<"----";
            object.X = int(attribute.get(i+1).asList()->get(0).asDouble());
            lookX = object.X;
            //new_tmp.push_back(object.X);
            //cout<<object.X;
            //cout<<"----";
            object.Y = int(attribute.get(i+1).asList()->get(1).asDouble());
            lookY = object.Y;
            //new_tmp.push_back(object.Y);
            //cout<<object.Y;
            //cout<<"----";
            object.Xmin = object.X - int(object.width / 2);
            //cout<<object.Xmin;
            //cout<<"----";
            object.Ymin = object.Y - int(object.height / 2);
            //cout<<object.Ymin;
            //cout<<"----";
            object.Xmax = object.X + int(object.width / 2);
            //cout<<object.Xmax;
            //cout<<"----";
            object.Ymax = object.Y + int(object.height / 2);
            //cout<<object.Ymax;
            //cout<<"----";
            object.poolIdx = -1;
            object.unitIdx = -1;

            object.shapeIdx = shapeBMU;
            objects.push_back(object);
            //usleep(2000000);
        //}

        //cout<<"BMU   "<<shapeBMU<<"\n";
        //cout<<"shape ";
        /*for(int j = 0;j < shapeVector.size();j++)
        {
            cout<<shapeVector[j];
            cout<<"------";
        }
        cout<<"\n";
        /*if(last.size() != 0)
        {
            dist = GetDistance(last,new_tmp);
            cout<<"distance  "<<dist;
        }
        last.resize(0);
        for(int j = 0; j < new_tmp.size(); j++)
        {
            last.push_back(new_tmp[j]);
        }*/

        //usleep(3000000);
    //cout<<"STOPm m  \n";

    }
}

bool Era::isInBMU(int tmp)
{
    bool inside = false;
    for(int i = 0; i < bmu_shape_list.size(); i++)
    {
        if (bmu_shape_list[i] == tmp)
        {
            inside = true;
        }
    }

    return inside;
}

void Era::getFovia(ImageOf<PixelRgb> *image, int xCenter, int yCenter, int foviaWidth, int foviaHeight)
{
    int HSV_bin = 0;
    int totalPixels = 0;
    float h,s,v;
    bool primaryColourOnly = false;
    //reset the color vector
    for (int i = 0; i < 36;i++) {
		colorVector[i] = 0.0;
	}
    for(int x=xCenter - (foviaWidth/2); x<xCenter + (foviaWidth/2) + 1; x++)
    {
        for(int y=yCenter - (foviaHeight/2); y<yCenter + (foviaHeight/2) + 1; y++)
        {
            if(image->isPixel(x,y)) //this makes sure the x,y co-ordinate is valid!
            {
                //get a pixel and convert it from RGB to HSV
                PixelRgb& pixel = image->pixel(x,y);
                RGBtoHSV((float)pixel.r/255, (float)pixel.g/255, (float)pixel.b/255, &h, &s, &v);

                //increment the colourSpectrum of the appropriate colour bin
                if(s > 0.07)
                {
                    HSV_bin = (int)h/10;            //if s == 0, then h = -1 (undefined)
                    colorVector[HSV_bin] ++;
                    totalPixels ++;
                }
            }
        }
    }
    if(totalPixels > 0)
    {
        if(primaryColourOnly)
        {

            /*for(int i=0; i<36; i++)
            {
                sprintf(message, "%s %2.2f", message, colourSpectrum[i]);
            }
            emit messageSet(message);*/


            //find the maximum colour
            float bestColour = 0;
            int bestColourIdx = -1;
            for(int i=0; i<36; i++)
            {
                if(colorVector[i] > bestColour)
                {
                    bestColour = colorVector[i];
                    bestColourIdx = i;
                }
            }
            for(int i=0; i<36; i++)
            {
                colorVector[i] = 0;
            }
            bestColourIdx = bestColourIdx / 10;
            colorVector[bestColourIdx] = 1;
            //sprintf(message, "best colour idx is %d", bestColourIdx);
            //emit messageSet(message);

            /*sprintf(message, "");
            for(int i=0; i<36; i++)
            {
                sprintf(message, "%s %2.2f", message, colourSpectrum[i]);
            }
            emit messageSet(message);*/
        }
        else
        {
            //normalise the colourSpectrum
            for(int i=0; i<36; i++)
            {
                colorVector[i] /= totalPixels;
                //sprintf(message, "%s %2.2f", message, colourSpectrum[i]);
            }
            //emit messageSet(message);
        }
    }
    /*for(int j = 0;j < colorVector.size();j++)
    {
        cout<<colorVector[j];
    }
    cout<<"-----------\n";*/
}

void Era::RGBtoHSV(float r, float g, float b, float *h, float *s, float *v)
{
    // r,g,b values are from 0 to 1
    // h = [0,360], s = [0,1], v = [0,1]
    //		if s == 0, then h = -1 (undefined)

    float min, max, delta;

    min = std::min( r, std::min( g, b ));
    max = std::max( r, std::max( g, b ));
    *v = max;				// v
    delta = max - min;
    if( max != 0 )
        *s = delta / max;		// s
    else {
        // r = g = b = 0		// s = 0, v is undefined
        *s = 0;
        *h = -1;
        return;
    }

    if( r == max )      *h = ( g - b ) / delta;		// between yellow & magenta
    else if( g == max )	*h = 2 + ( b - r ) / delta;	// between cyan & yellow
    else                *h = 4 + ( r - g ) / delta;	// between magenta & cyan

    *h *= 60;				// degrees
    if( *h < 0 )        *h += 360;
    //printf("hsv : %f %f %f\n",*h,*s,*v);
}

void Era::markImage(ImageOf<PixelRgb> *image, int target_x, int target_y, int r, int g, int b, int type)
{
    //draw an X at the target location in the original image
    int y1 = target_y - 10;
    int y2 = target_y + 10;
    for(int x=target_x - 10; x<target_x + 11; x++)
    {
        if(image->isPixel(x,y1))
        {
            PixelRgb& pixel = image->pixel(x,y1);
            pixel.r = r;
            pixel.g = g;
            pixel.b = b;
        }
        if(type) y1++;
        if(image->isPixel(x,y2))
        {
            PixelRgb& pixel = image->pixel(x,y2);
            pixel.r = r;
            pixel.g = g;
            pixel.b = b;
        }
        if(type) y2--;
    }
}

void Era::setBodyInput()    
{
    int loopTimeOut;
    char message[100];
    bodyBMU = -1;
    bodyVector.resize(0);
    //get the current body posture...
    //declare head bottle
    Bottle head;// = NULL;

    //read the head input port
    headInPort.read(head);

    //assign the values to the right bits...
    for(int i=0; i<6; i++)
    {
        bodyVector.push_back(head.get(i).asDouble());
        //cout<<bodyVector[i]<<"   ";

    }
    //cout<<"\n";
    //emit messageSet(message);
    bodyVector[6] = 0;

    //pass this to the shape SOM
    bodyBMU = body_som->GetBMU(bodyVector);
    //cout<<"body BMU    "<<bodyBMU<<"\n";
    //cout<<"\n";
    //sendToSom(2, bodyInfo, 7, fixSomUnitBody);
    //check for responce from SOM
 /*   loopTimeOut = 0;
    while(bodyWinner == -1 && loopTimeOut < 100)
    {
        usleep(100);
        loopTimeOut ++;
    }
*/
    if(bodyBMU != -1)
    {
        pool[2].state[bodyBMU].extInput = 1.0;
    }

    //sprintf(message, "body posture Winner is ************************** %d", bodyWinner);
    //emit messageSet(message);

    //check if fixSomUnitBody was actually used (this forces a particular SOM unit to take on the current target if the distance between the winner and target is great)
    /*if(fixSomUnitBody == bodyBMU && fixSomUnitBody < 99)
    {
        fixSomUnitBody ++;
        //sprintf(message, "fixSomUnitBody active, next value is %d **************************", fixSomUnitBody);
        cout<<"fixSomUnitBody active, next value is "<<fixSomUnitBody<<" **************************\n";
        //emit messageSet(message);
    }*/
}

void Era::setSpeechInput(const Bottle& command, int ignor)
{
    bool found = false;
    char message[100];
    int grammar = -1;

    //sprintf(message, "ignoring %d words, next word is:  %s", ignor, command.get(ignor).toString().c_str());
    //emit messageSet(message);

    QString instruction = "what";
    QString instruction2 = "where";
    QString Cinstruction = "What";
    QString Cinstruction2 = "Where";
    QString instruction3 = "table";
    QString Cinstruction3 = "Table";
    QString instruction4 = "learning";
    QString Cinstruction4 = "Learning";
    QString instruction5 = "this";
    QString Cinstruction5 = "This";
    QString CCinstruction5 = "look";
    QString CCCinstruction5 = "Look";
    QString instruction6 = "pickup";
    QString Cinstruction6 = "Pickup";
    QString CCinstruction6 = "grasp";
    QString CCCinstruction6 = "Grasp";
    QString instruction7 = "pick";
    QString Cinstruction7 = "Pick";
    QString instruction8 = "motion";
    QString Cinstruction8 = "Motion";
    QString instruction9 = "drop";
    QString Cinstruction9 = "Drop";

    //praxicon words
    QString instruction10 = "hide";
    QString Cinstruction10 = "Hide";

    //cout<<command.get(ignor).toString().c_str()<<"  ";
    //cout<<"\n";


    // this OR look
    if(command.get(ignor).toString().c_str() == instruction5 || command.get(ignor).toString().c_str() == Cinstruction5 || command.get(ignor).toString().c_str() == CCinstruction5 || command.get(ignor).toString().c_str() == CCCinstruction5)
    {
        //emit messageSet("This is a ...");
        setSpeechInput(command, ignor+3);   //assume "this is an X...." or "look at the X..." so ignore first 3 words to prime
    }

    // learning
    else if(command.get(0).toString().c_str() == instruction4 || command.get(0).toString().c_str() == Cinstruction4)
    {
        if(landa == landaDefault)
        {
            landa = 0.0f;
            //emit messageSet("LEARNING OFF");
        }
        else
        {
            landa = landaDefault;
            //emit messageSet("LEARNING ON");
        }
    }

    // table
    else if(command.get(0).toString().c_str() == instruction3 || command.get(0).toString().c_str() == Cinstruction3)
    {
        lookDown();
        //emit messageSet("look at the table...");
    }

    // what
    else if(command.get(0).toString().c_str() == instruction || command.get(0).toString().c_str() == Cinstruction)      //"what"
    {
        //emit messageSet("What?");
        speechCount = 2;
    }

    // where
    else if(command.get(ignor).toString().c_str() == instruction2 || command.get(ignor).toString().c_str() == Cinstruction2)
    {
        pointFlag = true;
        //emit messageSet("point Flag is true");
        setSpeechInput(command, ignor+3);   //assume "where is the X...." so ignore first 3 words to prime
    }

    //pickup
    else if(command.get(ignor).asString().c_str() == instruction6 || command.get(ignor).asString().c_str() == Cinstruction6 || command.get(ignor).asString().c_str() == CCinstruction6 || command.get(ignor).asString().c_str() == CCCinstruction6)
    {
        pointFlag = true;
        pickUp = true;
        //emit messageSet("pickup Flag is true");
        setSpeechInput(command, ignor+2);   //assume "pickup the X...." so ignore first 2 words to prime
    }

    // pick up
    else if(command.get(ignor).asString().c_str() == instruction7 || command.get(ignor).asString().c_str() == Cinstruction7)
    {
        pointFlag = true;
        pickUp = true;
        //emit messageSet("pickup Flag is true");
        setSpeechInput(command, ignor+3);   //assume "pick up the X...." so ignore first 3 words to prime
    }

    // motion
    else if(command.get(ignor).asString().c_str() == instruction8 || command.get(ignor).asString().c_str() == Cinstruction8)
    {
        //emit messageSet("Track Motion...");
        trackMotion();
    }

    // drop
    else if(command.get(ignor).asString().c_str() == instruction9 || command.get(ignor).asString().c_str() == Cinstruction9)
    {
        pointFlag = true;
        drop = true;
        //emit messageSet("pickup Flag is true... dropping");
        setSpeechInput(command, ignor+3);   //assume "drop in the X...." so ignore first 3 words to prime
    }

    // PRAXICON hide
    else if(command.get(ignor).asString().c_str() == instruction10 || command.get(ignor).asString().c_str() == Cinstruction10)
    {
        //emit messageSet("Engaging PRAXICON...");
        //praxiconOutput(command);
    }


    //else
    //{
        //emit messageSet("processing incoming words...");

        for(int i=ignor; i<command.size(); i++)
        {
            //sprintf(message, "This is word %d: %s, (there are %d words)", ignor, command.get(ignor).toString().c_str(), command.size());
            //emit messageSet(message);
            //mark grammar

            //cout<<command.get(i).toString().c_str()<<"  ";
            //cout<<"\n";


            if(command.size() == ignor+2) //2 words
            {
                if(i==ignor) grammar = 0; //colour field
                if(i==ignor+1) grammar = 1; //first new shape field
            }

            //read the word
            QString word = command.get(i).toString().c_str();
	    word.toLower();

            //find if the word exists
            for(int i=0; i<dictionary.size(); i++)
            {
                if(dictionary.at(i).contains(word))
                {
                    //qDebug(" -----> %s found at %d", word.toStdString().c_str(), i);
                    found = true;
                    //activate the winning unit in the word field
                    activatePoolUnit("speech", i);
                    pool[speechPool].state[i].grammar = grammar; //directly set the grammar value
                    learningScalar = 1.0;  //boost IAC learning
                    i = dictionary.size();  //to finish this loop without breaking the wider loop
                }
            }

            if(!found)
            {
                //add the new word
                dictionary.push_back(word);
                //activate the winning unit in the word field
                activatePoolUnit("speech", dictionary.size()-1);
                pool[speechPool].state[dictionary.size()-1].grammar = grammar; //directly set the grammar value
                learningScalar = 1.0;  //boost IAC learning
                //qDebug(" -----> %s", word.toStdString().c_str());
            }
        }
    //}
}

void Era::setupPools()
{
    //define a pool
    PoolType newPool;
    PoolType newPool2;
    PoolActivityType state;
    PoolActivityType state2;

    //populate the pools
    //newPool.size = 100;
    newPool.kind = "som";
    newPool.input = "fovia";
    for(int i=0; i<100; i++)
    {
        state.activity = 0.0;
        state.previousActivity = 0.0;
        state.previousPreviousActivity = 0.0;
        state.netInput = 0.0;
        state.extInput = 0.0;
        newPool.state.push_back(state);
    }
    pool.push_back(newPool);
    newPool.input = "shape";
    pool.push_back(newPool);
    newPool.input = "body";
    pool.push_back(newPool);

    //newPool2.size = 100;
    newPool2.kind = "field";
    newPool2.input = "speech";
    for(int i=0; i<100; i++)
    {
        state2.activity = 0.0;
        state2.previousActivity = 0.0;
        state2.previousPreviousActivity = 0.0;
        state2.netInput = 0.0;
        state2.extInput = 0.0;
        newPool2.state.push_back(state2);
    }
    pool.push_back(newPool2);
}

void Era::activatePoolUnit(QString inputFrom, int unitIdx)
{
    for(int i=0; i<pool.size(); i++)
    {
        if(pool.at(i).input == inputFrom)
        {
            pool[i].state[unitIdx].extInput = 1.0;
            pool[i].state[unitIdx].grammar = -1; //grammar;
        }
    }
    if(inputFrom == "speech")
    {
        speechWinner = unitIdx;
    }
    else if(inputFrom == "fovia")
    {
        foviaBMU = unitIdx;
    }
    else if(inputFrom == "shape")
    {
        shapeBMU = unitIdx;
    }
    else if(inputFrom == "body")
    {
        bodyBMU = unitIdx;
    }
}

void Era::setupConnectionMatrix(int hub, int maxIdx)
{

    //for now I am connecting all pools to the hub. find a more dynamic solution later...
    for(int idx=0; idx<maxIdx; idx++)
    {
        if(idx != hub) //dont connect the hub to itself!!!
        {
            //define the connection
            ConnectionMatrixType connection;
            connection.connectMaps[0] = hub;
            connection.connectMaps[1] = idx;

            //populate the weights matrix
            //printf("connecting - %d to %d, thats %d x %d = %d connections\n", hub, idx, poolSize.at(idx).size, poolSize.at(hub).size, poolSize.at(idx).size * poolSize.at(hub).size);
            for(int i=0; i<pool.at(idx).state.size()-1; i++)
            {
                for(int j=0; j<pool.at(hub).state.size()-1; j++)
                {
                    //define a single weight
                    WeightsType weights;
                    weights.i = i;
                    weights.j = j;
                    weights.weight = 0.0;
                    //connect the weight to the weights matrix
                    connection.weights.push_back(weights);
                }
            }
            //connect the weights matrix to the connection matrix
            connectionMatrix.push_back(connection);
        }
    }
}

void Era::iacStep(int learningPause)
{
    int poolIdxA, poolIdxB;
    int i,j;
    char message[500];
    float activity, netInput, activityA, activityB, weight;
    float poolMax = 0.0;
    int poolMaxIdx = -1;
    int grammarBoost = 1;
    //printf("enter the learning\n");
    //calculate the within pool inhibition
    for(int poolIdx=0; poolIdx<pool.size(); poolIdx++)                                                          //for each individual pool
    {
	//printf("pool number %d \n",poolIdx);
        for(int unitIdxUpdate=0; unitIdxUpdate<pool.at(poolIdx).state.size(); unitIdxUpdate++)                  //for each unit within that pool
        {
           /*printf(" in Pool %d, unit %d has activity %f and extInput %f\n",
                    poolIdx,
                    unitIdxUpdate,
                    pool[poolIdx].state[unitIdxUpdate].activity,
                    pool[poolIdx].state[unitIdxUpdate].extInput);
            //emit messageSet(message);
		*/
            pool[poolIdx].state[unitIdxUpdate].netInput = 0.0;                                                  //reset its netActivity to 0
            for(int unitIdxInfluencing=0; unitIdxInfluencing<pool.at(poolIdx).state.size(); unitIdxInfluencing++) //for every other unit in that same pool
            {
                if(unitIdxUpdate != unitIdxInfluencing && pool.at(poolIdx).state.at(unitIdxInfluencing).activity > 0.0) //NOTE: inhibition is only from positively active units (this prevents occilation of inactive maps)
                {                                                                                               //and add the activity of the other units in the same pool * inhibition
                    pool[poolIdx].state[unitIdxUpdate].netInput += inhibition * pool.at(poolIdx).state.at(unitIdxInfluencing).activity;
                }
            }
        }
    }

    //calculate the spread of activity between pools, and update the weights
    for(int connected=0; connected<connectionMatrix.size(); connected++)                                        //for each pair of connected pools
    {
        poolIdxA = connectionMatrix.at(connected).connectMaps[1];
        poolIdxB = connectionMatrix.at(connected).connectMaps[0];
        for(int connection=0; connection<connectionMatrix.at(connected).weights.size(); connection++)           //for each connection between those pools
        {
            i = connectionMatrix.at(connected).weights.at(connection).i;
            j = connectionMatrix.at(connected).weights.at(connection).j;
            activityA = pool.at(poolIdxA).state.at(i).activity;
            activityB = pool.at(poolIdxB).state.at(j).activity;

            //turn off words that are not heard right now
            if(poolIdxA == speechPool)
            {
                if(pool.at(poolIdxA).state.at(i).extInput < 0.05) activityA = 0;
            }
            if(poolIdxB == speechPool)
            {
                if(pool.at(poolIdxB).state.at(j).extInput < 0.05) activityB = 0;
            }

            weight = connectionMatrix.at(connected).weights.at(connection).weight;

            //check for rediculous values...
            if(activityA > 1 || activityA < -1)
            {
                printf("!!!!! %f : this is unit %d in pool %d\n", activityA, i, poolIdxA);
                //cout<<activityA<<" this is unit "<<i<<" in pool "<<poolIdxA;
            }
            if(activityB > 1 || activityB < -1)
            {
                printf(message, "!!!!! %f : this is unit %d in pool %d\n", activityB, j, poolIdxB);
                //cout<<activityB<<" this is unit "<<j<<" in pool "<<poolIdxB;
            }

            //is this an inter-pool drag down?
            int dragDownA = 1;
            int dragDownB = 1;
            /*if(weight > 0)
            {
                if(pool.at(poolIdxB).state.at(j).activity < 0) dragDownA = 10;
                if(pool.at(poolIdxA).state.at(j).activity < 0) dragDownB = 10;
            }*/

            //update the netInput
            pool[poolIdxA].state[i].netInput += dragDownA * pool.at(poolIdxB).state.at(j).activity * weight;                //sum the incoming acitivity by the weight of the connection between them
            pool[poolIdxB].state[j].netInput += dragDownB * pool.at(poolIdxA).state.at(i).activity * weight;

            //is there grammar
            if(grammarOnOff)
            {   //if this word has grammar information, and that grammar information links it to a pool we are NOT currently looking at then switch off learning
                if((poolIdxA == speechPool && pool.at(poolIdxA).state.at(i).grammar > -1 && pool.at(poolIdxA).state.at(i).grammar != poolIdxB) ||
                   (poolIdxB == speechPool && pool.at(poolIdxB).state.at(j).grammar > -1 && pool.at(poolIdxB).state.at(j).grammar != poolIdxA))
                {
                    grammarBoost = 0;
                }
                else
                {
                    grammarBoost = 1;
                }
            }

            //update the weights
            if(!learningPause)
            {
                // if at least one unit is active...
                if(activityA > 0 || activityB > 0)
                {
                    //NOTE: changed this so both OFF does not increase the weight between units, only when they are both ON!!!!
                    if(activityA > 0 && activityB > 0)
                    {
                        //update the weights for positive correlation
                        connectionMatrix[connected].weights[connection].weight += grammarBoost * learningScalar * landa * activityA * activityB * (1 - weight);
                        printf("Positive correlation between pool %d unit %d, and pool %d unit %d has weight of %f\n", poolIdxA, i, poolIdxB, j, connectionMatrix[connected].weights[connection].weight);
                        //emit messageSet(message);
                    }
                    else
                    {
                        //and for negative correlation
                        connectionMatrix[connected].weights[connection].weight += learningScalar * landa * activityA * activityB * (1 + weight);
                    }
                }

                //tell me about positive weights
                /*if(connectionMatrix[connected].weights[connection].weight > 0.01 && learningScalar > 0.05)
                {
                    sprintf(message, "Positive weight between pool %d unit %d [%f], and pool %d unit %d : [%f], has weight of %f", poolIdxA, i, activityA, poolIdxB, j, activityB, connectionMatrix[connected].weights[connection].weight);
                    emit messageSet(message);
                }*/

                //check weights, as the learning scalar can mess up the normailisation process
                if(connectionMatrix[connected].weights[connection].weight > 1.0)
                {
                    connectionMatrix[connected].weights[connection].weight = 1.0;
                    cout<<" Huge weight encountered, consider changing the learning scalar behaviour";
                }
                if(connectionMatrix[connected].weights[connection].weight < -1.0)
                {
                    connectionMatrix[connected].weights[connection].weight = -1.0;
                    cout<<" Huge weight encountered, consider changing the learning scalar behaviour";
                }

            }
        }
    }

    //bias the internal and external input and finally update the activity of each node
    for(int poolIdx=0; poolIdx<pool.size(); poolIdx++)                                                          //for each individual pool
    {
        poolMax = 0.0;
        poolMaxIdx = -1;
        for(int unitIdxUpdate=0; unitIdxUpdate<pool.at(poolIdx).state.size(); unitIdxUpdate++)                  //for each unit within that pool
        {
            //update the internal and external bias
            pool[poolIdx].state[unitIdxUpdate].netInput *= internalBias;                                        //multiply net input by the internal bias
            pool[poolIdx].state[unitIdxUpdate].netInput += externalBias * pool.at(poolIdx).state.at(unitIdxUpdate).extInput; //add the external input (scalled by the external Bias)
            pool[poolIdx].state[unitIdxUpdate].extInput *= 0.9f;                                                //reset the external input

            //update the final activity of each unit
            pool[poolIdx].state[unitIdxUpdate].previousPreviousActivity = pool.at(poolIdx).state.at(unitIdxUpdate).previousActivity;
            pool[poolIdx].state[unitIdxUpdate].previousActivity = pool.at(poolIdx).state.at(unitIdxUpdate).activity;
            activity = pool.at(poolIdx).state.at(unitIdxUpdate).activity;
            netInput = pool.at(poolIdx).state.at(unitIdxUpdate).netInput;

            if(netInput > 0)
            {                                                                                                   //positive update rule
                pool[poolIdx].state[unitIdxUpdate].activity += ((iacMax - activity) * netInput) - (decay * (activity - rest));
            }
            else
            {                                                                                                   //negative update rule
                pool[poolIdx].state[unitIdxUpdate].activity += ((activity - iacMin) * netInput) - (decay * (activity - rest));
            }

            //check that unit activity is still correcrly bounded
            activity = pool.at(poolIdx).state.at(unitIdxUpdate).activity;
            if(activity > iacMax)
            {
                pool[poolIdx].state[unitIdxUpdate].activity = iacMax;
            }
            if(activity < iacMin)
            {
                pool[poolIdx].state[unitIdxUpdate].activity = iacMin;
            }

            //find each pool winner
            if(pool[poolIdx].state[unitIdxUpdate].activity > poolMax)
            {
                poolMax = pool.at(poolIdx).state.at(unitIdxUpdate).activity;
                poolMaxIdx = unitIdxUpdate;
            }
        }
        //store the pool winner
        pool[poolIdx].currentWinner = poolMaxIdx;
    }
    if(learningScalar > 0.001) //0.0001)
    {
        learningScalar *= 0.99f;
    }
}

void Era::forceGrammarOnOff()
{
    if(grammarOnOff)
    {
        grammarOnOff = false;
        cout<<"Grammar is OFF";
    }
    else
    {
        grammarOnOff = true;
        cout<<"Grammar is ON";
    }
}

void Era::trackMotion()
{
    Bottle &b = movePort.prepare();
    b.clear();
    b.addString("trackMotion");
    movePort.write();
}

void Era::lookDown()
{
    Bottle &b = movePort.prepare();
    b.clear();
    b.addString("lookDown");
    movePort.write();
}

double Era::GetDistance(vector<double> inputs,vector<double> comp)
{
	double distance = 0;

	for(int i = 0; i < comp.size(); ++i)
	{
		distance += (inputs[i] - comp[i]) * (inputs[i] - comp[i]);
	}


	return sqrt(distance);
}

void Era::look(int x, int y, int maxPrimeIdx)
{
    Bottle &b = movePort.prepare();
    b.clear();
    b.addString("move");
    b.addString("head");
    b.addInt(x);
    b.addInt(y);
    movePort.write();
}

void Era::clean()
{

    //close camera ports
    leftCam.outputPort.close();
    leftCam.inputPort.close();

    //close speech ports
    speechInPort.close();
    speechOutPort.close();

    //close attribute port
    attributePort.close();

    //close move port
    movePort.close();
}

void Era::connectPorts()
{
    //logFileActivity = fopen("eraActivityLogFile.txt", "w");
    //fprintf(logFileActivity, "\n\n\nNew Log Created...\n");
/*    if(getSimulationMode())
    {
        Network::connect("/icubSim/cam/left", leftCam.inputPortName.toStdString().c_str());
    }
    else
    {
        //Network::connect("/icub/cam/left", leftCam.inputPortName.toStdString().c_str());
        //Network::connect("/icub/camcalib/left/out", leftCam.inputPortName.toStdString().c_str());
        //Network::connect("/sequentialLabeller/rawImg:o", leftCam.inputPortName.toStdString().c_str());
        //Network::connect("/blobDescriptor/viewImg:o", leftCam.inputPortName.toStdString().c_str());
        Network::connect("/blobDescriptor/rawImg:o", leftCam.inputPortName.toStdString().c_str());
        //Network::connect("/tracker/0/cam/left:o", leftCam.inputPortName.toStdString().c_str());
    } */
}

void Era::setSimulationMode(bool simulation)
{
    simulationMode = simulation;
}
