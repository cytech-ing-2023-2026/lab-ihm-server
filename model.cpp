#include "model.hpp"

Session::Session() : userId(-1) {}

int Session::getUserId() {
    return userId; 
}

void Session::setUserId(int id) {
    userId = id;
}

bool Session::hasUserId() {
    return userId != -1;
}