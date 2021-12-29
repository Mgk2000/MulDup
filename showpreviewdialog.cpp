#include "showpreviewdialog.h"
#include "ui_showpreviewdialog.h"

#include <QProcess>

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

void ShowPreviewDialog::showImage(const QString &fn)
{
    show();
    QPixmap pix(fn);
    ui->imageLabel->setPixmap(pix);
    imageFn = fn;
}

void ShowPreviewDialog::resizeEvent(QResizeEvent * /* *event */)
{
    showImage(imageFn);
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
