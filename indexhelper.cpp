#include "indexhelper.h"
#include <QThread>
#include <QSqlError>
#include <QSqlQuery>

IndexHelper::IndexHelper(QObject *parent) : QObject(parent)
{

}

void IndexHelper::run()
{
    QString dbFilename = "file_index.db";

    // Update the label in the Dialog UI
    emit updateLabel("Creating database...");

    if (createDatabase(dbFilename)) {
        // Update the label in the Dialog UI
        emit updateLabel("Database created successfully.");

        // Insert a random directory
        QSqlDatabase db = QSqlDatabase::database();
        int parent_id = 0; // Parent directory ID, set to 0 for root directory
        QString directory_name = "Random Directory";
        QString full_path = "/path/to/random/directory";
        QString permission = "rwxrwxrwx";

        int directory_id = insertDirectory(db, parent_id, directory_name, full_path, permission);
        if (directory_id != -1) {
            emit updateLabel(QString("Inserted directory with ID %1").arg(directory_id));
        }
    }

    // Emit the finished signal
    emit finished();
}

bool IndexHelper::createDatabase(const QString& dbFilename)
{
    // Create a new database connection
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbFilename);

    // Open the database connection
    if (!db.open()) {
        emit updateLabel(QString("Failed to open database: %1").arg(db.lastError().text()));
        return false;
    }

    QSqlQuery query(db);

    // Create directories table
    if (!query.exec("CREATE TABLE IF NOT EXISTS directories ("
                    "id INTEGER PRIMARY KEY,"
                    "parent_id INTEGER REFERENCES directories(id),"
                    "directory_name TEXT NOT NULL,"
                    "full_path TEXT NOT NULL,"
                    "permission TEXT)"))
    {
        emit updateLabel(QString("Failed to create directories table: %1").arg(query.lastError().text()));
        return false;
    }

    // Create files table with updated schema
    if (!query.exec("CREATE TABLE IF NOT EXISTS files ("
                    "id INTEGER PRIMARY KEY,"
                    "directory_id INTEGER REFERENCES directories(id),"
                    "file_name TEXT NOT NULL,"
                    "file_path TEXT NOT NULL,"
                    "size INTEGER,"
                    "permission TEXT)"))
    {
        emit updateLabel(QString("Failed to create files table: %1").arg(query.lastError().text()));
        return false;
    }

    // Create tags table with updated schema
    if (!query.exec("CREATE TABLE IF NOT EXISTS tags ("
                    "file_id INTEGER PRIMARY KEY REFERENCES files(id),"
                    "identifiers TEXT,"
                    "size TEXT,"
                    "format TEXT,"
                    "created DATETIME,"
                    "accessed DATETIME,"
                    "modified DATETIME,"
                    "owner TEXT,"
                    "indent INTEGER)"))
    {
        emit updateLabel(QString("Failed to create tags table: %1").arg(query.lastError().text()));
        return false;
    }

    // Close the database connection
    db.close();

    return true;
}

int IndexHelper::insertDirectory(QSqlDatabase& db, int parent_id, const QString& directory_name, const QString& full_path, const QString& permission)
{
    QSqlQuery query(db);
    query.prepare("INSERT OR IGNORE INTO directories (parent_id, directory_name, full_path, permission) VALUES (?, ?, ?, ?)");
    query.addBindValue(parent_id);
    query.addBindValue(directory_name);
    query.addBindValue(full_path);
    query.addBindValue(permission);

    if (!query.exec()) {
        qWarning() << "Failed to insert directory:" << query.lastError().text();
        return -1;
    }

    return query.lastInsertId().toInt();
}
