//
//  mainwindow.cpp
//  PyAircraft
//
//  Created by 田中伟 on 2017/4/30.
//  Copyright © 2017年 田中伟. All rights reserved.
//

#include <QtXml>

#include "mainwindow.h"
#include "inspector.h"
#include "methodview.h"
#include "modelview.h"
#include "console.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    option = new RunOption;

#ifdef Q_OS_MAC
    QString projectPath = "/Users/tzw/SDK/PyPropeller";
    option->pythonExecutable = "/usr/bin/python";
#else
    QString projectPath = QCoreApplication::applicationDirPath() + "/../..";
    option->pythonExecutable = "E:\\SDK\\Anaconda2\\python.exe";
#endif

    option->configPath = projectPath + "/core";
    option->temporaryPath = projectPath + "/temp";

    createView();
    createActions();
    createToolbar();
    
    setUnifiedTitleAndToolBarOnMac(true);
    
    readSettings();
    
    proc = new QProcess(this);
    proc->setProcessChannelMode(QProcess::SeparateChannels);
    connect(proc, SIGNAL(readyReadStandardOutput()), this, SLOT(onReadOutput()));
    connect(proc, SIGNAL(readyReadStandardError()), this, SLOT(onReadError()));
    connect(proc, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(onRunFinished(int,QProcess::ExitStatus)));
    connect(proc, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(onRunError(QProcess::ProcessError)));

    loadFromFile(option->configPath+"/res/propeller.xml");
    methodDock->loadFromFile(option->configPath+"/res/method.xml");

    updateRunFlags();

    connect(methodDock, SIGNAL(sigUpdateMethod()), this, SLOT(updateRunFlags()));
}

MainWindow::~MainWindow()
{
    delete option;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    writeSettings();
}

void MainWindow::open()
{
    QString filename = QFileDialog::getOpenFileName(this, "open project file", QDir::currentPath(), "XML files (*.xml)");
    if (filename=="")
        return;
    
    QString suffix = ".xml";
    if (!filename.endsWith(suffix))
        filename = filename + suffix;
    
//    loadFromFile(filename);
    updateFromFile(filename);
    
    QDir::setCurrent(QFileInfo(filename).absolutePath());

    run();
}

void MainWindow::save()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Save as..."), QDir::currentPath(), "XML files (*.xml)");
    if (filename.isNull())
        return;

    QString suffix = ".xml";
    if (!filename.endsWith(suffix))
        filename = filename + suffix;
    
    saveToFile(filename);
    
    QDir::setCurrent(QFileInfo(filename).absolutePath());
}

void MainWindow::run()
{
    option->runTitle = QString("%1-%2-%3").arg("run").arg(QDate::currentDate().toJulianDay()).arg(QTime::currentTime().toString("hhmmss"));
    QString runPath = option->temporaryPath+"/"+option->runTitle;
    QDir runDir(runPath);
    if (!runDir.exists()) {
        QDir tempDir(option->temporaryPath);
        if (!tempDir.mkdir(option->runTitle))
            return;
    }

    QString inputFileName = runPath + "/input.xml";
    QString outputFileName = runPath + "/output.xml";
    QString methodFileName = runPath + "/method.xml";

    saveToFile(inputFileName);
    methodDock->saveToFile(methodFileName);

    QStringList arguments;
    arguments << QString("%1/main.py").arg(option->configPath) << "-input" << inputFileName << "-output" << outputFileName << "-method" << methodFileName;

    proc->setWorkingDirectory(runPath);
    proc->start(option->pythonExecutable, arguments);

    runAct->setEnabled(false);
    indicator->append("Calculating...");
    console->clear();
    console->hide();
}

void MainWindow::onShowInspector(bool is)
{
    if (is) {
        methodAct->setChecked(false);
        propertyRegion->setCurrentWidget(inspector);
        propertyRegion->setVisible(true);
    }
    else
        propertyRegion->setVisible(false);
}

void MainWindow::onShowMethodDock(bool is)
{
    if (is) {
        inspectorAct->setChecked(false);
        propertyRegion->setCurrentWidget(methodDock);
        propertyRegion->setVisible(true);
    }
    else
        propertyRegion->setVisible(false);
}

void MainWindow::onShowConsole()
{
    console->show();
}

