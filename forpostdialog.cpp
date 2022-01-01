#include "forpostdialog.h"
#include "ui_forpostdialog.h"
#include "filesview.h"
#include <QClipboard>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QProcess>
#include "file.h"
#include "hash.h"
#include "mainwindow.h"

ForpostDialog::ForpostDialog(QWidget *parent) :
    QDialog(parent), process(0),
    ui(new Ui::ForpostDialog)
{
    ui->setupUi(this);
    forpostFolder = "U:/Rindex";
    VTMdir = "u:/VTM/VTMakerPortable/App/VideoThumbnailsMaker/";
    VTMexe = VTMdir + "VideoThumbnailsMaker.exe";
    VTMPresetsDir = VTMdir + "Options";
    VTMPreset = "R_5x6.vtm";
    ui->VTMPresetLabel->setText(VTMPreset);
    VTMImageDir = VTMdir + "Internet";
    winRar = "e:/Program files/Winrar/Rar.exe";
    password = "qwerty123456";
    ui->passwordEdit->setText(password);
    ui->volumeSizeEdit->setText("500");
    previewCreationStarted = false;
    archiveCreationStarted = false;
 //   timeId = startTimer(500);
    connect(&process, SIGNAL(readyReadStandardOutput()), this, SLOT(onProcessStandardOutput()));
    connect(&process, SIGNAL(readyReadStandardError()), this, SLOT(onProcessErrorOutput()));
    connect(&process, SIGNAL(finished(int,QProcess::ExitStatus )), this, SLOT(onProcessFinished(int , QProcess::ExitStatus)));
    ui->plainTextEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->plainTextEdit,SIGNAL(customContextMenuRequested(const QPoint&)),this,
            SLOT(onEditContextMenuRequested(const QPoint&)));

}

ForpostDialog::~ForpostDialog()
{
    delete ui;
}

void ForpostDialog::setVideo(File* _file, const QString &_video)
{
    video = _video;
    file = _file;
    ui->MD5Edit->setText(file->MD5);
    ui->archiveName->setText("");
    ui->parentFolderLabel->setText(forpostFolder);
    projectName = stripExtension(video);
    ui->projectNameEdit->setText(projectName);
    ui->projectFolderEdit->setText(file->forpost);
    if (file->forpost == "")
        ui->projectFolderEdit->setText(forpostFolder + "/" + projectName);
    QFileInfo f(video);
    videoDir = f.absolutePath();
    previewCreationStarted = false;
    archiveCreationStarted = false;
    ui->sizeEdit->setText(QString("%1").arg(QFileInfo(video).size()));
    timeId = startTimer(500);
    if (!readForpost())
        createNewForpost();
    qint64 i500 = 500 * 1024 * 1024;
    int partsize = i500;
//    ui->volumeSizeEdit->setText("500");
    if (f.size() > i500)
    {
        int nparts = f.size() / i500 + 1;
        partsize = (f.size() + 1024*1024-1) / nparts;
        if (partsize > i500)
            partsize = i500;
    }
    ui->volumeSizeEdit->setText(QString("%1").arg ((partsize + 1024*1024-1) / (1024*1024)));

}
QString ForpostDialog::parentFolder()
{
    return ui->parentFolderLabel->text();
}

QString ForpostDialog::projectDirName()
{
    return ui->projectFolderEdit->text();
}

QString ForpostDialog::stripExtension(const QString &fname)
{
    int lastdot = fname.lastIndexOf('.');
    int lastSlash = fname.lastIndexOf("/");
    if (lastSlash == -1)
        lastSlash = fname.lastIndexOf("\\");
    return fname.mid(lastSlash+1, lastdot-lastSlash -1);
}

bool ForpostDialog::readForpost()
{
    if (projectName == "")
        return false;
    ui->plainTextEdit->setPlainText("");
    QDir dir (projectDirName());
    QDir pardir = dir;
    bool b = pardir.cdUp();
    qDebug() << "cdUp=" << b;
    if (!b || pardir.path() == dir.path())
        qDebug() << "bad cdUp";
    QString pd= pardir.path();
//    ui->parentFolderLabel->setText(pardir.path());
    ui->parentFolderLabel->setText(pd);

    QList<QFileInfo> txtList = dir.entryInfoList(QStringList() <<"*.txt");
    if (txtList.count() > 0)
    {
        postFileName = txtList[0].absoluteFilePath();
        QFile f (postFileName);
        if (f.open(QIODevice::Text | QIODevice::ReadOnly))
        {
            QString s = f.readAll();
            ui->plainTextEdit->setPlainText(s);
            return true;
        }
    }
    return false;
}

