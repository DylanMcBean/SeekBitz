#include <QCoreApplication>
#include <QtSql/QSqlQuery>
#include <QFileInfo>
#include <QSqlError>
#include <QtSql>
#include <QDateTime>

#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

    // hide the close button at the top of the window
    setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
    setWindowFlags(windowFlags() | Qt::CustomizeWindowHint);

    // Create and start threads
    indexThread = new QThread(this);
    printThread = new QThread(this);
    connect(indexThread, &QThread::started, this, &Dialog::startIndex);
    connect(printThread, &QThread::started, this, &Dialog::printMessages);
    indexThread->start();
    printThread->start();

    // create database connection only once
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("mydatabase.db");
    if (!db.open()) {
        enqueueMessage(QString("Failed to open database: %1").arg(db.lastError().text()));
    }
}

Dialog::~Dialog()
{
    indexThread->quit();
    indexThread->wait();
    printThread->quit();
    printThread->wait();

    delete ui;
}

void Dialog::on_pushButton_clicked()
{
    if (ui->pushButton->text() == "Cancel" && isDone == false)
    {
        ui->pushButton->setText("Yes");
        ui->label->setText("Are you sure you want to cancel the indexer? This could break the database.");
    }
    else if (ui->pushButton->text() == "Yes" || isDone == true)
    {
        this->close();
    }
}

void Dialog::startIndex(){
    isDone = false;
    enqueueMessage("Starting Indexing");

    enqueueMessage("Creating Database");
    createDatabase();

    // End
    isDone = true;
    enqueueMessage(QString()); // Enqueue a null string to signal the end of the loop
}

// Insert a directory into the database
int insert_directory(QSqlDatabase db, int parent_id, QString directory_name, QString full_path, QString permission) {
    QSqlQuery query(db);
    query.prepare("INSERT OR IGNORE INTO directories (parent_id, directory_name, full_path, permission) "
                  "VALUES (:parent_id, :directory_name, :full_path, :permission)");
    query.bindValue(":parent_id", parent_id);
    query.bindValue(":directory_name", directory_name);
    query.bindValue(":full_path", full_path);
    query.bindValue(":permission", permission);
    query.exec();
    return query.lastInsertId().toInt();
}

void Dialog::createDatabase()
{
    // Check if the database file exists and delete it
    QString dbName = "file_index.db";
    QFile dbFile(dbName);
    if (dbFile.exists()) {
        enqueueMessage("Database file already exists. Deleting it...");
        dbFile.remove();
        enqueueMessage("Database file deleted.");
    } else {
        enqueueMessage("No existing database file found.");
    }

    enqueueMessage("Initializing database connection...");
    // Open the SQLite database
    QSqlDatabase db;
    if (QSqlDatabase::contains("fileIndexConnection")) {
        db = QSqlDatabase::database("fileIndexConnection");
        enqueueMessage("Using existing database connection.");
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE", "fileIndexConnection");
        enqueueMessage("Creating a new database connection.");
    }
    db.setDatabaseName(dbName);

    if (!db.open())
    {
        enqueueMessage("Failed to open the database.");
        return;
    }
    enqueueMessage("Database opened successfully.");

    // Get the absolute path of the created database file
    QFileInfo fileInfo(db.databaseName());
    QString dbAbsolutePath = fileInfo.absoluteFilePath();
    enqueueMessage("Database created successfully at: " + dbAbsolutePath);

    QSqlQuery query(db); // Pass the db object to query

    // Create directories table
    enqueueMessage("Creating directories table...");
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS directories (
            id INTEGER PRIMARY KEY,
            parent_id INTEGER REFERENCES directories(id),
            directory_name TEXT NOT NULL,
            full_path TEXT NOT NULL,
            permission TEXT
        )
    )");
    enqueueMessage("Directories table created successfully.");

    // Create files table
    enqueueMessage("Creating files table...");
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS files (
            id INTEGER PRIMARY KEY,
            directory_id INTEGER REFERENCES directories(id),
            file_name TEXT NOT NULL,
            file_path TEXT NOT NULL,
            size INTEGER,
            permission TEXT
        )
    )");
    enqueueMessage("Files table created successfully.");

    // Create tags table
    enqueueMessage("Creating tags table...");
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS tags (
            file_id INTEGER PRIMARY KEY REFERENCES files(id),
            identifiers TEXT,
            size TEXT,
            format TEXT,
            created DATETIME,
            accessed DATETIME,
            modified DATETIME,
            owner TEXT,
            indent INTEGER
        )
    )");
    enqueueMessage("Tags table created successfully.");



    enqueueMessage("Identifying System");
    identifySystem();

    enqueueMessage("Index File System");
    indexFileSystem(db,rootDir,-1);

    db.close();
    enqueueMessage("Database closed.");

    moveToThread(QApplication::instance()->thread());
}

// Insert a file into the database
int insert_file(QSqlDatabase db, int directory_id, QString file_name, QString file_path, qint64 size = -1, QString permission = "") {
    QSqlQuery query(db);
    query.prepare("INSERT OR IGNORE INTO files (directory_id, file_name, file_path, size, permission) "
                  "VALUES (:directory_id, :file_name, :file_path, :size, :permission)");
    query.bindValue(":directory_id", directory_id);
    query.bindValue(":file_name", file_name);
    query.bindValue(":file_path", file_path);
    query.bindValue(":size", size);
    query.bindValue(":permission", permission);
    query.exec();
    return query.lastInsertId().toInt();
}

