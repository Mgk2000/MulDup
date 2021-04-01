#include "freenetclipboard.h"
#include "ui_freenetclipboard.h"
#include "mainwindow.h"
#include "hash.h"
#include <QClipboard>
#include <QMimeData>
#include <QDesktopServices>

FreenetClipboard::FreenetClipboard(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::FreenetClipboard)
{
    ui->setupUi(this);
}

FreenetClipboard::~FreenetClipboard()
{
    delete ui;
}

void FreenetClipboard::readClipboard()
{

    if( QClipboard* c = QApplication::clipboard() )
        if( const QMimeData* m = c->mimeData() )
            if( m->hasText() )
            {
                ui->picsEdit->clear();
                ui->videoEdit->clear();
                pics.clear();
                videos.clear();
                QString s = m->text();
                int beg =0;
                for(;;)
                {
                    int end = s.indexOf('\n', beg);
                    if (end<0)
                         end = s.length();
                    QString ss = s.mid(beg, end-beg);
                    if (isFreenetKey(ss))
                    {
                        if (isVideo(ss))
                        {
                            videos.append(ss);
                            ui->videoEdit->setPlainText(ui->videoEdit->toPlainText() + ss + "\r\n");
                        }
                        else
                        {
                            pics.append(ss);
                            ui->picsEdit->setPlainText(ui->picsEdit->toPlainText() + ss + "\r\n");
                        }
                     }
                    beg = end+1;
                    if (beg >= s.length() - 10)
                        break;
                    for (int i = beg; i<s.length(); i++)
                        if (s[i] == '\r' || s[i] == '\n')
                            beg++;
                        else break;

                }
            }

}


int FreenetClipboard::lineOfPos(QPlainTextEdit* edit,  int pos)
{
    QString s = edit->toPlainText();
    int l=0;
    for (int i =0; i<pos; i++)
        if (s[i] == '\n') l++;
    return l;
}

void FreenetClipboard::on_actionRead_clipboard_triggered()
{
    readClipboard();
}

void FreenetClipboard::on_actionBrowser_triggered()
{
    openInBrowser();
}

void FreenetClipboard::openInBrowser()
{
    if (pics.length() <=0)
        return;
    int lbeg, lend;
    int cbeg = ui->picsEdit->textCursor().selectionStart();
    int cend = ui->picsEdit->textCursor().selectionEnd();
    if (cend>cbeg)
    {
       lbeg = lineOfPos(ui->picsEdit, cbeg);
       lend = lineOfPos(ui->picsEdit,cend);
    }
    else
    {
        lbeg =0;
        lend = pics.length();
    }
    qDebug() << "Beg=" << lbeg << " End=" << lend;
    for (int i= lbeg; i<lend; i++)
    {
        QString url= "http://127.0.0.1:8888/freenet:" + pics[i];
        QDesktopServices::openUrl ( url );
    }
}
