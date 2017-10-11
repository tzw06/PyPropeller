#include "modelview.h"
#include "manipulator.h"
#include "messenger.h"
#include <math.h>

#include <osg/LightModel>
#include <osgFX/Scribe>

ModelView::ModelView(QWidget *parent, const char *name, const QGLWidget *shareWidget)
    : AdapterWidget( parent, name, shareWidget )
{
    setThreadingModel(osgViewer::Viewer::CullDrawThreadPerContext);
    setRunFrameScheme(ON_DEMAND);

    option = new ViewerOption;
    option->cameraProjectionPerspective = true;
    
    messenger = new ViewerMessenger(this);

    initViewer();
}

ModelView::~ModelView()
{
    delete messenger;
    delete option;
}

void ModelView::initViewer()
{
    group = new osg::Group;
    group->setDataVariance(osg::Object::DYNAMIC);

    modelGroup = new osg::Group;
    group->addChild(modelGroup);

    osg::ref_ptr<osg::Camera> camera = getCamera();
    camera->setViewport(new osg::Viewport(0,0,width(),height()));
    camera->setProjectionMatrixAsPerspective(30.0f, static_cast<double>(width())/static_cast<double>(height()), 1.0f, 10000.0f);
    camera->setGraphicsContext(getGraphicsWindow());
    camera->setClearColor(osg::Vec4(0.5, 0.5, 1.0, 0.0));
    camera->setClearMask(GL_DEPTH_BUFFER_BIT);

    createBackgroundCamera();
    createAxesCamera();

    setSceneData(group);

    manipulator = new Manipulator(this, axesCamera.get());
    manipulator->setMessenger(messenger);
    manipulator->setAutoComputeHomePosition(false);
    manipulator->setAllowThrow(false);
    setCameraManipulator(manipulator);
      
    setKeyEventSetsDone(0);

    osg::ref_ptr<osg::LightModel> lightModel = new osg::LightModel;
    lightModel->setTwoSided(true);
    group->getOrCreateStateSet()->setAttributeAndModes(lightModel.get());

    firstRun = true;
}

void ModelView::setResourcePath(QString path)
{
    ResourcePath = path;

	QString filename = QString("%1/font.ttf").arg(ResourcePath);
	filename = QString::fromLocal8Bit(filename.toLocal8Bit());

    defaultFont = osgText::readFontFile(filename.toStdString());
}

void ModelView::updateScene()
{
    updateGL();
}

void ModelView::updateOption()
{
    //  Background Settings
    //===============================================================

    osg::ref_ptr<osg::Camera> camera = getCamera();
    bgCamera->setNodeMask(1);
    camera->setClearMask(GL_DEPTH_BUFFER_BIT);

    QColor c0(option->bgColorName0), c1(option->bgColorName1);
    float p = option->bgGradFactor;

    osg::ref_ptr<osg::Vec4Array> color_vertices = new osg::Vec4Array;

    for(int m=0;m<10;m++)
    {
        for(int n=0;n<10;n++)
        {
            float y0 = n / 10.0;
            float y1 = (n+1) / 10.0;

            float fp0, fp1;

            if ( (fabs(p)<1E-3) || (fabs(1-p)<1E-3) )
            {
                fp0 = y0;
                fp1 = y1;
            }
            else
            {
                float a = (p-0.5)/(p*p-p);
                float b = (p*p-2*p+0.5)/(p*p-p);

                fp0 = a*y0*y0 + b*y0;
                fp1 = a*y1*y1 + b*y1;
            }

            float r0 = c0.redF() * (1-fp0) + c1.redF() * fp0;
            float g0 = c0.greenF() * (1-fp0) + c1.greenF() * fp0;
            float b0 = c0.blueF() * (1-fp0) + c1.blueF() * fp0;

            float r1 = c0.redF() * (1-fp1) + c1.redF() * fp1;
            float g1 = c0.greenF() * (1-fp1) + c1.greenF() * fp1;
            float b1 = c0.blueF() * (1-fp1) + c1.blueF() * fp1;

            color_vertices->push_back(osg::Vec4(r0,g0,b0,1.0f));
            color_vertices->push_back(osg::Vec4(r0,g0,b0,1.0f));
            color_vertices->push_back(osg::Vec4(r1,g1,b1,1.0f));
            color_vertices->push_back(osg::Vec4(r1,g1,b1,1.0f));
        }
    }

    bgGeometry->setColorArray(color_vertices.get());
    bgGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    osgUtil::SmoothingVisitor::smooth(*bgGeometry);

    //  Camera Settings
    //===============================================================

    if (option->cameraProjectionPerspective)
        camera->setProjectionMatrixAsPerspective(30.0f, static_cast<double>(width())/static_cast<double>(height()), 1.0f, 10000.0f);
    else
        camera->setProjectionMatrixAsOrtho(-100,100,-100.0*height()/width(),100.0*height()/width(), 1.0f, 10000.0f);
}

