#include "indexworker.h"
#include "dialog.h"

IndexWorker::IndexWorker(QObject *parent) : QObject(parent)
{

}

void IndexWorker::processIndexFileSystem(Dialog *dialog, QSqlDatabase db, const QString& localRootDir, qint64 parent_id)
{
    qint64 result = dialog->indexFileSystem(db, localRootDir, parent_id);
    emit enqueueMessage("Finished indexing file system");
}
