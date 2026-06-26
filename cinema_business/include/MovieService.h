#pragma once
#include "../../cinema_data/include/IMovieRepository.h"
#include <memory>
#include <vector>
#include <string>

class MovieService {
    std::shared_ptr<IMovieRepository> m_repo;
    std::string m_lastError;
public:
    explicit MovieService(std::shared_ptr<IMovieRepository> repo)
        : m_repo(std::move(repo)) {}

    const std::string& GetLastError() const { return m_lastError; }

    bool AddMovie(const std::string& title, const std::string& genre, int year) {
        m_lastError = "";
        if (title.empty()) { m_lastError = "Title is required."; return false; }
        MovieRecord r; r.title = title; r.genre = genre; r.year = year;
        bool ok = m_repo->AddMovie(r);
        if (!ok) m_lastError = m_repo->GetLastError();
        return ok;
    }

    bool DeleteMovie(int id) {
        m_lastError = "";
        bool ok = m_repo->DeleteMovie(id);
        if (!ok) m_lastError = m_repo->GetLastError();
        return ok;
    }

    bool GetAllMovies(std::vector<MovieRecord>& out) {
        m_lastError = "";
        bool ok = m_repo->GetAllMovies(out);
        if (!ok) m_lastError = m_repo->GetLastError();
        return ok;
    }

    bool AddShow(const ShowRecord& s) {
        m_lastError = "";
        if (s.movieId <= 0) { m_lastError = "Movie is required."; return false; }
        if (s.hall.empty()) { m_lastError = "Hall is required."; return false; }
        bool ok = m_repo->AddShow(s);
        if (!ok) m_lastError = m_repo->GetLastError();
        return ok;
    }

    bool UpdateShow(const ShowRecord& s) {
        m_lastError = "";
        bool ok = m_repo->UpdateShow(s);
        if (!ok) m_lastError = m_repo->GetLastError();
        return ok;
    }

    bool DeleteShow(int id) {
        m_lastError = "";
        bool ok = m_repo->DeleteShow(id);
        if (!ok) m_lastError = m_repo->GetLastError();
        return ok;
    }

    bool GetAllShows(std::vector<ShowRecord>& out) {
        m_lastError = "";
        bool ok = m_repo->GetAllShows(out);
        if (!ok) m_lastError = m_repo->GetLastError();
        return ok;
    }
};
