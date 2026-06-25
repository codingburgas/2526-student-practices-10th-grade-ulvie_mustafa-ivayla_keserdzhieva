#pragma once
#include <string>

struct TicketRecord {
    std::string ticketId;
    std::string title;
    std::string hall;
    std::string seat;
    std::string fmt;
    std::string showTime;
    int         day        = 0;
    int         month      = 0;
    int         year       = 0;
    std::string ticketType;
    std::string city;
    std::string location;
};

class ITicketRepository {
public:
    virtual ~ITicketRepository() = default;
    virtual bool SaveTicket(const TicketRecord& ticket) = 0;
};
