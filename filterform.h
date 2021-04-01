#ifndef FILTERFORM_H
#define FILTERFORM_H

#include <QPushButton>
#include <QWidget>
#include "ui_filterform.h"
#include "filter.h"
namespace Ui {
class FilterForm;
}

class FilterForm : public QWidget
{
    Q_OBJECT

public:
    explicit FilterForm(QWidget *parent = nullptr);
    ~FilterForm();
    QPushButton* searchButton() {return ui->okButton;}
    QPushButton* appendButton() {return ui->appendButton;}
    QPushButton* discardButton() {return ui->discardButton;}
    QPushButton* unsortButton() {return ui->unsortButton;}
    QRadioButton * bytesButton() {return ui->bytesRadioButton;}
    QRadioButton * mbytesButton() {return ui->mbytesRadioButton;}
    QCheckBox* showExistingCheckBox() {return ui->showExistingCheckBox;}
    QCheckBox* showDeletedCheckBox() {return ui->showDeletedCheckBox;}
    Filter filter;
    bool calcFilter();
    void setSizeFilter(const QString &s);
    void clear();
    void clearMBytes();
private slots:
    void on_clearButton_clicked();
    void on_sizeMoreSpinBox_valueChanged(int arg1);

    void on_clearMBytesButton_clicked();

private:
    Ui::FilterForm *ui;
    QLineEdit* nameEdit[3];
};

#endif // FILTERFORM_H