void MainWindow::onShowUGNX()
{
    QString filename = option->temporaryPath+"/"+option->runTitle + "/blade.grs";
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly|QIODevice::Text)) {
        return;
    }

    editor->clear();
    editor->append(file.readAll());
    editor->show();

    file.close();
}

void MainWindow::onReadOutput()
{
    QByteArray ba = proc->readAllStandardOutput();
    console->append(ba);
    indicator->clear();
    indicator->append(ba);
}

void MainWindow::onReadError()
{
    QByteArray ba = proc->readAllStandardError();
    console->append(ba);
    console->show();
}

void MainWindow::onRunFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString runPath = option->temporaryPath + "/" + option->runTitle;
    if (exitStatus==QProcess::NormalExit)
    {
        if (exitCode==0) {
            updateFromFile(runPath + "/output.xml");
            viewer->open(runPath + "/blade.xyz", ModelView::PLOT3D);
            ugnxAct->setEnabled(true);
        }
        else {
            QString text = QString("<b>Error</b><p>Program exit with code = %1</p>").arg(exitCode);
            console->append(text);
            console->show();
            ugnxAct->setEnabled(false);
        }
    }
    else {
        QString msg = QString("Program crashed with code = %1").arg(exitCode);
        QMessageBox::information(this, "Error", msg);
        ugnxAct->setEnabled(false);
    }

    runAct->setEnabled(true);
    indicator->clear();
}

void MainWindow::onRunError(QProcess::ProcessError error)
{
    QString msg;
    switch(error)
    {
    case QProcess::FailedToStart:
        msg = "Failed to start";
        break;
    case QProcess::Crashed:
        msg = "Crashed";
        break;
    case QProcess::Timedout:
        msg = "Time out";
        break;
    case QProcess::ReadError:
        msg = "Read error";
        break;
    case QProcess::WriteError:
        msg = "Write error";
        break;
    case QProcess::UnknownError:
        msg = "Unknown error";
        break;
    }

    QMessageBox::information(this, "Error", msg);
}


void MainWindow::createView()
{
    viewer = new ViewerQT(this);
    ViewerOption *options = viewer->getOption();
    options->bgColorName1 = "#99ccff";
    options->bgColorName0 = "#ffffff";
    options->bgGradFactor = 0.7;
    options->cameraProjectionPerspective = true;
    viewer->updateOption();

    console = new Console(this);
    console->setWindowTitle("Information");
    console->hide();
    
    editor = new Console(this);
    editor->setWindowTitle("Text Editor");
    editor->hide();

    inspector = new Inspector(this);
    methodDock = new MethodDock(this);

    propertyRegion = new QStackedWidget(this);
    propertyRegion->addWidget(inspector);
    propertyRegion->addWidget(methodDock);
    
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(0);
    splitter->addWidget(viewer);
    splitter->addWidget(propertyRegion);
    splitter->setStretchFactor(0, 1);
    
    setCentralWidget(splitter);
}

void MainWindow::createActions()
{
    openAct = new QAction(QIcon(":images/open.png"), "Open", this);
    openAct->setShortcuts(QKeySequence::Open);
    connect(openAct, SIGNAL(triggered(bool)), this, SLOT(open()));

    saveAct = new QAction(QIcon(":images/save.png"), "Save", this);
    saveAct->setShortcuts(QKeySequence::Save);
    connect(saveAct, SIGNAL(triggered(bool)), this, SLOT(save()));

    ugnxAct = new QAction(QIcon(":images/ugnx.png"), "Siemens NX", this);
    ugnxAct->setEnabled(false);
    connect(ugnxAct, SIGNAL(triggered(bool)), this, SLOT(onShowUGNX()));
    
    runAct = new QAction(QIcon(":images/play.png"), "Run", this);
    runAct->setShortcut(QKeySequence(Qt::Key_F5));
    connect(runAct, SIGNAL(triggered(bool)), this, SLOT(run()));

    methodAct = new QAction(QIcon(":images/methods.png"), "Methods", this);
    methodAct->setCheckable(true);
    methodAct->setChecked(false);
    connect(methodAct, SIGNAL(triggered(bool)), this, SLOT(onShowMethodDock(bool)));

    inspectorAct = new QAction(QIcon(":images/property.png"), "Inspector", this);
    inspectorAct->setCheckable(true);
    inspectorAct->setChecked(true);
    connect(inspectorAct, SIGNAL(triggered(bool)), this, SLOT(onShowInspector(bool)));

    consoleAct = new QAction(QIcon(":images/console.png"), "Console", this);
    connect(consoleAct, SIGNAL(triggered(bool)), this, SLOT(onShowConsole()));
}

