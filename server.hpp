/**
 *@file server.hpp
 *@brief Server class definition
*/
#pragma once

#include <QList>
#include "model.hpp"

/**
 *@class Server
 *@brief The Server class manages client sessions and handles communication between clients and the server.
*/
class Server {
  public:
    /**
      *@brief Constructor for the Server class
    */
    Server();

    /**
     * @brief Gets a list of all active sessions.
     * @return A QList of shared pointers to Session objects representing all active sessions. 
    */
    QList<std::shared_ptr<Session>> getSessions();

    /**
      * @brief Gets a list of sessions associated with a specific user ID.
      * @param userId The ID of the user whose sessions are to be retrieved.
      * @return A QList of shared pointers to Session objects representing the sessions associated with the specified user ID.
      */
    QList<std::shared_ptr<Session>> getSessionsByUser(int userId);

    /**
      * @brief Removes a session from the server.
      * @param session A shared pointer to the Session object that is to be removed from the server.
      */
    void removeSession(std::shared_ptr<Session> session);

    /**
      * @brief Adds a new session to the server.
      * @param session A shared pointer to the Session object that is to be added to the server.
    */
    void addSession(std::shared_ptr<Session> session);

    /**
      * @brief Authenticates a user based on their username and password.
      * @param name The username of the user attempting to log in.
      */
    int login(QString name, QString password);
    /**
      * @brief Broadcasts a message to all connected clients.
      * @param message The ServerMessage object containing the message to be broadcasted to all clients
    */
    void broadcast(ServerMessage message);

    /**
      * @brief Handles incoming messages from clients.
      * @param session A shared pointer to the Session object representing the client session from which the message was received.
      * @param message The ClientMessage object containing the message received from the client.
    */
    void onReceive(std::shared_ptr<Session> session, ClientMessage message);

  private:
    QList<std::shared_ptr<Session>> sessions;
};