#include "../include/SqlMovieRepository.h"
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
static std::string odbcErr(SQLSMALLINT handleType, SQLHANDLE handle) {
    SQLCHAR    state[6]  = {};
    SQLCHAR    msg[512]  = {};
    SQLINTEGER native    = 0;
    SQLSMALLINT len      = 0;
    if (SQL_SUCCEEDED(SQLGetDiagRecA(handleType, handle, 1,
                                     state, &native, msg, sizeof(msg), &len)))
        return std::string((char*)state) + ": " + std::string((char*)msg, len);
    return "Unknown ODBC error";
}

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

SqlMovieRepository::SqlMovieRepository() {
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
}

SqlMovieRepository::~SqlMovieRepository() {
    Disconnect();
    if (hEnv != SQL_NULL_HENV) { SQLFreeHandle(SQL_HANDLE_ENV, hEnv); hEnv = SQL_NULL_HENV; }
}

// ---------------------------------------------------------------------------
// Connect / Disconnect
// ---------------------------------------------------------------------------

bool SqlMovieRepository::Connect() {
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
                  + odbcErr(SQL_HANDLE_DBC, hDbc);
    return false;
}

void SqlMovieRepository::Disconnect() {
    if (hDbc != SQL_NULL_HDBC) {
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        hDbc = SQL_NULL_HDBC;
    }
}

// ---------------------------------------------------------------------------
// AddMovie
// INSERT INTO Movies (Title,Genre,Year,Description,PosterFile) VALUES (?,?,?,?,?)
// ---------------------------------------------------------------------------

bool SqlMovieRepository::AddMovie(const MovieRecord& m) {
    if (!Connect()) return false;

    SQLHSTMT hStmt = SQL_NULL_HSTMT;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

    SQLPrepareA(hStmt,
        (SQLCHAR*)"INSERT INTO Movies (Title,Genre,Year,Description,PosterFile)"
                  " VALUES (?,?,?,?,?)",
        SQL_NTS);

    SQLLEN lt = m.title.size(),       lg = m.genre.size(),
           ld = m.description.size(), lp = m.posterFile.size();
    SQLLEN lyear = 0; // indicator for integer parameter
    SQLINTEGER year = (SQLINTEGER)m.year;

    SQLBindParameter(hStmt,1,SQL_PARAM_INPUT,SQL_C_CHAR, SQL_VARCHAR,256,0,(SQLPOINTER)m.title.c_str(),       m.title.size(),       &lt);
    SQLBindParameter(hStmt,2,SQL_PARAM_INPUT,SQL_C_CHAR, SQL_VARCHAR,100,0,(SQLPOINTER)m.genre.c_str(),       m.genre.size(),       &lg);
    SQLBindParameter(hStmt,3,SQL_PARAM_INPUT,SQL_C_LONG, SQL_INTEGER,  0,0,&year,                             sizeof(year),         &lyear);
    SQLBindParameter(hStmt,4,SQL_PARAM_INPUT,SQL_C_CHAR, SQL_VARCHAR,1000,0,(SQLPOINTER)m.description.c_str(),m.description.size(), &ld);
    SQLBindParameter(hStmt,5,SQL_PARAM_INPUT,SQL_C_CHAR, SQL_VARCHAR,256,0,(SQLPOINTER)m.posterFile.c_str(),  m.posterFile.size(),  &lp);

    SQLRETURN ret = SQLExecute(hStmt);
    if (!SQL_SUCCEEDED(ret))
        m_lastError = "AddMovie failed: " + odbcErr(SQL_HANDLE_STMT, hStmt);

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    Disconnect();
    return SQL_SUCCEEDED(ret);
}

// ---------------------------------------------------------------------------
// DeleteMovie
// DELETE FROM Movies WHERE Id=?
// ---------------------------------------------------------------------------

bool SqlMovieRepository::DeleteMovie(int id) {
    if (!Connect()) return false;

    SQLHSTMT hStmt = SQL_NULL_HSTMT;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

    SQLPrepareA(hStmt, (SQLCHAR*)"DELETE FROM Movies WHERE Id=?", SQL_NTS);

    SQLINTEGER sid = (SQLINTEGER)id;
    SQLLEN     ind = 0;
    SQLBindParameter(hStmt,1,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&sid,sizeof(sid),&ind);

    SQLRETURN ret = SQLExecute(hStmt);
    if (!SQL_SUCCEEDED(ret))
        m_lastError = "DeleteMovie failed: " + odbcErr(SQL_HANDLE_STMT, hStmt);

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    Disconnect();
    return SQL_SUCCEEDED(ret);
}

