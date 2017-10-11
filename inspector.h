//
//  inspector.hpp
//  PyAircraft
//
//  Created by 田中伟 on 2017/4/30.
//  Copyright © 2017年 田中伟. All rights reserved.
//

#ifndef inspector_hpp
#define inspector_hpp

#include <QtWidgets>
#include <QtXml>

class Inspector : public QTreeView
{
    Q_OBJECT
    
public:
    Inspector(QWidget *parent=NULL);
    ~Inspector();
    
    enum IOFlag {
        INPUT,
        OUTPUT,
        UNUSED,
    };

    void setContent(QDomElement &parent);
    void updateContent(QDomElement &parent);
    void saveContent(QDomDocument &document, QDomElement &parent);
    
    void setIOFlags(QStringList inputVarIDs, QStringList outputVarIDs, QStringList unusedVarIDs);
    void loadIOFlag(QString filename);
    void loadSettings(QDomDocument &document, QStringList methods);

private:
    void appendChild(QDomElement &parent, QModelIndex parentIndex);
    void updateChild(QDomElement &parent, QModelIndex parentIndex);
    void writeChild(QDomDocument &document, QDomElement &parent, QModelIndex parentIndex);

    void setIOFlag(QString ID, int flag);
    void updateDisplay(QModelIndex parentIndex);


    QStandardItemModel *model;
};

#endif /* inspector_hpp */
