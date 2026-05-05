#include <sodium.h>

#include <QCoreApplication>
#include <QJsonDocument>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>

#include "model.hpp"
#include "server.hpp"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
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

    Server server;
    QTcpServer tcpServer(&app);

    QObject::connect(&tcpServer, &QTcpServer::newConnection, &app, [&server, &tcpServer] {
        while (tcpServer.hasPendingConnections()) {
            QTcpSocket* client = tcpServer.nextPendingConnection();
            if (!client) {
                qWarning() << "nextPendingConnection returned nullptr.";
                continue;
            }

            auto session = std::make_shared<Session>(client);
            server.addSession(session);

            QObject::connect(client, &QTcpSocket::readyRead, &tcpServer, [&server, client, session] {
                while (client->canReadLine()) {
                    QByteArray line = client->readLine();
                    qDebug() << "Received line" << line;
                    QJsonDocument document = QJsonDocument::fromJson(line);
                    ClientMessage message = readClientMessage(document.object());
                    server.onReceive(session, message);
                }
            });
        }
    });



    if (!tcpServer.listen(QHostAddress("0.0.0.0"), 8080)) {
        qCritical() << "Unable to listen";
    }

    qInfo("Listening to port");

    return app.exec();
}
