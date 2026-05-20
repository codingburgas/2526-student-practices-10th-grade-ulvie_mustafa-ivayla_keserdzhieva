#pragma once
#include "ITicketRepository.h"
#include <windows.h>
#include <sql.h>
#include <sqlext.h>

class SqlTicketRepository : public ITicketRepository {
private:
    SQLHENV hEnv = SQL_NULL_HENV;
    SQLHDBC hDbc = SQL_NULL_HDBC;
    void FreeResources();

public:
    SqlTicketRepository();
    ~SqlTicketRepository() override;
    bool SaveTicket(const std::string& ticketId, const std::string& movieName) override;
};