// ---------------------------------------------------------------------------
// GetAllMovies
// SELECT Id,Title,Genre,Year,Description,PosterFile FROM Movies ORDER BY Id
// ---------------------------------------------------------------------------

bool SqlMovieRepository::GetAllMovies(std::vector<MovieRecord>& out) {
    out.clear();
    if (!Connect()) return false;

    SQLHSTMT hStmt = SQL_NULL_HSTMT;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

    SQLRETURN ret = SQLExecDirectA(hStmt,
        (SQLCHAR*)"SELECT Id,Title,Genre,Year,Description,PosterFile FROM Movies ORDER BY Id",
        SQL_NTS);

    if (!SQL_SUCCEEDED(ret)) {
        m_lastError = "GetAllMovies failed: " + odbcErr(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        Disconnect();
        return false;
    }

    // Bind output columns into local buffers
    SQLINTEGER colId   = 0, colYear = 0;
    SQLCHAR    colTitle[256]  = {},
               colGenre[100]  = {},
               colDesc[1001]  = {},
               colPoster[256] = {};
    SQLLEN     indId   = 0, indTitle = 0, indGenre = 0,
               indYear = 0, indDesc  = 0, indPoster = 0;

    SQLBindCol(hStmt,1,SQL_C_LONG, &colId,        sizeof(colId),          &indId);
    SQLBindCol(hStmt,2,SQL_C_CHAR,  colTitle,      sizeof(colTitle),       &indTitle);
    SQLBindCol(hStmt,3,SQL_C_CHAR,  colGenre,      sizeof(colGenre),       &indGenre);
    SQLBindCol(hStmt,4,SQL_C_LONG, &colYear,       sizeof(colYear),        &indYear);
    SQLBindCol(hStmt,5,SQL_C_CHAR,  colDesc,       sizeof(colDesc),        &indDesc);
    SQLBindCol(hStmt,6,SQL_C_CHAR,  colPoster,     sizeof(colPoster),      &indPoster);

    while (SQL_SUCCEEDED(SQLFetch(hStmt))) {
        MovieRecord r;
        r.id          = (int)colId;
        r.title       = (indTitle  > 0) ? std::string((char*)colTitle,  indTitle)  : "";
        r.genre       = (indGenre  > 0) ? std::string((char*)colGenre,  indGenre)  : "";
        r.year        = (int)colYear;
        r.description = (indDesc   > 0) ? std::string((char*)colDesc,   indDesc)   : "";
        r.posterFile  = (indPoster > 0) ? std::string((char*)colPoster, indPoster) : "";
        out.push_back(r);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    Disconnect();
    return true;
}

// ---------------------------------------------------------------------------
// AddShow
// INSERT INTO Shows (MovieId,Hall,Format,Time1,Time2,Time3,Time4) VALUES (?,?,?,?,?,?,?)
// ---------------------------------------------------------------------------

bool SqlMovieRepository::AddShow(const ShowRecord& s) {
    if (!Connect()) return false;

    SQLHSTMT hStmt = SQL_NULL_HSTMT;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

    SQLPrepareA(hStmt,
        (SQLCHAR*)"INSERT INTO Shows (MovieId,Hall,Format,Time1,Time2,Time3,Time4)"
                  " VALUES (?,?,?,?,?,?,?)",
        SQL_NTS);

    SQLINTEGER movieId = (SQLINTEGER)s.movieId;
    SQLLEN     indMovieId = 0;
    SQLLEN     lh = s.hall.size(),   lf = s.format.size(),
               lt0 = s.times[0].size(), lt1 = s.times[1].size(),
               lt2 = s.times[2].size(), lt3 = s.times[3].size();

    SQLBindParameter(hStmt,1,SQL_PARAM_INPUT,SQL_C_LONG, SQL_INTEGER, 0,0,&movieId,              sizeof(movieId),    &indMovieId);
    SQLBindParameter(hStmt,2,SQL_PARAM_INPUT,SQL_C_CHAR, SQL_VARCHAR,100,0,(SQLPOINTER)s.hall.c_str(),    s.hall.size(),    &lh);
    SQLBindParameter(hStmt,3,SQL_PARAM_INPUT,SQL_C_CHAR, SQL_VARCHAR, 50,0,(SQLPOINTER)s.format.c_str(),  s.format.size(),  &lf);
    SQLBindParameter(hStmt,4,SQL_PARAM_INPUT,SQL_C_CHAR, SQL_VARCHAR, 20,0,(SQLPOINTER)s.times[0].c_str(),s.times[0].size(),&lt0);
    SQLBindParameter(hStmt,5,SQL_PARAM_INPUT,SQL_C_CHAR, SQL_VARCHAR, 20,0,(SQLPOINTER)s.times[1].c_str(),s.times[1].size(),&lt1);
    SQLBindParameter(hStmt,6,SQL_PARAM_INPUT,SQL_C_CHAR, SQL_VARCHAR, 20,0,(SQLPOINTER)s.times[2].c_str(),s.times[2].size(),&lt2);
    SQLBindParameter(hStmt,7,SQL_PARAM_INPUT,SQL_C_CHAR, SQL_VARCHAR, 20,0,(SQLPOINTER)s.times[3].c_str(),s.times[3].size(),&lt3);

    SQLRETURN ret = SQLExecute(hStmt);
    if (!SQL_SUCCEEDED(ret))
        m_lastError = "AddShow failed: " + odbcErr(SQL_HANDLE_STMT, hStmt);

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    Disconnect();
    return SQL_SUCCEEDED(ret);
}

// ---------------------------------------------------------------------------
// UpdateShow
// UPDATE Shows SET Hall=?,Format=?,Time1=?,Time2=?,Time3=?,Time4=? WHERE Id=?
// ---------------------------------------------------------------------------

bool SqlMovieRepository::UpdateShow(const ShowRecord& s) {
    if (!Connect()) return false;

    SQLHSTMT hStmt = SQL_NULL_HSTMT;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

    SQLPrepareA(hStmt,
        (SQLCHAR*)"UPDATE Shows SET Hall=?,Format=?,Time1=?,Time2=?,Time3=?,Time4=? WHERE Id=?",
        SQL_NTS);

    SQLLEN lh  = s.hall.size(),     lf  = s.format.size(),
           lt0 = s.times[0].size(), lt1 = s.times[1].size(),
           lt2 = s.times[2].size(), lt3 = s.times[3].size();
    SQLINTEGER sid    = (SQLINTEGER)s.id;
    SQLLEN     indSid = 0;

    SQLBindParameter(hStmt,1,SQL_PARAM_INPUT,SQL_C_CHAR, SQL_VARCHAR,100,0,(SQLPOINTER)s.hall.c_str(),    s.hall.size(),    &lh);
    SQLBindParameter(hStmt,2,SQL_PARAM_INPUT,SQL_C_CHAR, SQL_VARCHAR, 50,0,(SQLPOINTER)s.format.c_str(),  s.format.size(),  &lf);
    SQLBindParameter(hStmt,3,SQL_PARAM_INPUT,SQL_C_CHAR, SQL_VARCHAR, 20,0,(SQLPOINTER)s.times[0].c_str(),s.times[0].size(),&lt0);
    SQLBindParameter(hStmt,4,SQL_PARAM_INPUT,SQL_C_CHAR, SQL_VARCHAR, 20,0,(SQLPOINTER)s.times[1].c_str(),s.times[1].size(),&lt1);
    SQLBindParameter(hStmt,5,SQL_PARAM_INPUT,SQL_C_CHAR, SQL_VARCHAR, 20,0,(SQLPOINTER)s.times[2].c_str(),s.times[2].size(),&lt2);
    SQLBindParameter(hStmt,6,SQL_PARAM_INPUT,SQL_C_CHAR, SQL_VARCHAR, 20,0,(SQLPOINTER)s.times[3].c_str(),s.times[3].size(),&lt3);
    SQLBindParameter(hStmt,7,SQL_PARAM_INPUT,SQL_C_LONG, SQL_INTEGER,  0,0,&sid,                          sizeof(sid),      &indSid);

    SQLRETURN ret = SQLExecute(hStmt);
    if (!SQL_SUCCEEDED(ret))
        m_lastError = "UpdateShow failed: " + odbcErr(SQL_HANDLE_STMT, hStmt);

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    Disconnect();
    return SQL_SUCCEEDED(ret);
}

// ---------------------------------------------------------------------------
// DeleteShow
// DELETE FROM Shows WHERE Id=?
// ---------------------------------------------------------------------------

bool SqlMovieRepository::DeleteShow(int id) {
    if (!Connect()) return false;

    SQLHSTMT hStmt = SQL_NULL_HSTMT;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

    SQLPrepareA(hStmt, (SQLCHAR*)"DELETE FROM Shows WHERE Id=?", SQL_NTS);

    SQLINTEGER sid = (SQLINTEGER)id;
    SQLLEN     ind = 0;
    SQLBindParameter(hStmt,1,SQL_PARAM_INPUT,SQL_C_LONG,SQL_INTEGER,0,0,&sid,sizeof(sid),&ind);

    SQLRETURN ret = SQLExecute(hStmt);
    if (!SQL_SUCCEEDED(ret))
        m_lastError = "DeleteShow failed: " + odbcErr(SQL_HANDLE_STMT, hStmt);

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    Disconnect();
    return SQL_SUCCEEDED(ret);
}

// ---------------------------------------------------------------------------
// GetAllShows
// SELECT s.Id,s.MovieId,m.Title,s.Hall,s.Format,s.Time1,s.Time2,s.Time3,s.Time4
// FROM Shows s JOIN Movies m ON s.MovieId=m.Id ORDER BY s.Id
// ---------------------------------------------------------------------------

bool SqlMovieRepository::GetAllShows(std::vector<ShowRecord>& out) {
    out.clear();
    if (!Connect()) return false;

    SQLHSTMT hStmt = SQL_NULL_HSTMT;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);

    SQLRETURN ret = SQLExecDirectA(hStmt,
        (SQLCHAR*)"SELECT s.Id,s.MovieId,m.Title,s.Hall,s.Format,"
                  "s.Time1,s.Time2,s.Time3,s.Time4"
                  " FROM Shows s JOIN Movies m ON s.MovieId=m.Id"
                  " ORDER BY s.Id",
        SQL_NTS);

    if (!SQL_SUCCEEDED(ret)) {
        m_lastError = "GetAllShows failed: " + odbcErr(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        Disconnect();
        return false;
    }

    // Bind output columns into local buffers
    SQLINTEGER colId = 0, colMovieId = 0;
    SQLCHAR    colTitle[256] = {},
               colHall[100]  = {},
               colFormat[50] = {},
               colT0[20]     = {},
               colT1[20]     = {},
               colT2[20]     = {},
               colT3[20]     = {};
    SQLLEN     indId = 0, indMovieId = 0, indTitle  = 0, indHall   = 0,
               indFmt = 0, indT0 = 0,    indT1      = 0, indT2     = 0,
               indT3  = 0;

    SQLBindCol(hStmt,1,SQL_C_LONG, &colId,      sizeof(colId),      &indId);
    SQLBindCol(hStmt,2,SQL_C_LONG, &colMovieId, sizeof(colMovieId), &indMovieId);
    SQLBindCol(hStmt,3,SQL_C_CHAR,  colTitle,   sizeof(colTitle),   &indTitle);
    SQLBindCol(hStmt,4,SQL_C_CHAR,  colHall,    sizeof(colHall),    &indHall);
    SQLBindCol(hStmt,5,SQL_C_CHAR,  colFormat,  sizeof(colFormat),  &indFmt);
    SQLBindCol(hStmt,6,SQL_C_CHAR,  colT0,      sizeof(colT0),      &indT0);
    SQLBindCol(hStmt,7,SQL_C_CHAR,  colT1,      sizeof(colT1),      &indT1);
    SQLBindCol(hStmt,8,SQL_C_CHAR,  colT2,      sizeof(colT2),      &indT2);
    SQLBindCol(hStmt,9,SQL_C_CHAR,  colT3,      sizeof(colT3),      &indT3);

    while (SQL_SUCCEEDED(SQLFetch(hStmt))) {
        ShowRecord r;
        r.id         = (int)colId;
        r.movieId    = (int)colMovieId;
        r.movieTitle = (indTitle > 0) ? std::string((char*)colTitle, indTitle) : "";
        r.hall       = (indHall  > 0) ? std::string((char*)colHall,  indHall)  : "";
        r.format     = (indFmt   > 0) ? std::string((char*)colFormat, indFmt)  : "";
        r.times[0]   = (indT0   > 0) ? std::string((char*)colT0,    indT0)    : "";
        r.times[1]   = (indT1   > 0) ? std::string((char*)colT1,    indT1)    : "";
        r.times[2]   = (indT2   > 0) ? std::string((char*)colT2,    indT2)    : "";
        r.times[3]   = (indT3   > 0) ? std::string((char*)colT3,    indT3)    : "";
        out.push_back(r);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    Disconnect();
    return true;
}
