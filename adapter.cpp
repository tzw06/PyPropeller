#include "adapter.h"

AdapterWidget::AdapterWidget( QWidget * parent, const char * name, const QGLWidget * shareWidget)
    : QGLWidget(parent, shareWidget)
{
    setObjectName(QString(name));

    osg::DisplaySettings* ds = osg::DisplaySettings::instance().get();
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;

    traits->windowName = "pypropeller";
	traits->windowDecoration = false;
    traits->x = x();
    traits->y = y();
    traits->width = width();
    traits->height = height();
    traits->doubleBuffer = true;
    traits->alpha = ds->getMinimumNumAlphaBits();
    traits->stencil = ds->getMinimumNumStencilBits();
    traits->sampleBuffers = ds->getMultiSamples();
    traits->samples = ds->getNumMultiSamples();

    _gw = new osgViewer::GraphicsWindowEmbedded(traits.get());
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

//    _timer.start(10);
    connect(&_timer, SIGNAL(timeout()), this, SLOT(updateGL()));
}

void AdapterWidget::resizeGL( int width, int height )
{
    _gw->getEventQueue()->windowResize(x(), y(), width, height );
    _gw->resized(x(),y(),width,height);
}

osgGA::GUIEventAdapter::KeySymbol AdapterWidget::mapKeyToOSG(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_Shift:
        return osgGA::GUIEventAdapter::KEY_Shift_L;
    case Qt::Key_Control:
        return osgGA::GUIEventAdapter::KEY_Control_L;
    case Qt::Key_Escape:
        return osgGA::GUIEventAdapter::KEY_Escape;
    case Qt::Key_F8:
        return osgGA::GUIEventAdapter::KEY_F8;
    default:
        return (osgGA::GUIEventAdapter::KeySymbol) *(event->text().toLatin1().data());
    }
}

void AdapterWidget::keyPressEvent( QKeyEvent* event )
{
    _gw->getEventQueue()->keyPress(mapKeyToOSG(event));
    updateGL();
}

void AdapterWidget::keyReleaseEvent( QKeyEvent* event )
{
    _gw->getEventQueue()->keyRelease(mapKeyToOSG(event));
    updateGL();
}

void AdapterWidget::mousePressEvent( QMouseEvent* event )
{
	int button = 0;
	switch(event->button())
	{
    case(Qt::LeftButton):
        button = 1;
        break;
    case(Qt::MidButton):
        button = 2;
        break;
    case(Qt::RightButton):
        button = 3;
        break;
    case(Qt::NoButton):
        button = 0;
        break;
    default:
        button = 0;
        break;
	}

#ifndef RETINA_DISPLAY
    _gw->getEventQueue()->mouseButtonPress(event->x(), event->y(), button);
#else
    _gw->getEventQueue()->mouseButtonPress(event->x()*2, event->y()*2, button);
#endif

    updateGL();
}

void AdapterWidget::mouseReleaseEvent( QMouseEvent* event )
{
	int button = 0;
	switch(event->button())
	{
    case(Qt::LeftButton): button = 1; break;
	case(Qt::MidButton): button = 2; break;
	case(Qt::RightButton): button = 3; break;
	case(Qt::NoButton): button = 0; break;
	default: button = 0; break;
	}

#ifndef RETINA_DISPLAY
    _gw->getEventQueue()->mouseButtonRelease(event->x(), event->y(), button);
#else
    _gw->getEventQueue()->mouseButtonRelease(event->x()*2, event->y()*2, button);
#endif

    updateGL();
}

void AdapterWidget::mouseMoveEvent( QMouseEvent* event )
{
#ifndef RETINA_DISPLAY
    _gw->getEventQueue()->mouseMotion(event->x(), event->y());
#else
    _gw->getEventQueue()->mouseMotion(event->x()*2, event->y()*2);
#endif

    updateGL();
}

void AdapterWidget::wheelEvent(QWheelEvent *event)
{
    float delta = event->delta() / 8.0;
    if( delta==0 )
        return;
    
    osgGA::GUIEventAdapter* ea = _gw->getEventQueue()->createEvent();
    ea->setTime( _gw->getEventQueue()->getTime() );
    ea->setX( event->x() );
    ea->setY( event->y() );
        
    ea->setEventType( osgGA::GUIEventAdapter::SCROLL );
    if( event->orientation() == Qt::Vertical ) {
        if( delta>0 ){
            ea->setScrollingMotionDelta(0.0, delta);
            ea->setScrollingMotion( osgGA::GUIEventAdapter::SCROLL_UP );
            _gw->getEventQueue()->mouseScroll(osgGA::GUIEventAdapter::SCROLL_UP);
        }
        else{
            ea->setScrollingMotionDelta(0.0, -delta);
            ea->setScrollingMotion( osgGA::GUIEventAdapter::SCROLL_DOWN );
            _gw->getEventQueue()->mouseScroll(osgGA::GUIEventAdapter::SCROLL_DOWN);
        }
    } else {
        if( delta>0 ){
            ea->setScrollingMotionDelta(delta, 0.0);
            ea->setScrollingMotion( osgGA::GUIEventAdapter::SCROLL_LEFT );
            _gw->getEventQueue()->mouseScroll(osgGA::GUIEventAdapter::SCROLL_LEFT);
        }
        else{
            ea->setScrollingMotionDelta(-delta, 0.0);
            ea->setScrollingMotion( osgGA::GUIEventAdapter::SCROLL_RIGHT );
            _gw->getEventQueue()->mouseScroll(osgGA::GUIEventAdapter::SCROLL_RIGHT);
        }
    }
    
    updateGL();
}

