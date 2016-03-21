#include <QDebug>
#include <QDateTime>
#include <QTextStream>
#include <QStringList>
#include <QDir>
#include <QStandardPaths>

#include "pump_report.h"

pumpReport::pumpReport()
{
    mOpened = false;

    QDateTime local(QDateTime::currentDateTime());
    QString fileName("PumpReport_" + local.toString("dd_MM_yy") + ".csv" );

    QString path  = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    //check if folder exists
    QDir appDataDir(path);
    if(!appDataDir.exists())
    {
        if(appDataDir.mkpath(path))
            qDebug() << "Created path: " << path;
    }
    path.append("/Pump_Reports/");

    //check if directory exists
    QDir directory(path);
    if(!directory.exists())
    {
        if(appDataDir.mkpath(path))
            qDebug() << "Created path: " << path;
    }
    path.append(fileName);

    openReport(path);
}

void pumpReport::openReport(QString filename)
{
    mFile = new QFile(filename);

    if(!mFile->exists())
    {
        qDebug() << "File "<< filename << " does not exist, creating...";
        createReport(filename);
    }

    if(mFile->open(QIODevice::ReadWrite | QIODevice::Text))
    {
        mOpened = true;
        mEntryCount = entryCount() + 1;
    }
}

void pumpReport::createReport(QString filename)
{
    QFile newFile(filename);
    if(newFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&newFile);
        out << "#,date,Port,Pump State\n";
    }

    newFile.close();
}

bool pumpReport::addEntry(QDateTime time, int port, int state)
{
    if(!mOpened)
        return false;

    mFile->seek(mFile->size());
    QTextStream out(mFile);
    out << QString::number(mEntryCount++) << ",";
    out << time.toString("yyyy-MM-dd HH:mm:ss") << ",";
    out << QString::number(port) << ",";
    out << QString::number(state) << "\n";

    return true;
}
int pumpReport::entryCount()
{
    if(!mOpened)
        return -1;

    QString line;
    line = readLastLine();

    QStringList args = line.split(",");
    return args.first().toInt();
}

QString pumpReport::readLastLine()
{
    QString line;

    //seek for next new line
    int k = mFile->size() - 3;
    while(k)
    {
        char rChar;
        mFile->seek(k--);
        mFile->getChar(&rChar);

        if(rChar == '\n')
            break;

    }

    QTextStream in(mFile);
    line = in.readLine();

    return line;
}

pumpReport::~pumpReport()
{
   mFile->close();
   delete mFile;
}
