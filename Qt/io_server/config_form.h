#ifndef CONFIG_FORM_H
#define CONFIG_FORM_H

#include <QWidget>

namespace Ui {
class configForm;
}

class configForm : public QWidget
{
    Q_OBJECT

public:
    explicit configForm(QWidget *parent = 0);
    ~configForm();

private:
    Ui::configForm *ui;
};

#endif // CONFIG_FORM_H
