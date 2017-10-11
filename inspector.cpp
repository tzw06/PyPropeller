//
//  inspector.cpp
//  PyAircraft
//
//  Created by 田中伟 on 2017/4/30.
//  Copyright © 2017年 田中伟. All rights reserved.
//

#include "inspector.h"

Inspector::Inspector(QWidget *parent)
    : QTreeView(parent)
{
    setWindowTitle("Inspector");
    setObjectName("Inspector");
    
    model = new QStandardItemModel(0,3,this);
    model->setHeaderData(0, Qt::Horizontal, "Item");
    model->setHeaderData(1, Qt::Horizontal, "Value");
    model->setHeaderData(2, Qt::Horizontal, "Unit");
    
    setModel(model);
    setAlternatingRowColors(true);
    setUniformRowHeights(false);
}

Inspector::~Inspector()
{
    
}

void Inspector::setContent(QDomElement &parent)
{
    model->removeRows(0, model->rowCount());    
    appendChild(parent, QModelIndex());
}

void Inspector::updateContent(QDomElement &parent)
{
    updateChild(parent, QModelIndex());
}

void Inspector::saveContent(QDomDocument &document, QDomElement &parent)
{
    writeChild(document, parent, QModelIndex());
}

void Inspector::appendChild(QDomElement &parent, QModelIndex parentIndex)
{
    if (parentIndex.isValid())
        model->insertColumns(0, 3, parentIndex);
    
    QDomElement element = parent.firstChildElement();
    while (!element.isNull())
    {
        int row = model->rowCount(parentIndex);
        model->insertRow(row, parentIndex);
        
        QModelIndex index0 = model->index(row,0,parentIndex);
        QModelIndex index1 = index0.sibling(row, 1);
        QModelIndex index2 = index0.sibling(row, 2);
        
        model->setData(index0, element.tagName());
        model->itemFromIndex(index0)->setEditable(false);
        
        if (element.hasChildNodes() && element.firstChild().isText())
            model->setData(index1, element.text());
        else
            setFirstColumnSpanned(row, parentIndex, true);
        
        if (element.hasAttribute("unit"))
            model->setData(index2, element.attribute("unit"));

        QStringList strs;
        QDomNamedNodeMap attrs = element.attributes();
        for(int i=0;i<attrs.size();i++)
        {
            QDomNode node = attrs.item(i);
            strs << QString("%1=%2").arg(node.nodeName()).arg(node.nodeValue());
        }
        
        model->setData(index0, strs.join("\n"), Qt::ToolTipRole);
        
        if (element.hasChildNodes())
            appendChild(element, index0);
        
        element = element.nextSiblingElement();
    }
}

void Inspector::updateChild(QDomElement &parent, QModelIndex parentIndex)
{
    int nrow = model->rowCount(parentIndex);
    for(int irow=0;irow<nrow;irow++)
    {
        QModelIndex index0 = model->index(irow,0,parentIndex);
        QModelIndex index1 = index0.sibling(irow, 1);
        
        QString tagName = model->data(index0).toString();
        
        QDomElement element = parent.firstChildElement(tagName);
        if (!element.isNull())
        {
            if ( element.hasChildNodes() && element.firstChild().isText())
                model->setData(index1, element.text());
            
            if (element.hasChildNodes())
                updateChild(element, index0);
        }
    }
}

void Inspector::writeChild(QDomDocument &document, QDomElement &parent, QModelIndex parentIndex)
{
    int nrow = model->rowCount(parentIndex);
    for(int irow=0;irow<nrow;irow++)
    {
        QModelIndex index0 = model->index(irow,0,parentIndex);
        QModelIndex index1 = index0.sibling(irow, 1);
        
        QString tagName = model->data(index0).toString();
        QString text    = model->data(index1).toString();
        
        QDomElement element = document.createElement(tagName);
        element.appendChild(document.createTextNode(text));
        
        QStringList attrs = model->data(index0, Qt::ToolTipRole).toString().split("\n");
        for(int i=0;i<attrs.size();i++)
        {
            QStringList strs = attrs.at(i).split("=");
            if (strs.size()>1)
                element.setAttribute(strs.first(), strs.last());
        }
        
        if (model->hasChildren(index0))
            writeChild(document, element, index0);
        
        parent.appendChild(element);
    }
}

void Inspector::setIOFlags(QStringList inputVarIDs, QStringList outputVarIDs, QStringList unusedVarIDs)
{
    for(int i=0;i<inputVarIDs.size();i++)
        setIOFlag(inputVarIDs.at(i), INPUT);
    for(int i=0;i<outputVarIDs.size();i++)
        setIOFlag(outputVarIDs.at(i), OUTPUT);
    for(int i=0;i<unusedVarIDs.size();i++)
        setIOFlag(unusedVarIDs.at(i), UNUSED);

    updateDisplay(QModelIndex());
}

