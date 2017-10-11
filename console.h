#ifndef CONSOLE_H
#define CONSOLE_H

#include <QtWidgets>

class Console : public QDialog
{
    Q_OBJECT

public:
    Console(QWidget *parent=NULL);

    void append(QString msg);
    void clear();

private:
    QTextEdit *edit;
};

#endif // CONSOLE_H