void ForpostDialog::timerEvent(QTimerEvent *)
{
    if (previewCreationStarted)
    {
        if (process.state() == QProcess::Running)
            return;
        previewCreationStarted = false;

        QFile f (videoDir + "/" + file->name + ".jpg");
        if (!f.exists())
            return;
        QString previewName = projectDirName() + "/" + file->name + ".jpg";
        QFile::remove(previewName);
        if (!f.rename(previewName))
            qDebug() << "Cannot move preview from " << videoDir << " to " << projectDirName();
        return;
        QString previewArcName = "Pv" + ui->archiveName->text();
        QStringList arguments;
        arguments << "a";
        arguments << "-hp" + password;
        arguments << projectDirName() + "/" + previewArcName + ".rar";
        arguments << previewName;
        process.startDetached(winRar, arguments);

    }
    ui->createArchiveButton->setEnabled(isReadyToGo());
    ui->createArchiveButton->setEnabled(isReadyToGo());
    ui->projectFolderEdit->setText(parentFolder() + '/' + projectName);

}

void ForpostDialog::closeEvent(QCloseEvent *)
{
    killTimer(timeId);
    previewCreationStarted = false;
    archiveCreationStarted = false;

}

bool ForpostDialog::isReadyToGo()
{

    if (!QFile(video).exists())
        return false;
    if (projectName == "")
        return false;
    return true;
}

void ForpostDialog::createPreview()
{
    if (ui->archiveName->text().trimmed().isEmpty())
    {
        QMessageBox::question(this, "Error", "Enter archive name first");
        return;
    }
    qDebug() << "create preview";
    previewCreationStarted = true;
    QStringList arguments;
    QString preset = VTMPresetsDir +"\\" + ui->VTMPresetLabel->text();
    preset = preset.replace('/', '\\');
    //preset = ui->VTMPresetLabel->text();
    arguments << preset;
    arguments << video;
    arguments << "/nc";
    process.start(VTMexe, arguments);
//    process.startDetached(VTMexe, arguments);
    process.waitForStarted();
}

void ForpostDialog::createArchive()
{
    if (ui->archiveName->text().trimmed().isEmpty())
    {
        QMessageBox::question(this, "Error", "Enter archive name first");
        return;
    }
    qDebug() << "create Archive";
    createProjectFolder();
    archiveCreationStarted = false;
    QStringList arguments;
    arguments << "a";
    arguments << "-hp" + password;
    arguments << "-m1";
    if (ui->excludePathCheckBox->isChecked())
        arguments << "-ep1";
    QString  s = "500";
    if (ui->volumeSizeEdit->text() !="")
        s = ui->volumeSizeEdit->text();
    arguments << "-v" + s + "m";
    s = ui->archiveName->text();
    if (s=="")
        s = projectName;
    arguments << projectDirName() + "/" + s + ".rar";
    arguments << video;
    process.start(winRar, arguments);
    process.waitForStarted();
}

void ForpostDialog::createProjectFolder()
{
    QString pdn = parentFolder() + "/" + projectName;
    QDir pdir(parentFolder());
    if (pdir.exists(projectName))
        return;
    pdir.mkdir(projectName);
    ui->projectFolderEdit->setText(pdn);
}

void ForpostDialog::createNewForpost()
{
    QString s;
    bool addInfo = file->size >= 10 *1024 * 1024;
//    s+= "[VID] \r\n\r\n\r\n";
      s+= "\r\n\r\n";
//    s+= "Preview backup:\r\n\r\n\r\n";
    if (addInfo)
        {
        s+= "\r\n\r\n\r\n";
        s += "Name: " + file->name + "\r\n";
        s+= QString("Size: %L2 bytes\r\n").arg(file->size);
    //    s+= "Duration: \r\n";
        s+= QString("MD5: %1 \r\n\r\n").arg(file->MD5);
        }
    else s+= "\r\n\r\n";
    s+= "Video:\r\n\r\n\r\n\r\n\r\n";
    s+= "P:\r\n";
    s+=  password + "\r\n";
    ui->plainTextEdit->setPlainText(s);

}

