#include "methodview.h"

MethodView::MethodView(QWidget *parent)
    : QTreeView(parent)
{
    model = new QStandardItemModel(0,1,this);
    model->setHeaderData(0, Qt::Horizontal, "Item");

    setModel(model);
    header()->setVisible(false);

    connect(this, SIGNAL(clicked(QModelIndex)), this, SLOT(onClicked(QModelIndex)));
}

void MethodView::setContent(QDomElement &parent)
{
    model->removeRows(0, model->rowCount());

    QDomElement element = parent.firstChildElement("item");
    while(!element.isNull())
    {
        QString text = element.text();
        int nrow = model->rowCount();
        model->insertRow(nrow);
        model->setData(model->index(nrow,0), text);
        model->setData(model->index(nrow,0), Qt::Checked, Qt::CheckStateRole);
        model->itemFromIndex(model->index(nrow,0))->setCheckable(true);

        element = element.nextSiblingElement("item");
    }
}

void MethodView::saveContent(QDomDocument &document, QDomElement &parent)
{
    int nrow = model->rowCount();
    for(int irow=0;irow<nrow;irow++)
    {
        if (model->data(model->index(irow,0), Qt::CheckStateRole)!=Qt::Checked)
            continue;

        QString text = model->data(model->index(irow,0)).toString();
        QDomElement element = document.createElement("item");
        element.appendChild(document.createTextNode(text));

        parent.appendChild(element);
    }
}


QStringList MethodView::getMethods()
{
    QStringList strs;
    int nrow = model->rowCount();
    for(int irow=0;irow<nrow;irow++)
    {
        if (model->data(model->index(irow,0), Qt::CheckStateRole)!=Qt::Checked)
            continue;

        QString text = model->data(model->index(irow,0)).toString();
        strs << text;
    }

    return strs;
}

void MethodView::onClicked(QModelIndex index)
{
    emit sigUpdateMethod();
}

//=============================================================================

MethodDock::MethodDock(QWidget *parent)
    : QWidget(parent)
{
    view = new MethodView(this);

    QGridLayout *layout = new QGridLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(view, 0,0,1,1);

    setLayout(layout);

    connect(view, SIGNAL(sigUpdateMethod()), this, SLOT(onUpdateMethod()));
}

void MethodDock::loadFromFile(QString filename)
{
    QFile file(QString::fromLocal8Bit(filename.toLocal8Bit()));
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Loading from file"),
                             QString(tr("Cannot read file %1:\n%2")).arg(filename).arg(file.errorString()));
        return;
    }

    QDomDocument doc;
    doc.setContent(&file);
    QDomElement rootElement = doc.documentElement();
    view->setContent(rootElement);

    file.close();
}

void MethodDock::saveToFile(QString filename)
{
    QFile file(QString::fromLocal8Bit(filename.toLocal8Bit()));
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Save to file"),
                             QString(tr("Cannot save to %1:\n%2")).arg(filename).arg(file.errorString()));
        return;
    }

    QTextStream out(&file);

    QDomDocument doc;
    QDomElement element = doc.createElement("propeller");
    doc.appendChild(element);

    view->saveContent(doc, element);

    doc.save(out,4);

    file.close();
}

QStringList MethodDock::getMethods()
{
    return view->getMethods();
}

void MethodDock::onUpdateMethod()
{
    emit sigUpdateMethod();
}

