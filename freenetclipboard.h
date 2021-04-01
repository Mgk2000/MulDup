#ifndef FREENETCLIPBOARD_H
#define FREENETCLIPBOARD_H

#include <QMainWindow>

namespace Ui {
class FreenetClipboard;
}
class QPlainTextEdit;
class FreenetClipboard : public QMainWindow
{
    Q_OBJECT

public:
    explicit FreenetClipboard(QWidget *parent = nullptr);
    ~FreenetClipboard();
    void readClipboard();
    QStringList pics, videos;
    int lineOfPos(QPlainTextEdit* edit,int pos);
private slots:
    void on_actionRead_clipboard_triggered();

    void on_actionBrowser_triggered();
    void openInBrowser();

private:
    Ui::FreenetClipboard *ui;
};

#endif // FREENETCLIPBOARD_H
