#ifndef VIEWERMESSENGER_H
#define VIEWERMESSENGER_H

#include <QObject>

class ViewerMessenger : public QObject
{
    Q_OBJECT

public:
    ViewerMessenger(QObject *parent);

signals:
    void sigShowMessage(QString msg);
    void sigSetViewDirection(int idir);

public:
    void showMessage(QString msg);
    void setViewDirection(int idir);
};

#endif // VIEWERMESSENGER_H
