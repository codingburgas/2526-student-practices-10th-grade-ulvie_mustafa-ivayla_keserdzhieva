#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace Cinema {
namespace Presentation {

// Structure to hold movie information for UI display
struct MovieInfo {
    std::string title;
    std::string language;
    std::string genre;
    std::string releaseDate;
};

// Structure to hold showtime information
struct ShowtimeInfo {
    std::string time;
    std::string cinema;
    std::string hall;
    int availableSeats = 0;
};

// Structure to hold seat information
struct SeatInfo {
    int row = 0;
    int column = 0;
    bool isBooked = false;
    std::string seatType; // "silver", "gold", "platinum"
    int price = 0;
};

// Structure to hold booking details
struct BookingDetails {
    std::string movieTitle;
    std::string selectedShowtime;
    std::vector<SeatInfo> selectedSeats;
    int totalPrice = 0;
    std::string cinema;
    std::string hall;
};

// Callbacks interface - implement this in your backend
class IUICallbacks {
public:
    virtual ~IUICallbacks() = default;
    
    virtual void onMovieSearch(const std::string& searchTerm) = 0;
    virtual void onMovieSelect(const std::string& movieTitle) = 0;
    virtual void onShowtimeSelect(const std::string& showtime) = 0;
    virtual void onSeatSelect(int row, int col) = 0;
    virtual void onBookingConfirm(const BookingDetails& booking) = 0;
    virtual void onBookingCancel() = 0;
    virtual void onNavigateToPage(const std::string& page) = 0;
};

// UI State Manager - manages the UI state and communicates between Slint and business logic
class UIStateManager {
public:
    UIStateManager() = default;
    ~UIStateManager() = default;
    
    // Movie management
    void setMovieList(const std::vector<MovieInfo>& movies);
    void setSelectedMovie(const MovieInfo& movie);
    const MovieInfo& getSelectedMovie() const;
    
    // Showtime management
    void setAvailableShowtimes(const std::vector<ShowtimeInfo>& showtimes);
    void setSelectedShowtime(const ShowtimeInfo& showtime);
    const ShowtimeInfo& getSelectedShowtime() const;
    
    // Seat management
    void setSeating(const std::vector<std::vector<SeatInfo>>& seats);
    void toggleSeatSelection(int row, int col);
    const std::vector<SeatInfo>& getSelectedSeats() const;
    
    // Booking management
    BookingDetails getCurrentBookingDetails() const;
    void resetBooking();
    void calculateTotalPrice();
    int getTotalPrice() const;
    
    // Page navigation
    void setCurrentPage(const std::string& page);
    const std::string& getCurrentPage() const;
    
private:
    std::vector<MovieInfo> movieList;
    MovieInfo selectedMovie;
    std::vector<ShowtimeInfo> availableShowtimes;
    ShowtimeInfo selectedShowtime;
    std::vector<std::vector<SeatInfo>> seating;
    std::vector<SeatInfo> selectedSeats;
    std::string currentPage = "home";
    int totalPrice = 0;
};

// UI Adapter - adapts Slint callbacks to call the IUICallbacks interface
class UIAdapter {
public:
    UIAdapter(std::shared_ptr<IUICallbacks> callbacks, 
              std::shared_ptr<UIStateManager> stateManager);
    
    void onSearchMovie(const std::string& searchTerm);
    void onSelectMovie(const std::string& movieTitle);
    void onSelectShowtime(const std::string& showtime);
    void onSelectSeat(int row, int col);
    void onConfirmBooking();
    void onCancelBooking();
    void onNavigateToPage(const std::string& page);
    
private:
    std::shared_ptr<IUICallbacks> uiCallbacks;
    std::shared_ptr<UIStateManager> stateManager;
};

} // namespace Presentation
} // namespace Cinema
