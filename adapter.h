#pragma once

#include <QtGui>
#include <QtWidgets>
#include <QGLWidget>

#include <osg/ArgumentParser>
#include <osgViewer/Viewer>
#include <osgViewer/CompositeViewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgGA/TrackballManipulator>
#include <osg/MatrixTransform>

#ifdef Q_OS_MAC
#define RETINA_DISPLAY
#endif

class AdapterWidget : public QGLWidget
{
public:
    AdapterWidget( QWidget * parent = 0, const char * name = 0, const QGLWidget * shareWidget = 0);

    virtual ~AdapterWidget() {_gw.release();}

	osgViewer::GraphicsWindow* getGraphicsWindow() { return _gw.get(); }
	const osgViewer::GraphicsWindow* getGraphicsWindow() const { return _gw.get(); }

protected:

	void init();

	virtual void resizeGL( int width, int height );
	virtual void keyPressEvent( QKeyEvent* event );
	virtual void keyReleaseEvent( QKeyEvent* event );
	virtual void mousePressEvent( QMouseEvent* event );
	virtual void mouseReleaseEvent( QMouseEvent* event );
	virtual void mouseMoveEvent( QMouseEvent* event );
    virtual void wheelEvent(QWheelEvent *event);

    osgGA::GUIEventAdapter::KeySymbol mapKeyToOSG(QKeyEvent *event);
    
	osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> _gw;

    QTimer _timer;
};

