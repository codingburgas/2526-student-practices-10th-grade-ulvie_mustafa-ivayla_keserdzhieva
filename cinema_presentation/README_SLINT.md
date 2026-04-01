# Slint Frontend Implementation for Cinema Ticket Booking System

This directory contains the complete Slint-based frontend implementation for the Cinema Ticket Booking application.

## 📁 Project Structure

```
cinema_presentation/
├── CMakeLists.txt                 # CMake build configuration
├── build.bat                       # Windows build script
├── build.sh                        # Linux/macOS build script
├── SLINT_SETUP.md                 # Detailed setup instructions
│
├── include/
│   ├── UIStateManager.h           # UI state management interface
│   └── CinemaUIController.h       # Controller for handling UI events
│
├── src/
│   ├── cinema.slint               # Main Slint UI definition (❗ KEY FILE)
│   ├── main.cpp                   # Application entry point
│   ├── UIStateManager.cpp         # UI state manager implementation
│   └── CinemaUIController.cpp    # UI controller implementation
│
└── [original project files]
    ├── cinema_presentation.vcxproj
    ├── cinema_presentation.vcxproj.filters
    └── cinema_presentation.vcxproj.user
```

## 🎯 Key Files Explanation

### 1. **cinema.slint** ⭐
The main UI definition file written in Slint markup language. Contains:
- Movie listing and search interface
- Showtime selection view
- Interactive seat selection grid
- Booking confirmation screen
- Admin panel
- Navigation between pages

**What you see**: The complete visual layout and user interactions

### 2. **CinemaUIController.h/cpp**
C++ component that bridges the Slint UI with your business logic:
- Receives events from the Slint UI
- Calls your backend services (cinema_logic, cinema_business)
- Updates UI with results
- Implements the `IUICallbacks` interface

**What you do**: Implement actual business logic calls in these files

### 3. **UIStateManager.h/cpp**
Manages the state of the application:
- Tracks selected movie, showtime, seats
- Calculates total price
- Manages page navigation
- Provides data structures for UI ↔️ backend communication

**What you use**: Reference this to understand how data flows through the system

### 4. **main.cpp**
Application entry point that:
- Initializes Slint UI
- Creates CinemaUIController and UIStateManager
- Connects UI callbacks to controller methods
- Runs the application

**How it works**: The "glue" that connects everything together

## 🚀 Quick Start

### Option 1: Windows Users (Recommended)

1. **Install Prerequisites:**
   - Download and install CMake from https://cmake.org/download/
   - Download and install Git from https://git-scm.com/

2. **Build the Project:**
   ```bash
   cd cinema_presentation
   build.bat
   ```
   This will:
   - Create a build directory
   - Run CMake configuration
   - Build the entire project
   - Ask if you want to run it

3. **Run the Application:**
   ```bash
   .\build\Debug\cinema_presentation.exe
   ```

### Option 2: Linux/macOS Users

1. **Install Prerequisites:**
   ```bash
   # On macOS with Homebrew:
   brew install cmake git
   
   # On Linux (Ubuntu):
   sudo apt-get install cmake git build-essential
   ```

2. **Build the Project:**
   ```bash
   cd cinema_presentation
   chmod +x build.sh
   ./build.sh
   ```

## 📋 Features Implemented

### ✅ Completed
- [x] Complete Slint UI with 6 main pages
- [x] Movie browsing and search interface
- [x] Showtime selection with cinema details
- [x] Interactive seat selection grid (8×10)
- [x] Booking confirmation flow
- [x] Admin panel structure
- [x] Responsive navigation between pages
- [x] State management system
- [x] Callback/event system for UI interactions
- [x] Price calculation for selected seats
- [x] CMake build configuration
- [x] Build scripts for Windows, Linux, macOS

### 🔲 To Be Implemented (Your Task)
- [ ] Connect to cinema_logic layer services
- [ ] Implement actual movie database queries
- [ ] Implement showtime fetching from backend
- [ ] Implement seat availability checking
- [ ] Add payment processing integration
- [ ] Implement email notifications
- [ ] Add user authentication
- [ ] Implement admin functionality
- [ ] Add error handling and validation
- [ ] Implement concurrent booking prevention
- [ ] Add booking history/my bookings view

## 🔗 Integration Points

To connect with your backend, implement these methods in `CinemaUIController`:

```cpp
void CinemaUIController::onMovieSearch(const std::string& searchTerm)
    // → Call cinema_logic::MovieService::searchMovies()

void CinemaUIController::onMovieSelect(const std::string& movieTitle)
    // → Call cinema_logic::MovieService::getShowtimesForMovie()

void CinemaUIController::onShowtimeSelect(const std::string& showtime)
    // → Call cinema_logic::SeatingService::getSeatingLayout()

void CinemaUIController::onSeatSelect(int row, int col)
    // → Validate seat is not booked in cinema_business::Seat

void CinemaUIController::onBookingConfirm(const BookingDetails& booking)
    // → Call cinema_logic::BookingService::createBooking()
    // → Call cinema_logic::PaymentService::processPayment()
    // → Call cinema_logic::NotificationService::sendEmail()
```

