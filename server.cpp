#include "server.hpp"

#include <sodium.h>

#include <QDebug>
#include <QList>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTcpServer>
#include <QTcpSocket>

static int hashPassword(QString password, char hash[crypto_pwhash_STRBYTES]) {
    return crypto_pwhash_str(hash, password.toUtf8().constData(), password.toUtf8().size(),
                             crypto_pwhash_OPSLIMIT_INTERACTIVE,
                             crypto_pwhash_MEMLIMIT_INTERACTIVE);
}

static bool verifyPassword(QString password, QString hash) {
    return crypto_pwhash_str_verify(hash.toUtf8().constData(), password.toUtf8().constData(),
                                    password.toUtf8().size()) == 0;
}

Server::Server() : sessions() {}

QList<std::shared_ptr<Session>> Server::getSessions() { return sessions; }

QList<std::shared_ptr<Session>> Server::getSessionsByUser(int userId) {
    QList<std::shared_ptr<Session>> result;
    for (std::shared_ptr<Session> session : sessions) {
        if (session.get()->getUserId() == userId) result.append(session);
    }

    return result;
}

void Server::addSession(std::shared_ptr<Session> session) {
    qInfo() << "New session";
    sessions.append(session);
}

void Server::removeSession(std::shared_ptr<Session> session) { sessions.removeOne(session); }

int Server::login(QString username, QString password) {
    QSqlDatabase::database("QSQLITE");

    QSqlQuery query;
    query.prepare("SELECT id, name, password FROM users WHERE name = :name");
    query.bindValue(":name", username);

    if (!query.exec() || !query.next()) {
        qDebug() << "User not found or query failed";
        return -1;
    }

    int userId = query.value(0).toInt();
    QString hash = query.value(2).toString();

    if (verifyPassword(password, hash)) {
        qDebug() << "Login successful for user:" << username;
        return userId;
    } else {
        qDebug() << "Invalid password for user:" << username;
        return -1;
    }
}

void Server::onReceive(std::shared_ptr<Session> session, ClientMessage message) {
    switch (message.id) {
        case ClientMessageId::LOGIN: {
            ClientLoginContent login = std::get<ClientLoginContent>(message.content);
            int userId = this->login(login.name, login.password);
            if (userId == -1)
                session->sendMessage({ServerMessageId::LOGIN_FAILURE, ""});
            else {
                session->setUserId(userId);
                session->sendMessage({ServerMessageId::LOGIN_SUCCESS, userId});
            }
            break;
        }
        case ClientMessageId::SEND: {
            ClientSendContent send = std::get<ClientSendContent>(message.content);
            qInfo() << "Session connected: " << session->hasUserId();
            if (session->hasUserId()) {
              ServerMessage message = {
                            ServerMessageId::MESSAGE,
                            MessageInfo{session->getUserId(), send.receiver, send.content}};
                if (send.receiver == -1) {
                  for(auto s : this->getSessions()) s->sendMessage(message);
                } else {
                    QList<std::shared_ptr<Session>> receiverSessions =
                        this->getSessionsByUser(send.receiver);
                    if (receiverSessions.isEmpty()) {
                        session->sendMessage({ServerMessageId::ERROR, "receiver not connected"});
                    } else {
                        QList<std::shared_ptr<Session>> senderSessions =
                            this->getSessionsByUser(session->getUserId());
                        for (auto senderSession : senderSessions) senderSession->sendMessage(message);
                        for (auto receiverSession : receiverSessions) receiverSession->sendMessage(message);
                    }
                }
            } else {
                qInfo("Not logged in");
                session->sendMessage({ServerMessageId::ERROR, "need to be logged in"});
            }
            break;
        }

        case ClientMessageId::ERROR: {
            QString content = std::get<QString>(message.content);
            qDebug() << "Received bad message" << content;
            session->sendMessage({ServerMessageId::ERROR, "Invalid message: " + content});
            break;
        }
    }
}