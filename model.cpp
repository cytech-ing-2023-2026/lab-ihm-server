#include <QJsonObject>
#include <QJsonDocument>
#include <QTcpSocket>
#include "model.hpp"

Session::Session(QTcpSocket *s) : userId(-1), socket(s) {}

int Session::getUserId() {
    return userId; 
}

void Session::setUserId(int id) {
    userId = id;
}

bool Session::hasUserId() {
    return userId != -1;
}

void Session::close() {
    socket->close();
}

void Session::sendMessage(ServerMessage message) {
    QJsonDocument document(writeServerMessage(message));
    socket->write(document.toJson(QJsonDocument::JsonFormat::Compact) + "\n");
}

ClientMessage errorClientMessage(QString json) {
    return {ClientMessageId::ERROR, json};
}
ClientMessage errorClientMessage(QJsonObject json) {
    return errorClientMessage(QJsonDocument(json).toJson(QJsonDocument::JsonFormat::Compact));
}

ClientMessage readClientMessage(QJsonObject json) {
    QJsonValueRef type = json["type"];
    QJsonValueRef content = json["content"];
    if(!type.isString() || !content.isObject()) return errorClientMessage(json);
    QString typeStr = type.toString();
    QJsonObject contentObj = content.toObject();
    if(typeStr == "login") {
        QJsonValueRef name = contentObj["name"];
        QJsonValueRef password = contentObj["password"];
        if(!name.isString() || !password.isString()) return errorClientMessage(json);

        return {ClientMessageId::LOGIN, ClientLoginContent{name.toString(), password.toString()}};
    } else if(typeStr == "send") {
        QJsonValueRef receiver = contentObj["receiver"];
        QJsonValueRef message = contentObj["message"];
        if(!receiver.isDouble() || !message.isString()) return errorClientMessage(json);
        return {ClientMessageId::SEND, ClientSendContent{receiver.toInt(), message.toString()}};
    } else {
        return errorClientMessage(json);
    }
}

QJsonObject writeServerMessage(ServerMessage message) {
    QJsonObject json;
    switch(message.id) {
        case ServerMessageId::LOGIN_SUCCESS: {
            json["type"] = "login_success";
            json["user_id"] = std::get<int>(message.content);
            break;
        }
        case ServerMessageId::LOGIN_FAILURE: {
            json["type"] = "login_failure";
            break;
        }
        case ServerMessageId::MESSAGE: {
            MessageInfo info = std::get<MessageInfo>(message.content);
            json["type"] = "message";
            json["sender"] = info.sender;
            json["receiver"] = info.receiver;
            json["content"] = info.content;
            break;
        }
        case ServerMessageId::ERROR: {
            json["type"] = "error";
            json["content"] = std::get<QString>(message.content);
            break;
        }  

    }
    return json;
}