## 📖 Documentation Files

- **[SLINT_SETUP.md](SLINT_SETUP.md)** - Detailed setup and troubleshooting
- **[INTEGRATION_GUIDE.md](../INTEGRATION_GUIDE.md)** - How to connect with business logic
- **[README.md](../README.md)** - Project overview and requirements

## 🎨 UI Pages Overview

### 1. **Home Page**
- Welcome message
- Three main buttons: Book Tickets, Admin Panel, My Bookings

### 2. **Movies Page**
- Search bar for finding movies
- Scrollable list of available movies
- Movie cards with title, language, genre
- Click to select and view showtimes

### 3. **Showtimes Page**
- Shows selected movie title
- Displays cinema and hall information
- Shows available showtimes as buttons
- Click to proceed to seat selection

### 4. **Seats Page**
- Interactive 8×10 seat grid
- Green seats = Available
- Gray seats = Already booked
- Click to select/deselect
- Real-time price display
- "Proceed to Payment" button

### 5. **Booking Confirmation**
- Review booking details
- Show selected movie, showtime, total price
- Option to Cancel or Confirm & Pay

### 6. **Admin Panel**
- Buttons to add/edit/delete movies and shows
- (Implementation to be completed)

## 💡 Code Examples

### Example 1: Accessing Selected Seat Count
```cpp
auto details = state_manager->getCurrentBookingDetails();
int seat_count = details.selectedSeats.size();
int total_price = details.totalPrice;
```

### Example 2: Adding New Movie to UI
```cpp
Cinema::Presentation::MovieInfo movie;
movie.title = "Your Movie Title";
movie.language = "Your Language";
movie.genre = "Your Genre";
movie.releaseDate = "2024-01-01";
// Add to state manager or update UI directly
```

### Example 3: Connecting Backend Service
```cpp
void CinemaUIController::onMovieSelect(const std::string& movieTitle) {
    // Call your backend
    auto showtimes = cinemaLogicService->GetShowtimesForMovie(movieTitle);
    
    // Convert to UI format
    std::vector<ShowtimeInfo> uiShowtimes;
    for (auto& showtime : showtimes) {
        uiShowtimes.push_back(ShowtimeInfo{
            showtime.GetTime(),
            showtime.GetCinemaName(),
            showtime.GetHallName(),
            showtime.GetAvailableSeats()
        });
    }
    
    // Update UI
    updateUIWithShowtimes(uiShowtimes);
}
```

## 🐛 Troubleshooting

### Issue: CMake not found
**Solution**: Download from https://cmake.org and add to PATH

### Issue: "cinema.slint" not found error
**Solution**: Make sure cinema.slint is in `src/` directory, check CMakeLists.txt path

### Issue: Linker errors with cinema_logic
**Solution**: Verify CMakeLists.txt has proper `add_subdirectory()` calls

### Issue: UI shows but buttons don't work
**Solution**: Check that callbacks are properly connected in main.cpp

## 📞 Support

If you encounter issues:
1. Check [SLINT_SETUP.md](SLINT_SETUP.md) for common setup issues
2. Review [INTEGRATION_GUIDE.md](../INTEGRATION_GUIDE.md) for architecture questions
3. Check the Slint documentation: https://slint.dev/docs

## 🎓 Learning Resources

- [Slint Official Documentation](https://slint.dev)
- [Slint Language Reference](https://slint.dev/docs/reference/elements)
- [C++ Slint API](https://docs.rs/slint)
- Example projects: https://github.com/slint-ui/slint/tree/master/examples

## ✅ Verification Checklist

Before considering the Slint implementation complete:
- [ ] Application compiles without errors
- [ ] UI displays correctly and is responsive
- [ ] All navigation buttons work
- [ ] Seat selection grid is interactive
- [ ] Price calculation works correctly
- [ ] Can navigate to all 6 pages
- [ ] Admin panel is accessible
- [ ] Backend is connected to UI events
- [ ] Sample data loads successfully

## 📝 Next Steps

1. **Immediate**: Get the application running locally
2. **Short-term**: Implement backend data loading in CinemaUIController
3. **Medium-term**: Add payment and confirmation logic
4. **Long-term**: Complete admin panel functionality
5. **Final**: Production testing and optimization

---

**Status**: ✅ Slint Frontend Implementation Complete

Created with Slint 1.6+ and C++17
