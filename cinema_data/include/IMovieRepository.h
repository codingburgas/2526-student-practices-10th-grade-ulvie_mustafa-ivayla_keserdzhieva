#pragma once
#include <string>
#include <vector>

struct MovieRecord {
    int         id = 0;
    std::string title;
    std::string genre;
    int         year = 2024;
    std::string description;
    std::string posterFile;
};

struct ShowRecord {
    int         id         = 0;
    int         movieId    = 0;
    std::string movieTitle;
    std::string hall;
    std::string format;
    std::string times[4];
};

class IMovieRepository {
public:
    virtual ~IMovieRepository() = default;
    virtual bool AddMovie(const MovieRecord& m) = 0;
    virtual bool DeleteMovie(int id) = 0;
    virtual bool GetAllMovies(std::vector<MovieRecord>& out) = 0;
    virtual bool AddShow(const ShowRecord& s) = 0;
    virtual bool UpdateShow(const ShowRecord& s) = 0;
    virtual bool DeleteShow(int id) = 0;
    virtual bool GetAllShows(std::vector<ShowRecord>& out) = 0;
    virtual const std::string& GetLastError() const = 0;
};
