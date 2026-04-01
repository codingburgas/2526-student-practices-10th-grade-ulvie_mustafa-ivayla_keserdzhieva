# Integration Guide: Connecting Slint UI with Business Logic

This guide explains how to integrate the Slint frontend with your Cinema Logic and Cinema Business layers.

## Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│                    Slint UI Layer                       │
│            (cinema_presentation)                        │
│                                                         │
│  ┌─────────────────────────────────────────────────┐   │
│  │         cinema.slint (UI Definition)            │   │
│  │  - Movie listing & search                       │   │
│  │  - Showtime selection                           │   │
│  │  - Seat selection grid                          │   │
│  │  - Booking confirmation                         │   │
│  │  - Admin panel                                  │   │
│  └─────────────────────────────────────────────────┘   │
│                        ↓ (callbacks)                    │
│  ┌─────────────────────────────────────────────────┐   │
│  │      CinemaUIController & UIStateManager        │   │
│  │  - Handles UI events                            │   │
│  │  - Manages UI state                             │   │
│  │  - Adapts to backend                            │   │
│  └─────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
                        ↓ (function calls)
┌─────────────────────────────────────────────────────────┐
│                 Cinema Logic Layer                      │
│             (cinema_logic)                              │
│                                                         │
│  - Movie management & search                           │
│  - Showtime management                                 │
│  - Booking management                                  │
│  - Payment processing                                  │
│  - Seat availability checking                          │
│  - Notification handling                               │
└─────────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────────┐
│                Business Model Layer                     │
│             (cinema_business)                           │
│                                                         │
│  - Movie entity                                        │
│  - Cinema entity                                       │
│  - Showtime entity                                     │
│  - Booking entity                                      │
│  - Seat entity                                         │
│  - User entity                                         │
│  - Payment entity                                      │
└─────────────────────────────────────────────────────────┘
                        ↓
┌─────────────────────────────────────────────────────────┐
│                  Database / Persistence                 │
│                                                         │
│  - SQLite / SQL Server                                 │
│  - File-based storage                                  │
│  - API calls to backend services                       │
└─────────────────────────────────────────────────────────┘
```

## Step-by-Step Integration Process

### Step 1: Include Business Logic Headers in CinemaUIController

Update `include/CinemaUIController.h`:

```cpp
#pragma once

#include "UIStateManager.h"
// Add your business logic headers here:
// #include "../../../cinema_logic/include/MovieService.h"
// #include "../../../cinema_logic/include/BookingService.h"
// #include "../../../cinema_business/include/Movie.h"
// #include "../../../cinema_business/include/Cinema.h"

namespace Cinema {
namespace Presentation {

class CinemaUIController : public IUICallbacks {
private:
    // Add service references
    // std::shared_ptr<Cinema::Logic::MovieService> movieService;
    // std::shared_ptr<Cinema::Logic::BookingService> bookingService;
    // std::shared_ptr<Cinema::Business::User> currentUser;
};

} // namespace Presentation
} // namespace Cinema
```

### Step 2: Implement Backend Connection in CinemaUIController

Edit `src/CinemaUIController.cpp` to connect with your services:

```cpp
void CinemaUIController::initialize() {
    // Initialize your backend services
    movieService = std::make_shared<Cinema::Logic::MovieService>();
    bookingService = std::make_shared<Cinema::Logic::BookingService>();
    
    // Load initial data from backend
    loadMovies();
}

void CinemaUIController::loadMovies() {
    // Call your backend to get real data
    std::vector<MovieInfo> movies;
    
    auto businessMovies = movieService->getAllMovies();
    
    for (const auto& bMovie : businessMovies) {
        MovieInfo uiMovie;
        uiMovie.title = bMovie.getTitle();
        uiMovie.language = bMovie.getLanguage();
        uiMovie.genre = bMovie.getGenre();
        uiMovie.releaseDate = bMovie.getReleaseDate();
        
        movies.push_back(uiMovie);
    }
    
    updateUIWithMovies(movies);
}

void CinemaUIController::onMovieSelect(const std::string& movieTitle) {
    // When user selects a movie, fetch showtimes from backend
    auto showtimes = movieService->getShowtimesForMovie(movieTitle);
    
    std::vector<ShowtimeInfo> uiShowtimes;
    for (const auto& showtime : showtimes) {
        ShowtimeInfo uiShowtime;
        uiShowtime.time = showtime.getTime();
        uiShowtime.cinema = showtime.getCinemaName();
        uiShowtime.hall = showtime.getHallName();
        uiShowtime.availableSeats = showtime.getAvailableSeatsCount();
        
        uiShowtimes.push_back(uiShowtime);
    }
    
    updateUIWithShowtimes(uiShowtimes);
}

