#include "server.hpp"

#include <sodium.h>

#include <QDebug>
#include <QList>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSqlError>

static int hashPassword(QString password, char hash[crypto_pwhash_STRBYTES]) {
    return crypto_pwhash_str(hash, password.toUtf8().constData(), password.toUtf8().size(),
                             crypto_pwhash_OPSLIMIT_INTERACTIVE,
                             crypto_pwhash_MEMLIMIT_INTERACTIVE);
}

static bool verifyPassword(QString password, QString hash) {
    return crypto_pwhash_str_verify(hash.toUtf8().constData(), password.toUtf8().constData(),
                                    password.toUtf8().size()) == 0;
}

static void addMessageToDatabase(int senderId, int receiverId, QString content) {
    QSqlDatabase::database("QSQLITE");

    QSqlQuery query;
    query.prepare(
        "INSERT INTO messages (sender, receiver, content) VALUES (:sender, :receiver, :content)");
    query.bindValue(":sender", senderId);
    if (receiverId == -1) {
        query.bindValue(":receiver", QVariant());
    } else {
        query.bindValue(":receiver", receiverId);
    }
    query.bindValue(":content", content);
    if (!query.exec()) {
        qDebug() << "Failed to insert message into database:" << query.lastError().text();
    }
}

static QList<MessageInfo> getMessagesForUser(int userId) {
    QSqlDatabase::database("QSQLITE");

    QSqlQuery query;
    query.prepare(
        "SELECT sender, receiver, content FROM messages WHERE sender = :userId OR receiver = :userId OR receiver IS NULL ORDER BY created_at DESC LIMIT 1000");
    query.bindValue(":userId", userId);

    QList<MessageInfo> messages;
    if (query.exec()) {
        while (query.next()) {
            MessageInfo message;
            message.sender = query.value(0).toInt();
            message.receiver = query.value(1).isNull() ? -1 : query.value(1).toInt();
            message.content = query.value(2).toString();
            messages.append(message);
        }
    } else {
        qDebug() << "Failed to retrieve messages from database:" << query.lastError().text();
    }

    return messages;
}


static QList<UserInfo> getAllUsers() {
    QSqlDatabase::database("QSQLITE");

    QSqlQuery query;
    query.prepare("SELECT id, name FROM users");

    QList<UserInfo> users;
    if (query.exec()) {
        while (query.next()) {
            UserInfo user;
            user.userId = query.value(0).toInt();
            user.name = query.value(1).toString();
            users.append(user);
        }
    } else {
        qDebug() << "Failed to retrieve users from database:" << query.lastError().text();
    }

    return users;
}

static bool deleteUser(int userId) {
    QSqlDatabase::database("QSQLITE");

    QSqlQuery query;

    query.prepare("SELECT admin FROM users WHERE id = :userId");

    query.bindValue(":userId", userId);
    if (!query.exec() || !query.next()) {
        qDebug() << "User not found or query failed";
        return false;
    }

    bool isAdmin = query.value(0).toBool();
    if (isAdmin) {
        qDebug() << "Cannot delete admin user";
        return false;
    }

    query.prepare("DELETE FROM messages WHERE sender = :userId OR receiver = :userId");
    query.bindValue(":userId", userId);
    if (!query.exec()) {
        qDebug() << "Failed to delete messages from database:" << query.lastError().text();
        return false;
    }

    query.prepare("DELETE FROM users WHERE id = :userId");
    query.bindValue(":userId", userId);
    if (!query.exec()) {
        qDebug() << "Failed to delete user from database:" << query.lastError().text();
        return false;
    }

    return true;
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

void Server::broadcast(ServerMessage message) {
    for(auto session : this->getSessions()) {
        if(session->hasUserId()) session->sendMessage(message);
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
                QList<UserInfo> users = getAllUsers();
                QList<MessageInfo> messages = getMessagesForUser(userId);
                session->sendMessage({ServerMessageId::HISTORY, MessageHistory{users, messages}});
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
                    this->broadcast(message);
                    addMessageToDatabase(session->getUserId(), send.receiver, send.content);
                } else {
                    QList<std::shared_ptr<Session>> receiverSessions =
                        this->getSessionsByUser(send.receiver);
                    if (receiverSessions.isEmpty()) {
                        session->sendMessage({ServerMessageId::ERROR, "receiver not connected"});
                    } else {
                        if(session->getUserId() != send.receiver) {
                            QList<std::shared_ptr<Session>> senderSessions = this->getSessionsByUser(session->getUserId());
                            for (auto senderSession : senderSessions)
                                senderSession->sendMessage(message);
                        }
                        for (auto receiverSession : receiverSessions)
                            receiverSession->sendMessage(message);

                        addMessageToDatabase(session->getUserId(), send.receiver, send.content);
                    }
                }
            } else {
                qInfo("Not logged in");
                session->sendMessage({ServerMessageId::ERROR, "need to be logged in"});
            }
            break;
        }
        case ClientMessageId::DELETE: {
            qInfo() << "Delete user request";
            if (session->hasUserId()) {
                int deleteContent = std::get<int>(message.content);

                QList<std::shared_ptr<Session>> sessionsToDelete = this->getSessionsByUser(deleteContent);
                for (auto s : sessionsToDelete) s->close();

                if (deleteUser(deleteContent)) {
                    this->broadcast({ServerMessageId::USER_DELETED, deleteContent});
                } else {
                    session->sendMessage({ServerMessageId::ERROR, "Failed to delete user"});
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