void ModelView::setViewDirection(int direction)
{
    osg::BoundingSphere boundingSphere = group->getBound();
    
    osg::Vec3d eye, up;
    osg::Vec3d center = boundingSphere.center();
    double dist = 3.5f * boundingSphere.radius();
    
    switch (direction) {
        case BACK:
            eye = center + osg::X_AXIS * dist;
            up = osg::Y_AXIS;
            break;
        case FRONT:
            eye = center - osg::X_AXIS * dist;
            up = osg::Y_AXIS;
            break;
        case TOP:
            eye = center + osg::Y_AXIS * dist;
            up = osg::Z_AXIS * (-1);
            break;
        case BOTTOM:
            eye = center - osg::Y_AXIS * dist;
            up = osg::Z_AXIS;
            break;
        case LEFT:
            eye = center + osg::Z_AXIS * dist;
            up = osg::Y_AXIS;
            break;
        case RIGHT:
            eye = center - osg::Z_AXIS * dist;
            up = osg::Y_AXIS;
            break;
        case ISO:
            eye = center + (osg::X_AXIS + osg::Y_AXIS + osg::Z_AXIS) * dist / sqrt(3);
            up = osg::Y_AXIS;
            break;
        default:
        {
            double alpha = osg::DegreesToRadians(15.0);
            double beta = osg::DegreesToRadians(30.0);
            eye = center - osg::X_AXIS * dist * cos(alpha) * cos(beta) + osg::Y_AXIS * dist * sin(alpha) + osg::Z_AXIS * dist * cos(alpha) * sin(beta);
            up = - osg::X_AXIS * cos(beta) / cos(alpha) +  osg::Y_AXIS + osg::Z_AXIS * sin(beta) / cos(alpha);
            break;
        }
    }
    
    setCameraDirection(eye, center, up);
}

void ModelView::setCameraDirection(osg::Vec3d &eye, osg::Vec3d &center, osg::Vec3d &up)
{   
    manipulator->setHomePosition(eye, center, up);
    manipulator->home(0);

    eye = eye - center;
    eye.normalize();
    eye = eye * 10;
    axesCamera->setViewMatrixAsLookAt(eye, osg::Vec3d(0,0,0), up);
    
    updateGL();
}


void ModelView::capture(QString filename)
{
    updateGL();

    osg::ref_ptr<osg::Image> image = new osg::Image;

    int w = width();
    int h = height();

#ifdef RETINA_DISPLAY
    w = w * 2;
    h = h * 2;
#endif

    image->allocateImage(w,h,1, GL_RGB, GL_UNSIGNED_BYTE);
    image->readPixels(0,0,w,h, GL_RGB, GL_UNSIGNED_BYTE);

	filename = QString::fromLocal8Bit(filename.toLocal8Bit());
	osgDB::writeImageFile(*image.get(), filename.toStdString());
}

void ModelView::open(QString filename, int type)
{
    switch(type)
    {
    case STL:
        loadFromSTL(filename);
        break;
    case PLOT3D:
        loadFromPlot3D(filename);
        break;
    default:
        return;
    }

    if (firstRun) {
        setViewDirection(ISO);
        manipulator->home(0);
        firstRun = false;
    }

    updateGL();
}

void ModelView::write(QString filename)
{
    if (!osgDB::writeNodeFile(*group.get(), QString::fromLocal8Bit(filename.toLocal8Bit()).toStdString()))
		QMessageBox::information(this, "Error", QString("failed to write %1").arg(filename));
}

void ModelView::paintGL()
{
    frame();
}

