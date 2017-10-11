#ifndef METHODVIEW_H
#define METHODVIEW_H

#include <QtWidgets>
#include <QtXml>

class MethodView : public QTreeView
{
    Q_OBJECT

public:
    MethodView(QWidget *parent=NULL);

    void setContent(QDomElement &parent);
    void saveContent(QDomDocument &document, QDomElement &parent);

    QStringList getMethods();

signals:
    void sigUpdateMethod();

private slots:
    void onClicked(QModelIndex index);

private:
    QStandardItemModel *model;
};

class MethodDock: public QWidget
{
    Q_OBJECT

public:
    MethodDock(QWidget *parent=NULL);

    void loadFromFile(QString filename);
    void saveToFile(QString filename);

    QStringList getMethods();

signals:
    void sigUpdateMethod();

private slots:
    void onUpdateMethod();

private:
    MethodView *view;
};

#endif // METHODVIEW_H
