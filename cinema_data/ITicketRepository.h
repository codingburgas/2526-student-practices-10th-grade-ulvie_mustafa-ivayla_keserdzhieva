#pragma once
#include <string>

// The Business layer will talk to this interface, 
// completely unaware of *how* the data is actually stored.
class ITicketRepository {
public:
    virtual ~ITicketRepository() = default;
    virtual bool SaveTicket(const std::string& ticketId, const std::string& movieName) = 0;
};