void ModelView::resizeGL(int w, int h)
{
    AdapterWidget::resizeGL(w,h);

    bgCamera->setProjectionMatrixAsOrtho2D(0,w,0,h);
    bgCamera->setViewport(0,0,w,h);

    osg::ref_ptr<osg::Vec3Array> vertex = new osg::Vec3Array;

    for(int m=0;m<10;m++)
    {
        float x0 = w * m / 10.0;
        float x1 = w * (m+1) / 10.0;

        for(int n=0;n<10;n++)
        {
            float y0 = h * n / 10.0;
            float y1 = h * (n+1) / 10.0;

            vertex->push_back(osg::Vec3(x0,y0,0));
            vertex->push_back(osg::Vec3(x1,y0,0));
            vertex->push_back(osg::Vec3(x1,y1,0));
            vertex->push_back(osg::Vec3(x0,y1,0));
        }
    }

    bgGeometry->setVertexArray(vertex);

    //==========================================================================
    
#ifdef RETINA_DISPLAY
    int a = 500;
#else
    int a = 250;
#endif
    int border = 0;
    axesCamera->setViewport(w-a-border,h-a-border,a,a);
}

void ModelView::createBackgroundCamera()
{
    //  Background Camera
    bgCamera = new osg::Camera;
    bgCamera->setProjectionMatrixAsOrtho2D(0, width(), 0, height());
    bgCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    bgCamera->setRenderOrder(osg::Camera::PRE_RENDER);
    bgCamera->setViewport(0,0,width(),height());
    bgCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    bgCamera->setAllowEventFocus(false);
    bgCamera->setViewMatrix(osg::Matrix::identity());

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->getOrCreateStateSet()->setMode(GL_LIGHTING,osg::StateAttribute::OFF);

    bgGeometry = new osg::Geometry;

    osg::ref_ptr<osg::Vec3Array> vertex = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec2Array> coord  = new osg::Vec2Array;
    for(int m=0;m<10;m++)
    {
        float x0 = width() * m / 10.0;
        float x1 = width() * (m+1) / 10.0;

        for(int n=0;n<10;n++)
        {
            float y0 = height() * n / 10.0;
            float y1 = height() * (n+1) / 10.0;

            // 压入顶点
            vertex->push_back(osg::Vec3(x0,y0,0));
            vertex->push_back(osg::Vec3(x1,y0,0));
            vertex->push_back(osg::Vec3(x1,y1,0));
            vertex->push_back(osg::Vec3(x0,y1,0));

            // 纹理坐标
            coord->push_back(osg::Vec2(m/10.0,n/10.0));
            coord->push_back(osg::Vec2((m+1)/10.0,n/10.0));
            coord->push_back(osg::Vec2((m+1)/10.0,(n+1)/10.0));
            coord->push_back(osg::Vec2(m/10.0,(n+1)/10.0));
        }
    }

    bgGeometry->setVertexArray(vertex);
    bgGeometry->setTexCoordArray(0, coord);
    bgGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,vertex->size()));

    geode->addDrawable(bgGeometry);
    bgCamera->addChild(geode);

    group->addChild(bgCamera);
}

