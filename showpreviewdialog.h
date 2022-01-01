#ifndef SHOWPREVIEWDIALOG_H
#define SHOWPREVIEWDIALOG_H

#include <QDialog>

namespace Ui {
class ShowPreviewDialog;
}

class ShowPreviewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ShowPreviewDialog(QWidget *parent = nullptr);
    ~ShowPreviewDialog();
    void showImage();
    void resizeEvent(QResizeEvent *event);
    QString imageFn;
    QPixmap origPixmap, newPixmap;
    int imW, imH, newImW;
    void run(const QString & fn);
    void writeLabel();
private slots:
    void on_okButton_clicked();

    void on_editButton_clicked();

    void on_spinBox_valueChanged(int arg1);

    void on_saveButton_clicked();

private:
    Ui::ShowPreviewDialog *ui;
};

#endif // SHOWPREVIEWDIALOG_H
