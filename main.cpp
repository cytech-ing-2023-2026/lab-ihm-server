#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    qInfo() << "Hello World";

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("database.sqlite");

    if (!db.open()) {
        qDebug() << "Error:" << db.lastError().text();
    }

    return a.exec();
}
