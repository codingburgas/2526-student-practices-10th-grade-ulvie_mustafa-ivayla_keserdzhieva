#pragma once
#include <string>

class ITicketRepository {
public:
    virtual ~ITicketRepository() = default;
    virtual bool SaveTicket(const std::string& ticketId, const std::string& movieName) = 0;
};