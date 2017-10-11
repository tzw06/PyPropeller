//
//  manipulator.cpp
//  viewer
//
//  Created by 田中伟 on 2017/6/11.
//  Copyright © 2017年 田中伟. All rights reserved.
//

#include "manipulator.h"
#include "messenger.h"

#include <QColor>
#include <osg/Material>
#include <osg/Point>

Manipulator::Manipulator(osgViewer::View *pView, osg::Camera *pAxesCamera)
    : osgGA::MultiTouchTrackballManipulator()
{
    _shiftKeyDown = false;
    _ctrlKeyDown = false;
    
    view = pView;
    axesCamera = pAxesCamera;
}

bool Manipulator::handle(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa)
{
    switch (ea.getEventType())
    {
        case osgGA::GUIEventAdapter::FRAME:
            return handleFrame(ea, aa);
        case osgGA::GUIEventAdapter::RESIZE:
            return handleResize(ea, aa);
        default:
            break;
    }
    
    if (ea.getHandled())
        return false;
    
    switch (ea.getEventType())
    {
        case osgGA::GUIEventAdapter::MOVE:
            return handleMouseMove(ea, aa);
            
        case osgGA::GUIEventAdapter::DRAG:
            return handleMouseDrag(ea, aa);
            
        case osgGA::GUIEventAdapter::PUSH:
            return handleMousePush(ea, aa);
            
        case osgGA::GUIEventAdapter::RELEASE:
            return handleMouseRelease(ea, aa);
        case osgGA::GUIEventAdapter::KEYDOWN:
            return handleKeyDown(ea, aa);
            
        case osgGA::GUIEventAdapter::KEYUP:
            return handleKeyUp(ea, aa);
            
        case osgGA::GUIEventAdapter::SCROLL:
            return handleMouseWheel(ea, aa);
            
        default:
            return false;
    }
}

bool Manipulator::handleFrame(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa)
{
    osgViewer::Viewer *viewer = dynamic_cast<osgViewer::Viewer*>(&aa);
    if (viewer) {
        osg::Vec3d eye, center, up;
        viewer->getCamera()->getViewMatrixAsLookAt(eye, center, up);
        if (axesCamera) {
            eye = eye - center;
            eye.normalize();
            eye = eye * 10;
            axesCamera->setViewMatrixAsLookAt(eye, osg::Vec3d(0,0,0), up);
        }
    }
    
    return osgGA::MultiTouchTrackballManipulator::handleFrame(ea, aa);
}

bool Manipulator::handleMouseMove(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa)
{    
    return osgGA::MultiTouchTrackballManipulator::handleMouseMove(ea, aa);
}

bool Manipulator::handleMouseRelease( const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa )
{
    if (ea.getButton()==osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON) {
        pick(ea.getX(), ea.getY());
    }

    return osgGA::MultiTouchTrackballManipulator::handleMouseRelease(ea, aa);
}

bool Manipulator::handleKeyDown( const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &us )
{
    switch (ea.getKey())
    {
        case osgGA::GUIEventAdapter::KEY_Shift_L:
        case osgGA::GUIEventAdapter::KEY_Shift_R:
            _shiftKeyDown = true;
            return true;
        case osgGA::GUIEventAdapter::KEY_Control_L:
        case osgGA::GUIEventAdapter::KEY_Control_R:
            _ctrlKeyDown = true;
            return true;
        default:
            return false;
    }
}

bool Manipulator::handleKeyUp( const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &us )
{
    switch (ea.getKey())
    {
        case osgGA::GUIEventAdapter::KEY_Shift_L:
        case osgGA::GUIEventAdapter::KEY_Shift_R:
            _shiftKeyDown = false;
            return true;
        case osgGA::GUIEventAdapter::KEY_Control_L:
        case osgGA::GUIEventAdapter::KEY_Control_R:
            _ctrlKeyDown = false;
            return true;
        default:
            return false;
    }}


bool Manipulator::performMovementLeftMouseButton( const double eventTimeDelta, const double dx, const double dy )
{
    return true;
}


bool Manipulator::performMovementMiddleMouseButton( const double eventTimeDelta, const double dx, const double dy )
{
    if (_shiftKeyDown)  //  pan model
    {
        float scale = -0.3f * _distance * getThrowScale( eventTimeDelta );
        panModel( dx*scale, dy*scale );
    }
    else if (_ctrlKeyDown)  // zoom model
    {
        zoomModel( dy * getThrowScale( eventTimeDelta ), true );
    }
    else    // rotate camera
    {
        if( getVerticalAxisFixed() )
            rotateWithFixedVertical( dx, dy );
        else
            rotateTrackball( _ga_t0->getXnormalized(), _ga_t0->getYnormalized(),
                            _ga_t1->getXnormalized(), _ga_t1->getYnormalized(),
                            getThrowScale( eventTimeDelta )*10.0 );
    }
    
    return true;
}


bool Manipulator::performMovementRightMouseButton( const double eventTimeDelta, const double /*dx*/, const double dy )
{
    return true;
}

void Manipulator::pick(float x, float y)
{
    osgUtil::LineSegmentIntersector::Intersections intersections;
    if (!view->computeIntersections(x,y,intersections)) {
        messenger->showMessage("");
        return;
    }

    QStringList axisTitles;
    axisTitles << "X+" << "Y+" << "Z+" << "X-" << "Y-" << "Z-" << "ISO";

    bool firstNodeFound = false;

    for (auto iter=intersections.begin();iter!=intersections.end();iter++)
    {
        if (iter->nodePath.empty())
            continue;

        const osg::NodePath np = iter->nodePath;
        for(int i=np.size()-1;i>=0;i--)
        {
            osg::ref_ptr<osg::Node> node = dynamic_cast<osg::Node *>(np[i]);
            if (node && node->getName()!="")
            {
                QString title = QString::fromStdString(node->getName());

                if (axisTitles.contains(title))
                {
                    int idir = axisTitles.indexOf(title);
                    messenger->setViewDirection(idir);
                }

                firstNodeFound = true;
                break;
            }
        }

        if (firstNodeFound)
            break;
    }
}

