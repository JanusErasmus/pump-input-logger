#include "config_form.h"
#include "ui_config_form.h"

configForm::configForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::configForm)
{
    ui->setupUi(this);
}

configForm::~configForm()
{
    delete ui;
}
