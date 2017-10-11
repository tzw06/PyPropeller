#ifndef MODELVIEW_H
#define MODELVIEW_H

#include "adapter.h"

#include <osg/Shape>
#include <osg/ShapeDrawable>
#include <osg/Point>
#include <osg/LineWidth>
#include <osg/Geode>
#include <osg/Depth>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osg/TexEnv>
#include <osg/TexGen>
#include <osg/Texture2D>
#include <osgText/Font>
#include <osgText/Text>
#include <osg/Material>
#include <osgUtil/SmoothingVisitor>
#include <osg/Multisample>
#include <osg/BlendFunc>
#include <osg/LightSource>

struct ViewerOption
{
    QString bgColorName, bgColorName0, bgColorName1;
    float bgGradFactor;

    bool cameraProjectionPerspective;
};

class Manipulator;
class ViewerMessenger;
class AnimationPathManipulator;
class CameraAnimationCompletedCallback;

struct ViewerOption;

class ModelView : public osgViewer::Viewer, public AdapterWidget
{
public:
    ModelView(QWidget * parent = 0, const char * name = 0, const QGLWidget * shareWidget = 0);
    virtual ~ModelView();

    enum DirectionType {
        FRONT,
        BACK,
        TOP,
        BOTTOM,
        LEFT,
        RIGHT,
        ISO
    };

    enum ModelType {
        STL,
        PLOT3D,
    };

    void setSurrounding(QString title);

    void initViewer();

    void setResourcePath(QString path);

    void updateScene();
    void updateOption();
    
    void setViewDirection(int direction=ISO);

    void capture(QString filename);

    void open(QString filename, int type);
    void write(QString filename);

    ViewerOption * getOption() {return option;}

protected:
    virtual void paintGL();
    void resizeGL(int w, int h);
    void createBackgroundCamera();
    void createAxesCamera();
    
    void setCameraDirection(osg::Vec3d &eye, osg::Vec3d &center, osg::Vec3d &up);

    void loadFromSTL(QString filename);
    void loadFromPlot3D(QString filename);
    
    osg::ref_ptr<osg::Group> group, modelGroup;
    osg::ref_ptr<osg::Camera> bgCamera, axesCamera;
    osg::ref_ptr<Manipulator> manipulator;

    osg::ref_ptr<osg::Geometry> bgGeometry;
    osg::ref_ptr<osgText::Font> defaultFont;

    ViewerOption *option;
    ViewerMessenger *messenger;

    QString ResourcePath;
    
    bool firstRun;
};

class ViewerQT : public ModelView
{
    Q_OBJECT

public:
    ViewerQT(QWidget * parent = 0, const char * name = 0, const QGLWidget * shareWidget = 0);

private slots:
    void onSetViewDirection(int idir);
};

#endif // MODELVIEW_H
