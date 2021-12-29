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
    void showImage(const QString & fn);
    void resizeEvent(QResizeEvent *event);
    QString imageFn;
private slots:
    void on_okButton_clicked();

    void on_editButton_clicked();

private:
    Ui::ShowPreviewDialog *ui;
};

#endif // SHOWPREVIEWDIALOG_H
