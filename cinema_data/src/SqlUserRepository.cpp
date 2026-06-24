#include "../include/SqlUserRepository.h"
#include <random>
#include <string>

// Drivers tried in order — covers modern and legacy SQL Server installations
static const char* DRIVERS[] = {
    "ODBC Driver 18 for SQL Server",
    "ODBC Driver 17 for SQL Server",
    "ODBC Driver 13 for SQL Server",
    "SQL Server",
    nullptr
};
static const char* DB_SERVER = "(localdb)\\MSSQLLocalDB";
static const char* DB_NAME   = "CinemaDB";

// Pulls the first diagnostic record from an ODBC handle into a readable string
static std::string odbcError(SQLSMALLINT handleType, SQLHANDLE handle) {
    SQLCHAR  state[6]  = {};
    SQLCHAR  msg[512]  = {};
    SQLINTEGER native  = 0;
    SQLSMALLINT len    = 0;
    if (SQL_SUCCEEDED(SQLGetDiagRecA(handleType, handle, 1,
                                     state, &native, msg, sizeof(msg), &len)))
        return std::string((char*)state) + ": " + std::string((char*)msg, len);
    return "Unknown ODBC error";
}

SqlUserRepository::SqlUserRepository() {
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
}

SqlUserRepository::~SqlUserRepository() {
    Disconnect();
    if (hEnv != SQL_NULL_HENV) { SQLFreeHandle(SQL_HANDLE_ENV, hEnv); hEnv = SQL_NULL_HENV; }
}

