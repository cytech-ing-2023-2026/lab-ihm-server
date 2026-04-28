#pragma once

#include <QString>

class Session {
public:
    Session();

    int getUserId();
    void setUserId(int userId);
    bool hasUserId();

private:
    int userId;
};

typedef struct {
    int sender;
    int receiver;
    QString content;
} MessageInfo;

typedef struct {
  int userId;
  QString name;
} UserInfo;