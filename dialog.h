#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QThread>
#include <QMutex>
#include <QtSql/QSqlDatabase>
#include <QQueue>
#include <QWaitCondition>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr);
    ~Dialog();

private slots:
    void on_pushButton_clicked();

private:
    void startIndex();
    void createDatabase();
    void printMessages();
    void identifySystem();
    void enqueueMessage(const QString &message);
    int indexFileSystem(QSqlDatabase db, const QString& rootDir, qint64 parent_id);

    Ui::Dialog *ui;
    QThread *indexThread;
    QThread *printThread;
    QSqlDatabase db;
    QMutex mutex;
    QQueue<QString> messageQueue;
    QWaitCondition messageQueueNotEmpty;
    QString rootDir;

    bool isDone;
};

#endif // DIALOG_H
