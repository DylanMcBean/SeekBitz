#ifndef INDEXHELPER_H
#define INDEXHELPER_H

#include <QSqlDatabase>
#include <QObject>
#include "dialog.h"

class IndexHelper : public QObject
{
    Q_OBJECT
public:
    explicit IndexHelper(QObject *parent = nullptr);

signals:
    void updateLabel(const QString& text);
    void finished();

public slots:
    void run();

public:
    Dialog* dialog;

private:
    bool createDatabase(const QString& dbFilename);
    int insertDirectory(QSqlDatabase& db, int parent_id, const QString& directory_name, const QString& full_path, const QString& permission);
};

#endif // INDEXHELPER_H
