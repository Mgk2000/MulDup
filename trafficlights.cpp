#include "trafficlights.h"
#include <QPainter>
#include <QTimer>
#include <QDebug>

TrafficLights::TrafficLights(QWidget *parent) : QWidget(parent), colorLabel(this)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setGeometry(0, 0, 50, 50);
    colorLabel.setGeometry(geometry());
    /*
     *     QSoundEffect * effect = new QSoundEffect;;
    effect->setSource(QUrl::fromLocalFile(QString(":sounds/") + fname));
    effect->setVolume(static_cast<double>(volume));
    if (infinite)
        effect->setLoopCount(QSoundEffect::Infinite);
    sounds.append(effect);
     */
    soundYes.setSource(QUrl::fromLocalFile(QString(":sounds/bad.wav")));
    soundYes.setVolume(1.0);
    qDebug() << soundYes.source();
    soundNo.setSource(QUrl::fromLocalFile(QString(":sounds/good.wav")));
}

void TrafficLights::showLight(int exists)
{
    if (exists==2)
    {
        colorLabel.setStyleSheet("QLabel { background-color : red; }");
        soundYes.play();
    }
    else if (exists==1)
    {
        colorLabel.setStyleSheet("QLabel { background-color : yellow; }");
        soundYes.play();
    }
    else
    {
        colorLabel.setStyleSheet("QLabel { background-color : green; }");
        soundNo.play();
    }
    setWindowState( (windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    activateWindow(); // for Windows
    show();
    QTimer::singleShot(500, this, SLOT(timerShot()));

}

void TrafficLights::timerShot()
{
    close();
}
