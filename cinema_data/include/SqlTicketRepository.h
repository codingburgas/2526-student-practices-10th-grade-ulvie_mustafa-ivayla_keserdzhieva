#pragma once
#include "ITicketRepository.h"
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <string>

class SqlTicketRepository : public ITicketRepository {
    SQLHENV     hEnv = SQL_NULL_HENV;
    SQLHDBC     hDbc = SQL_NULL_HDBC;
    std::string m_lastError;
    bool Connect();
    void Disconnect();
public:
    SqlTicketRepository();
    ~SqlTicketRepository() override;
    bool SaveTicket(const TicketRecord& ticket) override;
    const std::string& GetLastError() const { return m_lastError; }
};
