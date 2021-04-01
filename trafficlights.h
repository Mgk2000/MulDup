#ifndef TRAFFICLIGHTS_H
#define TRAFFICLIGHTS_H
#include <QSoundEffect>
#include <QWidget>
#include <QLabel>
class TrafficLights : public QWidget
{
    Q_OBJECT
public:
    explicit TrafficLights(QWidget *parent = nullptr);
    QLabel colorLabel;
    QColor color;
    QSoundEffect soundYes, soundNo;
    void showLight(int exists);
signals:
public slots:
    void timerShot();
};

#endif // TRAFFICLIGHTS_H
