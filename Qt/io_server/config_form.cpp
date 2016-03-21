#include "config_form.h"
#include "ui_config_form.h"

#include "user_variables.h"

configForm::configForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::configForm)
{
    ui->setupUi(this);

    blueText.setColor(QPalette::Text,Qt::blue);
    defaultText.setColor(QPalette ::WindowText, defaultText.color(QPalette ::WindowText));

    mLineEdits.append(ui->rateEdit);
    mLineEdits.append(ui->onEdit);
    mLineEdits.append(ui->stopEdit);
    mLineEdits.append(ui->startEdit);
    mLineEdits.append(ui->restEdit);

    userVariables v;
    ui->rateEdit->setText(v.getVarString("rate"));
    ui->startEdit->setText(v.getVarString("start"));
    ui->stopEdit->setText(v.getVarString("stop"));
    ui->onEdit->setText(v.getVarString("on"));
    ui->restEdit->setText(v.getVarString("rest"));

    connectWidgets();

    connect(ui->pushButton, SIGNAL(clicked(bool)), SLOT(updateVarables()));
}

void configForm::connectWidgets()
{
    foreach(QLineEdit* edit, mLineEdits)
        connect(edit, SIGNAL(textChanged(QString)), SLOT(textEdit()));
}

void configForm::textEdit()
{
    QLineEdit * line = (QLineEdit*) sender();
    line->setPalette(blueText);

    ui->pushButton->setEnabled(true);
}

void configForm::updateVarables()
{
    userVariables v;
    v.setVar("rate",ui->rateEdit->text());
    v.setVar("start", ui->startEdit->text());
    v.setVar("stop", ui->stopEdit->text ());
    v.setVar("on", ui->onEdit->text   ());
    v.setVar("rest", ui->restEdit->text ());

    v.updateVariables();

    foreach(QLineEdit * line, mLineEdits)
        line->setPalette(defaultText);

    ui->pushButton->setEnabled(false);
}

configForm::~configForm()
{
    delete ui;
}
