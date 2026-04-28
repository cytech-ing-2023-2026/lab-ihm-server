#include <sodium.h>
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

int hashPassword(QString password, char hash[crypto_pwhash_STRBYTES]) {
    return crypto_pwhash_str(
        hash,
        password.toUtf8().constData(),
        password.toUtf8().size(),
        crypto_pwhash_OPSLIMIT_INTERACTIVE,
        crypto_pwhash_MEMLIMIT_INTERACTIVE
    );
}

bool verifyPassword(QString password, QString hash) {
    return crypto_pwhash_str_verify(hash.toUtf8().constData(), password.toUtf8().constData(), password.toUtf8().size()) == 0;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    qInfo() << "Hello World";

    if (sodium_init() < 0) {
        return 1;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("database.sqlite");

    if (!db.open()) {
        qDebug() << "Error:" << db.lastError().text();
        return 1;
    }

    QSqlQuery query;
    query.prepare("SELECT id, name, password FROM users WHERE name = :name");
    query.bindValue(":name", "admin");

    if(!query.exec() || !query.next()) {
        qDebug() << "User not found or query failed";
        return 1;
    }

    int id = query.value(0).toInt();
    QString name = query.value(1).toString();
    QString hash = query.value(2).toString().toUtf8().constData();

    qDebug() << id << name << hash;

    QString password = "admin";
    qDebug() << "Check:" << verifyPassword(password, hash);

    return a.exec();
}
