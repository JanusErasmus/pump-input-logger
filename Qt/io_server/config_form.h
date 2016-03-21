#ifndef CONFIG_FORM_H
#define CONFIG_FORM_H
#include <QLineEdit>
#include <QWidget>

namespace Ui {
class configForm;
}

class configForm : public QWidget
{
    Q_OBJECT

    Ui::configForm *ui;

    QPalette blueText;
    QPalette defaultText;

    QList<QLineEdit*> mLineEdits;

    void connectWidgets();

public:
    explicit configForm(QWidget *parent = 0);
    ~configForm();

private slots:
    void textEdit();
    void updateVarables();
};

#endif // CONFIG_FORM_H
