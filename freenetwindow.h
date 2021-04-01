#ifndef FREENETWINDOW_H
#define FREENETWINDOW_H
#include <QAbstractTableModel>

#include <QMainWindow>

namespace Ui {
class FreenetWindow;
}
struct FreenetVideo
{
    FreenetVideo() : stored(false){}
    QString videoKey;
    QString videoName;
    QString picKey;
    QString picName;
    bool stored;
};
struct StoredVideo
{
    QString fmame, key;
};

class FreenetVideoModel : public QAbstractTableModel
{
public:
    FreenetVideoModel() : QAbstractTableModel(nullptr){}
    int rowCount(const QModelIndex &) const ;
    int columnCount(const QModelIndex &parent) const {return 2;}
    void refresh();
    void openInBrowser();
    int openInd;

    QStringList pictures;
    QList <FreenetVideo> videos;
    void readClipboard();
    void calcStored();
    void checkStored(FreenetVideo & fn);
    void storeVideos(int beg, int end);

    QVariant headerData(int section,
                     Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVector<StoredVideo> storedVideos;
    void readStoredVideos();


};

class FreenetWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit FreenetWindow(QWidget *parent = nullptr);
    ~FreenetWindow();
    void readClipboard();
    void refresh();
    bool eventFilter(QObject *obj, QEvent *event);
    void mouseEvent(QMouseEvent* event);
    void contextMenu(int row, const QPoint& pos);

    FreenetVideoModel* model();
private slots:
    void on_actionRead_clipboard_triggered();

    void on_actionBrowse_triggered();

    void on_actionStore_all_triggered();

private:
    Ui::FreenetWindow *ui;
};

#endif // FREENETWINDOW_H
