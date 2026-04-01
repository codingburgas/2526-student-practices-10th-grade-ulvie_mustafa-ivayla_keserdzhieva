#include "CinemaUIController.h"
#include <algorithm>
#include <iostream>

namespace Cinema {
namespace Presentation {

void CinemaUIController::initialize() {
    // TODO: Initialize your backend services here
    // Example:
    // movieService = std::make_shared<Cinema::Logic::MovieService>();
    // bookingService = std::make_shared<Cinema::Logic::BookingService>();
    
    // Load initial data
    loadMovies();
}

void CinemaUIController::loadMovies() {
    // TODO: Replace this with actual backend call
    // Example code showing how to structure the data:
    
    std::vector<MovieInfo> movies;
    
    movies.push_back({
        "The Matrix",
        "English",
        "Sci-Fi",
        "1999-03-31"
    });
    
    movies.push_back({
        "Inception",
        "English",
        "Sci-Fi",
        "2010-07-16"
    });
    
    movies.push_back({
        "Interstellar",
        "English",
        "Sci-Fi",
        "2014-11-07"
    });
    
    movies.push_back({
        "Avatar",
        "English",
        "Action",
        "2009-12-18"
    });
    
    updateUIWithMovies(movies);
}

void CinemaUIController::loadShowtimesForMovie(const std::string& movieTitle) {
    // TODO: Call your business logic to fetch real showtimes
    // Example:
    // auto showtimes = movieService->getShowtimesForMovie(movieTitle);
    
    // For now, return sample data
    std::vector<ShowtimeInfo> showtimes;
    
    showtimes.push_back({
        "10:00 AM",
        "PVR Cinemas",
        "Hall 3",
        80
    });
    
    showtimes.push_back({
        "1:00 PM",
        "PVR Cinemas",
        "Hall 1",
        45
    });
    
    showtimes.push_back({
        "4:00 PM",
        "PVR Cinemas",
        "Hall 2",
        120
    });
    
    showtimes.push_back({
        "7:00 PM",
        "Inox Cinemas",
        "Hall 4",
        30
    });
    
    updateUIWithShowtimes(showtimes);
}

void CinemaUIController::loadSeatingLayout(const std::string& showtimeId) {
    // TODO: Call backend to get actual seat availability for this showtime
    // For now, create a demo seating layout
    
    const int ROWS = 8;
    const int COLS = 10;
    const int PRICE_SILVER = 150;
    const int PRICE_GOLD = 200;
    const int PRICE_PLATINUM = 300;
    
    std::vector<std::vector<SeatInfo>> seating(ROWS, std::vector<SeatInfo>(COLS));
    
    // Populate seating with demo data
    for (int row = 0; row < ROWS; ++row) {
        for (int col = 0; col < COLS; ++col) {
            SeatInfo seat;
            seat.row = row;
            seat.column = col;
            
            // Random booking pattern for demo
            seat.isBooked = (row * col) % 7 == 0; // Some seats are booked
            
            // Determine seat type based on position
            if (row < 2) {
                seat.seatType = "platinum";
                seat.price = PRICE_PLATINUM;
            } else if (row < 5) {
                seat.seatType = "gold";
                seat.price = PRICE_GOLD;
            } else {
                seat.seatType = "silver";
                seat.price = PRICE_SILVER;
            }
            
            seating[row][col] = seat;
        }
    }
}

void CinemaUIController::updateUIWithMovies(const std::vector<MovieInfo>& movies) {
    // TODO: Update the Slint UI with the provided movies
    // This will be called from main.cpp to pass data to Slint
    std::cout << "Loading " << movies.size() << " movies into UI" << std::endl;
}

void CinemaUIController::updateUIWithShowtimes(const std::vector<ShowtimeInfo>& showtimes) {
    // TODO: Update the Slint UI with the provided showtimes
    std::cout << "Loading " << showtimes.size() << " showtimes into UI" << std::endl;
}

void CinemaUIController::onMovieSearch(const std::string& searchTerm) {
    std::cout << "Searching for movies: " << searchTerm << std::endl;
    
    // TODO: Implement search logic
    // Example:
    // auto results = movieService->searchMovies(searchTerm);
    // updateUIWithMovies(results);
}

void CinemaUIController::onMovieSelect(const std::string& movieTitle) {
    std::cout << "Movie selected: " << movieTitle << std::endl;
    
    // Load showtimes for this movie
    loadShowtimesForMovie(movieTitle);
}

void CinemaUIController::onShowtimeSelect(const std::string& showtime) {
    std::cout << "Showtime selected: " << showtime << std::endl;
    
    // Load seating layout for this showtime
    loadSeatingLayout(showtime);
}

void CinemaUIController::onSeatSelect(int row, int col) {
    std::cout << "Seat selected: Row " << row << ", Column " << col << std::endl;
    
    // TODO: Add any seat selection validation
    // Example: Check if seat is already booked, check concurrent bookings, etc.
}

void CinemaUIController::onBookingConfirm(const BookingDetails& booking) {
    std::cout << "Booking confirmed!" << std::endl;
    std::cout << "  Movie: " << booking.movieTitle << std::endl;
    std::cout << "  Showtime: " << booking.selectedShowtime << std::endl;
    std::cout << "  Total Price: ₹" << booking.totalPrice << std::endl;
    std::cout << "  Seats: " << booking.selectedSeats.size() << std::endl;
    
    // TODO: Process booking through your backend
    // This should:
    // 1. Verify seats are still available
    // 2. Process payment
    // 3. Create booking record in database
    // 4. Send confirmation email
    // Example:
    // auto bookingId = bookingService->createBooking(booking);
    // paymentService->processPayment(booking.totalPrice);
    // emailService->sendConfirmation(bookingId);
}

void CinemaUIController::onBookingCancel() {
    std::cout << "Booking cancelled" << std::endl;
    
    // TODO: Clean up any resources
    // Reset the booking state
}

void CinemaUIController::onNavigateToPage(const std::string& page) {
    std::cout << "Navigating to page: " << page << std::endl;
    
    // TODO: Add any page-specific initialization
    if (page == "admin") {
        // Load admin data
        std::cout << "  Loading admin panel data..." << std::endl;
    } else if (page == "bookings") {
        // Load user's existing bookings
        std::cout << "  Loading user bookings..." << std::endl;
    }
}

} // namespace Presentation
} // namespace Cinema
