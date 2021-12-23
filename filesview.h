#ifndef FILESVIEW_H
#define FILESVIEW_H

#include <QTableView>
#include <QAbstractTableModel>
#include <QMouseEvent>
#include <QMutex>
#include <QAction>
#include <QProcess>
#include <QThread>

class MainWindow;
class File;
class FilesView;
class FilesModel : public QAbstractTableModel
{
public:
    FilesModel(QVector<File*>* files);
    virtual ~FilesModel();
    int rowCount(const QModelIndex &parent) const ;
    int columnCount(const QModelIndex &parent) const;
    void refresh();
    QVariant headerData(int section,
                     Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    void setFiles(QVector<File*>* _files, bool immed = true);
    void discardFilter();
    QVector<File*> files,  savedFiles ;
    bool showInBytes;
    bool showBirth;
    bool sortOrder[7] = {false};
    void sortByName();
    void sortBySize();
    void sortByDate();
    void sortByMD5();
    void sortByEd2k();
    void sortByForpost();
    void sortByDir();
    void unSort();
    void appendFiles(const QVector<File*> & afiles);
    void showDuplicates();
    FilesView* view;
    File* activeFile;
};
class FilterForm;
class FilesView : public QTableView
{
    Q_OBJECT

public:
    FilesView(QWidget* parent,FilterForm *  ff,  QVector<File *> *_files);
    FilesModel fmodel;
    void refresh();
    void setFiles(QVector<File*>* _files, bool immed = true);
    void mousePressEvent(QMouseEvent *event) override;
    int sortColumn;
    FilterForm* filterForm;
    void contextMenu(int row, const QPoint& pos);
    void renameMove(int row, const QString& fname);
    void forpost(File * _file);
    void deleteFile(int row, const QString& fname);
    void deleteDeleted(File* file);
    void copyFilePath(File* file);
    bool showExisting();
    bool showDeleted();
    void activateFile(File * file);
    int rowToSet;
    int currRow;
    struct OpenWith
    {
        OpenWith(const QString& _menu, const QString& _program);
        QAction action;
        QString program;
    };
    QProcess openWithProcess;
    QList<OpenWith*> openWithActions;
    void openWith(File* file, const QString & program );
    void fillOpenWithActions();
    void searchBySize(const QString & s);
    void copyFileSize(File *file);
    bool showBirth() const;
    void setTabText(const QString & s);
public slots:
    void headerPressed(int index);
    void filterSearchPressed();
    void filterAppendPressed();
    void filterDiscardPressed();
    void bytesClicked();
    void mbytesClicked();
    void birthClicked();
    void unsortPressed();
    void onRefresh();
    void timerShot();
    //void showExistingClicked (bool checked);
    //void showDeletedClicked (bool checked);
};
class DeleteFileThread : public QThread
{
public:
    DeleteFileThread (const QString & fn);
    void run() override;
    QString fname;
};

#endif // FILESVIEW_H
