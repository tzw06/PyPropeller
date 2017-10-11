//
//  manipulator.hpp
//  viewer
//
//  Created by 田中伟 on 2017/6/11.
//  Copyright © 2017年 田中伟. All rights reserved.
//

#ifndef manipulator_hpp
#define manipulator_hpp

#include <osgGA/AnimationPathManipulator>
#include <osgGA/MultiTouchTrackballManipulator>
#include <osgViewer/Viewer>
#include <QString>

class ViewerMessenger;

class Manipulator : public osgGA::MultiTouchTrackballManipulator
{
public:
    Manipulator(osgViewer::View *pView, osg::Camera *pAxesCamera);
      
    void setMessenger(ViewerMessenger *msg) {messenger=msg;}

    virtual bool handle(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa);

    virtual bool handleFrame( const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa );
    virtual bool handleMouseMove( const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa );
    virtual bool handleMouseRelease( const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa );
    virtual bool handleKeyDown( const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &us );
    virtual bool handleKeyUp( const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &us );

    virtual bool performMovementLeftMouseButton( const double eventTimeDelta, const double dx, const double dy );
    virtual bool performMovementMiddleMouseButton( const double eventTimeDelta, const double dx, const double dy );
    virtual bool performMovementRightMouseButton( const double eventTimeDelta, const double dx, const double dy );

private:
    void pick(float x, float y);

    bool _shiftKeyDown, _ctrlKeyDown;
    
    osgViewer::View *view;
    osg::Camera *axesCamera;
    ViewerMessenger *messenger;
};

#endif /* manipulator_hpp */