QString ForpostDialog::percentMask(const QString &sss)
{
    QString s=sss;
    if (s.mid(0,5) == "http:")
        s=s.mid(7);
    if (s.mid(0,6) == "https:")
        s=s.mid(8);
    QString s1 = s.mid(0,3);
    QString s2 = s.mid(3);
    QByteArray ba = QUrl::toPercentEncoding(s2, "/?=%0123456789", "abcdefghijklmnopqrstuvwxyz.");
    QString ss = s1 + QString(ba);
    qDebug() << QUrl(ss).isValid();
//    QClipboard* c = QApplication::clipboard();
//    c->setText(ss.toLower());
    return ss;
}

QString ForpostDialog::pinkMask(const QString &s)
{
    int n = s.length() - 7;
    QString ss = "[url]" + s.mid(0,n) + "[/url]" + "[color=#FF00FF]DudukA[/color]" + s.mid(n);
    return ss;
}

QString ForpostDialog::captchaMask(const QString &s)
{
    QString ss = "[url=" + s + "]Rindexxx[/url]";
    return ss;
}

QString ForpostDialog::downloadMask(const QString &url)
{
    QString sh = "Download";
    if (url.contains("anonfiles"))
        sh = " Anon ";
    else if (url.contains("bayfiles"))
        sh = " Bay ";
    else if (url.contains("solidfiles"))
        sh = " Solid ";
    else if (url.contains("1fichier"))
        sh = " 1Fich ";
    else if (url.contains("userscloud"))
        sh = " Ucloud ";
    else if (url.contains("siasky"))
        sh = "Siask";
    else if (url.contains("tusfiles"))
        sh = " Tusf ";
    else if (url.contains("iobb"))
        sh = " Intel ";
    else if (url.contains("hxfile"))
        sh = " HxFil ";
    else if (url.contains("upload.ee"))
        sh = " UploadEE ";
    QString ss = "[url=" + url + "]" + sh+ "[/url]";
    return ss;
}

void ForpostDialog::onEditContextMenuRequested(const QPoint & )
{
    QMenu* editContextMenu = ui->plainTextEdit->createStandardContextMenu();
    QAction pasteUrlAct ("Unmasked");
    QAction pasteUrlAct1 ("Percent");
    QAction pasteUrlAct2 ("Pink");
    QAction pasteUrlAct3 ("Captcha");
    QAction pasteUrlAct4 ("Download");
    QMenu pasteUrlMenu ("Paste Url");
    pasteUrlMenu.addAction(&pasteUrlAct);
    pasteUrlMenu.addAction(&pasteUrlAct1);
    pasteUrlMenu.addAction(&pasteUrlAct2);
    pasteUrlMenu.addAction(&pasteUrlAct3);
    pasteUrlMenu.addAction(&pasteUrlAct4);
    editContextMenu->addMenu(&pasteUrlMenu);
    QAction pasteImageAction("Paste [img]");
    editContextMenu->addAction(&pasteImageAction);
    QString url;
    if( QClipboard* c = QApplication::clipboard() )
    {
        if( const QMimeData* m = c->mimeData() )
        {
            if( m->hasText() )
                url = m->text();
        }
    }
    bool emp = url.isEmpty();
    pasteUrlMenu.setEnabled(!emp);
    QAction * act = editContextMenu ->exec(QCursor::pos());
    if (act == &pasteUrlAct || act == &pasteUrlAct1 || act == &pasteUrlAct2 || act == &pasteUrlAct3 || act == &pasteUrlAct4)
    {
        QTextCursor  cursor = ui->plainTextEdit->textCursor();
        QString s1;
        if (act == &pasteUrlAct)
            s1 = url;
        else if (act == &pasteUrlAct1)
            s1 = percentMask(url);
        else if (act == &pasteUrlAct2)
            s1 = pinkMask(url);
        else if (act == &pasteUrlAct3)
            s1 = captchaMask(url);
        else if (act == &pasteUrlAct4)
            s1 = downloadMask(url);
        if (ui->crCheckBox->isChecked())
            s1 += "\r\n";
        ui->plainTextEdit->insertPlainText(s1);
    }
    if (act == &pasteImageAction)
        ui->plainTextEdit->insertPlainText("[img]" + url + "[/img]");
}