void ModelView::createAxesCamera()
{
    int radius = 4;
    
    axesCamera = new osg::Camera;
    axesCamera->setRenderOrder(osg::Camera::POST_RENDER);
    axesCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF_INHERIT_VIEWPOINT);
    axesCamera->setViewMatrix(osg::Matrix::identity());
    axesCamera->setProjectionMatrixAsOrtho(-radius, radius, -radius, radius, 4, 4+2*radius);
    
    axesCamera->setDataVariance(osg::Geometry::DYNAMIC);
    axesCamera->setClearColor(osg::Vec4(0.5, 0.5, 1.0, 0.0));
    axesCamera->setClearMask(GL_DEPTH_BUFFER_BIT);

    osg::StateSet * axesStateSet = axesCamera->getOrCreateStateSet();
    axesStateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);
    axesStateSet->setMode(GL_DEPTH_TEST,osg::StateAttribute::ON);
    axesStateSet->setMode(GL_BLEND,osg::StateAttribute::ON);
    axesStateSet->setRenderBinDetails( 99999, "RenderBin" );

    osg::ref_ptr<osg::TessellationHints> hints = new osg::TessellationHints;

    QList<osg::Vec4> colors;

    float c = 0.335;
    float a = 0.53;
    osg::ref_ptr<osg::ShapeDrawable> box = new osg::ShapeDrawable(new osg::Box(osg::Vec3(c,c,c), a,a,a));
    box->setColor(osg::Vec4(0.25f,0.25f,0.25f, 0.5f));
    box->setName("ISO");
    colors << osg::Vec4(0.25f,0.25f,0.25f, 0.5f);

    axesCamera->addChild(box);

    
    QStringList titles;
    titles << "X+" << "Y+" << "Z+";
    titles << "X-" << "Y-" << "Z-";

    colors << osg::Vec4(1.0f,0.0f,0.0f,1.0f);
    colors << osg::Vec4(0.0f,1.0f,0.0f,1.0f);
    colors << osg::Vec4(0.0f,0.0f,1.0f,1.0f);

    colors << osg::Vec4(1.0f,1.0f,1.0f,0.0f);
    colors << osg::Vec4(1.0f,1.0f,1.0f,0.0f);
    colors << osg::Vec4(1.0f,1.0f,1.0f,0.0f);

    for (int iax=0; iax<6; iax++)
    {
        float cyl_s = 0.0;
        float cyl_e = 1.0;
        
        if (iax>2) {
            cyl_s = 0.35;
            cyl_e = 1.0;
        }
        
        osg::ref_ptr<osg::ShapeDrawable> cylinder = new osg::ShapeDrawable(new osg::Cylinder(osg::Vec3(0,0,0.5*(cyl_s+cyl_e)),0.09,(cyl_e-cyl_s)), hints);
        osg::ref_ptr<osg::ShapeDrawable> arrow = new osg::ShapeDrawable(new osg::Cone(osg::Vec3(0,0,1),0.2,0.5), hints);
        
        cylinder->setColor(colors.at(iax));
        arrow->setColor(colors.at(iax));
        
        osg::ref_ptr<osg::MatrixTransform> tsfm = new osg::MatrixTransform;
        tsfm->addChild(cylinder);
        tsfm->addChild(arrow);
        
        if (iax==0) {
            tsfm->setMatrix(osg::Matrix::rotate(osg::DegreesToRadians(90.0), osg::Y_AXIS));
        }
        else if (iax==1) {
            tsfm->setMatrix(osg::Matrix::rotate(osg::DegreesToRadians(-90.0), osg::X_AXIS));
        }
        else if (iax==3) {
            tsfm->setMatrix(osg::Matrix::rotate(osg::DegreesToRadians(-90.0), osg::Y_AXIS));
        }
        else if (iax==4) {
            tsfm->setMatrix(osg::Matrix::rotate(osg::DegreesToRadians(90.0), osg::X_AXIS));
        }
        else if (iax==5) {
            tsfm->setMatrix(osg::Matrix::rotate(osg::DegreesToRadians(180.0), osg::X_AXIS));
        }
        
        tsfm->setName(titles.at(iax).toStdString());
        axesCamera->addChild(tsfm);
    }
    
    osg::ref_ptr<osg::ShapeDrawable> origin = new osg::ShapeDrawable(new osg::Sphere(osg::Vec3d(0,0,0),0.15), hints);
    origin->setColor(osg::Vec4(0.5f,0.5f,0.5f,1.0f));
    colors << osg::Vec4(0.5f,0.5f,0.5f,1.0f);
    
    axesCamera->addChild(origin);
    
    int N = axesCamera->getNumChildren();
    for (int i=0; i<N; i++)
    {
        osg::Node *pNode = axesCamera->getChild(i);
        osg::Vec4 color = colors.at(i);

        osg::ref_ptr<osg::StateSet> stateset = pNode->getOrCreateStateSet();
        osg::ref_ptr<osg::Material> material = new osg::Material();

        material->setAmbient(osg::Material::FRONT_AND_BACK,   osg::Vec4(0.1,0.1,0.1,1));
        material->setDiffuse(osg::Material::FRONT_AND_BACK,   color);
        material->setSpecular(osg::Material::FRONT_AND_BACK,  color);
        material->setEmission(osg::Material::FRONT_AND_BACK,  osg::Vec4(0.2,0.2,0.2,0.2));
        material->setShininess(osg::Material::FRONT_AND_BACK, 3);

        stateset->setAttribute(material.get(),osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
    }
    
    group->addChild(axesCamera);
}

void ModelView::loadFromSTL(QString filename)
{
    modelGroup->removeChildren(0, modelGroup->getNumChildren());
    osg::Node *node = osgDB::readNodeFile(filename.toStdString());
    if (node)
        modelGroup->addChild(node);
}

