#include "showpreviewdialog.h"
#include "ui_showpreviewdialog.h"

#include <QProcess>
#include <QDebug>

ShowPreviewDialog::ShowPreviewDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ShowPreviewDialog)
{
    ui->setupUi(this);
}

ShowPreviewDialog::~ShowPreviewDialog()
{
    delete ui;
}

void ShowPreviewDialog::showImage()
{
    show();
    writeLabel();
    newPixmap = origPixmap.scaled(newImW, imH);
    //ui->imageLabel->setGeometry(ui->imageLabel->x(), ui->imageLabel->height(), newImW, imH);
    //this->setGeometry(this->x(), y(), newImW + 40, height());
    ui->imageLabel->setPixmap(newPixmap);
    //qDebug() << "label=" << ui->imageLabel->width() << "Dialog =" << width();
}

void ShowPreviewDialog::resizeEvent(QResizeEvent * /* *event */)
{
    //showImage();
}

void ShowPreviewDialog::run(const QString &fn)
{
    imageFn = fn;
    origPixmap = QPixmap(fn);
    newImW = imW = origPixmap.width();
    imH = origPixmap.height();
    ui->spinBox->setValue(newImW);
    showImage();
}

void ShowPreviewDialog::writeLabel()
{
    ui->label->setText(QString("%1x%2").arg(newImW).arg(imH));
}

void ShowPreviewDialog::on_okButton_clicked()
{
    close();
}

void ShowPreviewDialog::on_editButton_clicked()
{
    QProcess pr;
    QStringList arguments;
    QString fn = imageFn.replace("/", "\\");
    arguments << fn;
    pr.startDetached("E:\\Program Files (x86)\\IrfanView\\i_view32.exe", arguments);
}

void ShowPreviewDialog::on_spinBox_valueChanged(int arg1)
{
    newImW = arg1;
    showImage();
}

void ShowPreviewDialog::on_saveButton_clicked()
{
    newPixmap.save(imageFn);
}
