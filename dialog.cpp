#include "dialog.h"
#include "ui_dialog.h"
#include "indexhelper.h"
#include <QThread>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

    // Create a new thread
    QThread* thread = new QThread();

    // Create an instance of indexhelper
    IndexHelper* indexhelper = new IndexHelper();

    // Move the indexhelper instance to the new thread
    indexhelper->moveToThread(thread);

    // Connect the signals and slots for the thread and indexhelper instance
    connect(thread, &QThread::started, indexhelper, &IndexHelper::run);
    connect(indexhelper, &IndexHelper::finished, thread, &QThread::quit);
    connect(indexhelper, &IndexHelper::finished, indexhelper, &IndexHelper::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    connect(indexhelper, &IndexHelper::updateLabel, this, &Dialog::updateLabel);
    connect(indexhelper, &IndexHelper::finished, this, &Dialog::onIndexingFinished);

    // Start the thread
    thread->start();

    // Hide the close button at the top of the window
    setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
    setWindowFlags(windowFlags() | Qt::CustomizeWindowHint);

    // Set isDone to false
    isDone = false;
}

Dialog::~Dialog()
{
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

void Dialog::onIndexingFinished()
{
    // Set isDone to true
    isDone = true;
}

void Dialog::updateLabel(const QString& text)
{
    ui->textEdit->append(text);
}