// Insert tags for a file into the database
int insert_tag(QSqlDatabase db, int file_id, qint64 file_size = -1, QString file_format = "", qint64 file_created = -1,
               qint64 file_accessed = -1, qint64 file_modified = -1, QString file_owner = "", QString file_indent = "") {
    QSqlQuery query(db);
    QDateTime creation_datetime = QDateTime::fromSecsSinceEpoch(file_created);
    QDateTime accessed_datetime = QDateTime::fromSecsSinceEpoch(file_accessed);
    QDateTime modified_datetime = QDateTime::fromSecsSinceEpoch(file_modified);
    QString identifiers = file_format.isEmpty() ? "" : "IDENTIFIERS.get(" + file_format + ", NULL)";
    query.prepare("INSERT OR IGNORE INTO tags (file_id, identifiers, size, format, created, accessed, modified, owner, indent) "
                  "VALUES (:file_id, :identifiers, :size, :format, :created, :accessed, :modified, :owner, :indent)");
    query.bindValue(":file_id", file_id);
    query.bindValue(":identifiers", identifiers);
    query.bindValue(":size", file_size);
    query.bindValue(":format", file_format);
    query.bindValue(":created", creation_datetime);
    query.bindValue(":accessed", accessed_datetime);
    query.bindValue(":modified", modified_datetime);
    query.bindValue(":owner", file_owner);
    query.bindValue(":indent", file_indent);
    query.exec();
    return query.lastInsertId().toInt();
}

// Get the size category for a file
QString get_file_size_category(qint64 file_size_in_bytes) {
    const qint64 BYTE = 1LL;
    const qint64 KILOBYTE = 1024 * BYTE;
    const qint64 MEGABYTE = 1024 * KILOBYTE;
    const qint64 GIGABYTE = 1024 * MEGABYTE;

    if (file_size_in_bytes == 0) {
        return "Empty";
    } else if (0 < file_size_in_bytes && file_size_in_bytes <= 16 * KILOBYTE) {
        return "Tiny";
    } else if (16 * KILOBYTE < file_size_in_bytes && file_size_in_bytes <= 1 * MEGABYTE) {
        return "Small";
    } else if (1 * MEGABYTE < file_size_in_bytes && file_size_in_bytes <= 128 * MEGABYTE) {
        return "Medium";
    } else if (128 * MEGABYTE < file_size_in_bytes && file_size_in_bytes <= 1 * GIGABYTE) {
        return "Large";
    } else if (1 * GIGABYTE < file_size_in_bytes && file_size_in_bytes <= 4 * GIGABYTE) {
        return "Huge";
    } else {
        return "Gigantic";
    }
}

// Get the depth of a file path
int get_folder_depth(QString file_path) {
    int folder_depth = 0;
    QFileInfo file_info(file_path);
    while (file_path != file_info.dir().path()) {
        folder_depth += 1;
        file_path = file_info.dir().path();
        file_info = QFileInfo(file_path);
    }
    return folder_depth;
}

void Dialog::identifySystem() {
#ifdef _WIN32
    enqueueMessage("Running on Windows Machine");
    rootDir = "C:\\";
#elif __linux__
    enqueueMessage("Running on Linux Machine");
    rootDir = "/";
#elif __APPLE__
    enqueueMessage("Running on macOS Machine");
    rootDir = "/";
#else
    enqueueMessage("Unknown OS, defaulting to unix style");
    rootDir = "/";
#endif
}

int Dialog::indexFileSystem(QSqlDatabase db, const QString& localRootDir, qint64 parent_id) {
    try {
        if (parent_id == -1) {
            // Insert root directory
            QFileInfo rootInfo(localRootDir);
            QString permissionsStr = QString::number(rootInfo.permissions(), 8);
            qint64 root_id = insert_directory(db, -1, rootInfo.fileName(), rootInfo.absoluteFilePath(), permissionsStr);
            parent_id = root_id;
            enqueueMessage("Indexed: " + rootInfo.absoluteFilePath());
        } else {
            // Insert subdirectory
            QFileInfo subInfo(localRootDir);
            QString permissionsStr = QString::number(subInfo.permissions(), 8);
            qint64 sub_id = insert_directory(db, parent_id, subInfo.fileName(), subInfo.absoluteFilePath(), permissionsStr);
            parent_id = sub_id;
            enqueueMessage("Indexed: " + subInfo.absoluteFilePath());
        }

        // Recursively loop over all files in root directory
        QStringList fileFilters;
        fileFilters << "*";

        QFileInfoList fileInfoList = QDir(localRootDir).entryInfoList(fileFilters, QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System | QDir::Dirs);
        foreach (const QFileInfo& fileInfo, fileInfoList) {
            QString filePath = fileInfo.absoluteFilePath();
            if (fileInfo.isSymLink()) {
                enqueueMessage("Skipping Symlink: " + filePath);
                continue;
            }
            if (fileInfo.isDir()) {
                indexFileSystem(db, filePath, parent_id);
            }
        }
    } catch (std::exception& e) {
        enqueueMessage(QString("Error: %1").arg(e.what()));
    }

    return parent_id;
}


void Dialog::enqueueMessage(const QString &message)
{
    QMutexLocker locker(&mutex);
    messageQueue.enqueue(message);
    messageQueueNotEmpty.wakeAll();
}

void Dialog::printMessages()
{
    while (true)
    {
        QMutexLocker locker(&mutex);

        while (messageQueue.isEmpty())
            messageQueueNotEmpty.wait(&mutex);

        QString message = messageQueue.dequeue();
        locker.unlock();

        if (message.isNull()) break;

        ui->textEdit->append(message);
        qDebug() << message;
    }
}
