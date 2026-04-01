#include "UIStateManager.h"
#include <algorithm>
#include <numeric>

namespace Cinema {
namespace Presentation {

// UIStateManager Implementation

void UIStateManager::setMovieList(const std::vector<MovieInfo>& movies) {
    movieList = movies;
}

void UIStateManager::setSelectedMovie(const MovieInfo& movie) {
    selectedMovie = movie;
}

const MovieInfo& UIStateManager::getSelectedMovie() const {
    return selectedMovie;
}

void UIStateManager::setAvailableShowtimes(const std::vector<ShowtimeInfo>& showtimes) {
    availableShowtimes = showtimes;
}

void UIStateManager::setSelectedShowtime(const ShowtimeInfo& showtime) {
    selectedShowtime = showtime;
}

const ShowtimeInfo& UIStateManager::getSelectedShowtime() const {
    return selectedShowtime;
}

void UIStateManager::setSeating(const std::vector<std::vector<SeatInfo>>& seats) {
    seating = seats;
}

void UIStateManager::toggleSeatSelection(int row, int col) {
    if (row >= 0 && row < static_cast<int>(seating.size()) &&
        col >= 0 && col < static_cast<int>(seating[row].size())) {
        
        if (!seating[row][col].isBooked) {
            // Check if seat is already selected
            auto it = std::find_if(selectedSeats.begin(), selectedSeats.end(),
                [row, col](const SeatInfo& seat) {
                    return seat.row == row && seat.column == col;
                });
            
            if (it != selectedSeats.end()) {
                selectedSeats.erase(it);
            } else {
                selectedSeats.push_back(seating[row][col]);
            }
            
            calculateTotalPrice();
        }
    }
}

const std::vector<SeatInfo>& UIStateManager::getSelectedSeats() const {
    return selectedSeats;
}

BookingDetails UIStateManager::getCurrentBookingDetails() const {
    BookingDetails details;
    details.movieTitle = selectedMovie.title;
    details.selectedShowtime = selectedShowtime.time;
    details.selectedSeats = selectedSeats;
    details.totalPrice = totalPrice;
    details.cinema = selectedShowtime.cinema;
    details.hall = selectedShowtime.hall;
    return details;
}

void UIStateManager::resetBooking() {
    selectedSeats.clear();
    totalPrice = 0;
    currentPage = "home";
}

void UIStateManager::calculateTotalPrice() {
    totalPrice = std::accumulate(selectedSeats.begin(), selectedSeats.end(), 0,
        [](int sum, const SeatInfo& seat) {
            return sum + seat.price;
        });
}

int UIStateManager::getTotalPrice() const {
    return totalPrice;
}

void UIStateManager::setCurrentPage(const std::string& page) {
    currentPage = page;
}

const std::string& UIStateManager::getCurrentPage() const {
    return currentPage;
}

// UIAdapter Implementation

UIAdapter::UIAdapter(std::shared_ptr<IUICallbacks> callbacks,
                     std::shared_ptr<UIStateManager> stateManager)
    : uiCallbacks(callbacks), stateManager(stateManager) {
}

void UIAdapter::onSearchMovie(const std::string& searchTerm) {
    if (uiCallbacks) {
        uiCallbacks->onMovieSearch(searchTerm);
    }
}

void UIAdapter::onSelectMovie(const std::string& movieTitle) {
    if (uiCallbacks) {
        uiCallbacks->onMovieSelect(movieTitle);
    }
}

void UIAdapter::onSelectShowtime(const std::string& showtime) {
    if (uiCallbacks) {
        uiCallbacks->onShowtimeSelect(showtime);
    }
}

void UIAdapter::onSelectSeat(int row, int col) {
    stateManager->toggleSeatSelection(row, col);
    if (uiCallbacks) {
        uiCallbacks->onSeatSelect(row, col);
    }
}

void UIAdapter::onConfirmBooking() {
    if (uiCallbacks) {
        BookingDetails details = stateManager->getCurrentBookingDetails();
        uiCallbacks->onBookingConfirm(details);
    }
}

void UIAdapter::onCancelBooking() {
    stateManager->resetBooking();
    if (uiCallbacks) {
        uiCallbacks->onBookingCancel();
    }
}

void UIAdapter::onNavigateToPage(const std::string& page) {
    stateManager->setCurrentPage(page);
    if (uiCallbacks) {
        uiCallbacks->onNavigateToPage(page);
    }
}

} // namespace Presentation
} // namespace Cinema
