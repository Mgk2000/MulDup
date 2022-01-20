#ifndef FORPOSTDIALOG_H
#define FORPOSTDIALOG_H

#include <QDialog>
#include <QMenu>
#include <QProcess>

namespace Ui {
class ForpostDialog;
}
class File;
class ForpostDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ForpostDialog(QWidget *parent = nullptr);
    ~ForpostDialog();
    void setVideo(File * _file, const QString& _video);
    QString parentFolder();
    QString video, videoDir;
    QString projectName;
    QString projectDirName();
    QString forpostFolder, winRar;
    QString password;
    QString VTMdir, VTMPresetsDir, VTMPreset, VTMImageDir, VTMexe;
    QString stripExtension(const QString & fname);
    QString postFileName;
    bool previewCreationStarted, archiveCreationStarted;
    bool readForpost();
    int timeId;
    void timerEvent(QTimerEvent*) override;
    void closeEvent(QCloseEvent *) override;
    bool isReadyToGo();
    QProcess process;
    void createPreview();
    void createArchive();
    void createProjectFolder();
    void createNewForpost();
    File * file;
    QString percentMask(const QString & s);
    QString pinkMask(const QString & s);
    QString captchaMask(const QString & s);
    QString downloadMask(const QString & s);

private slots:
    void onEditContextMenuRequested(const QPoint&);
    void on_selectParentFolderButton_clicked();
    void on_selectVTMpresetButton_clicked();
    void on_createPreviewButton_clicked();
    void on_createArchiveButton_clicked();
    void onProcessStandardOutput();
    void onProcessErrorOutput();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

    void on_MD5Button_clicked();

    void on_getrProjectButton_clicked();

    void on_saveButton_clicked();

    void on_openDirButton_clicked();

    void on_showPreviewButton_clicked();

    void on_onTopCheckBox_clicked(bool checked);

private:
    Ui::ForpostDialog *ui;
};


#endif // FORPOSTDIALOG_H
