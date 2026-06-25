#include "../include/SqlTicketRepository.h"
#include <string>

static const char* TK_DRIVERS[] = {
    "ODBC Driver 18 for SQL Server",
    "ODBC Driver 17 for SQL Server",
    "ODBC Driver 13 for SQL Server",
    "SQL Server",
    nullptr
};
static const char* TK_SERVER = "(localdb)\\MSSQLLocalDB";
static const char* TK_DB     = "CinemaDB";

static std::string tkOdbcError(SQLSMALLINT type, SQLHANDLE h) {
    SQLCHAR state[6] = {}, msg[512] = {};
    SQLINTEGER native = 0; SQLSMALLINT len = 0;
    if (SQL_SUCCEEDED(SQLGetDiagRecA(type, h, 1, state, &native, msg, sizeof(msg), &len)))
        return std::string((char*)state) + ": " + std::string((char*)msg, len);
    return "Unknown ODBC error";
}

SqlTicketRepository::SqlTicketRepository() {
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
}

SqlTicketRepository::~SqlTicketRepository() {
    Disconnect();
    if (hEnv != SQL_NULL_HENV) { SQLFreeHandle(SQL_HANDLE_ENV, hEnv); hEnv = SQL_NULL_HENV; }
}

bool SqlTicketRepository::Connect() {
    SQLCHAR out[1024]; SQLSMALLINT outLen;
    for (int i = 0; TK_DRIVERS[i]; ++i) {
        if (hDbc != SQL_NULL_HDBC) { SQLFreeHandle(SQL_HANDLE_DBC, hDbc); hDbc = SQL_NULL_HDBC; }
        SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
        SQLSetConnectAttr(hDbc, SQL_ATTR_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);
        std::string cs = std::string("DRIVER={") + TK_DRIVERS[i] + "};"
                       + "SERVER=" + TK_SERVER + ";DATABASE=" + TK_DB + ";Trusted_Connection=yes;";
        SQLRETURN r = SQLDriverConnectA(hDbc, nullptr, (SQLCHAR*)cs.c_str(), SQL_NTS,
                                        out, sizeof(out), &outLen, SQL_DRIVER_NOPROMPT);
        if (SQL_SUCCEEDED(r)) { m_lastError = ""; return true; }
    }
    m_lastError = "Cannot connect to CinemaDB. " + tkOdbcError(SQL_HANDLE_DBC, hDbc);
    return false;
}

void SqlTicketRepository::Disconnect() {
    if (hDbc != SQL_NULL_HDBC) {
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        hDbc = SQL_NULL_HDBC;
    }
}

bool SqlTicketRepository::SaveTicket(const TicketRecord& t) {
    if (!Connect()) return false;

    SQLHSTMT hStmt = SQL_NULL_HSTMT;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    SQLPrepareA(hStmt,
        (SQLCHAR*)"INSERT INTO Tickets "
                  "(TicketId,Title,Hall,Seat,Format,ShowTime,ShowDay,ShowMonth,ShowYear,TicketType,City,Location)"
                  " VALUES (?,?,?,?,?,?,?,?,?,?,?,?)",
        SQL_NTS);

    SQLLEN l1  = (SQLLEN)t.ticketId.size(),   l2  = (SQLLEN)t.title.size(),
           l3  = (SQLLEN)t.hall.size(),        l4  = (SQLLEN)t.seat.size(),
           l5  = (SQLLEN)t.fmt.size(),         l6  = (SQLLEN)t.showTime.size(),
           l10 = (SQLLEN)t.ticketType.size(),  l11 = (SQLLEN)t.city.size(),
           l12 = (SQLLEN)t.location.size();
    SQLLEN lInt = sizeof(SQLINTEGER);
    SQLINTEGER day = t.day, month = t.month, year = t.year;

    SQLBindParameter(hStmt, 1,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR, 64,0,(SQLPOINTER)t.ticketId.c_str(),  t.ticketId.size(),  &l1);
    SQLBindParameter(hStmt, 2,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,256,0,(SQLPOINTER)t.title.c_str(),     t.title.size(),     &l2);
    SQLBindParameter(hStmt, 3,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR, 10,0,(SQLPOINTER)t.hall.c_str(),      t.hall.size(),      &l3);
    SQLBindParameter(hStmt, 4,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR, 10,0,(SQLPOINTER)t.seat.c_str(),      t.seat.size(),      &l4);
    SQLBindParameter(hStmt, 5,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR, 20,0,(SQLPOINTER)t.fmt.c_str(),       t.fmt.size(),       &l5);
    SQLBindParameter(hStmt, 6,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR, 20,0,(SQLPOINTER)t.showTime.c_str(),  t.showTime.size(),  &l6);
    SQLBindParameter(hStmt, 7,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,  0,0,&day,   sizeof(day),   &lInt);
    SQLBindParameter(hStmt, 8,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,  0,0,&month, sizeof(month), &lInt);
    SQLBindParameter(hStmt, 9,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,  0,0,&year,  sizeof(year),  &lInt);
    SQLBindParameter(hStmt,10,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR, 50,0,(SQLPOINTER)t.ticketType.c_str(),t.ticketType.size(),&l10);
    SQLBindParameter(hStmt,11,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,256,0,(SQLPOINTER)t.city.c_str(),      t.city.size(),      &l11);
    SQLBindParameter(hStmt,12,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,256,0,(SQLPOINTER)t.location.c_str(),  t.location.size(),  &l12);

    SQLRETURN ret = SQLExecute(hStmt);
    if (!SQL_SUCCEEDED(ret))
        m_lastError = tkOdbcError(SQL_HANDLE_STMT, hStmt);

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    Disconnect();
    return SQL_SUCCEEDED(ret);
}