bool SqlUserRepository::Connect() {
    SQLCHAR out[1024]; SQLSMALLINT outLen;

    for (int i = 0; DRIVERS[i]; ++i) {
        // Re-allocate the connection handle for each attempt
        if (hDbc != SQL_NULL_HDBC) {
            SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
            hDbc = SQL_NULL_HDBC;
        }
        SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
        SQLSetConnectAttr(hDbc, SQL_ATTR_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

        std::string cs = std::string("DRIVER={") + DRIVERS[i] + "};"
                       + "SERVER=" + DB_SERVER + ";"
                       + "DATABASE=" + DB_NAME + ";"
                       + "Trusted_Connection=yes;";

        SQLRETURN r = SQLDriverConnectA(hDbc, nullptr,
            (SQLCHAR*)cs.c_str(), SQL_NTS,
            out, sizeof(out), &outLen, SQL_DRIVER_NOPROMPT);

        if (SQL_SUCCEEDED(r)) { m_lastError = ""; return true; }
    }

    m_lastError = "Cannot connect to SQL Server (tried all drivers). "
                  "Make sure SQL Server is running, CinemaDB exists, and Windows Auth is enabled. "
                  + odbcError(SQL_HANDLE_DBC, hDbc);
    return false;
}

void SqlUserRepository::Disconnect() {
    if (hDbc != SQL_NULL_HDBC) {
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        hDbc = SQL_NULL_HDBC;
    }
}

bool SqlUserRepository::RegisterUser(const UserRecord& u) {
    if (!Connect()) return false;

    SQLHSTMT hStmt = SQL_NULL_HSTMT;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

    SQLPrepareA(hStmt,
        (SQLCHAR*)"INSERT INTO Users (Name,Surname,Username,Password,Email,Role,Company,City)"
                  " VALUES (?,?,?,?,?,?,?,?)",
        SQL_NTS);

    SQLLEN ln  = u.name.size(),    ls  = u.surname.size(),
           lu  = u.username.size(),lp  = u.password.size(),
           le  = u.email.size(),   lr  = u.role.size(),
           lco = u.company.size(), lci = u.city.size();

    SQLBindParameter(hStmt,1,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,256,0,(SQLPOINTER)u.name.c_str(),    u.name.size(),    &ln);
    SQLBindParameter(hStmt,2,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,256,0,(SQLPOINTER)u.surname.c_str(), u.surname.size(), &ls);
    SQLBindParameter(hStmt,3,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,256,0,(SQLPOINTER)u.username.c_str(),u.username.size(),&lu);
    SQLBindParameter(hStmt,4,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,256,0,(SQLPOINTER)u.password.c_str(),u.password.size(),&lp);
    SQLBindParameter(hStmt,5,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,256,0,(SQLPOINTER)u.email.c_str(),   u.email.size(),   &le);
    SQLBindParameter(hStmt,6,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR, 50,0,(SQLPOINTER)u.role.c_str(),    u.role.size(),    &lr);
    SQLBindParameter(hStmt,7,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,256,0,(SQLPOINTER)u.company.c_str(), u.company.size(), &lco);
    SQLBindParameter(hStmt,8,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,256,0,(SQLPOINTER)u.city.c_str(),    u.city.size(),    &lci);

    SQLRETURN ret = SQLExecute(hStmt);
    if (!SQL_SUCCEEDED(ret)) {
        // Native error 2627 / 2601 = UNIQUE constraint violation
        SQLCHAR state[6] = {}; SQLINTEGER native = 0;
        SQLCHAR msg[512] = {}; SQLSMALLINT len = 0;
        SQLGetDiagRecA(SQL_HANDLE_STMT, hStmt, 1, state, &native, msg, sizeof(msg), &len);
        if (native == 2627 || native == 2601)
            m_lastError = "Username is already taken. Please choose a different one.";
        else
            m_lastError = "Database error: " + std::string((char*)msg, len);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    Disconnect();
    return SQL_SUCCEEDED(ret);
}

bool SqlUserRepository::ValidateLogin(const std::string& username, const std::string& password) {
    if (!Connect()) return false;

    SQLHSTMT hStmt = SQL_NULL_HSTMT;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    SQLPrepareA(hStmt,
        (SQLCHAR*)"SELECT COUNT(*) FROM Users WHERE Username=? AND Password=?", SQL_NTS);

    SQLLEN lu = username.size(), lp = password.size();
    SQLBindParameter(hStmt,1,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,256,0,(SQLPOINTER)username.c_str(),username.size(),&lu);
    SQLBindParameter(hStmt,2,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,256,0,(SQLPOINTER)password.c_str(),password.size(),&lp);

    bool valid = false;
    if (SQL_SUCCEEDED(SQLExecute(hStmt))) {
        SQLINTEGER count = 0; SQLLEN ind = 0;
        SQLBindCol(hStmt, 1, SQL_C_LONG, &count, sizeof(count), &ind);
        if (SQL_SUCCEEDED(SQLFetch(hStmt))) valid = (count > 0);
    }
    if (!valid && m_lastError.empty())
        m_lastError = "Incorrect username or password.";

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    Disconnect();
    return valid;
}

bool SqlUserRepository::CreateVerificationCode(const std::string& email, std::string& outCode) {
    std::mt19937 gen(std::random_device{}());
    outCode = std::to_string(std::uniform_int_distribution<int>(100000, 999999)(gen));

    if (!Connect()) return false;

    SQLHSTMT hStmt = SQL_NULL_HSTMT;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    SQLPrepareA(hStmt,
        (SQLCHAR*)"INSERT INTO PendingAdminVerifications (Email, Code, ExpiresAt)"
                  " VALUES (?, ?, DATEADD(MINUTE,15,GETDATE()))",
        SQL_NTS);

    SQLLEN le = email.size(), lc = outCode.size();
    SQLBindParameter(hStmt,1,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,256,0,(SQLPOINTER)email.c_str(),  email.size(),  &le);
    SQLBindParameter(hStmt,2,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR, 10,0,(SQLPOINTER)outCode.c_str(),outCode.size(),&lc);

    bool ok = SQL_SUCCEEDED(SQLExecute(hStmt));
    if (!ok) m_lastError = odbcError(SQL_HANDLE_STMT, hStmt);
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    Disconnect();
    return ok;
}

bool SqlUserRepository::ValidateVerificationCode(const std::string& email, const std::string& code) {
    if (!Connect()) return false;

    SQLHSTMT hCheck = SQL_NULL_HSTMT;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hCheck);
    SQLPrepareA(hCheck,
        (SQLCHAR*)"SELECT COUNT(*) FROM PendingAdminVerifications"
                  " WHERE Email=? AND Code=? AND ExpiresAt>GETDATE() AND Used=0",
        SQL_NTS);

    SQLLEN le = email.size(), lc = code.size();
    SQLBindParameter(hCheck,1,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,256,0,(SQLPOINTER)email.c_str(),email.size(),&le);
    SQLBindParameter(hCheck,2,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR, 10,0,(SQLPOINTER)code.c_str(), code.size(), &lc);

    bool valid = false;
    if (SQL_SUCCEEDED(SQLExecute(hCheck))) {
        SQLINTEGER cnt = 0; SQLLEN ind = 0;
        SQLBindCol(hCheck, 1, SQL_C_LONG, &cnt, sizeof(cnt), &ind);
        if (SQL_SUCCEEDED(SQLFetch(hCheck))) valid = (cnt > 0);
    }
    SQLFreeHandle(SQL_HANDLE_STMT, hCheck);

    if (!valid) {
        m_lastError = "Verification code is invalid or has expired. Request a new one.";
        Disconnect();
        return false;
    }

    SQLHSTMT hUpd = SQL_NULL_HSTMT;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hUpd);
    SQLPrepareA(hUpd,
        (SQLCHAR*)"UPDATE PendingAdminVerifications SET Used=1 WHERE Email=? AND Code=? AND Used=0",
        SQL_NTS);

    SQLLEN le2 = email.size(), lc2 = code.size();
    SQLBindParameter(hUpd,1,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR,256,0,(SQLPOINTER)email.c_str(),email.size(),&le2);
    SQLBindParameter(hUpd,2,SQL_PARAM_INPUT,SQL_C_CHAR,SQL_VARCHAR, 10,0,(SQLPOINTER)code.c_str(), code.size(), &lc2);

    bool ok = SQL_SUCCEEDED(SQLExecute(hUpd));
    if (!ok) m_lastError = odbcError(SQL_HANDLE_STMT, hUpd);
    SQLFreeHandle(SQL_HANDLE_STMT, hUpd);
    Disconnect();
    return ok;
}
