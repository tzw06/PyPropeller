#include "messenger.h"

ViewerMessenger::ViewerMessenger(QObject *parent)
    : QObject(parent)
{

}

void ViewerMessenger::showMessage(QString msg)
{
    emit sigShowMessage(msg);
}

void ViewerMessenger::setViewDirection(int idir)
{
    emit sigSetViewDirection(idir);
}
