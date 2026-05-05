#pragma once

#include <QList>
#include "model.hpp"

class Server {
  public:
    Server();
    QList<std::shared_ptr<Session>> getSessions();
    QList<std::shared_ptr<Session>> getSessionsByUser(int userId);
    void removeSession(std::shared_ptr<Session> session);
    void addSession(std::shared_ptr<Session> session);
    int login(QString name, QString password);

    void onReceive(std::shared_ptr<Session> session, ClientMessage message);

  private:
    QList<std::shared_ptr<Session>> sessions;
};