void CinemaUIController::onBookingConfirm(const BookingDetails& booking) {
    // Create booking in backend
    auto newBooking = bookingService->createBooking(
        booking.movieTitle,
        booking.selectedShowtime,
        booking.selectedSeats
    );
    
    // Process payment
    bool paymentSuccessful = bookingService->processPayment(booking.totalPrice);
    
    if (paymentSuccessful) {
        // Send confirmation email
        bookingService->sendConfirmationEmail(newBooking.getId());
        
        std::cout << "Booking confirmed with ID: " << newBooking.getId() << std::endl;
    } else {
        std::cout << "Payment failed" << std::endl;
    }
}
```

### Step 3: Update CMakeLists.txt to Include All Projects

Edit `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.21)
project(cinema_presentation VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

# Include FetchContent
include(FetchContent)

# Add Slint as a dependency
FetchContent_Declare(Slint
    GIT_REPOSITORY https://github.com/slint-ui/slint.git
    GIT_TAG release/1.6
    SOURCE_SUBDIR internal/cmake
)
FetchContent_MakeAvailable(Slint)

# Add cinema_logic as a subdirectory
add_subdirectory(../cinema_logic ${CMAKE_BINARY_DIR}/cinema_logic)

# Add cinema_business as a subdirectory
add_subdirectory(../cinema_business ${CMAKE_BINARY_DIR}/cinema_business)

# Define the executable
add_executable(cinema_presentation
    src/main.cpp
    src/UIStateManager.cpp
    src/CinemaUIController.cpp
)

# Compile the Slint file
slint_target_sources(cinema_presentation src/cinema.slint)

# Link libraries
target_link_libraries(cinema_presentation PRIVATE 
    Slint::Slint
    cinema_logic
    cinema_business
)

# Include directories
target_include_directories(cinema_presentation PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../cinema_logic/include
    ${CMAKE_SOURCE_DIR}/../cinema_business/include
)
```

## Data Flow Examples

### Example 1: Movie Search and Display

```
User enters search term → Slint UI calls on_search_movie()
                           ↓
CinemaUIController::onMovieSearch(searchTerm)
                           ↓
movieService->searchMovies(searchTerm)  [Call backend]
                           ↓
Backend searches database and returns Cinema::Business::Movie objects
                           ↓
Convert Cinema::Business::Movie to MovieInfo struct
                           ↓
updateUIWithMovies(movieList)  [Update Slint UI]
                           ↓
UI re-renders with new movie list
```

### Example 2: Seat Selection and Booking

```
User selects seats → Slint triggers on_select_seat(row, col)
                      ↓
CinemaUIController::onSelectSeat(row, col)
                      ↓
UIStateManager::toggleSeatSelection(row, col)
                      ↓
User clicks "Confirm Booking" → on_confirm_booking()
                      ↓
CinemaUIController::onBookingConfirm(BookingDetails)
                      ↓
bookingService->createBooking(details)  [Create in backend]
     ↓
Payment processing
     ↓
Database update
     ↓
Email notification sent
     ↓
UI shows confirmation message
```

## Testing the Integration

### 1. Create Mock Backend (for testing)

Create a simple mock service while your backend is in development:

```cpp
class MockMovieService {
public:
    std::vector<Cinema::Business::Movie> getAllMovies() {
        std::vector<Cinema::Business::Movie> movies;
        movies.push_back(Cinema::Business::Movie("The Matrix", "English", "Sci-Fi"));
        movies.push_back(Cinema::Business::Movie("Inception", "English", "Sci-Fi"));
        return movies;
    }
};
```

### 2. Unit Test the UIStateManager

```cpp
void testUIStateManager() {
    auto manager = std::make_shared<Cinema::Presentation::UIStateManager>();
    
    MovieInfo movie{"Test", "English", "Drama", "2024"};
    manager->setSelectedMovie(movie);
    
    assert(manager->getSelectedMovie().title == "Test");
    
    std::cout << "UIStateManager test passed!" << std::endl;
}
```

## Best Practices

1. **Separate Concerns**: Keep UI logic in CinemaUIController, business logic in cinema_logic, and data models in cinema_business

2. **Use Shared Pointers**: Manage memory safely with `std::shared_ptr`

3. **Error Handling**: Add try-catch blocks when calling backend services

4. **State Management**: Use UIStateManager to maintain consistent UI state

5. **Asynchronous Operations**: For long-running operations (database queries, payments), consider using threading or async/await patterns

6. **Data Validation**: Validate all user inputs before passing to backend

7. **Logging**: Add debug logging to trace data flow during development

## Common Integration Issues

| Issue | Solution |
|-------|----------|
| Linker errors for cinema_logic/business | Ensure CMakeLists.txt includes add_subdirectory() and target_link_libraries() |
| Cinema.slint not found | Verify slint_target_sources() uses correct file path |
| Business objects in UI | Convert Cinema::Business objects to UI structs (MovieInfo, ShowtimeInfo, etc.) |
| Callback not triggering | Ensure ui_adapter is captured in lambda by value or shared_ptr |
| UI not updating | Call ui->set_current_page() or property setters after data changes |

## Next Steps

1. ✅ Slint UI framework is set up
2. ⏳ Implement cinema_logic services
3. ⏳ Implement cinema_business models
4. ⏳ Set up database/persistence layer
5. ⏳ Connect all components together
6. ⏳ Test end-to-end workflow
7. ⏳ Add payment gateway integration
8. ⏳ Implement email notifications

Start by implementing your Cinema Logic layer functions and gradually connect them to the UI through the CinemaUIController!
