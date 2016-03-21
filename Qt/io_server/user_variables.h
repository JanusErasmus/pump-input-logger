#ifndef USER_VARIABLES_H
#define USER_VARIABLES_H
#include <QString>
#include <QFile>
#include <QDate>

class userVariables
{
    QFile* file;

    struct sVariable
    {
        QString name;
        QString value;

        sVariable(QString name, QString value);
        sVariable(QString line);
    };

    QList<sVariable*> mVariables;


    sVariable * getVar(QString name);

public:
    userVariables();
    ~userVariables();

    static QString getAppDataPath();

    void setVar(QString name, QString value);
    void setVar(QString name, int value);
    QString getVarString(QString name);
    int getVarValue(QString name);

    void updateVariables();

};

#endif // USER_VARIABLES_H
