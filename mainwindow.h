//
//  mainwindow.hpp
//  PyAircraft
//
//  Created by 田中伟 on 2017/4/30.
//  Copyright © 2017年 田中伟. All rights reserved.
//

#ifndef mainwindow_hpp
#define mainwindow_hpp

#include <QtWidgets>

class Inspector;
class MethodDock;

class ViewerQT;
class Console;

struct RunOption
{
    QString pythonExecutable;
    QString temporaryPath, configPath;
    QString runTitle;
};


class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(QWidget *parent=NULL);
    ~MainWindow();
 
    void closeEvent(QCloseEvent *event);
    
private slots:
    void open();
    void save();
    void run();
    void onShowUGNX();
    void onShowInspector(bool is);
    void onShowMethodDock(bool is);
    void onShowConsole();
    void onReadOutput();
    void onReadError();
    void onRunFinished(int,QProcess::ExitStatus);
    void onRunError(QProcess::ProcessError error);

    void updateRunFlags();

private:
    void createView();
    void createActions();
    void createToolbar();
    
    void readSettings();
    void writeSettings();

    void loadFromFile(QString filename);
    void updateFromFile(QString filename);
    void saveToFile(QString filename);
    
    QAction *openAct, *saveAct, *ugnxAct;
    QAction *inspectorAct, *runAct, *methodAct, *consoleAct;
    QTextBrowser *indicator;

    ViewerQT *viewer;
    Console *console, *editor;
    Inspector *inspector;
    MethodDock *methodDock;

    QStackedWidget *propertyRegion;
    
    QProcess *proc;
    RunOption *option;
};


#endif /* mainwindow_hpp */
