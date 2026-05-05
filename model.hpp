#pragma once

#include <QString>
#include <variant>
#include <QTcpSocket>
#include <QJsonObject>

typedef struct {
    int sender;
    int receiver;
    QString content;
} MessageInfo;

typedef struct {
  int userId;
  QString name;
} UserInfo;

enum class ClientMessageId {
    LOGIN,
    SEND,
    ERROR
};

typedef struct {
    QString name;
    QString password;
} ClientLoginContent;

typedef struct {
    int receiver;
    QString content;
} ClientSendContent;

using ClientMessageContent = std::variant<ClientLoginContent, ClientSendContent, QString>;

typedef struct {
    ClientMessageId id;
    ClientMessageContent content;
} ClientMessage;

ClientMessage errorClientMessage(QString json);
ClientMessage errorClientMessage(QJsonObject json);
ClientMessage readClientMessage(QJsonObject json);


enum class ServerMessageId {
    LOGIN_SUCCESS,
    LOGIN_FAILURE,
    MESSAGE,
    ERROR
};

using ServerMessageContent = std::variant<QString, int, MessageInfo>;

typedef struct {
    ServerMessageId id;
    ServerMessageContent content;
} ServerMessage;

QJsonObject writeServerMessage(ServerMessage message);

class Session {
public:
    Session(QTcpSocket *socket);

    int getUserId();
    void setUserId(int userId);
    bool hasUserId();

    void close();
    void sendMessage(ServerMessage message);

private:
    int userId;
    QTcpSocket *socket;
};