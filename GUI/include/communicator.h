#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include <QObject>

class Communicator : public QObject
{
    Q_OBJECT
public:
    explicit Communicator(QObject* parent = nullptr);

signals:
    void setError(int row);
    void showError(QString msg);
    void done(int row, double pro, quint64 time);
    void allDone(quint64 totalTime);
    void logInfo(QString info);
    void updateProgress(double v, double elpsed, double remaining);
};

#endif // COMMUNICATOR_H