void ModelView::loadFromPlot3D(QString filename)
{
    QFile file(QString::fromLocal8Bit(filename.toLocal8Bit()));
    if (!file.open(QFile::ReadOnly|QFile::Text)) {
        QMessageBox::warning(this, tr("Loading from file"),
                             QString(tr("Cannot read file %1:\n%2")).arg(filename).arg(file.errorString()));
        return;
    }

    modelGroup->removeChildren(0, modelGroup->getNumChildren());

    QTextStream in(&file);

    QString line = in.readLine();
    int N = line.toInt();

    double *IMAX = new double[N];
    double *JMAX = new double[N];

    for(int n=0;n<N;n++) {
        QStringList strs = in.readLine().split(',');
        IMAX[n] = strs.at(0).toInt();
        JMAX[n] = strs.at(1).toInt();
    }

    for(int n=0;n<N;n++)
    {
        int I = IMAX[n];
        int J = JMAX[n];

        double *X = new double[I*J];
        double *Y = new double[I*J];
        double *Z = new double[I*J];
        QList<double*> ptrs;

        ptrs << X << Y << Z;
        for(int idim=0;idim<3;idim++)
        {
            double *ptr = ptrs.at(idim);
            QStringList strs = in.readLine().split(',');
            for(int k=0;k<strs.size();k++)
                ptr[k] = strs.at(k).toDouble();
        }

        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        osg::ref_ptr<osg::Geometry>  surfaceGeom = new osg::Geometry;
        osg::ref_ptr<osg::Vec3Array> vertices    = new osg::Vec3Array;

        geode->addDrawable(surfaceGeom.get());
        modelGroup->addChild(geode);

        for(int j=0;j<J-1;j++)
        {
            for(int i=0;i<I-1;i++)
            {
                int k00 = i + j*I;
                int k01 = i + (j+1)*I;
                int k11 = i+1 + (j+1)*I;
                int k10 = i+1 + j*I;

                if (n%2!=0)
                {
                    vertices->push_back(osg::Vec3(X[k00], Y[k00], Z[k00]));
                    vertices->push_back(osg::Vec3(X[k01], Y[k01], Z[k01]));
                    vertices->push_back(osg::Vec3(X[k11], Y[k11], Z[k11]));
                    vertices->push_back(osg::Vec3(X[k10], Y[k10], Z[k10]));
                }
                else
                {
                    vertices->push_back(osg::Vec3(X[k00], Y[k00], Z[k00]));
                    vertices->push_back(osg::Vec3(X[k10], Y[k10], Z[k10]));
                    vertices->push_back(osg::Vec3(X[k11], Y[k11], Z[k11]));
                    vertices->push_back(osg::Vec3(X[k01], Y[k01], Z[k01]));
                }

            }
        }

        delete[] X;
        delete[] Y;
        delete[] Z;

        //  Setting of openscenegraph surface

        surfaceGeom->setVertexArray(vertices.get());

        QColor color("#ffff00");
        osg::Vec4Array *colors = new osg::Vec4Array;
        colors->push_back(osg::Vec4(color.redF(), color.greenF(), color.blueF(), 0.5f));
        surfaceGeom->setColorArray(colors);
        surfaceGeom->setColorBinding(osg::Geometry::BIND_OVERALL);

        surfaceGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, vertices->size()));

        osg::ref_ptr<osg::StateSet> stateset = surfaceGeom->getOrCreateStateSet();
        stateset->setMode(GL_BLEND,osg::StateAttribute::OFF|osg::StateAttribute::OVERRIDE);


        osg::ref_ptr<osg::Material>material=new osg::Material();
        float f = 0.75;

        material->setAmbient(osg::Material::FRONT_AND_BACK,   osg::Vec4(0.1,0.1,0.1,1));
        material->setDiffuse(osg::Material::FRONT_AND_BACK,   osg::Vec4(f*color.redF(), f*color.greenF(), f*color.blueF(), f));
        material->setSpecular(osg::Material::FRONT_AND_BACK,  osg::Vec4(f*color.redF(), f*color.greenF(), f*color.blueF(), f));
        material->setEmission(osg::Material::FRONT_AND_BACK,  osg::Vec4(0.2,0.2,0.2,0.2));
        material->setShininess(osg::Material::FRONT_AND_BACK, 10);

        stateset->setAttribute(material.get(),osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);

        osgUtil::SmoothingVisitor::smooth(*surfaceGeom, osg::PI);
    }

    delete[] IMAX;
    delete[] JMAX;

    file.close();
}

//===============================================================================================

ViewerQT::ViewerQT(QWidget *parent, const char *name, const QGLWidget *shareWidget)
    : ModelView(parent,name,shareWidget)
{
    connect(messenger, SIGNAL(sigSetViewDirection(int)), this, SLOT(onSetViewDirection(int)));
}

void ViewerQT::onSetViewDirection(int idir)
{
    setViewDirection(idir);
}




