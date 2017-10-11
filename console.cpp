#include "console.h"

Console::Console(QWidget *parent)
    : QDialog(parent)
{
    edit = new QTextEdit(this);
    edit->setFontFamily("Consolas");

    QGridLayout *layout = new QGridLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(edit, 0,0,1,1);

    setLayout(layout);
}

void Console::append(QString msg)
{
    edit->append(msg);
}

void Console::clear()
{
    edit->clear();
}
