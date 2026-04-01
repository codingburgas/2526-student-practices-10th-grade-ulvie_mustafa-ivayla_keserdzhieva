#pragma once

#include "UIStateManager.h"
#include <vector>
#include <memory>

namespace Cinema {
namespace Presentation {

/**
 * Example Implementation of IUICallbacks
 * This demonstrates how to connect UI events to your business logic layer
 */
class CinemaUIController : public IUICallbacks {
public:
    CinemaUIController() = default;
    ~CinemaUIController() = default;
    
    // Initialize with backend services
    void initialize();
    
    // IUICallbacks implementations
    void onMovieSearch(const std::string& searchTerm) override;
    void onMovieSelect(const std::string& movieTitle) override;
    void onShowtimeSelect(const std::string& showtime) override;
    void onSeatSelect(int row, int col) override;
    void onBookingConfirm(const BookingDetails& booking) override;
    void onBookingCancel() override;
    void onNavigateToPage(const std::string& page) override;
    
    // Additional helper methods
    void loadMovies();
    void loadShowtimesForMovie(const std::string& movieTitle);
    void loadSeatingLayout(const std::string& showtimeId);
    void updateUIWithMovies(const std::vector<MovieInfo>& movies);
    void updateUIWithShowtimes(const std::vector<ShowtimeInfo>& showtimes);
    
private:
    // You can add references to your business logic services here
    // Example:
    // std::shared_ptr<Cinema::Logic::MovieService> movieService;
    // std::shared_ptr<Cinema::Logic::BookingService> bookingService;
    // std::shared_ptr<Cinema::Business::User> currentUser;
};

} // namespace Presentation
} // namespace Cinema
