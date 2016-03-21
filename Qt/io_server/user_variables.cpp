#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include "user_variables.h"

userVariables::userVariables()
{
    QString filename = getAppDataPath().append("user.var");

    file = new QFile(filename);

    //qDebug() << "Openning userVariables in: " << filename;
    bool status = file->open(QIODevice::ReadOnly | QIODevice::Text);
    if(status)
    {
        QTextStream in(file);

        while(!in.atEnd())
        {
            QString line = in.readLine().trimmed();

            mVariables.append(new sVariable(line));
        }
        file->close();
    }

}

QString userVariables::getAppDataPath()
{
    QString path  = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    //check if folder exists
    QDir appDataDir(path);
    if(!appDataDir.exists())
    {
        if(appDataDir.mkpath(path))
            qDebug() << "Created path: " << path;
    }
    return path.append("/");
}

void userVariables::updateVariables()
{
    if(file->open(QIODevice::WriteOnly | QIODevice::Text))
    {

        QTextStream out(file);
        foreach(sVariable * v, mVariables)
        {
            out << v->name << "=" << v->value << "\n";
        }

        file->close();

        qDebug() << "Saved uservariables";
    }
}

userVariables::sVariable * userVariables::getVar(QString name)
{
    foreach(sVariable * l, mVariables)
    {
        if(l->name == name)
        {
            return l;
        }
    }
    return 0;
}

void userVariables::setVar(QString name, QString value)
{
    sVariable * v = getVar(name);
    if(v)
    {
        v->value = value;
        return;
    }

    mVariables.append(new sVariable(name, value));
}

void userVariables::setVar(QString name, int value)
{
    sVariable * v = getVar(name);
    if(v)
    {
        v->value = QString::number(value);
        return;
    }

    mVariables.append(new sVariable(name, QString::number(value)));
}

QString userVariables::getVarString(QString name)
{
    foreach(sVariable * l, mVariables)
    {
        if(l->name == name)
        {
            return l->value;
        }
    }

    return QString();
}

int userVariables::getVarValue(QString name)
{
    foreach(sVariable * l, mVariables)
    {
        if(l->name == name)
        {
            return l->value.toInt();
        }
    }

    return INT_MIN;
}

userVariables::~userVariables()
{
    foreach(sVariable * l, mVariables)
    {
        delete l;
    }
}

userVariables::sVariable::sVariable(QString name, QString value)
{
    this->name = name;
    this->value = value;
}

userVariables::sVariable::sVariable(QString line)
{
    QStringList pair = line.split("=");
    if(pair.length() > 1)
    {
        name = pair.at(0);
        value = pair.at(1);

        //qDebug() << "sVariable(" << name << " " << value << ")";
    }
}
