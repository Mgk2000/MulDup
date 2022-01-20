#include "filterform.h"
#include "ui_filterform.h"

FilterForm::FilterForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FilterForm)
{
    ui->setupUi(this);
    nameEdit[0] = ui->name1Edit;
    nameEdit[1] = ui->name2Edit;
    nameEdit[2] = ui->name3Edit;
    clear();
}

FilterForm::~FilterForm()
{
    delete ui;
}

bool FilterForm::calcFilter()
{
    for (int i=0; i<filter.filterAtoms.count(); i++)
        delete filter.filterAtoms[i];
    filter.filterAtoms.clear();
    bool ok;
    bool sizeEqFlag = false;
    if (ui->sizeEqEdit->text().trimmed() != "")
    {
        QString s1 = ui->sizeEqEdit->text();
        QString s;
        for (int i=0; i< s1.length(); i++)
            if (!s1[i].isNull() && !s1[i].isSpace() && s1[i] != ',')
                s += s1[i];
        quint64 eqsize = s.toULongLong(&ok);
        if (!ok)
            return false;
        FilterSizeEq * fse = new FilterSizeEq(eqsize);
        filter.filterAtoms.append(fse);
        sizeEqFlag = true;
    }
    if (!sizeEqFlag)
    {
        qreal r;
        quint64 size;
        if (ui->sizeMoreSpinBox->text().trimmed() != "")
        {
            r = ui->sizeMoreSpinBox->text().toDouble(&ok);
            if (!ok)
                return false;
            size = r * 1024 * 1024;
            FilterSizeMore * fsm = new FilterSizeMore(size);
            filter.filterAtoms.append(fsm);
        }
        if (ui->sizeLessSpinBox->text().trimmed() != "")
        {
            r = ui->sizeLessSpinBox->text().toDouble(&ok);
            if (!ok)
                return false;
            size = r * 1024 * 1024;
            FilterSizeLess * fsl = new FilterSizeLess(size);
            filter.filterAtoms.append(fsl);
        }
    }
    for (int i=0; i< 3; i++)
    {
        if (nameEdit[i]->text().trimmed() != "")
        {
            FilterNameContains * f = new FilterNameContains(nameEdit[i]->text());
            filter.filterAtoms.append(f);
        }
    }
    if (!ui->showExistingCheckBox->isChecked())
        filter.filterAtoms.append(new FilterExisting);
    if (!ui->showDeletedCheckBox->isChecked())
        filter.filterAtoms.append(new FilterDeleted);
    if (!ui->showPartCheckBox->isChecked())
        filter.filterAtoms.append(new FilterPart);
    return true;
}

void FilterForm::setSizeFilter(qint64 sz)
{
    for (int i=0; i<filter.filterAtoms.count(); i++)
        delete filter.filterAtoms[i];
    filter.filterAtoms.clear();
    clear();
    FilterSizeEq * fse = new FilterSizeEq(sz);
    filter.filterAtoms.append(fse);
    ui->sizeEqEdit->setText(QString("%1").arg(sz));
}

void FilterForm::setMD5Filter(const QString &md5)
{
    for (int i=0; i<filter.filterAtoms.count(); i++)
        delete filter.filterAtoms[i];
    filter.filterAtoms.clear();
    clear();
    FilterMD5 * fm = new FilterMD5(md5);
    filter.filterAtoms.append(fm);
}

void FilterForm::setSizeFilterEdit(const QString &s)
{
    ui->sizeEqEdit->setText(s);
}

void FilterForm::clear()
{
    ui->name1Edit->clear();
    ui->name2Edit->clear();
    ui->name3Edit->clear();
    ui->sizeEqEdit->clear();
    ui->sizeLessSpinBox->clear();
    ui->sizeMoreSpinBox->clear();
}

void FilterForm::clearMBytes()
{
    ui->sizeLessSpinBox->clear();
    ui->sizeMoreSpinBox->clear();

}

QString FilterForm::stringForTab() const
{
    if (!ui->name1Edit->text().isEmpty())
        return ui->name1Edit->text();
    else if (!ui->sizeEqEdit->text().isEmpty())
        return ui->sizeEqEdit->text();
    return "<All>";
}



void FilterForm::on_clearButton_clicked()
{
    clear();
}

void FilterForm::on_sizeMoreSpinBox_valueChanged(int arg1)
{
    ui->sizeLessSpinBox->setValue(++arg1);
}

void FilterForm::on_clearMBytesButton_clicked()
{
    clearMBytes();
}
