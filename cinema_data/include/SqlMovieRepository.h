#pragma once
#include "IMovieRepository.h"
#include <windows.h>
#include <sql.h>
#include <sqlext.h>

class SqlMovieRepository : public IMovieRepository {
    SQLHENV hEnv = SQL_NULL_HENV;
    SQLHDBC hDbc = SQL_NULL_HDBC;
    std::string m_lastError;
    bool Connect();
    void Disconnect();
public:
    SqlMovieRepository();
    ~SqlMovieRepository() override;
    bool AddMovie(const MovieRecord& m) override;
    bool DeleteMovie(int id) override;
    bool GetAllMovies(std::vector<MovieRecord>& out) override;
    bool AddShow(const ShowRecord& s) override;
    bool UpdateShow(const ShowRecord& s) override;
    bool DeleteShow(int id) override;
    bool GetAllShows(std::vector<ShowRecord>& out) override;
    const std::string& GetLastError() const override { return m_lastError; }
};
