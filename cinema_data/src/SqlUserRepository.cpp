#include "../include/SqlUserRepository.h"

static const char* CONN_STR =
    "DRIVER={SQL Server};"
    "SERVER=localhost;"
    "DATABASE=CinemaDB;"
    "Trusted_Connection=yes;";

SqlUserRepository::SqlUserRepository() {
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
}

SqlUserRepository::~SqlUserRepository() {
    Disconnect();
    if (hEnv != SQL_NULL_HENV) { SQLFreeHandle(SQL_HANDLE_ENV, hEnv); hEnv = SQL_NULL_HENV; }
}

bool SqlUserRepository::Connect() {
    if (hDbc == SQL_NULL_HDBC)
        SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
    SQLCHAR out[1024]; SQLSMALLINT outLen;
    return SQL_SUCCEEDED(SQLDriverConnectA(hDbc, nullptr,
        (SQLCHAR*)CONN_STR, SQL_NTS, out, sizeof(out), &outLen, SQL_DRIVER_NOPROMPT));
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
        (SQLCHAR*)"INSERT INTO Users (Name, Surname, Username, Password) VALUES (?, ?, ?, ?)",
        SQL_NTS);

    SQLLEN ln = u.name.size(), ls = u.surname.size(),
           lu = u.username.size(), lp = u.password.size();
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 256, 0, (SQLPOINTER)u.name.c_str(),     u.name.size(),     &ln);
    SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 256, 0, (SQLPOINTER)u.surname.c_str(),  u.surname.size(),  &ls);
    SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 256, 0, (SQLPOINTER)u.username.c_str(), u.username.size(), &lu);
    SQLBindParameter(hStmt, 4, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 256, 0, (SQLPOINTER)u.password.c_str(), u.password.size(), &lp);

    SQLRETURN ret = SQLExecute(hStmt);
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    Disconnect();
    return SQL_SUCCEEDED(ret);
}

bool SqlUserRepository::ValidateLogin(const std::string& username, const std::string& password) {
    if (!Connect()) return false;

    SQLHSTMT hStmt = SQL_NULL_HSTMT;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

    SQLPrepareA(hStmt,
        (SQLCHAR*)"SELECT COUNT(*) FROM Users WHERE Username = ? AND Password = ?",
        SQL_NTS);

    SQLLEN lu = username.size(), lp = password.size();
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 256, 0, (SQLPOINTER)username.c_str(), username.size(), &lu);
    SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 256, 0, (SQLPOINTER)password.c_str(), password.size(), &lp);

    bool valid = false;
    if (SQL_SUCCEEDED(SQLExecute(hStmt))) {
        SQLINTEGER count = 0; SQLLEN ind = 0;
        SQLBindCol(hStmt, 1, SQL_C_LONG, &count, sizeof(count), &ind);
        if (SQL_SUCCEEDED(SQLFetch(hStmt))) valid = (count > 0);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    Disconnect();
    return valid;
}
