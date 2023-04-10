#ifndef INDEXWORKER_H
#define INDEXWORKER_H

#include <QObject>
#include <QSqlDatabase>
#include <QString>

class Dialog;

class IndexWorker : public QObject
{
    Q_OBJECT

public:
    explicit IndexWorker(QObject *parent = nullptr);

public slots:
    void processIndexFileSystem(Dialog *dialog, QSqlDatabase db, const QString& localRootDir, qint64 parent_id);

signals:
    void enqueueMessage(const QString &message);

};

#endif // INDEXWORKER_H