void Inspector::setIOFlag(QString ID, int flag)
{
    QStringList titles = ID.split("/");
    QModelIndex parent;
    while (!titles.isEmpty())
    {
        QString title = titles.takeFirst();

        int nrow = model->rowCount(parent);
        for(int irow=0;irow<nrow;irow++) {
            QString text = model->data(model->index(irow,0,parent)).toString();
            if (title==text) {
                parent = model->index(irow,0,parent);
                break;
            }
        }
    }

    if (parent.isValid())
    {
        setRowHidden(parent.row(), parent.parent(), flag==UNUSED);

        QModelIndex index1 = parent.sibling(parent.row(), 1);
        QModelIndex index2 = parent.sibling(parent.row(), 2);

        switch(flag)
        {
        case INPUT:
            model->setData(index1, QColor(Qt::black), Qt::ForegroundRole);
            model->setData(index2, QColor(Qt::black), Qt::ForegroundRole);
            break;
        case OUTPUT:
            model->setData(index1, QColor(Qt::gray), Qt::ForegroundRole);
            model->setData(index2, QColor(Qt::gray), Qt::ForegroundRole);
            break;
        case UNUSED:
            break;
        }
    }
}

void Inspector::updateDisplay(QModelIndex parentIndex)
{
    if (model->hasChildren(parentIndex))
    {
        QList<bool> rowHiddens;
        int nrow = model->rowCount(parentIndex);
        for(int irow=0;irow<nrow;irow++) {
            rowHiddens << isRowHidden(irow,parentIndex);
            updateDisplay(model->index(irow,0,parentIndex));
        }

        if (!rowHiddens.contains(false))
            setRowHidden(parentIndex.row(), parentIndex.parent(), true);
    }
}

void Inspector::loadIOFlag(QString filename)
{
    QFile file(QString::fromLocal8Bit(filename.toLocal8Bit()));
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Save to file"),
                             QString(tr("Cannot save to %1:\n%2")).arg(filename).arg(file.errorString()));
        return;
    }

    QStringList inputVarIDs, outputVarIDs, unusedVarIDs;
    QTextStream in(&file);
    while(!in.atEnd())
    {
        QString line = in.readLine();

        QStringList strs = line.split(QRegExp("[,\ =]"), QString::SkipEmptyParts);
        if (strs.first()=="input")
            inputVarIDs = strs;
        else if (strs.first()=="output")
            outputVarIDs = strs;
        else if (strs.first()=="unused")
            unusedVarIDs = strs;
    }
    file.close();

    inputVarIDs.removeFirst();
    outputVarIDs.removeFirst();
    unusedVarIDs.removeFirst();

    setIOFlags(inputVarIDs, outputVarIDs, unusedVarIDs);
}

void Inspector::loadSettings(QDomDocument &document, QStringList methods)
{
    QMap<QString, QString> varIDs;
    QDomElement rootElement = document.documentElement();

    QDomElement dataElement = rootElement.firstChildElement("data");
    if (!dataElement.isNull())
    {
        QDomElement varElement = dataElement.firstChildElement("var");
        while(!varElement.isNull())
        {
            QString name = varElement.attribute("name");
            QString text = varElement.text();
            varIDs[name] = text;

            varElement = varElement.nextSiblingElement("var");
        }
    }

    QStringList inputVarIDs, outputVarIDs;

    QDomElement methodsElement = rootElement.firstChildElement("methods");
    if (!methodsElement.isNull())
    {
        QDomElement element = methodsElement.firstChildElement("method");
        while(!element.isNull())
        {
            QString title = element.attribute("title");
            if (methods.contains(title))
            {

                QDomElement inputElement = element.firstChildElement("input");
                if (!inputElement.isNull()) {
                    QStringList strs = inputElement.text().split(QRegExp("[,\ ]+"));
                    for(int i=0;i<strs.size();i++) {
                        QString name = strs.at(i);
                        QString id = varIDs.value(name);
                        if (varIDs.contains(name) && !inputVarIDs.contains(id))
                            inputVarIDs << id;
                    }
                }

                QDomElement outputElement = element.firstChildElement("output");
                if (!outputElement.isNull()) {
                    QStringList strs = outputElement.text().split(QRegExp("[,\ ]+"));
                    for(int i=0;i<strs.size();i++) {
                        QString name = strs.at(i);
                        QString id = varIDs.value(name);
                        if(varIDs.contains(name) && !outputVarIDs.contains(id))
                            outputVarIDs << id;
                    }
                }
            }

            element = element.nextSiblingElement("method");
        }
    }

    for(int i=0;i<inputVarIDs.size();i++) {
        QString id = inputVarIDs.at(i);
        if (outputVarIDs.contains(id))
            inputVarIDs.removeAll(id);
    }

    QStringList usedVarIDs = inputVarIDs + outputVarIDs;
    QStringList unusedVarIDs = varIDs.values();
    for(int i=0;i<usedVarIDs.size();i++) {
        QString id = usedVarIDs.at(i);
        unusedVarIDs.removeAll(id);
    }

    setIOFlags(inputVarIDs, outputVarIDs, unusedVarIDs);
}