void ForpostDialog::on_selectParentFolderButton_clicked()
{
    QFileDialog fd;
    fd.setDirectory(forpostFolder);
    fd.setFileMode(QFileDialog::Directory);
    fd.setOption(QFileDialog::ShowDirsOnly);
    auto rc= fd.exec();
    if (rc == QDialog::Accepted)
    {
        ui->parentFolderLabel->setText(fd.directory().path());
        ui->projectFolderEdit->setText(fd.directory().path() + '/' + projectName);
        forpostFolder = fd.directory().path();
    }
}

void ForpostDialog::on_selectVTMpresetButton_clicked()
{
    QFileDialog fd;
    fd.setDirectory(VTMPresetsDir);
//    fd.setFileMode(QFileDialog::Directory);
//    fd.setOption(QFileDialog::ShowDirsOnly);
    auto rc= fd.exec();
    if (rc == QDialog::Accepted)
    {
        VTMPreset = QFileInfo(fd.selectedFiles()[0]).fileName();
        ui->VTMPresetLabel->setText(VTMPreset);
    }
}

void ForpostDialog::on_createPreviewButton_clicked()
{
    projectName = ui->projectNameEdit->text();
    createProjectFolder();
    createPreview();

}

void ForpostDialog::on_createArchiveButton_clicked()
{
    projectName = ui->projectNameEdit->text();
    createProjectFolder();
    createArchive();
}

void ForpostDialog::onProcessStandardOutput()
{
    QByteArray ba = process.readAllStandardOutput();
    QString s (ba);
    qInfo() << s;
}

void ForpostDialog::onProcessErrorOutput()
{
    QByteArray ba = process.readAllStandardError();
    QString s (ba);
    qDebug() << "==Error==";
    qDebug() << s;
    qInfo() << s;

}

void ForpostDialog::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
   qInfo() << "Process finished exitCode=" << exitCode << " exitStatus=" << exitStatus;
}

void ForpostDialog::on_MD5Button_clicked()
{
    calcMD5(file);
    ui->MD5Edit->setText(file->MD5);

}

void ForpostDialog::on_getrProjectButton_clicked()
{
    QFileDialog fd;
    fd.setDirectory(forpostFolder);
    fd.setFileMode(QFileDialog::Directory);
    fd.setOption(QFileDialog::ShowDirsOnly);
    auto rc= fd.exec();
    if (rc == QDialog::Accepted)
    {
        QStringList dirnames = fd.selectedFiles();
        if(dirnames.length() <=0)
            return;
        QString pdn = dirnames[0];
        ui->projectFolderEdit->setText(pdn);
        int i = pdn.lastIndexOf('/');
        int i1 = pdn.lastIndexOf('\\');
        i = std::max(i, i1);
        projectName = pdn.mid(i+1, 1000);
        readForpost();
    }

}

void ForpostDialog::on_saveButton_clicked()
{
    if (projectDirName() == "")
    {
        QMessageBox::question(mainWin(), "No project",
               "First, select project directory");
        return;
    }
    createProjectFolder();
    QString fn = postFileName;
    if (true || fn=="")
        fn = projectDirName() + "/" +projectName + ".txt";
    QFile f (fn );
    if(f.open(QIODevice::WriteOnly  | QIODevice::Text))
    {
        QTextStream out(&f);
             out << ui->plainTextEdit->toPlainText();
        f.close();
        file->forpost = projectDirName();
        mainWin()->storeFile(file);
    }
    else QMessageBox::question(mainWin(),"Save Error", f.errorString());
}

void ForpostDialog::on_openDirButton_clicked()
{
    QString program = "explorer.exe";
    QStringList arguments;
    arguments << QString("/n,");

    arguments << file->forpost.replace("/","\\");
    QProcess::startDetached(program, arguments);

}

void ForpostDialog::on_showPreviewButton_clicked()
{
    mainWin()->showPreview(file);
}