void MainWindow::createToolbar()
{
    QToolBar *bar = addToolBar("toolBar");
    bar->setObjectName("toolBar");
    bar->setIconSize(QSize(24,24));
    bar->setMovable(false);
    bar->setFloatable(false);
    bar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    
    indicator = new QTextBrowser(this);
    indicator->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    indicator->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    indicator->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);

    QWidget *spacer = new QWidget(this);
    spacer->setFixedWidth(20);

    QWidget *spacer1 = new QWidget(this);
    spacer1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QWidget *spacer2 = new QWidget(this);
    spacer2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QWidget *spacer3 = new QWidget(this);
    spacer3->setFixedWidth(20);


    bar->addAction(openAct);
    bar->addAction(saveAct);
    bar->addWidget(spacer);
    bar->addAction(ugnxAct);
    bar->addWidget(spacer1);
    bar->addAction(runAct);
    bar->addWidget(indicator);
    bar->addWidget(spacer2);
    bar->addAction(consoleAct);
    bar->addWidget(spacer3);
    bar->addAction(methodAct);
    bar->addAction(inspectorAct);
}

void MainWindow::readSettings()
{
    QSettings settings("SIEMENS", "PyPropeller");
    
    QDir::setCurrent(settings.value("current-path", "").toString());
    
    //  Window Geometry
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("state").toByteArray());
    
    move(settings.value("pos", QPoint(200,200)).toPoint());
    resize(settings.value("size", QSize(800,600)).toSize());
    
    QString winstate = settings.value("window-state", "normal").toString();
    if (winstate=="maximized")
        showMaximized();
    else
        showNormal();
}

void MainWindow::writeSettings()
{
    QSettings settings("SIEMENS", "PyPropeller");
    
    //  Window Geometry
    if ( (!isMaximized()) && (!isMinimized()) ) {
        settings.setValue("pos", pos());
        settings.setValue("size", size());
    }
    if (isMaximized())
        settings.setValue("window-state", "maximized");
    else
        settings.setValue("window-state", "normal");
    settings.setValue("state", saveState());
    
    settings.setValue("current-path", QDir::currentPath());
}

void MainWindow::updateRunFlags()
{
    QString filename = option->configPath + "/res/setting.xml";
    QFile file(QString::fromLocal8Bit(filename.toLocal8Bit()));
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Loading from file"),
                             QString(tr("Cannot read file %1:\n%2")).arg(filename).arg(file.errorString()));
        return;
    }

    QDomDocument doc;
    doc.setContent(&file);

    inspector->loadSettings(doc, methodDock->getMethods());
}

void MainWindow::loadFromFile(QString filename)
{
    QFile file(QString::fromLocal8Bit(filename.toLocal8Bit()));
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Loading from file"),
                             QString(tr("Cannot read file %1:\n%2")).arg(filename).arg(file.errorString()));
        return;
    }
    
    QDomDocument doc;
    QDomElement rootElement;
    
    doc.setContent(&file);
    rootElement = doc.documentElement();
    
    inspector->setContent(rootElement);
    
    file.close();
}

void MainWindow::updateFromFile(QString filename)
{
    QFile file(QString::fromLocal8Bit(filename.toLocal8Bit()));
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Update from file"),
                             QString(tr("Cannot read file %1:\n%2")).arg(filename).arg(file.errorString()));
        return;
    }
    
    QDomDocument doc;
    QDomElement rootElement;
    
    doc.setContent(&file);
    rootElement = doc.documentElement();
    
    inspector->updateContent(rootElement);
    
    file.close();
}

void MainWindow::saveToFile(QString filename)
{
    QFile file(QString::fromLocal8Bit(filename.toLocal8Bit()));
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Save to file"),
                             QString(tr("Cannot save to %1:\n%2")).arg(filename).arg(file.errorString()));
        return;
    }
    
    QTextStream out(&file);
    
    QDomDocument doc;
    QDomElement element = doc.createElement("aircraft");
    doc.appendChild(element);
    
    inspector->saveContent(doc, element);
    
    doc.save(out,4);
    
    file.close();
}
