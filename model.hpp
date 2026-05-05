/**
* @file model.hpp  
* @brief Defines the data structures and functions for handling client and server messages, as well as the Session class for managing client connections.
*/
#pragma once

#include <QString>
#include <variant>
#include <QTcpSocket>
#include <QJsonObject>

/**
 * @struct MessageInfo
 * @brief Represents a message sent from one user to another, containing the sender's ID, receiver's ID, and the message content.
*/
typedef struct {
    int sender;
    int receiver;
    QString content;
} MessageInfo;

/**
 * @struct UserInfo
 * @brief Represents a user's information, including their unique ID and name.
*/
typedef struct {
  int userId;
  QString name;
} UserInfo;

/**
 * @struct MessageHistory
 * @brief Represents the message history for a user, containing a list of users and their corresponding messages.
*/
typedef struct {
    QList<UserInfo> users;
    QList<MessageInfo> messages;
} MessageHistory;

/**
 * @enum ClientMessageId
 * @brief Enumerates the types of messages that can be sent from the client to the server, including login, send, delete, and error messages.
*/
enum class ClientMessageId {
    LOGIN,
    SEND,
    DELETE,
    ERROR
};

/**
 * @struct ClientLoginContent
 * @brief Represents the content of a login message from the client, containing the user's name and password.
*/
typedef struct {
    QString name;
    QString password;
} ClientLoginContent;

/**
 * @struct ClientSendContent
 * @brief Represents the content of a send message from the client, containing the receiver's ID and the message content.
*/
typedef struct {
    int receiver;
    QString content;
} ClientSendContent;

using ClientMessageContent = std::variant<ClientLoginContent, ClientSendContent, int, QString>;

/**
 * @struct ClientMessage
 * @brief Represents a message sent from the client to the server, containing the message ID and its corresponding content.
*/
typedef struct {
    ClientMessageId id;
    ClientMessageContent content;
} ClientMessage;

ClientMessage errorClientMessage(QString json);
ClientMessage errorClientMessage(QJsonObject json);
ClientMessage readClientMessage(QJsonObject json);

/**
 * @enum ServerMessageId
 * @brief Enumerates the types of messages that can be sent from the server to the client, including login success, login failure, message delivery, history retrieval, user deletion, and error messages.
*/
enum class ServerMessageId {
    LOGIN_SUCCESS,
    LOGIN_FAILURE,
    MESSAGE,
    HISTORY,
    USER_DELETED,
    ERROR
};

using ServerMessageContent = std::variant<QString, int, MessageInfo, MessageHistory>;

/**
 * @struct ServerMessage
 * @brief Represents a message sent from the server to the client, containing the message ID and its corresponding content.
*/
typedef struct {
    ServerMessageId id;
    ServerMessageContent content;
} ServerMessage;

QJsonObject writeServerMessage(ServerMessage message);

/**
 * @class Session
 * @brief Manages a client's session, including their user ID and the socket connection. Provides methods for sending messages to the client and closing the session.
*/
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