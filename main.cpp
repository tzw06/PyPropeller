//
//  main.cpp
//  PyAircraft
//
//  Created by 田中伟 on 2017/4/30.
//  Copyright © 2017年 田中伟. All rights reserved.
//

#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_DontShowIconsInMenus);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
    
    MainWindow win;
    win.show();
    
    return app.exec();
}
